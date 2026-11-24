#!/usr/bin/env python3
"""AeroBuddy v2 (PuffPal v4) — battery-life duty-cycle simulation.

Cell: 301230 LiPo, 110 mAh nominal, usable ~100 mAh (regulator losses +
self-discharge + cold-weather derate, per task brief).
Panel: GDEH0154D67 1.54" 200x200 e-ink, SSD1681.
  Datasheet-anchored refresh cost: full refresh ~2 s at ~2.5 mW (report 13
  §1.4, consistent with SSD1681 panel power ~8 mW peak / ~2.5 mW avg during
  update at 3.3 V); partial refresh 0.3 s.
  Full refresh  ~1.0 uAh, partial refresh ~0.15 uAh.
Board: Seeed XIAO nRF52840 (v1 firmware: System-ON WFI sleep, RTC kept).
  Sleep: 5 uA documented deep sleep; firmware README expects 5-15 uA
  System-ON (nRF52 ~2 uA + regulator quiescent). Use 10 uA typical,
  5 uA best, 15 uA worst. SuperMini dispute: Elecrow pages say 1 mA or
  20 uA standby -- 1 mA case shown as sensitivity.
Two cases: (a) as-built v1 firmware, NO BLE; (b) Find My advertising
added later, AirTag-class 15-25 uA average (report 13 §1.4).

Wake events: each puff = reed wake + CPU active ~0.5 s @ ~5 mA (nRF52
run current) + LED blink 50 ms @ ~10 mA + e-ink partial refresh.
Button presses: CPU active ~1 s + full refresh (view change redraws).
Avatar redraw: 1 full refresh per day (face state change).
"""
import math

USABLE_MAH = 100.0          # usable capacity, mAh
SLEEP_TYP = 10e-3           # mA (10 uA) typical System-ON
SLEEP_BEST = 5e-3
SLEEP_WORST = 15e-3
SLEEP_SUPERMINI_1MA = 1.0   # mA, disputed Elecrow figure
FINDMY_LO, FINDMY_HI = 15e-3, 25e-3   # mA average (AirTag class)
FINDMY_TYP = 20e-3

# Per-event charge (mAh)
CPU_RUN_MA = 5.0            # nRF52840 run current
PUFF_CPU_S = 0.5            # wake, count, log to flash
LED_MA, LED_S = 10.0, 0.05  # 3mm LED via 220R @3.3V ~5-10 mA
FULL_REFRESH_UAH = 1.0      # ~2.5 mW x 2 s
PARTIAL_REFRESH_UAH = 0.15  # ~2.5 mW x 0.3 s (scaled by time)
PUFF_UAH = (CPU_RUN_MA * PUFF_CPU_S + LED_MA * LED_S) / 3600 * 1000 \
    + PARTIAL_REFRESH_UAH
BUTTON_UAH = (CPU_RUN_MA * 1.0) / 3600 * 1000 + FULL_REFRESH_UAH

SCENARIOS = {
    "Light":   dict(puffs=2,  buttons=1, full_refreshes=1),
    "Typical": dict(puffs=4,  buttons=3, full_refreshes=2),
    "Heavy":   dict(puffs=12, buttons=6, full_refreshes=4),
}

def daily_mah(sleep_ma, findmy_ma, sc):
    base = (sleep_ma + findmy_ma) * 24.0
    events = (sc["puffs"] * PUFF_UAH + sc["buttons"] * BUTTON_UAH
              + sc["full_refreshes"] * FULL_REFRESH_UAH) / 1000.0
    return base + events

def days(mah_per_day):
    return USABLE_MAH / mah_per_day

print(f"Per-puff charge: {PUFF_UAH:.2f} uAh  |  per-button-press: "
      f"{BUTTON_UAH:.2f} uAh  |  full refresh: {FULL_REFRESH_UAH} uAh")
print(f"Usable capacity: {USABLE_MAH} mAh")
print()
hdr = f"{'Scenario':<9} {'mAh/day':>8} {'Days':>7} {'Months':>7}"
for case, fm in [("(a) v1 firmware, NO BLE", 0.0),
                 ("(b) + Find My advertising (~20 uA avg)", FINDMY_TYP)]:
    print(case)
    print(hdr)
    for name, sc in SCENARIOS.items():
        d = daily_mah(SLEEP_TYP, fm, sc)
        print(f"{name:<9} {d:>8.3f} {days(d):>7.0f} {days(d)/30.44:>7.1f}")
    print()

print("Sensitivity (Typical use, 4 puffs/day):")
print(f"{'Sleep current':<28} {'No BLE (days)':>14} {'+Find My (days)':>16}")
for label, s in [("5 uA (XIAO doc, best)", SLEEP_BEST),
                 ("10 uA (typical)", SLEEP_TYP),
                 ("15 uA (worst, README)", SLEEP_WORST),
                 ("20 uA (SuperMini low claim)", 20e-3),
                 ("1 mA (SuperMini high claim)", SLEEP_SUPERMINI_1MA)]:
    sc = SCENARIOS["Typical"]
    print(f"{label:<28} {days(daily_mah(s, 0.0, sc)):>14.0f} "
          f"{days(daily_mah(s, FINDMY_TYP, sc)):>16.0f}")
print()
print("Find My range sensitivity (Typical, 10 uA sleep):")
for label, fm in [("15 uA", FINDMY_LO), ("20 uA", FINDMY_TYP),
                  ("25 uA", FINDMY_HI)]:
    d = daily_mah(SLEEP_TYP, fm, SCENARIOS["Typical"])
    print(f"  {label}: {days(d):.0f} days ({days(d)/30.44:.1f} months)")
print()
print("Sanity: sleep dominates. No-BLE typical: sleep = "
      f"{SLEEP_TYP*24:.2f} mAh/day of "
      f"{daily_mah(SLEEP_TYP, 0.0, SCENARIOS['Typical']):.2f} total "
      f"({100*SLEEP_TYP*24/daily_mah(SLEEP_TYP,0.0,SCENARIOS['Typical']):.0f}%).")
