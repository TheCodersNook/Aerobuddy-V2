#!/usr/bin/env python3
"""AeroBuddy v2 (PuffPal v4) — market simulation re-run.

Model: report 07 EXACTLY.
  - Adoption ladder by retail tier; $99 -> $50-99 tier:
    bear 0.03%, base 0.10%, bull 0.25%.
  - Y1 units = population x rate, capped at 50,000.
  - Y3 = Y1 x {bear 2x, base 4x, bull 8x}. Price constant.
  - Revenue = units x retail. No subscription.
  - Contribution = retail - BOM - $10 (assembly/pack/ship/returns).
  - Fixed = $150k launch + $50k MFi/Find My cert (report 13 PuffPal).
    Reg 4 (wellness path) -> no +$100k 510(k) adder.
  - Break-even = fixed / contribution.

What changed vs report 13 PuffPal (R17):
  - Population: SAME 8M (adult pMDI users with iPhone; Apple-only Find My).
  - Retail: SAME $99.
  - Panel: SAME 1.54" panel, SAME $9 anchor (report 13's anchor was
    already for a 1.54" panel; v3's 0.97" was the deviation, v4 returns
    to the report-13 panel). So the panel change costs $0 vs report 13.
  - BOM: report 13 PuffPal BOM was $30 (incl. $4 injection-moulded sleeve,
    $2 PCB, $0.50 TP4056, $0.50 USB-C, $0.30 RTC crystal, $3 300 mAh cell).
    v4 prototype BOM (Amazon parts, printed shell): nRF board $8, panel $9,
    110 mAh cell $3 (anchor; v2 non-Amazon check $6), reed+magnet $0.50,
    button+LED $0.50, FPC adapter ~$3 (no report-13 anchor; DESPI-C154
    class adapter, estimate), passives/misc $1.70, filament ~$2 (80 g
    PETG at ~$25/kg), screws ~$1, epoxy ~$1. On-board charger + USB-C
    mean TP4056/USB-C/PCB lines drop out ($0). No RTC crystal (nRF RTC).
    No moulded sleeve (printed shell replaces $4 sleeve with ~$2 filament).
    => v4 BOM ~$30.70 at the $3 cell anchor, ~$33.70 at the $6 cell check.
    Report 13 BOM was $30. Net change: +$0.70 to +$3.70 -- essentially
    flat; the adapter + filament + screws/epoxy roughly cancel the
    dropped PCB/charger/USB-C/RTC lines.
  - Fixed: SAME $200k (MFi unchanged).

Run three BOM cases: $30 (report 13), $30.70 (v4 anchor), $33.70 (v4 w/
$6 cell). Contribution and break-even shift slightly; units/revenue are
unchanged (population, price, ladder identical).
"""
POP = 8_000_000
RETAIL = 99
LADDER = {"Bear": 0.0003, "Base": 0.0010, "Bull": 0.0025}
Y3_MULT = {"Bear": 2, "Base": 4, "Bull": 8}
FIXED = 200_000
CAP = 50_000

for bom_label, bom in [("Report 13 BOM $30.00", 30.00),
                       ("v4 BOM $30.70 (anchor)", 30.70),
                       ("v4 BOM $33.70 ($6 cell)", 33.70)]:
    contrib = RETAIL - bom - 10
    be = FIXED / contrib
    print(f"{bom_label}: contribution ${contrib:.2f}/unit, "
          f"break-even {be:,.0f} units")
print()
print(f"{'Scenario':<8} {'Y1 units':>9} {'Y1 revenue':>12} "
      f"{'Y3 units':>9} {'Y3 revenue':>12}")
for name, rate in LADDER.items():
    y1 = min(POP * rate, CAP)
    y3 = y1 * Y3_MULT[name]
    print(f"{name:<8} {y1:>9,.0f} ${y1*RETAIL:>11,.0f} "
          f"{y3:>9,.0f} ${y3*RETAIL:>11,.0f}")
print()
print("Break-even vs Y1 base (8,000 units): reached in base case for all")
print("three BOM cases; missed in bear (2,400 units).")
print()
print("NOTE: stated-assumption simulation, NOT a forecast (report 07 §7).")
print("Units/revenue identical to report 13 PuffPal because population,")
print("price and ladder are unchanged; only BOM/contribution moved.")
