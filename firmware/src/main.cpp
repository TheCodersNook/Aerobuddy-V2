// AeroBuddy v2 firmware (developed as PuffPal v4; Nov 23, 2026)
//
// Folding replacement actuator for an albuterol inhaler: on-device dose
// tracing, 1.54" e-ink screen with an avatar face, NO companion app.
// Board: Seeed XIAO nRF52840 (documented 5 uA deep sleep; do NOT use the
// Elecrow SuperMini here — its sleep current is disputed 1 mA vs 20 uA).
// Panel: GDEH0154D67-class 1.54" 200x200 e-ink, driven by GxEPD2.
//
// How it works:
//   * Sleeps in System ON idle (WFI) between events. The nRF52 RTC keeps
//     millis() running across sleeps, so the debounce window, the dose
//     timestamps, and the day counter all survive sleep. (SYSTEMOFF
//     would draw less but kills the clock — see README for the tradeoff.)
//   * Reed close (canister pressed) -> GPIO interrupt wakes the CPU;
//     one dose counted per press. Debounce: presses <400 ms apart count
//     as one (bounce bursts and double-triggers are ignored by timestamp).
//   * E-ink UI: avatar face (happy / sleepy / concerned / empty), doses
//     left (200 per canister), today / 7-day bars / total views.
//   * Button short press cycles views. Button long-press (3 s) shows a
//     "NEW CAN?" confirm screen; short press within 10 s confirms.
//   * USB serial: auto-dumps the dose log as CSV on connect; commands
//     LOG / STATUS / RESET / SETTIME <unix epoch> / HELP.
//   * Counter + log persist in internal flash (LittleFS, Adafruit core).
//   * Find My is NOT in this firmware (Apple MFi gate — README).
//     No BLE advertising in v1.
//
// Honest limits (see README for all of them):
//   * No RTC hardware: the day counter is days since first power-on
//     unless the clock is set once over USB (SETTIME). No wall clock
//     until then; "midnight" is 24 h after first boot.
//   * millis() wraps at ~49.7 days; the day index restarts then.
//   * NOT hardware-tested. Compile-checked only. Bench tests in README.
//
// Wiring (must match the README table exactly):
//   E-ink (SPI, default XIAO SPI: SCK D8, MISO D9, MOSI D10)
//     CS=D2  DC=D3  RST=D1  BUSY=D0
//   Reed switch (NO, to GND): D6    Button (to GND): D7
//   LED anode (via 220 ohm to GND): D4
//   LiPo 301230 to the XIAO's battery pads; USB-C is the XIAO's own port.

#include <Arduino.h>
#include <SPI.h>
#include <GxEPD2_BW.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>

using namespace Adafruit_LittleFS_Namespace;

// ---------------------------------------------------------------- pins
#define EPD_CS    2   // D2
#define EPD_DC    3   // D3
#define EPD_RST   1   // D1
#define EPD_BUSY  0   // D0 — GDEH0154D67 BUSY is HIGH while busy
#define REED_PIN  6   // D6, INPUT_PULLUP, LOW = canister pressed
#define BTN_PIN   7   // D7, INPUT_PULLUP, LOW = pressed
#define LED_PIN   4   // D4, HIGH = LED on (via 220 ohm to GND)

// ------------------------------------------------------------ constants
#define DOSES_PER_CAN   200
#define LOW_DOSE_WARN   20
#define DEBOUNCE_MS     400   // presses closer than this count as one
#define LONG_PRESS_MS   3000  // button hold = new-canister confirm screen
#define CONFIRM_MS      10000 // confirm window after long-press
#define SLEEPY_HOURS    12    // no dose for this long -> sleepy face
#define MAX_LOG_LINES   1000  // then the log restarts (flash budget)
#define DAY_MS          86400000UL

// Display: GDEH0154D67 200x200 (GxEPD2_154_D67), full-buffer mode.
GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT>
  display(GxEPD2_154_D67(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));

// ---------------------------------------------------------------- state
static uint16_t g_dosesLeft = DOSES_PER_CAN;
static uint32_t g_totalPuffs = 0;
static uint32_t g_boots = 0;
static uint32_t g_canisters = 1;
static uint32_t g_lastDoseMs = 0;   // millis() of last counted dose
static bool     g_haveLastDose = false;
static int32_t  g_timeOffsetSec = 0; // unix epoch minus millis()/1000
static bool     g_timeSet = false;   // true after SETTIME over USB
static uint8_t  g_view = 0;          // 0 main, 1 today, 2 week, 3 total
static bool     g_confirmPending = false;
static uint32_t g_confirmSinceMs = 0;

static volatile bool g_reedEvent = false;
static volatile bool g_btnEvent = false;

static void reedISR() { g_reedEvent = true; }
static void btnISR()  { g_btnEvent = true; }

// ------------------------------------------------------------- time
// Unix time if SETTIME was given, else 0 (unset). Day index is the UTC
// day when the clock is set, else days since first power-on (day 0 =
// the first 24 h after the very first boot).
static uint32_t nowUnixSec(uint32_t nowMs) {
  if (!g_timeSet) return 0;
  return (uint32_t)g_timeOffsetSec + nowMs / 1000UL;
}
static int32_t dayIndex(uint32_t nowMs) {
  if (g_timeSet) return (int32_t)(nowUnixSec(nowMs) / 86400UL);
  return (int32_t)(nowMs / DAY_MS);
}
static uint32_t msInDay(uint32_t nowMs) {
  if (g_timeSet) return (nowUnixSec(nowMs) % 86400UL) * 1000UL;
  return nowMs % DAY_MS;
}

// ------------------------------------------------------------- storage
// state.txt: dosesLeft,totalPuffs,boots,canisters,timeOffsetSec,timeSet
// puff.csv:  one line per dose: day,ms_in_day,boot,doses_left
static void saveState() {
  File f = InternalFS.open("/state.txt", FILE_O_WRITE);
  if (!f) return;
  char buf[64];
  snprintf(buf, sizeof(buf), "%u,%lu,%lu,%lu,%ld,%d\n",
           g_dosesLeft, (unsigned long)g_totalPuffs,
           (unsigned long)g_boots, (unsigned long)g_canisters,
           (long)g_timeOffsetSec, g_timeSet ? 1 : 0);
  f.write(buf, strlen(buf));
  f.truncate(f.position());
  f.close();
}
static void loadState() {
  File f = InternalFS.open("/state.txt", FILE_O_READ);
  if (!f) { saveState(); return; }  // first boot: defaults stand
  char buf[64];
  int n = f.read(buf, sizeof(buf) - 1);
  f.close();
  if (n <= 0) return;
  buf[n] = 0;
  unsigned d = 0, t = 0, b = 0, c = 1;
  long off = 0; int ts = 0;
  if (sscanf(buf, "%u,%u,%u,%u,%ld,%d", &d, &t, &b, &c, &off, &ts) >= 3) {
    g_dosesLeft = (uint16_t)d;
    g_totalPuffs = t;
    g_boots = b;
    g_canisters = c ? c : 1;
    g_timeOffsetSec = (int32_t)off;
    g_timeSet = (ts != 0);
  }
}
static void logPuff(uint32_t nowMs) {
  File f = InternalFS.open("/puff.csv", FILE_O_WRITE);
  if (!f) return;
  if (f.size() > MAX_LOG_LINES * 24) {  // rotate: restart the log
    f.close();
    InternalFS.remove("/puff.csv");
    f = InternalFS.open("/puff.csv", FILE_O_WRITE);
    if (!f) return;
  }
  f.seek(f.size());
  char buf[40];
  snprintf(buf, sizeof(buf), "%ld,%lu,%lu,%u\n",
           (long)dayIndex(nowMs), (unsigned long)msInDay(nowMs),
           (unsigned long)g_boots, g_dosesLeft);
  f.write(buf, strlen(buf));
  f.close();
}
// Count log entries per day for the 7 days ending on `today`.
// out[0] = 6 days ago ... out[6] = today.
static void dailyCounts(int32_t today, uint16_t out[7]) {
  for (int i = 0; i < 7; i++) out[i] = 0;
  File f = InternalFS.open("/puff.csv", FILE_O_READ);
  if (!f) return;
  char line[40]; int li = 0;
  while (f.available()) {
    char c = (char)f.read();
    if (c == '\n' || li >= (int)sizeof(line) - 1) {
      line[li] = 0; li = 0;
      long day = 0;
      if (sscanf(line, "%ld,", &day) == 1) {
        long d = (long)today - day;
        if (d >= 0 && d < 7) out[6 - (int)d]++;
      }
    } else line[li++] = c;
  }
  f.close();
}

// -------------------------------------------------------------- serial
static void dumpCsvOverSerial() {
  Serial.println("day,ms_in_day,boot,doses_left");
  File f = InternalFS.open("/puff.csv", FILE_O_READ);
  if (!f) { Serial.println("(no puffs logged yet)"); }
  else {
    while (f.available()) Serial.write((char)f.read());
    f.close();
  }
  // Daily totals section (comment lines, so the CSV above stays clean).
  uint16_t dc[7];
  dailyCounts(dayIndex(millis()), dc);
  Serial.println("# daily totals, last 7 days (6 days ago .. today)");
  Serial.print("# ");
  for (int i = 0; i < 7; i++) {
    Serial.print(dc[i]);
    Serial.print(i < 6 ? "," : "\n");
  }
  Serial.println("# --- end of log ---");
  Serial.println("# Commands: LOG, STATUS, RESET, SETTIME <unix>, HELP");
}
static void printStatus() {
  uint32_t now = millis();
  Serial.print("doses_left="); Serial.println(g_dosesLeft);
  Serial.print("total_puffs="); Serial.println(g_totalPuffs);
  Serial.print("canister="); Serial.println(g_canisters);
  Serial.print("boots="); Serial.println(g_boots);
  Serial.print("day_index="); Serial.println(dayIndex(now));
  Serial.print("clock_set="); Serial.println(g_timeSet ? "yes" : "no");
  if (g_timeSet) {
    Serial.print("unix_now="); Serial.println(nowUnixSec(now));
  }
  Serial.print("uptime_ms="); Serial.println(now);
}
static void doResetCanister() {
  g_dosesLeft = DOSES_PER_CAN;
  g_canisters++;
  saveState();
}
static char g_cmdBuf[32];
static uint8_t g_cmdLen = 0;
static void handleSerialLine(const char *line);
static void pollSerial() {
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\r') continue;
    if (c == '\n') {
      g_cmdBuf[g_cmdLen] = 0;
      if (g_cmdLen > 0) handleSerialLine(g_cmdBuf);
      g_cmdLen = 0;
    } else if (g_cmdLen < sizeof(g_cmdBuf) - 1) {
      g_cmdBuf[g_cmdLen++] = c;
    }
  }
}
static void handleSerialLine(const char *line) {
  if (!strcasecmp(line, "LOG")) {
    dumpCsvOverSerial();
  } else if (!strcasecmp(line, "STATUS")) {
    printStatus();
  } else if (!strcasecmp(line, "RESET")) {
    doResetCanister();
    Serial.println("New canister: 200 doses.");
  } else if (!strncasecmp(line, "SETTIME ", 8)) {
    long epoch = atol(line + 8);
    if (epoch > 1700000000L) {  // sanity: after Nov 2023
      g_timeOffsetSec = (int32_t)(epoch - (long)(millis() / 1000UL));
      g_timeSet = true;
      saveState();
      Serial.println("Clock set. Day counter now follows UTC days.");
    } else {
      Serial.println("SETTIME needs a unix epoch, e.g. SETTIME 1796000000");
    }
  } else if (!strcasecmp(line, "HELP")) {
    Serial.println("LOG, STATUS, RESET, SETTIME <unix>, HELP");
  } else {
    Serial.print("Unknown command: ");
    Serial.println(line);
  }
}

// ------------------------------------------------------------- drawing
static void centeredText(const char *s, int y, const GFXfont *font) {
  display.setFont(font);
  int16_t x1, y1; uint16_t w, h;
  display.getTextBounds(s, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((200 - (int)w) / 2 - x1, y);
  display.print(s);
}
static void arcPts(int cx, int cy, int rx, int ry, int a0, int a1) {
  for (int a = a0; a <= a1; a += 4) {
    float t = a * 3.14159f / 180.0f;
    display.fillRect(cx + (int)(rx * cos(t)), cy + (int)(ry * sin(t)), 2, 2, GxEPD_BLACK);
  }
}
// Avatar faces. Kind, never shaming — the face reacts only to device
// facts (doses left, time since last dose), never to the user's health.
//   0 empty (round mouth)  1 concerned (flat mouth, brows)
//   2 sleepy (closed eyes) 3 happy (smile)
static uint8_t faceForNow(uint32_t nowMs) {
  if (g_dosesLeft == 0) return 0;
  if (g_dosesLeft <= LOW_DOSE_WARN) return 1;
  if (g_haveLastDose &&
      (nowMs - g_lastDoseMs) > (uint32_t)SLEEPY_HOURS * 3600000UL)
    return 2;
  return 3;
}
static void drawAvatar(int cx, int cy, uint8_t face) {
  display.drawCircle(cx, cy, 30, GxEPD_BLACK);
  if (face == 2) {  // sleepy: closed eyes, small smile, "z"s
    display.drawLine(cx - 18, cy - 8, cx - 8, cy - 8, GxEPD_BLACK);
    display.drawLine(cx + 8, cy - 8, cx + 18, cy - 8, GxEPD_BLACK);
    arcPts(cx, cy + 2, 10, 7, 20, 160);
    display.setFont(&FreeSans9pt7b);
    display.setCursor(cx + 32, cy - 22); display.print("z");
    display.setCursor(cx + 40, cy - 30); display.print("z");
    return;
  }
  display.fillCircle(cx - 12, cy - 8, 3, GxEPD_BLACK);
  display.fillCircle(cx + 12, cy - 8, 3, GxEPD_BLACK);
  if (face == 0) {          // empty: round mouth
    display.drawCircle(cx, cy + 10, 6, GxEPD_BLACK);
  } else if (face == 1) {   // concerned: flat mouth + worried brows
    display.drawLine(cx - 10, cy + 12, cx + 10, cy + 12, GxEPD_BLACK);
    display.drawLine(cx - 17, cy - 16, cx - 8, cy - 13, GxEPD_BLACK);
    display.drawLine(cx + 8, cy - 13, cx + 17, cy - 16, GxEPD_BLACK);
  } else {                  // happy: smile
    arcPts(cx, cy + 2, 12, 9, 20, 160);
  }
}
static void refreshBegin() {
  display.init();
  display.setFullWindow();
  display.firstPage();
}
static void refreshEnd() {
  while (display.nextPage()) { }
  display.hibernate();  // panel to deep sleep: ~0 draw while idle
}
static void showMain(uint32_t nowMs) {
  refreshBegin();
  do {
    display.fillScreen(GxEPD_WHITE);
    drawAvatar(100, 62, faceForNow(nowMs));
    char buf[24];
    if (g_dosesLeft == 0) centeredText("EMPTY", 128, &FreeSansBold12pt7b);
    else if (g_dosesLeft <= LOW_DOSE_WARN)
      centeredText("GET A REFILL SOON", 128, &FreeSans9pt7b);
    else centeredText("DOSES LEFT", 128, &FreeSansBold12pt7b);
    snprintf(buf, sizeof(buf), "%u", g_dosesLeft);
    centeredText(buf, 168, &FreeSansBold18pt7b);
    snprintf(buf, sizeof(buf), "PUFFS %lu", (unsigned long)g_totalPuffs);
    centeredText(buf, 192, &FreeSans9pt7b);
  } while (false);
  refreshEnd();
}
static void showToday(uint32_t nowMs) {
  uint16_t dc[7];
  dailyCounts(dayIndex(nowMs), dc);
  char buf[24];
  refreshBegin();
  do {
    display.fillScreen(GxEPD_WHITE);
    centeredText("TODAY", 40, &FreeSansBold12pt7b);
    snprintf(buf, sizeof(buf), "%u", dc[6]);
    centeredText(buf, 100, &FreeSansBold18pt7b);
    centeredText("PUFFS", 128, &FreeSans9pt7b);
    snprintf(buf, sizeof(buf), "%u LEFT IN CAN", g_dosesLeft);
    centeredText(buf, 165, &FreeSans9pt7b);
  } while (false);
  refreshEnd();
}
static void showWeek(uint32_t nowMs) {
  uint16_t dc[7];
  dailyCounts(dayIndex(nowMs), dc);
  uint16_t mx = 1, total = 0;
  for (int i = 0; i < 7; i++) { if (dc[i] > mx) mx = dc[i]; total += dc[i]; }
  char buf[24];
  refreshBegin();
  do {
    display.fillScreen(GxEPD_WHITE);
    centeredText("LAST 7 DAYS", 30, &FreeSansBold12pt7b);
    // Bars: 7 bars, 22 px wide, baseline y=160, max height 90 px.
    for (int i = 0; i < 7; i++) {
      int x = 14 + i * 26;
      int h = dc[i] ? (int)((long)dc[i] * 90 / mx) : 0;
      if (h > 0) display.fillRect(x, 160 - h, 20, h, GxEPD_BLACK);
      display.drawRect(x, 70, 20, 90, GxEPD_BLACK);
      display.setFont(&FreeSans9pt7b);
      display.setCursor(x + 4, 160 - h - 6);
      display.print(dc[i]);
      display.setCursor(x + 4, 178);
      if (i == 6) display.print("T");
      else { snprintf(buf, sizeof(buf), "%dd", 6 - i); display.print(buf); }
    }
    snprintf(buf, sizeof(buf), "WEEK TOTAL %u", total);
    centeredText(buf, 196, &FreeSans9pt7b);
  } while (false);
  refreshEnd();
}
static void showTotal() {
  char buf[24];
  refreshBegin();
  do {
    display.fillScreen(GxEPD_WHITE);
    centeredText("TOTAL", 40, &FreeSansBold12pt7b);
    snprintf(buf, sizeof(buf), "%lu", (unsigned long)g_totalPuffs);
    centeredText(buf, 100, &FreeSansBold18pt7b);
    centeredText("PUFFS, ALL TIME", 128, &FreeSans9pt7b);
    snprintf(buf, sizeof(buf), "CANISTER %lu", (unsigned long)g_canisters);
    centeredText(buf, 165, &FreeSans9pt7b);
  } while (false);
  refreshEnd();
}
static void showConfirm() {
  refreshBegin();
  do {
    display.fillScreen(GxEPD_WHITE);
    centeredText("NEW CAN?", 60, &FreeSansBold12pt7b);
    centeredText("PRESS AGAIN", 100, &FreeSans9pt7b);
    centeredText("TO RESET TO 200", 125, &FreeSans9pt7b);
    centeredText("WAIT TO CANCEL", 165, &FreeSans9pt7b);
  } while (false);
  refreshEnd();
}
static void showView(uint32_t nowMs) {
  switch (g_view) {
    case 1: showToday(nowMs); break;
    case 2: showWeek(nowMs); break;
    case 3: showTotal(); break;
    default: showMain(nowMs); break;
  }
}
static void blinkLed(int times) {
  for (int i = 0; i < times; i++) {
    digitalWrite(LED_PIN, HIGH); delay(120);
    digitalWrite(LED_PIN, LOW);
    if (i + 1 < times) delay(120);
  }
}

// -------------------------------------------------------- dose handling
static void countDose(uint32_t nowMs) {
  if (g_dosesLeft > 0) g_dosesLeft--;
  g_totalPuffs++;
  g_lastDoseMs = nowMs;
  g_haveLastDose = true;
  saveState();
  logPuff(nowMs);
  blinkLed(1);
  g_view = 0;  // a puff always returns to the main screen
  showMain(nowMs);
}
static void onReedEvent(uint32_t nowMs) {
  // Timestamp debounce: bounce bursts and double-triggers inside the
  // 400 ms window count as the same press. A real second puff is a
  // full press-release-press cycle (~1 s), well outside the window.
  if (g_haveLastDose && (nowMs - g_lastDoseMs) < DEBOUNCE_MS) return;
  countDose(nowMs);
}
static void onShortPress(uint32_t nowMs) {
  if (g_confirmPending) {
    g_confirmPending = false;
    doResetCanister();
    blinkLed(3);
    g_view = 0;
    showMain(nowMs);
    return;
  }
  g_view = (g_view + 1) % 4;  // main -> today -> week -> total
  showView(nowMs);
}
static void onLongPress(uint32_t nowMs) {
  g_confirmPending = true;
  g_confirmSinceMs = nowMs;
  blinkLed(2);
  showConfirm();
}

// ---------------------------------------------------------------- setup
static bool g_usbDumped = false;
static bool g_serialWasUp = false;

void setup() {
  pinMode(REED_PIN, INPUT_PULLUP);
  pinMode(BTN_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  if (!InternalFS.begin()) {
    // Flash FS failed: screen still works, nothing persists.
  }
  loadState();
  g_boots++;
  saveState();

  attachInterrupt(digitalPinToInterrupt(REED_PIN), reedISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(BTN_PIN), btnISR, FALLING);

  // If we booted with the canister already pressed, count it (a press
  // that happened during a dead-battery boot is still a dose).
  if (digitalRead(REED_PIN) == LOW) onReedEvent(millis());

  Serial.begin(115200);
  uint32_t t0 = millis();
  while (!Serial && millis() - t0 < 3000) delay(10);
  if (Serial) { dumpCsvOverSerial(); g_usbDumped = true; g_serialWasUp = true; }

  showMain(millis());
}

// ----------------------------------------------------------------- loop
// Button state machine (polled; the interrupt only wakes the CPU):
// press start -> on release, <3 s = short press, >=3 s = long press.
static bool     g_btnDown = false;
static uint32_t g_btnDownMs = 0;
static bool     g_longFired = false;

void loop() {
  uint32_t now = millis();

  if (g_reedEvent) { g_reedEvent = false; onReedEvent(now); now = millis(); }

  bool btnLow = (digitalRead(BTN_PIN) == LOW);
  if (btnLow && !g_btnDown) {
    g_btnDown = true; g_btnDownMs = now; g_longFired = false;
  } else if (btnLow && g_btnDown && !g_longFired &&
             (now - g_btnDownMs) >= LONG_PRESS_MS) {
    g_longFired = true;
    onLongPress(now);
  } else if (!btnLow && g_btnDown) {
    g_btnDown = false;
    if (!g_longFired) onShortPress(now);
  }

  if (g_confirmPending && (now - g_confirmSinceMs) >= CONFIRM_MS) {
    g_confirmPending = false;  // timeout: cancel, back to main screen
    g_view = 0;
    showMain(now);
  }

  // USB serial: auto-dump once per fresh plug-in, then serve commands.
  bool serialUp = (bool)Serial;
  if (serialUp && !g_serialWasUp && !g_usbDumped) {
    dumpCsvOverSerial();
    g_usbDumped = true;
  }
  if (!serialUp) g_usbDumped = false;
  g_serialWasUp = serialUp;
  if (serialUp) pollSerial();

  // Sleep: WFI halts the CPU until the next interrupt (reed/button
  // GPIOTE, or USB). The RTC keeps millis() running, so timestamps,
  // debounce, and the day counter all survive sleep. While USB is
  // connected we stay awake to serve serial (USB-powered anyway);
  // while the button is held we keep polling for the long-press.
  if (!serialUp && !btnLow) {
    __WFI();
  } else {
    delay(5);
  }
}
