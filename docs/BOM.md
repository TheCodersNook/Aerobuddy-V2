# AeroBuddy v5 — Bill of Materials (Amazon only, parts only)

*(Developed as PuffPal — this is the v5 hardware, PuffPal v5 lineage.
Internal file names keep the `puffpal-v5` prefix.)*

**How to use this BOM:** search Amazon with the **exact search term** in each row.
Do not use any ASIN or URL from anywhere else — none are given here, because
Amazon listings change and I cannot browse Amazon (it is bot-walled).
**You find the listings; the parent agent verifies each one before you buy.**
Prices in the "Anchor" column are **estimates carried from report 13's BOM**
(`13-folding-find-my-concepts.md`, PuffPal A) — they are NOT live Amazon
prices and NOT quotes. Where the v2 BOM checked a non-Amazon price on
Nov 23, 2026, it is given as a sanity anchor only (flagged "v2 non-Amazon").

**Changed vs v4:** the reed switch and magnet are GONE (dose sensing is now
a microswitch — line 5, the same part as the UI button, line 6); the
experimental needle-orifice nozzle is gone (transplant only — no needles
to buy); the spacer shim is gone. Everything else is the v4 BOM unchanged.

You already have (not BOM lines): your albuterol canister, and your stock
actuator (the transplant donor — you cut the stem block out of it).

| # | Part | Qty | Exact Amazon search term | Notes | Anchor* |
|---|---|---|---|---|---|
| 1 | nRF52840 dev board — **Seeed XIAO nRF52840 (21×17.8 mm)** (recommended) or Elecrow SuperMini nRF52840 (17.8×33 mm) | 1 | `Seeed XIAO nRF52840` **or** `Elecrow SuperMini nRF52840` | Either fits the bay (sized for the larger SuperMini). Both have an onboard LiPo charger + USB-C, so no separate charger or USB breakout is needed. **Sleep-current warning (v2 BOM):** Elecrow's pages disagree (1 mA vs 20 µA standby); the XIAO documents 5 µA deep sleep. At 1 mA a 110 mAh cell lasts ~4–5 days, not months — pick the XIAO if months-per-charge matters. | $8 (report 13) |
| 2 | **E-ink panel — GooDisplay GDEH0154D67 (or GDEY0154D67), 1.54", 200×200, SSD1681, 24-pin FPC, 31.8×37.32×1.05 mm** | 1 | `GDEH0154D67 e-paper display` **or** `GDEY0154D67 e-paper display` **or** `1.54 inch e-ink display 200x200 SSD1681` | **Bare panel** — you solder the FPC or use line 3. **Buy the bare panel, NOT a Waveshare-style module** — the module's 48×33 mm driver board does not fit the shell. **Two checks before you buy:** (a) the GDEH0154D67 is discontinued — the GDEY0154D67 is the current part with the same outline (31.8×37.32, 1.0 mm thick) and pinout; either fits the pocket; (b) verify the FPC exits the panel's short (31.8 mm) edge and is ≥20 mm long — the FPC channel is sized for that. Panel dims verified from the Good Display GDEH0154D67 specification rev 2.0 (outline 37.32×31.80×1.05 mm, active 27×27 mm, weight 2.1±0.2 g). | $9 (report 13 — that anchor was already for a 1.54" panel) |
| 3 | **FPC adapter — DESPI-C154 (or similar 24-pin e-paper adapter board)** | 1 | `DESPI-C154 e-paper adapter` **or** `24 pin FPC e-ink adapter board` | Avoids soldering the fine 24-pin FPC directly. Only needed if you buy the bare panel (line 2); skip if your panel comes on a breakout that fits the bay. The adapter mounts in the bay beside the battery with a foam pad (no screws). | — (no report 13 anchor) |
| 4 | LiPo 301230, 3.7 V, 110 mAh, with JST connector + protection (PCM) | 1 | `301230 lipo battery 110mAh JST` | 30×12×3 mm — fits the battery bay (Y 42..72). The SuperMini has B+/B− solder pads (no JST socket): solder the JST leads to the pads, or cut the JST off. **Do not** substitute a larger cell — the bay is sized for 301230. | $3 (report 13, for 300 mAh — the 110 mAh should cost less; v2 non-Amazon: $6.00) |
| 5 | **Tactile push button, 6×6×5 mm — the DOSE microswitch** | 1 (sold in packs) | `6x6x5mm tactile push button switch` | **NEW in v5 — replaces the reed switch + magnet.** One switch sits in the pocket in the front bore wall, plunger facing the can; the cam rib on the press cap pushes it ~1.4 mm into the ~3.2 mm fire stroke. **This is the SAME part as the UI button (line 6) — buy one pack and use two switches.** Solder to D6 + GND (see the build notes §7 step 4). | $0.50 (report 13, button + LED together — shared with line 6) |
| 6 | Tactile push button, 6×6×5 mm — the UI button | 1 (sold in packs) | `6x6x5mm tactile push button switch` | **Same part as line 5 — one pack covers both.** Sits in the button pocket (6.2×6.2×5 mm, below the screen) behind the faceplate; **in v5 there is no printed button cap — you press the switch directly through the faceplate hole.** | (with line 5) |
| 7 | LED, 3 mm | 1 (sold in packs) | `3mm LED` | Any colour; red or green reads best through the 3.2 mm faceplate hole. Needs line 8. | (with lines 5+6) |
| 8 | Resistor, 220 Ω, 1/4 W | 1 (sold in packs) | `220 ohm resistor 1/4 watt` | LED current-limiting. | — (report 13 "passives/misc $1.70") |
| 9 | M2 screws — M2×5 (faceplate, 4), M2×8 (backplate, 4), M2×10 self-tapping (nozzle block, 2) | 10 total | `M2 screws assortment kit` (covers all three lengths) **or** `M2x5 screws`, `M2x8 screws`, `M2x10 self tapping screws` | An M2 assortment kit is the cheapest way to get all three lengths. The nozzle screws must be **self-tapping** (they cut into the printed block's 1.8 mm holes); the faceplate/backplate screws can be machine screws (they self-tap into the 1.8 mm square pilot slots). | — (report 13 "passives/misc $1.70") |
| 10 | Hinge pin — **printed (STL included) or 27 mm of 1.75 mm filament** | 1 | *(no purchase needed)* — metal option if you want it: `1.8mm steel dowel pin` or `M2x30 dowel pin` | The design uses a printed Ø1.8×27 mm pin, or a 27 mm length of the 1.75 mm filament you are already printing with (cut flush, recommended — it is the right material and free). A metal pin is optional, not required. | $0 (printed/filament) |
| 11 | Hookup wire, 30 AWG silicone (several colours) | 1 pack | `30 AWG silicone wire` | Microswitch, button, and LED runs to the board. Short runs only — a small pack is plenty. | — (report 13 "passives/misc $1.70") |
| 12 | Filament — PETG, 1.75 mm, **white + red** | 1 spool of each (~80 g used total) | `PETG filament 1.75mm white` **and** `PETG filament 1.75mm red` | **PETG is recommended, not PLA:** it resists heat (a pocket or car dashboard in summer softens PLA), does not creep under the can's spring load, has better layer adhesion for the thin hinge knuckles and the 1 mm press-cap wall, and resists cleaning solvents. PLA is fine for a fit-test print only. **Colour split (v5): white shell** — body, faceplate, backplate, hinge pin, nozzle-transplant block; **red accents** — mouthpiece, press cap. The red cross on the backplate is either a red-filament swap at that layer or red paint in the pocket (see the build notes §7 step 6) — no separate filament needed for it. | — (not in report 13 BOM — the v5 shell is printed, not moulded) |
| 13 | Two-part epoxy (5-minute or 30-minute) | 1 | `two part epoxy` | Epoxies the transplanted stem block into its pocket. (No magnet or needle to epoxy in v5.) | — (no report 13 anchor) |
| 14 | Red paint (acrylic or enamel) — **only if you paint the cross instead of a filament swap** | 1 small bottle | `red acrylic paint` | Fills the 15 mm cross pocket on the backplate (1 mm deep). Optional — a red filament swap at the cross layer does the same job with no paint. | — (no report 13 anchor) |
| 15 | USB-C breakout board | 0 — **not needed** | — | The SuperMini/XIAO's **onboard USB-C is the port** — it pokes through the slot in the bottom wall. A separate breakout would be redundant. Listed here only so the BOM is explicit about it. | $0.50 (report 13 — not spent on this build) |
| 16 | TP4056 charger module (USB-C, with protection) | 0 — **not needed for the recommended build** | `TP4056 charger module USB-C` (only if you use a bare nRF52840 module) | **Module, not IC** — a module is the practical Amazon buy (an IC alone needs a PCB). Only needed if you build with a bare nRF52840 module instead of the SuperMini/XIAO (both have onboard chargers). If you buy one, it is a spare, not a v5 part. | $0.50 (report 13 — not spent on this build) |

\* **Anchor** = report 13 estimate, NOT a live Amazon price. Add the anchors for
lines 1, 2, 4 and 5+6+7 and you get ~$20.50 — that is a rough planning number
from report 13, not a quote, and it excludes the adapter, screws, wire,
filament, epoxy, and shipping. **Check each listing's actual price before
buying; the parent agent verifies the listings you find.**

## What is NOT in this BOM (and why)

- **The canister and stock actuator** — you already own them.
- **A reed switch and magnet** — REMOVED in v5. Dose sensing is a
  microswitch (line 5); there is no reed sled and no magnet boss.
- **Blunt dispensing needles** — REMOVED in v5. The experimental
  needle-orifice nozzle variant is gone; transplant only.
- **Spacer shims** — REMOVED in v5. The nozzle pocket no longer takes a
  1 mm spacer shim.
- **A printed button cap** — REMOVED in v5. The UI button is pressed
  directly through the faceplate hole.
- **A PCB** — the prototype wires the board, panel, microswitch, button,
  and LED point-to-point (report 13's $2 PCB is a production item, not a
  prototype part).
- **An MFi / Find My tag** — the Find My feature needs Apple MFi certification
  for a shipped product (+$50k, report 13 §1.2). A bench prototype can prove
  the radio with an open-source implementation (OpenHaystack-class), which is
  not the commercial path. No tag is in this BOM.
- **The 300 mAh cell from report 13** — v5 uses the 110 mAh 301230 from the
  user's research file (the bay is sized for it). The 300 mAh cell does not fit.
- **The 0.97" panel from v3** — superseded by the 1.54" panel (line 2). The
  v3 faceplate/screen pocket does not fit the 1.54" panel; print the v5 parts.
