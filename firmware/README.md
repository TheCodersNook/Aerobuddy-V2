# AeroBuddy v5 — Firmware

> v5 firmware lineage: PuffPal v4 / AeroBuddy v2. The v5 change vs v4
> is dose detection: a microswitch under the press cap (same D6 pin,
> same interrupt + debounce). Battery, UI, beacon, and logging are
> unchanged. Internal file names still say `puffpal` — only the
> user-facing title changed.

Real, compile-checked PlatformIO + Arduino firmware for the folding
replacement actuator: Seeed XIAO nRF52840 + 1.54" 200×200 e-ink panel
(GDEH0154D67 class, GxEPD2). Dose tracing on-device, avatar face, **no
companion app** — plus an **OpenHaystack Find My beacon** (DIY tracking,
no MFi — see `OPENHAYSTACK.md`).

**Status: compile-checked only — NOT hardware-tested.** Bench tests in §5
are the proof that still has to happen on the real board.

## 1. What it does

- **Sleep:** System ON idle (`__WFI()`) between events. The nRF52 RTC
  keeps `millis()` running across sleeps, so the debounce window, dose
  timestamps, and day counter all survive sleep. Target <30 µA (see §6).
- **Dose counting:** a microswitch under the press cap closes at the
  bottom of the ~3.2 mm fire stroke → GPIO interrupt wakes the CPU →
  one dose counted. Timestamp debounce: presses <400 ms apart count
  as one (kills bounce bursts and double-triggers; a real second puff
  is a full press cycle, ~1 s).
- **Screen:** avatar face + doses left + lifetime puffs. Faces: happy
  (>20 left), concerned (≤20), empty (0), sleepy (no dose for 12 h+).
  Kind, never shaming — faces react only to device facts.
- **Button short press:** cycles views — main → today → 7-day bars →
  total → main. **Long press (3 s):** "NEW CAN?" confirm screen; press
  again within 10 s to reset to 200, or wait and it cancels.
- **USB serial (115200):** auto-dumps the dose log as CSV on connect.
  Commands: `LOG` (dump), `STATUS`, `RESET` (new canister),
  `SETTIME <unix epoch>` (sets the clock — see §6), `HELP`.
- **OpenHaystack Find My beacon:** broadcasts a rotating P-224 public
  key as an Apple Offline Finding advertisement (1 s interval, +4 dBm,
  key rotates every 15 min). Keys are provisioned over USB serial
  (`HAYSTACK_PUT`, `HAYSTACK_ON`/`OFF`, `HAYSTACK_STATUS` —
  see `OPENHAYSTACK.md`); the beacon auto-starts at boot once keys
  exist, and the main screen shows "FIND MY ON" while it advertises.
  Battery cost ≈ +20 µA average → ~4.5 months on the 110 mAh cell
  (vs ~13 months with the beacon off). Not hardware-tested.
- **Persistence:** counter + log in internal flash (LittleFS, Adafruit
  core). Log line: `day,ms_in_day,boot,doses_left`. Rotates at 1000 lines.

## 2. Pinout (XIAO nRF52840)

| Signal | XIAO pin | Goes to |
|---|---|---|
| E-ink SCK | D8 | Panel SCK (default SPI) |
| E-ink MISO | D9 | Panel MISO (unused by panel) |
| E-ink MOSI | D10 | Panel MOSI (default SPI) |
| E-ink CS | D2 | Panel CS |
| E-ink DC | D3 | Panel DC |
| E-ink RST | D1 | Panel RST |
| E-ink BUSY | D0 | Panel BUSY (HIGH = busy) |
| Microswitch (NO) | D6 | Microswitch under the press cap → GND (LOW = pressed) |
| Button | D7 | Button → GND (LOW = pressed) |
| LED | D4 | LED anode → 220 Ω → GND (HIGH = on) |
| LiPo 301230 | BAT pads | XIAO battery pads (onboard charger + USB-C) |

All pins are real XIAO nRF52840 pins (D0–D10; SPI on D8/D9/D10). Same
pinout as the v4 firmware — the microswitch lands on the same D6 pin.

## 3. Build + flash

```bash
cd ~/workspace/asthma-tech/puffpal-v5/firmware
pio run -e seeed_xiao_nrf52840
```

First build downloads Seeed's PlatformIO platform + the Adafruit nRF52
core (a few minutes). Note: mainline `platform-nordicnrf52` v11.0.0
does **not** ship a XIAO nRF52840 board — there is no
`seeed_xiao_nrf52840` board ID in it (verified Nov 23, 2026), so this
project uses Seeed's own platform per the Seeed wiki; the v3
firmware's `platformio.ini` names that nonexistent ID and cannot build
as-is. The Sense and non-Sense XIAO share this board definition (same
pinout; the Sense only adds IMU + mic, unused here).

Flash: `pio run -t upload` (nrfutil DFU; double-press the XIAO's reset
button first if the port doesn't appear). No `.uf2` is emitted by this
build — the manual path is `firmware.hex` converted with uf2conv.py.
Arduino IDE also works: board **"Seeed XIAO nRF52840"** (Adafruit nRF52
core), install the GxEPD2 library, open `src/main.cpp` as a sketch.

## 4. How to use

1. **First boot:** screen shows the avatar + 200 doses left. The day
   counter starts at day 0 (see §6 — no wall clock until you set one).
2. **Set the clock (optional, once):** plug in USB, open a serial
   terminal at 115200, send `SETTIME <unix epoch>` (e.g. from
   `date +%s` on your computer). After that the day counter follows UTC
   days and "today" is a real calendar day. The offset persists in flash.
3. **Use it:** press the canister, one puff = one count + one LED blink.
   Tap the button to flip through today / week / total.
4. **New canister:** hold the button 3 s → "NEW CAN?" → tap once to
   confirm (200 doses, canister number +1). Lifetime puff total is kept.
5. **Export the log:** plug in USB — the CSV dumps automatically. Or
   send `LOG` any time. Daily totals print as `#` comment lines after
   the CSV so the CSV itself stays clean.

## 5. Bench tests (do these before trusting it with a real dose)

1. **Count test:** 100 presses of the can, zero missed / zero double
   (v3 build notes §5 step 9). If it double-counts, the microswitch is
   bouncing past the 400 ms window — lengthen `DEBOUNCE_MS` in
   `src/main.cpp`, don't add a capacitor first.
2. **Sleep current:** meter in series with the battery, board asleep,
   screen showing. Target <30 µA. Unplug USB before measuring.
3. **Clock survival:** note `uptime_ms` from `STATUS`, unplug USB,
   wait 10 min, plug back in — uptime must have advanced ~600,000 ms
   (proves the RTC kept running through sleep). If it reset, WFI sleep
   isn't holding the RTC and the firmware needs the SYSTEMOFF fallback.
4. **Screen:** each wake refreshes in ~1–2 s; avatar + count must match
   the flash log (check over Serial after a USB boot).
5. **New canister:** hold 3 s → "NEW CAN?" → tap → 200 doses. Hold
   3 s and wait 10 s → cancels, count unchanged.
6. **Spray-pattern validation** (v3 build notes §7) still applies —
   the firmware counts presses, it does not validate the drug path.
7. **Beacon (needs keys provisioned first — see `OPENHAYSTACK.md`):**
   after provisioning, the screen shows "FIND MY ON"; `HAYSTACK_STATUS`
   over serial prints the BLE address + full payload. With a BLE
   scanner app (nRF Connect / LightBlue), look for an advert whose
   manufacturer data starts `4C 00 12 19` — that is the Find My frame.
   Leave it advertising 15 min and confirm the key (and address)
   change; then confirm a location report appears in your fetch tool
   (macless-haystack web UI / FindMy.py) — first reports can take
   minutes to hours depending on nearby Apple devices.

## 6. Honest limits

- **No RTC hardware.** Until `SETTIME` is sent, "day" = 24 h since
  first power-on and "midnight" is 24 h after first boot. The clock
  survives sleep (RTC runs) but not a dead/disconnected battery.
- **millis() wraps at ~49.7 days**; the day index restarts then.
  (A dose log spanning a wrap will show a day jump — the puff count
  itself stays correct.)
- **Sleep current is a measurement, not a promise.** The XIAO documents
  5 µA deep sleep (SYSTEMOFF). This firmware uses System ON idle + RTC
  instead (the clock is worth more than the last few µA): the nRF52840
  draws ~2 µA in System ON with RTC + RAM retention, plus the XIAO's
  regulator quiescent current, so expect roughly 5–15 µA on battery —
  comfortably under the 30 µA target, but measure it (test 2). If it
  measures high, the fallback is SYSTEMOFF (loses the clock; day
  counter would need rethinking).
- **Find My is OpenHaystack, not MFi.** Apple MFi certification is
  the gate for a *shipped* product (+$50k and Apple's approval process). This
  firmware uses the unofficial DIY route instead: it impersonates an
  AirTag's advertisement with your own keys. Apple could break it,
  locations do NOT appear in Apple's Find My app (they appear in the
  fetch tool's own map — see `OPENHAYSTACK.md`), and there is no
  anti-stalking chirp, so iPhones near it may show "unknown accessory"
  alerts. Bench prototype only — see `OPENHAYSTACK.md` §8 for the
  full caveat list.
- **E-ink BUSY polarity assumed HIGH = busy** (GDEH0154D67 standard).
  If the panel never refreshes, check BUSY with a meter first.
- **No mass-storage CSV.** Serial dump only in v1 (v3 build notes §6
  allows serial as the v1 export path).

## 7. Files

- `platformio.ini` — board, framework, GxEPD2 dependency
- `src/main.cpp` — the whole firmware (pins, sleep, debounce, UI,
  LittleFS log, serial commands, OpenHaystack beacon)
- `sim/dose_logic_sim.py` — scripted 7-day logic simulation (mirrors
  the firmware state machine; see `sim/results.md`, 10/10 cases pass)
- `OPENHAYSTACK.md` — Find My beacon: protocol, key generation,
  provisioning, retrieval, battery math, caveats
- `tools/haystack_keys.py` — key generation + USB provisioning script
  (P-224 keypairs, 0600 private-key store, serial upload)
