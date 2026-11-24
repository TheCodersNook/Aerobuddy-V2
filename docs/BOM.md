# AeroBuddy v2 — Bill of Materials (Amazon only, parts only)

*(Developed as PuffPal — this is the v4 hardware, PuffPal v3/v4 lineage.
Internal file names keep the `puffpal-v4` prefix.)*

**How to use this BOM:** search Amazon with the **exact search term** in each row.
Do not use any ASIN or URL from anywhere else — none are given here, because
Amazon listings change and I cannot browse Amazon (it is bot-walled).
**You find the listings; the parent agent verifies each one before you buy.**
Prices in the "Anchor" column are **estimates carried from report 13's BOM**
(`13-folding-find-my-concepts.md`, PuffPal A) — they are NOT live Amazon
prices and NOT quotes. Where the v2 BOM checked a non-Amazon price on
Nov 23, 2026, it is given as a sanity anchor only (flagged "v2 non-Amazon").

**Changed vs v3:** lines 2 and 3 only (the 1.54" panel + its adapter).
Everything else is the v3 BOM unchanged.

You already have (not BOM lines): your albuterol canister, and your stock
actuator (the transplant donor — you cut the stem block out of it).

| # | Part | Qty | Exact Amazon search term | Notes | Anchor* |
|---|---|---|---|---|---|
| 1 | nRF52840 dev board — Elecrow SuperMini (17.8×33 mm) **or** Seeed XIAO nRF52840 (21×17.8 mm) | 1 | `Elecrow SuperMini nRF52840` **or** `Seeed XIAO nRF52840` | Either fits the bay (sized for the larger SuperMini). Both have an onboard LiPo charger + USB-C, so no separate charger or USB breakout is needed. **Sleep-current warning (v2 BOM):** Elecrow's pages disagree (1 mA vs 20 µA standby); the XIAO documents 5 µA deep sleep. At 1 mA a 110 mAh cell lasts ~4–5 days, not months — pick the XIAO if months-per-charge matters. | $8 (report 13) |
| 2 | **E-ink panel — GooDisplay GDEH0154D67 (or GDEY0154D67), 1.54", 200×200, SSD1681, 24-pin FPC, 31.8×37.32×1.05 mm** | 1 | `GDEH0154D67 e-paper display` **or** `GDEY0154D67 e-paper display` **or** `1.54 inch e-ink display 200x200 SSD1681` | **CHANGED in v4 — was the 0.97" GDEM0097T61.** **Bare panel** — you solder the FPC or use line 3. **Buy the bare panel, NOT a Waveshare-style module** — the module's 48×33 mm driver board does not fit the shell. **Two checks before you buy:** (a) the GDEH0154D67 is discontinued — the GDEY0154D67 is the current part with the same outline (31.8×37.32, 1.0 mm thick) and pinout; either fits the pocket; (b) verify the FPC exits the panel's short (31.8 mm) edge and is ≥20 mm long — the FPC channel is sized for that. Panel dims verified from the Good Display GDEH0154D67 specification rev 2.0 (outline 37.32×31.80×1.05 mm, active 27×27 mm, weight 2.1±0.2 g). | $9 (report 13 — that anchor was already for a 1.54" panel) |
| 3 | **FPC adapter — DESPI-C154 (or similar 24-pin e-paper adapter board)** | 1 | `DESPI-C154 e-paper adapter` **or** `24 pin FPC e-ink adapter board` | **CHANGED in v4 — was the 18-pin DESPI-C097.** Avoids soldering the fine 24-pin FPC directly. Only needed if you buy the bare panel (line 2); skip if your panel comes on a breakout that fits the bay. The adapter mounts in the bay beside the battery with a foam pad (no screws). | — (no report 13 anchor) |
| 4 | LiPo 301230, 3.7 V, 110 mAh, with JST connector + protection (PCM) | 1 | `301230 lipo battery 110mAh JST` | 30×12×3 mm — fits the battery bay (Y 42..72). The SuperMini has B+/B− solder pads (no JST socket): solder the JST leads to the pads, or cut the JST off. **Do not** substitute a larger cell — the bay is sized for 301230. | $3 (report 13, for 300 mAh — the 110 mAh should cost less; v2 non-Amazon: $6.00) |
| 5 | Reed switch, glass, 2×14 mm, normally open (NO) | 1 (sold as 10-pack) | `2x14mm reed switch normally open` | You need 1; the 10-pack is the usual sale unit. Goes in the reed sled (2.4 mm pocket). | $0.50 (report 13, reed + magnet together) |
| 6 | Magnet, N52 neodymium disc, 6×2 mm | 1 (sold in packs) | `N52 neodymium magnet 6x2mm disc` | Goes in the press-cap magnet boss (6 mm pocket). | (with line 5) |
| 7 | Tactile push button, 6×6×5 mm | 1 (sold in packs) | `6x6x5mm tactile push button switch` | Sits in the button pocket (6.2×6.2×5 mm, now at Y ~47, below the screen) behind the faceplate; the printed button cap presses it. | $0.50 (report 13, button + LED together) |
| 8 | LED, 3 mm | 1 (sold in packs) | `3mm LED` | Any colour; red or green reads best through the 3.2 mm faceplate hole. Needs line 9. | (with line 7) |
| 9 | Resistor, 220 Ω, 1/4 W | 1 (sold in packs) | `220 ohm resistor 1/4 watt` | LED current-limiting. | — (report 13 "passives/misc $1.70") |
| 10 | M2 screws — M2×5 (faceplate, 4), M2×8 (backplate, 4), M2×10 self-tapping (nozzle block, 2) | 10 total | `M2 screws assortment kit` (covers all three lengths) **or** `M2x5 screws`, `M2x8 screws`, `M2x10 self tapping screws` | An M2 assortment kit is the cheapest way to get all three lengths. The nozzle screws must be **self-tapping** (they cut into the printed block's 1.8 mm holes); the faceplate/backplate screws can be machine screws (they self-tap into the 1.8 mm square pilot slots). | — (report 13 "passives/misc $1.70") |
| 11 | Hinge pin — **printed (STL included) or 27 mm of 1.75 mm filament** | 1 | *(no purchase needed)* — metal option if you want it: `1.8mm steel dowel pin` or `M2x30 dowel pin` | The design uses a printed Ø1.8×27 mm pin, or a 27 mm length of the 1.75 mm filament you are already printing with (cut flush, recommended — it is the right material and free). A metal pin is optional, not required. | $0 (printed/filament) |
| 12 | Blunt dispensing needles, 23G (OD ~0.64 mm, ID ~0.34 mm) — **EXPERIMENTAL VARIANT ONLY** | 1 pack | `23G blunt tip dispensing needle` | **Only for the printed needle-orifice nozzle variant** (`puffpal-v4-nozzle-printed.stl`). NOT needed for the recommended transplant variant. See the build notes: the needle variant changes the drug path and must be validated before any real use. | — (no report 13 anchor; v2 BOM listed it in unpriced bench parts ~$2–4 for the kit) |
| 13 | Hookup wire, 30 AWG silicone (several colours) | 1 pack | `30 AWG silicone wire` | Reed switch, button, and LED runs to the board. Short runs only — a small pack is plenty. | — (report 13 "passives/misc $1.70") |
| 14 | Filament — PETG, 1.75 mm | 1 spool (~80 g used) | `PETG filament 1.75mm` | **PETG is recommended, not PLA:** it resists heat (a pocket or car dashboard in summer softens PLA), does not creep under the can's spring load, has better layer adhesion for the thin hinge knuckles and the 1 mm press-cap wall, and resists cleaning solvents. PLA is fine for a fit-test print only. Any colour; matte reads least like a vape. | — (not in report 13 BOM — the v4 shell is printed, not moulded) |
| 15 | Two-part epoxy (5-minute or 30-minute) | 1 | `two part epoxy` | Epoxies the transplanted stem block into its pocket, the magnet into its boss, and (experimental) the needle into its pocket. | — (no report 13 anchor) |
| 16 | USB-C breakout board | 0 — **not needed** | — | The SuperMini/XIAO's **onboard USB-C is the port** — it pokes through the slot in the bottom wall (stage A0). A separate breakout would be redundant. Listed here only so the BOM is explicit about it. | $0.50 (report 13 — not spent on this build) |
| 17 | TP4056 charger module (USB-C, with protection) | 0 — **not needed for the recommended build** | `TP4056 charger module USB-C` (only if you use a bare nRF52840 module) | **Module, not IC** — a module is the practical Amazon buy (an IC alone needs a PCB). Only needed if you build with a bare nRF52840 module instead of the SuperMini/XIAO (both have onboard chargers). If you buy one, it is a spare, not a v4 part. | $0.50 (report 13 — not spent on this build) |

\* **Anchor** = report 13 estimate, NOT a live Amazon price. Add the anchors for
lines 1, 2, 4, 5+6, 7+8 and you get ~$21 — that is a rough planning number
from report 13, not a quote, and it excludes the adapter, screws, wire,
filament, epoxy, and shipping. **Check each listing's actual price before
buying; the parent agent verifies the listings you find.**

## What is NOT in this BOM (and why)

- **The canister and stock actuator** — you already own them.
- **A PCB** — the prototype wires the board, panel, reed, button, and LED
  point-to-point (report 13's $2 PCB is a production item, not a prototype
  part).
- **An MFi / Find My tag** — the Find My feature needs Apple MFi certification
  for a shipped product (+$50k, report 13 §1.2). A bench prototype can prove
  the radio with an open-source implementation (OpenHaystack-class), which is
  not the commercial path. No tag is in this BOM.
- **The 300 mAh cell from report 13** — v4 uses the 110 mAh 301230 from the
  user's research file (the bay is sized for it). The 300 mAh cell does not fit.
- **The 0.97" panel from v3** — superseded by the 1.54" panel (line 2). The
  v3 faceplate/screen pocket does not fit the 1.54" panel; print the v4 parts.
