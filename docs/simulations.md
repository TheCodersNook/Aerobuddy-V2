# AeroBuddy v2 (PuffPal v4) — Simulations

Date: 2026-11-23. All numbers below are scripted (scripts in this folder);
nothing here is hand-waved. Raw script outputs are saved alongside
(`battery_out.txt`, `montecarlo_out.txt`, `market_out.txt`,
`structural_out.txt`). Sims are stated-assumption simulations, not
forecasts and not measurements — the inputs that are still unmeasured
are flagged in §6.

## 1. Battery life — duty-cycle model (`battery_sim.py`)

Cell: 301230 LiPo, 110 mAh nominal, **100 mAh usable** (regulator losses,
self-discharge, cold derate). Panel: GDEH0154D67 1.54" e-ink — full refresh
~2 s at ~2.5 mW ≈ 1.0 µAh, partial refresh ~0.15 µAh (report 13 §1.4
anchors). Board: XIAO nRF52840, System-ON WFI sleep, 10 µA typical
(firmware README: expect 5–15 µA; XIAO documents 5 µA deep sleep).
Per puff ≈ 0.98 µAh (CPU 0.5 s @ 5 mA + LED blink + partial refresh);
per button press ≈ 2.39 µAh (CPU 1 s + full refresh).

### (a) As-built v1 firmware — NO BLE

| Scenario | Puffs/day | mAh/day | Days | Months |
|---|---|---|---|---|
| Light | 2 | 0.245 | 408 | 13.4 |
| Typical | 4 | 0.253 | 395 | 13.0 |
| Heavy | 12 | 0.270 | 370 | 12.2 |

Sleep dominates: at typical use, sleep is 0.24 of 0.25 mAh/day (95%).
Usage barely matters — the device is a battery with a screen attached.

### (b) With Find My advertising added later (~20 µA avg, AirTag-class)

| Scenario | Puffs/day | mAh/day | Days | Months |
|---|---|---|---|---|
| Light | 2 | 0.725 | 138 | 4.5 |
| Typical | 4 | 0.733 | 136 | 4.5 |
| Heavy | 12 | 0.750 | 133 | 4.4 |

Find My costs ~2/3 of the battery life: ~13 months → ~4.5 months.
Find My range (typical use): 15 µA → 5.4 mo; 20 µA → 4.5 mo; 25 µA →
3.9 mo. Note the 110 mAh cell is 1/3 of report 13's 300 mAh cell — that
is why (b) lands at 4.5 months vs report 13's 4–6 months despite the
same radio budget: the smaller cell and the same ~30 µA total draw
roughly cancel to the same answer. The honest headline: **months per
charge in both cases; ~13 months without Find My, ~4.5 months with it.**

### Sleep-current sensitivity (typical use)

| Sleep current | No BLE | + Find My |
|---|---|---|
| 5 µA (XIAO doc) | 751 days | 163 days |
| 10 µA (typical) | 395 days | 136 days |
| 15 µA (README worst) | 268 days | 117 days |
| 20 µA (SuperMini low claim) | 203 days | 103 days |
| **1 mA (SuperMini high claim)** | **4 days** | **4 days** |

The SuperMini dispute is the whole ballgame: at 1 mA the cell lasts
4 days regardless of Find My. **Measure sleep current on the real board
before trusting any number here** (firmware README bench test 2).

## 2. Tolerance Monte Carlo (`tolerance_montecarlo.py`, N = 100,000)

Assumptions (stated; no measured data exists yet — caliper measurements
are still owed, v4 build notes §4):
- Can OD: Normal(22.10, 0.10) mm, truncated [21.8, 22.4] ("generics vary").
- FDM bore error: Normal(0, 0.10) mm for the can bore; Normal(0, 0.08) mm
  for the nozzle-block socket (0.12 mm layers). Also run with a −0.10 mm
  mean bias (FDM holes print small).
- Stem OD: Normal(3.16, 0.03) mm (Aptar DF30 moulded stem).

| Fit | Nominal clearance | P(fail) | 95% clearance band |
|---|---|---|---|
| Can in 22.80 mm bore (jam, ≤0) | 0.70 mm diametral | **0.000%** (0/100k) | [0.42, 0.98] mm |
| Can, stiff slide (<0.20 mm) | — | 0.02% | — |
| Can, with −0.10 mm hole bias | — | 0.000% | [0.32, 0.88] mm |
| Stem in 3.30 mm socket, too tight (≤0.02 mm) | 0.14 mm | **8.02%** | [−0.03, 0.31] mm |
| Stem in socket, too loose (≥0.30 mm) | — | 3.02% | — |
| Stem, any fail | — | **11.04%** | — |
| Stem, with −0.10 mm hole bias, too tight | — | **40.77%** | — |

**Reading:** the can bore is safe — 0.35 mm/side radial clearance swallows
can variation and print error together. The printed stem socket is NOT
safe: 11% fail even unbiased, 41% too-tight if holes print 0.10 mm small
(as FDM holes usually do). **This is the quantitative case for the
transplant variant as primary** — it keeps the stock socket, so the stem
Monte Carlo does not apply to it at all. The printed needle-orifice
variant needs either a reamed socket (drill/ream the Ø3.3 bore after
printing) or a socket bore opened to ~3.45 mm with a leak check — do not
print-and-hope at 3.30 mm.

## 3. Market simulation re-run (`market_sim_v4.py`)

Report 07 model exactly: population 8M (adult pMDI + iPhone, Apple-only
Find My), retail $99, ladder tier $50–$99 (bear 0.03% / base 0.10% /
bull 0.25%), Y1 cap 50,000, Y3 multiples 2×/4×/8×, fixed $200k
($150k launch + $50k MFi/Find My cert), contribution = retail − BOM − $10.

What changed vs report 13 PuffPal: **nothing in the demand side**
(population, price, ladder identical), so units and revenue are identical.
Panel cost unchanged ($9 anchor was already for a 1.54" panel — v3's 0.97"
was the deviation; v4 returns to the report-13 panel). BOM moves only
slightly: v4 prototype BOM ≈ **$30.70** (adapter ~$3 + filament ~$2 +
screws/epoxy ~$2, minus dropped PCB/charger/USB-C/RTC lines) vs report
13's $30; **$33.70** if the 110 mAh cell costs the $6 v2 non-Amazon check
instead of the $3 anchor.

| Scenario | Y1 units | Y1 revenue | Y3 units | Y3 revenue |
|---|---|---|---|---|
| Bear | 2,400 | $237,600 | 4,800 | $475,200 |
| Base | 8,000 | **$792,000** | 32,000 | $3,168,000 |
| Bull | 20,000 | $1,980,000 | 160,000 | $15,840,000 |

| BOM case | Contribution | Break-even |
|---|---|---|
| Report 13 ($30.00) | $59.00 | 3,390 units |
| v4 anchor ($30.70) | $58.30 | **3,431 units** |
| v4, $6 cell ($33.70) | $55.30 | 3,617 units |

Break-even is reached in the base case (8,000 units) under all three BOM
cases and missed in the bear case (2,400) — same verdict as report 13.
Stated-assumption simulation, NOT a forecast (report 07 §7); the ladder
is still the dominant judgment call.

## 4. Firmware logic simulation — already done, incorporated here

Done by the firmware agent; NOT redone. Script:
`~/workspace/asthma-tech/puffpal-firmware/sim/dose_logic_sim.py`,
results: `~/workspace/asthma-tech/puffpal-firmware/sim/results.md`.
**10/10 cases pass:** switch-bounce burst (12 edges/33 ms), rapid
double-press (200 ms vs 600 ms), normal day (4 puffs), midnight rollover,
long-press reset + confirm timeout, heavy-use day (12 puffs), empty
canister (200 + 5 presses), button view cycling (4 taps), SETTIME
midnight split (UTC days), 7-day week totals. Proven: no double-counts,
no missed doses, correct daily/weekly totals, correct reset behaviour
(confirm resets, timeout cancels, lifetime total kept).

## 5. Structural sanity — ANALYTIC HAND-CALCS, NOT FEA

`structural_sanity.py`. Closed-form beam/shear/bearing checks with
PETG FDM properties (Z/layer-adhesion strength 25 MPa assumed;
allowables 12 MPa peak, 8 MPa cyclic after knockdowns for print
variance). Design loads: 25 N fire force (typical pMDI actuation),
15 N worst-case fold yank at the 28 mm mouthpiece lever.

| Check | Stress | Allowable | Margin |
|---|---|---|---|
| Hinge pin shear (15 N yank) | 3.12 MPa | 12 MPa | 3.8× |
| Knuckle bearing on pin | 1.71 MPa | 12 MPa | 7.0× |
| Knuckle root bending (15 N yank) | 11.11 MPa | 12 MPa | **1.1×** |
| Press-cap wall compression (25 N) | 0.33 MPa | 8 MPa | 24.5× |
| Can-bore wall, off-axis press | 0.05 MPa | 8 MPa | 160× |
| Faceplate pocket floor, button 5 N | 3.83 MPa | 8 MPa | 2.1× |
| Nozzle-block screw shear (25 N / 2) | 6.22 MPa | 12 MPa | 1.9× |
| Nozzle-block screw pull-out | 0.50 MPa | 8 MPa | 16.1× |

**Reading:** the press path is comfortable (≥16× except the screws at
1.9×, which is conservative — the block also bears on the pocket floor).
The thinnest margin is the **hinge knuckle root, 1.1× at a 15 N yank** —
but 15 N is a hard yank; at a realistic 5 N fold force it is 3.2×. The
hinge is the part to cycle-test (1,000 folds, build notes §10), not the
press path. Real risks are fatigue and creep, not one-off yield — hence
PETG, not PLA. **What a real test must still prove:** 1,000 fold cycles
without knuckle cracking, 100 presses without screw loosening, and a
drop test (not modelled here at all — impact loads are outside hand-calc
scope). Do not quote these margins as FEA; they are order-of-magnitude
sanity checks.

## 6. What is too uncertain to trust

1. **Battery life — sleep current unmeasured.** Everything in §1 pivots
   on the board's real sleep current (5 µA vs 1 mA = 751 days vs 4 days).
   Measure it (firmware README test 2) before quoting months.
2. **Monte Carlo — distributions are assumed, not measured.** No can OD,
   stem OD, or print-error data exists yet. The can-bore verdict is robust
   (0% jam even with bias); the stem-socket verdict is directionally
   robust (too tight is the failure mode) but the exact 11%/41% figures
   move with the assumed sigmas. Caliper the can + stem, print a test
   bore, then re-run with measured sigmas.
3. **Market sim — the adoption ladder is a judgment call** (report 07 §6).
   No $99 DTC asthma device has launched at scale; the ladder is anchored
   to SOM, not measured demand.
4. **Structural — analytic only, no fatigue/drop data.** PETG Z-strength
   varies 20–30 MPa by printer/settings; margins scale with it.

## Files

- `battery_sim.py` / `battery_out.txt`
- `tolerance_montecarlo.py` / `montecarlo_out.txt`
- `market_sim_v4.py` / `market_out.txt`
- `structural_sanity.py` / `structural_out.txt`
- `simulations.md` (this report)
- Firmware logic sim (external, by firmware agent):
  `~/workspace/asthma-tech/puffpal-firmware/sim/dose_logic_sim.py`
  and `.../sim/results.md`
