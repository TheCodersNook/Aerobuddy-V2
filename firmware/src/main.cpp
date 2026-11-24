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
//   * OpenHaystack Find My beacon (v2 addition): broadcasts a rotating
//     P-224 public key as an Apple Offline Finding advertisement, so
//     nearby Apple devices file encrypted location reports you decrypt
//     with the matching private keys (see OPENHAYSTACK.md). No MFi,
//     no Apple approval — unofficial DIY tracking. Keys are provisioned
//     over USB serial (HAYSTACK_PUT) and stored in LittleFS; the beacon
//     auto-starts at boot once keys exist. The advert is non-connectable
//     and there is no companion app — nothing to pair to.
//
// Honest limits (see README for all of them):
//   * No RTC hardware: the day counter is days since first power-on
//     unless the clock is set once over USB (SETTIME). No wall clock
//     until then; "midnight" is 24 h after first boot.
//   * millis() wraps at ~49.7 days; the day index restarts then.
//   * NOT hardware-tested. Compile-checked only. Bench tests in README.
//   * The beacon is NOT hardware-tested either: the payload bytes and
//     address derivation follow OpenHaystack / PETS 2021 exactly, but
//     no Apple device has ever heard this firmware advertise. Bench
//     test 7 in README + OPENHAYSTACK.md section 6 are the proof that
//     still has to happen on the real board.
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
#include <bluefruit.h>  // Adafruit Bluefruit52Lib (bundled in the nRF52 core): BLE advertising + SoftDevice access for the OpenHaystack beacon

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

// ------------------------------------------------------- OpenHaystack
// DIY Find My tracking (v2 addition). How it works, in one paragraph:
// the beacon broadcasts a 28-byte NIST P-224 public key, split across
// the BLE random address (first 6 bytes) and the advertisement payload
// (the other 22 bytes), in Apple's Offline Finding format. Any nearby
// Apple device that hears it encrypts its own GPS fix to that public
// key (ECIES) and uploads the ciphertext to Apple, indexed by
// SHA-256(public key). Only the holder of the matching private key can
// decrypt the report — that is the whole security model, and it is why
// the private keys NEVER go on this device: only public keys are
// provisioned here (over USB serial, HAYSTACK_PUT). Key generation,
// provisioning, and report retrieval: see OPENHAYSTACK.md.
//
// Protocol references (the byte layout below follows these exactly):
//   * Heinrich, Stute, Kornhuber & Hollick, "Who Can Find My Devices?",
//     PoPETs/PETS 2021 (SEEMOO lab) — the OpenHaystack paper.
//   * seemoo-lab/openhaystack (2021) + dchristl/macless-haystack —
//     the reference firmwares (ESP32 + nRF5x) this mirrors.
//   * Apple company ID 0x004C, Offline Finding type 0x12.
//
// Rotation: Apple rotates keys every 15 min; this firmware does the
// same (HAYSTACK_ROTATE_MS). The key index follows wall-clock 15-min
// slots when the clock is set (SETTIME), else 15-min slots since boot.
// The owner's fetch tooling queries ALL provisioned keys, so either
// alignment works — rotation is for unlinkability, not correctness.
//
// NOT hardware-tested: no Apple device has heard this firmware
// advertise. The payload bytes, address derivation, and rotation flow
// are the same as the known-good nRF5x OpenHaystack firmwares, but
// bench test 7 (README) + OPENHAYSTACK.md section 6 are the proof
// that has to happen on the real board.

#define HAYSTACK_KEY_BYTES  28      // P-224 public-key X-coordinate
#define HAYSTACK_MAX_KEYS   1024    // 28 KB; the XIAO has 2 MB QSPI flash
#define HAYSTACK_ROTATE_MS  900000UL// 15 min, Apple's rotation interval
#define HAYSTACK_ADV_MS     1000    // advertising interval (1 s)
#define HAYSTACK_TX_POWER   4       // dBm (+4: better find range; ~+4 uA avg vs 0 dBm)
#define HAYSTACK_FILE       "/haystack.bin"
// File layout: 8-byte header + count*28-byte keys.
//   header: "HSTK" magic, version=1, enabled (0/1), count u16 LE.
#define HAYSTACK_HDR_LEN    8
#define HAYSTACK_MAGIC      "HSTK"

static uint16_t g_hsCount = 0;       // keys provisioned (0 = no beacon)
static bool     g_hsEnabled = true;  // persisted on/off preference
static bool     g_hsRunning = false; // advertising right now
static bool     g_hsBleUp = false;   // Bluefruit.begin() done
static uint16_t g_hsIndex = 0xFFFF;  // current key index (0xFFFF = none)
static bool     g_uiDirty = false;   // screen needs a refresh (beacon state changed)
static uint32_t g_hsLastTryMs = 0;   // backoff timer for failed beacon starts

static bool haystackLoadHeader() {
  File f = InternalFS.open(HAYSTACK_FILE, FILE_O_READ);
  if (!f) { g_hsCount = 0; g_hsEnabled = true; return false; }
  uint8_t hdr[HAYSTACK_HDR_LEN];
  int n = f.read(hdr, sizeof(hdr));
  f.close();
  if (n != HAYSTACK_HDR_LEN || memcmp(hdr, HAYSTACK_MAGIC, 4) != 0 ||
      hdr[4] != 1) {
    g_hsCount = 0; g_hsEnabled = true;
    return false;
  }
  g_hsEnabled = (hdr[5] != 0);
  g_hsCount = (uint16_t)hdr[6] | ((uint16_t)hdr[7] << 8);
  if (g_hsCount > HAYSTACK_MAX_KEYS) g_hsCount = HAYSTACK_MAX_KEYS;
  return true;
}
static bool haystackSaveHeader() {
  File f = InternalFS.open(HAYSTACK_FILE, FILE_O_WRITE);
  if (!f) return false;
  uint8_t hdr[HAYSTACK_HDR_LEN] = { 'H', 'S', 'T', 'K', 1,
    (uint8_t)(g_hsEnabled ? 1 : 0),
    (uint8_t)(g_hsCount & 0xFF), (uint8_t)(g_hsCount >> 8) };
  f.seek(0);
  size_t w = f.write(hdr, sizeof(hdr));
  f.close();
  return w == sizeof(hdr);
}
static bool haystackReadKey(uint16_t index, uint8_t key[HAYSTACK_KEY_BYTES]) {
  if (index >= g_hsCount) return false;
  File f = InternalFS.open(HAYSTACK_FILE, FILE_O_READ);
  if (!f) return false;
  if (!f.seek(HAYSTACK_HDR_LEN + (uint32_t)index * HAYSTACK_KEY_BYTES)) { f.close(); return false; }
  int n = f.read(key, HAYSTACK_KEY_BYTES);
  f.close();
  return n == HAYSTACK_KEY_BYTES;
}
static bool haystackKeyIsZero(const uint8_t key[HAYSTACK_KEY_BYTES]) {
  for (int i = 0; i < HAYSTACK_KEY_BYTES; i++) if (key[i]) return false;
  return true;
}
static bool haystackPutKey(uint16_t index, const uint8_t key[HAYSTACK_KEY_BYTES]) {
  // Sequential provisioning only: no gaps (a gap would be a zero key,
  // which is not a valid public key and would break rotation).
  if (index > g_hsCount || index >= HAYSTACK_MAX_KEYS) return false;
  if (haystackKeyIsZero(key)) return false;
  // First key (or a fresh/corrupt file): make sure a valid header exists.
  if (g_hsCount == 0 && !haystackSaveHeader()) return false;
  File f = InternalFS.open(HAYSTACK_FILE, FILE_O_WRITE);
  if (!f) return false;
  if (!f.seek(HAYSTACK_HDR_LEN + (uint32_t)index * HAYSTACK_KEY_BYTES)) { f.close(); return false; }
  size_t w = f.write(key, HAYSTACK_KEY_BYTES);
  f.close();
  if (w != HAYSTACK_KEY_BYTES) return false;
  if (index >= g_hsCount) { g_hsCount = index + 1; haystackSaveHeader(); }
  return true;
}
static void haystackStopBeacon() {
  if (g_hsRunning) Bluefruit.Advertising.stop();
  g_hsRunning = false;
  g_hsIndex = 0xFFFF;
}
static void haystackClear() {
  haystackStopBeacon();
  InternalFS.remove(HAYSTACK_FILE);
  g_hsCount = 0; g_hsEnabled = true; g_hsIndex = 0xFFFF;
}
// Build the 29-byte manufacturer-specific payload (the AD element's
// data — Bluefruit adds the length byte + 0xFF type byte):
//   [0..1]  4C 00  Apple company ID (little-endian)
//   [2]     12     Offline Finding type
//   [3]     19     payload length (25 bytes follow)
//   [4]     00     status byte (battery unknown, not separated;
//                  both reference firmwares use 0x00; bits 6-7 can
//                  carry battery level later — see macless-haystack
//                  firmware/nrf5x/battery.h)
//   [5..26] key[6..27]  (22 bytes — the middle of the public key)
//   [27]    key[0] >> 6 (top 2 bits of key[0], lost from the address)
//   [28]    00     hint byte (unused)
// The finder rebuilds the full 28-byte key as:
//   key[0] = (payload[27] << 6) | (addr[0] & 0x3F),
//   key[1..5] = addr[1..5], key[6..27] = payload[5..26].
static void haystackBuildPayload(const uint8_t key[HAYSTACK_KEY_BYTES], uint8_t out[29]) {
  out[0] = 0x4C; out[1] = 0x00;
  out[2] = 0x12; out[3] = 0x19;
  out[4] = 0x00;
  memcpy(out + 5, key + 6, 22);
  out[27] = (uint8_t)(key[0] >> 6);
  out[28] = 0x00;
}
static bool haystackStartBeacon(uint16_t index) {
  uint8_t key[HAYSTACK_KEY_BYTES];
  if (!haystackReadKey(index, key)) return false;
  if (haystackKeyIsZero(key)) return false;
  if (!g_hsBleUp) {
    Bluefruit.autoConnLed(false);  // no blinking LED while advertising
    Bluefruit.begin();
    Bluefruit.setTxPower(HAYSTACK_TX_POWER);
    Bluefruit.Advertising.setType(BLE_GAP_ADV_TYPE_NONCONNECTABLE_NONSCANNABLE_UNDIRECTED);
    Bluefruit.Advertising.setIntervalMS(HAYSTACK_ADV_MS, HAYSTACK_ADV_MS);
    g_hsBleUp = true;
  }
  // The BLE address MUST carry the key's first 6 bytes (the finder
  // rebuilds the public key from address + payload). The SoftDevice
  // forbids changing the identity address while advertising, so stop
  // first; setAddr is retried because the stop completes async.
  if (g_hsRunning) Bluefruit.Advertising.stop();
  ble_gap_addr_t addr;
  addr.addr_type = BLE_GAP_ADDR_TYPE_RANDOM_STATIC;
  addr.addr[0] = (uint8_t)(key[0] | 0xC0);  // top 2 bits = 0b11 (random static)
  for (int i = 1; i < 6; i++) addr.addr[i] = key[i];
  bool ok = false;
  for (int i = 0; i < 20 && !ok; i++) {
    ok = Bluefruit.setAddr(&addr);
    if (!ok) delay(10);
  }
  if (!ok) { g_hsRunning = false; return false; }
  uint8_t payload[29];
  haystackBuildPayload(key, payload);
  Bluefruit.Advertising.clearData();
  if (!Bluefruit.Advertising.addData(BLE_GAP_AD_TYPE_MANUFACTURER_SPECIFIC_DATA, payload, sizeof(payload))) {
    g_hsRunning = false; return false;
  }
  if (!Bluefruit.Advertising.start(0)) {  // 0 = advertise forever
    g_hsRunning = false; return false;
  }
  g_hsRunning = true;
  g_hsIndex = index;
  return true;
}
static uint16_t haystackDesiredIndex(uint32_t nowMs) {
  if (g_hsCount == 0) return 0xFFFF;
  uint32_t slot;
  if (g_timeSet) slot = nowUnixSec(nowMs) / 900UL;   // wall-clock 15-min slots
  else           slot = nowMs / HAYSTACK_ROTATE_MS;  // boot-relative slots
  return (uint16_t)(slot % g_hsCount);
}
// Called every loop iteration: start the beacon when keys exist, rotate
// the key every 15 min, stop it when disabled. Cheap (one division);
// the actual re-advertise only happens on an index change.
static void haystackPoll(uint32_t nowMs) {
  if (g_hsCount == 0 || !g_hsEnabled) {
    if (g_hsRunning) haystackStopBeacon();
    return;
  }
  uint16_t want = haystackDesiredIndex(nowMs);
  if (g_hsRunning && want == g_hsIndex) return;
  // Don't hammer a failing start: one attempt per 5 s. (A failed
  // rotation leaves the beacon dark until the next attempt — a few
  // seconds of no beacon, acceptable for a bench prototype.)
  if (nowMs - g_hsLastTryMs < 5000UL) return;
  g_hsLastTryMs = nowMs;
  bool was = g_hsRunning;
  if (haystackStartBeacon(want) && !was) g_uiDirty = true;  // first start -> show indicator
}
static int haystackHexVal(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}
// Decode exactly 56 hex chars into 28 bytes. Returns false on any bad char.
static bool haystackHexToKey(const char *hex, uint8_t key[HAYSTACK_KEY_BYTES]) {
  for (int i = 0; i < HAYSTACK_KEY_BYTES; i++) {
    int hi = haystackHexVal(hex[2*i]), lo = haystackHexVal(hex[2*i+1]);
    if (hi < 0 || lo < 0) return false;
    key[i] = (uint8_t)((hi << 4) | lo);
  }
  return true;
}
static void haystackPrintHex(const uint8_t *buf, uint8_t len) {
  const char *h = "0123456789abcdef";
  for (int i = 0; i < len; i++) {
    Serial.write(h[buf[i] >> 4]);
    Serial.write(h[buf[i] & 0x0F]);
  }
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
  Serial.println("# Haystack: HAYSTACK_PUT <idx> <56 hex>, HAYSTACK_COUNT, HAYSTACK_STATUS, HAYSTACK_ON, HAYSTACK_OFF, HAYSTACK_CLEAR");
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
  Serial.print("haystack_keys="); Serial.println(g_hsCount);
  Serial.print("haystack_beacon="); Serial.println(g_hsRunning ? "on" : "off");
  if (g_hsRunning) {
    Serial.print("haystack_key_index="); Serial.println(g_hsIndex);
  }
}
static void doResetCanister() {
  g_dosesLeft = DOSES_PER_CAN;
  g_canisters++;
  saveState();
}
static char g_cmdBuf[96];  // 96: HAYSTACK_PUT lines carry a 56-char hex key
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
    Serial.println("HAYSTACK_PUT <idx> <56 hex>, HAYSTACK_COUNT, HAYSTACK_STATUS, HAYSTACK_ON, HAYSTACK_OFF, HAYSTACK_CLEAR");
  } else if (!strncasecmp(line, "HAYSTACK_PUT ", 13)) {
    // HAYSTACK_PUT <index> <56 hex chars> — store one public key.
    const char *p = line + 13;
    char *end = NULL;
    long idx = strtol(p, &end, 10);
    if (end == p || *end != ' ') {
      Serial.println("ERR usage: HAYSTACK_PUT <index> <56 hex>");
    } else if (strlen(end + 1) != 56) {
      Serial.println("ERR key must be exactly 56 hex chars (28 bytes)");
    } else if (idx < 0 || idx >= HAYSTACK_MAX_KEYS) {
      Serial.println("ERR index out of range");
    } else {
      uint8_t key[HAYSTACK_KEY_BYTES];
      if (!haystackHexToKey(end + 1, key)) {
        Serial.println("ERR key is not valid hex");
      } else if (!haystackPutKey((uint16_t)idx, key)) {
        Serial.println("ERR store failed (provision in order from 0, no gaps; key must not be all zero)");
      } else {
        Serial.print("OK key "); Serial.print(idx);
        Serial.print(" stored ("); Serial.print(g_hsCount); Serial.println(" total)");
        if (g_hsCount == 1 && g_hsEnabled) {
          // First key: the beacon can start now.
          if (haystackStartBeacon(haystackDesiredIndex(millis()))) g_uiDirty = true;
        }
      }
    }
  } else if (!strcasecmp(line, "HAYSTACK_COUNT")) {
    Serial.print("keys="); Serial.println(g_hsCount);
    Serial.print("enabled="); Serial.println(g_hsEnabled ? "yes" : "no");
    Serial.print("advertising="); Serial.println(g_hsRunning ? "yes" : "no");
    Serial.print("key_index=");
    Serial.println(g_hsIndex == 0xFFFF ? -1 : (int)g_hsIndex);
  } else if (!strcasecmp(line, "HAYSTACK_STATUS")) {
    // COUNT + the current address + payload, so you can verify the
    // beacon with a BLE scanner app (nRF Connect / LightBlue).
    Serial.print("keys="); Serial.println(g_hsCount);
    Serial.print("enabled="); Serial.println(g_hsEnabled ? "yes" : "no");
    Serial.print("advertising="); Serial.println(g_hsRunning ? "yes" : "no");
    if (g_hsBleUp) {
      ble_gap_addr_t a = Bluefruit.getAddr();
      Serial.print("ble_addr=");
      for (int i = 5; i >= 0; i--) {  // MSB first, as scanners display it
        if (a.addr[i] < 0x10) Serial.print("0");
        Serial.print(a.addr[i], HEX);
        if (i > 0) Serial.print(":");
      }
      Serial.println();
    } else {
      Serial.println("ble_addr=(ble not started)");
    }
    if (g_hsRunning && g_hsIndex != 0xFFFF) {
      uint8_t key[HAYSTACK_KEY_BYTES];
      if (haystackReadKey(g_hsIndex, key)) {
        uint8_t payload[29];
        haystackBuildPayload(key, payload);
        Serial.print("key_index="); Serial.println(g_hsIndex);
        // Full on-air AD element: 1E (length) FF (manufacturer type)
        // + the 29 payload bytes. A scanner should show manufacturer
        // data starting 4C 00 12 19.
        Serial.print("payload=1eff");
        haystackPrintHex(payload, sizeof(payload));
        Serial.println();
      }
    }
  } else if (!strcasecmp(line, "HAYSTACK_ON")) {
    g_hsEnabled = true; haystackSaveHeader();
    if (g_hsCount > 0) {
      if (haystackStartBeacon(haystackDesiredIndex(millis()))) g_uiDirty = true;
      Serial.println("Beacon on.");
    } else {
      Serial.println("Beacon enabled, but no keys provisioned yet (HAYSTACK_PUT).");
    }
  } else if (!strcasecmp(line, "HAYSTACK_OFF")) {
    g_hsEnabled = false; haystackSaveHeader();
    haystackStopBeacon(); g_uiDirty = true;
    Serial.println("Beacon off.");
  } else if (!strcasecmp(line, "HAYSTACK_CLEAR")) {
    haystackClear(); g_uiDirty = true;
    Serial.println("All Haystack keys erased. Beacon stopped.");
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
    if (g_hsRunning) centeredText("FIND MY ON", 14, &FreeSans9pt7b);
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

  // OpenHaystack beacon: auto-starts if keys were provisioned earlier.
  haystackLoadHeader();
  if (g_hsCount > 0 && g_hsEnabled) {
    haystackStartBeacon(haystackDesiredIndex(millis()));
  }

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

  // OpenHaystack: start the beacon when keys exist, rotate the key
  // every 15 min, stop it when disabled. When the beacon is running,
  // the SoftDevice's radio events wake the CPU from WFI about once a
  // second, so rotation stays on schedule even with no button/reed
  // activity. When unprovisioned this is one cheap division.
  haystackPoll(now);
  if (g_uiDirty && g_view == 0) { g_uiDirty = false; showMain(millis()); }

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
