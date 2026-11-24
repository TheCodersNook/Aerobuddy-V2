# AeroBuddy v2 — Build Notes

*(Developed as PuffPal — this is the v4 hardware, PuffPal v3/v4 lineage.
Internal file names keep the `puffpal-v4` prefix.)*

**Personal prototype, not a medical device.** This is a replacement actuator
for your own albuterol inhaler, built for prototyping. It is not tested,
certified, or approved as a medical device. Do not give it to anyone else,
do not sell it, and do not trust it with a real dose until you have validated
it as described in §8.

## 1. What v4 is, and what changed vs v3

v4 is the v3 replacement-actuator design (bare can + printed nozzle block)
with a bigger screen and a usability pass. Same architecture, same folded
size:

| | v3 (measured from STLs) | v4 (measured from STLs) |
|---|---|---|
| Folded size | 50.0 × 94.0 × 38.0 mm | **50.0 × 94.0 × 38.0 mm (unchanged)** |
| Folded, inches | 1.97" × 3.70" × 1.50" | **1.97" × 3.70" × 1.50"** |
| Fits a 4" × 2" box? | Yes | **Yes — 3.70" × 1.97"** |
| Screen | 0.97" GDEM0097T61 (30×14.15 mm, 184×88) | **1.54" GDEH0154D67 / GDEY0154D67 (31.8×37.32 mm, 200×200)** |
| Screen pocket | X 9.7..40.3, Y 72.5..87.25 | **X 8.8..41.2, Y 50.9..88.9 (32.4×38.0×1.4 mm)** |
| Screen window | 22.26×10.65 mm | **27×27 mm (active area)** |
| FPC | 18-pin, exits panel side, slot X 35..40 | **24-pin, exits panel bottom edge, channel X 17..32 below the panel** |
| Button + LED | At Y 65, beside the screen | **At Y 47.15, below the screen** |
| Button/LED holes | **Overlapped (v3 defect: centres 4.9 apart, radii sum 5.45)** | **Separated (centres 6.4 apart — fixed)** |
| Press cap | Top 3.1 mm below the rim, flat pad | **Top 1.1 mm below the rim, Ø16×0.7 mm finger dish** |
| Spray tunnel | Y 6.1..18 (A1b had no tunnel cut) | **Y 4..18 — fixes a hinge catch (see below)** |
| Shell stages | 20 | **18 (C2c/D1/D2/D3 merged into D1b/D4a/D4b)** |

**The 1.54" panel fits without widening the body.** Portrait orientation
(31.8 mm across the 50 mm face, 37.32 mm tall) leaves 8.8 mm of shell on
each side of the pocket. The width stays 50.0 mm — 0.8 mm under the 2"
limit, still the tight dimension. Thickness stays 38 mm (same cap-bore +
mouthpiece-pocket floor as v3 — see the v3 notes §1).

**Two v3 defects fixed in v4 (found by the v4 verification, not by the
user):**

1. **Hinge catch (latent since v3).** The spray tunnel started at Y=6.1
   (stage A1c) because stage A1b [3.4, 6.1] had no tunnel cut. The
   mouthpiece stub tip swings back 0.39 mm past the pocket's back wall
   (Z=9) in the first ~8° of unfold — and at Y 5.5..6.1 that region was
   solid A1b. The mouthpiece would catch (0.38 mm interference) every
   time it started to unfold or finished folding. v4 extends the tunnel
   cut into A1b (tunnel now Y 4..18). Hinge sweep re-verified
   collision-free at 0–90° in 5° steps (see §9).
2. **Button/LED faceplate holes overlapped in v3.** The button hole
   (Ø7.7 at (21.1, 65.1)) and LED hole (Ø3.2 at (26, 65)) were 4.9 mm
   apart with radii summing to 5.45 mm — the holes merged into a
   figure-8, so the button cap could tilt into the LED hole. v4 moves
   the LED to 6.4 mm from the button centre (27.5, 47.15) and verifies
   the material between the holes is solid.

**What did NOT change:** can well, nozzle blocks (both variants), hinge,
mouthpiece, reed sled, back plate, board/battery bay, USB slot, screw
positions. If you printed v3 parts, only the body, faceplate, and press
cap need reprinting for v4 (plus the button cap is the same part, just
mounted lower).

## 2. The 1.54" panel — verified dimensions

Panel: **GooDisplay GDEH0154D67** (or its successor **GDEY0154D67** —
same outline and pinout; the GDEH is discontinued). Verified from the
Good Display **GDEH0154D67 Specification rev 2.0** (Jun 5, 2019),
§1.3–1.4:

| Dimension | Value |
|---|---|
| Outline | 37.32 (H) × 31.80 (V) × 1.05 (D) mm |
| Active area | 27.00 × 27.00 mm |
| Resolution | 200 × 200 px, 0.135 mm pitch (188 dpi) |
| Controller | SSD1681 (on-glass), 4-wire SPI, partial refresh 0.3 s / full 2 s |
| FPC | 24-pin, 0.5 mm pitch, 12.50 mm wide (11.50 mm contact span) |
| Weight | 2.1 ± 0.2 g |

FPC exit edge: the datasheet's mechanical drawing is a garbled scan in
the copy I could fetch — the FPC position is NOT cleanly readable from
it. The design assumes the FPC exits the panel's **short (31.8 mm)
edge**, centred, as on the Waveshare 1.54" module that carries this
panel. The FPC channel (X 17..32, 15 mm wide for a 12.5 mm FPC) has
±1.25 mm of lateral play. **Verify on receipt:** if your panel's FPC
exits a long edge instead, route it through the wire channel
(X 17..28.5) — the channel is wide enough — and check the FPC is
≥20 mm long before soldering (panel bottom to board is ~15 mm + bends).

## 3. De-vape choices (unchanged from v3)

- **Pebble/slab, not a tube.** 50 mm wide × 38 mm thick with 8 mm corner
  radii — a small power bank, not a vape pen. Flat top and bottom, no
  cylindrical body anywhere.
- **Mouthpiece folds flush.** Folded, the mouthpiece's front face is at Z=0,
  exactly flush with the faceplate — no protruding tube at all.
- **Flattened oval mouthpiece.** Unfolded it is 20×9 mm outside, 16×6 mm
  channel — the flattened-oval cross-section of an inhaler mouthpiece, not
  a round vape tip.
- **Screen on the broad flat face.** The 27×27 mm e-ink window is on the
  50 mm-wide face, not on a narrow edge.
- **No pocket clip.** A lanyard hole (4 mm square) in the back plate instead.
- **Matte two-tone** is the intended finish (matte PETG, faceplate a
  different colour from the shell). No branding, nothing that glows except
  the small LED.

## 4. Measurements to verify BEFORE printing the nozzle block

These dimensions are **estimated** in the research file. Measure your own
can with calipers and update the `P` parameters in `jscad/puffpal-v4.js`
before printing either nozzle block. The rest of the body is tolerant of
±0.5 mm; the nozzle block is not.

| Parameter (in `P`) | Estimated value | What to measure on your can |
|---|---|---|
| `valve_stack_h` | 12.0 mm | Base of can → ferrule face (the valve stack height) |
| `stem_protrusion` | 10.0 mm | Ferrule face → stem tip |
| `stem_socket_h` | 6.5 mm | Socket depth — verify the stem seats fully without bottoming |
| `fire_stroke` | ~3.2 mm | How far the can travels from rest to fire (press slowly, feel the click) |
| `can_od` | 22.1 mm | Can body diameter (verify — generics vary) |
| `stem_od` | 3.16 mm | Stem diameter (Aptar DF30 RCU nominal) |

Also measure: the ferrule OD (assumed Ø20 — the relief is Ø20.5, so you
have 0.25 mm/side), and your transplant cut (the pocket is 12×12×10 mm
default — **measure your cut** and edit `transplant_x/y/z` if it differs).
If any measured value differs by more than 0.5 mm from the estimate, change
the parameter and re-export (`node export-v4.js`) — do not print and hope.

## 5. Usability audit (design audit — no user testing was done)

This is a design audit against the CAD, not user-test results. Nobody has
held this device yet. What the geometry says:

**Press cap — fixed in v4, good now.** The fire force is set by the can's
own valve spring (same as a stock inhaler — the cap is just a bigger
button), and the travel is the can's ~3.2 mm fire stroke. v3's cap top
sat 3.1 mm below the body rim, so a fingertip had to reach into the Ø25
bore to press it. v4 raises the pad (top now 1.1 mm below the rim) and
adds a Ø16 × 0.7 mm finger dish, so the pad meets the fingertip at the
rim and the dish centres it. The Ø24.4 pad spreads the load over more
fingertip area than the stock can's bare top, so the same force feels
slightly lighter. One-handed press (grip the 50×38 body, press with the
index finger or thumb) is feasible — the body is wide enough to palm.

**Button + LED — reachable, and the v3 overlap is fixed.** The button
sits at Y 47.15 on the front face, mid-device — a thumb of the holding
hand reaches it without shifting grip. The cap is Ø7.5 in a Ø7.7 hole
with ~0.5 mm travel (standard 6×6×5 tactile). The LED is 6.4 mm to its
right, visible while pressing. In v3 the two faceplate holes merged
(§1) — fixed.

**Mouthpiece — shape is right, edges need finishing.** The flattened
oval (20×9 mm outside, 16×6 mm channel, 28 mm long) matches a stock
inhaler mouthpiece profile and gives an adequate lip seal. Two notes:
(1) the tip is rigid PETG with printed layer edges — **sand the tip
edges smooth** (220-grit, then wash) before first use, or the rim will
feel sharp on the lips; (2) there is **no detent** at either end of the
swing — the mouthpiece is held folded/unfolded by hinge friction only,
so it can flop open in a bag. A detent bump is a future-revision item,
not in v4.

**Screen readability — the big win.** 27×27 mm active at 200×200
(0.135 mm/px, 188 dpi). Sensible minimum sizes at arm's length:
dose-count digits **8–12 mm tall** (60–90 px — huge, readable at a
glance); labels/status text **≥3.5 mm** (26 px); nothing below ~3 mm
(22 px). The avatar face can use up to ~20×20 mm of the square. The
2 s full refresh is slow for a counter update — firmware should use
partial refresh (0.3 s) for the count and reserve full refresh for the
avatar redraw.

**One-handed fold/unfold — the weakest point.** Folding is easy (push
the mouthpiece back into its pocket). Unfolding one-handed is awkward:
folded, the mouthpiece tip is flush with the face (Z=0) with no finger
purchase — you need a fingernail under the tip edge or a second hand.
A finger notch was considered and rejected: the only clean spot is
behind the faceplate (which would cover it), and a notch in the pocket's
back wall would leave 0.6 mm of wall to the can bore. Known limitation;
two hands or a fingernail for now.

**Weight — honest number: ~115–120 g.** Computed, not weighed:
printed parts 86.6 cm³ solid volume (measured from the STLs) at PETG
1.27 g/cm³, derated for 25% infill + 4 walls (≈65% of solid for the
thin-walled body, 80% for plates, 100% for the nozzle block) ≈ **79 g**;
canister ≈ 25 g (estimate — weigh yours); panel 2.1 g (datasheet);
board ≈ 3 g, battery ≈ 3 g, adapter ≈ 2 g, screws/magnet/reed/button/
LED/wire ≈ 4 g. A stock inhaler is ~50 g, so this is roughly **2.3× a
stock inhaler** — a pocket brick, not a pocket afterthought. The weight
is the price of the screen, the folding mechanism, and the battery.
If that bothers you in use, the only real diet is a smaller battery or
a thinner shell — both are future revisions.

**Anything that would make a real user put it down:** the weight and
38 mm thickness (it will not disappear in a pocket like a stock
inhaler); the mouthpiece flopping open in a bag (no detent); the
2 s full-refresh screen feeling laggy if firmware updates the whole
display per dose; and the one-handed unfold. None of these is a
print-stopper — they are the known costs of this architecture.

## 6. Print settings

**Material: PETG** (see the BOM for why). PLA is fine for a fit-test print
only — it creeps under the can's spring load and softens in heat.

| Part | Orientation | Layer height | Walls / infill | Supports |
|---|---|---|---|---|
| Body | Standing on its bottom (Y=0 down) | 0.20 mm | 4 walls, 25% | Tree supports (bay ceiling, pocket ceilings, tunnel). Remove through the back opening. |
| Faceplate | Flat, front (Z=0) down | 0.20 mm | 4 walls, 25% | None |
| Back plate | Flat, Z=33 down | 0.20 mm | 4 walls, 25% | None |
| Mouthpiece | Standing on hinge end | 0.20 mm | 4 walls, 25% | Tree (hinge arms only) |
| Press cap | Open face up (cup opening up) | 0.20 mm | 4 walls, 25% | None |
| Nozzle block (either) | Standing, socket/pocket up | **0.12 mm** | 4 walls, **100%** | None (the Ø4 tunnel bridges) |
| Reed sled | Flat | 0.20 mm | 3 walls, 25% | None |
| Spacer (×3) | Flat | 0.20 mm | 3 walls, 100% | None |
| Button cap | Hat down, stem up | 0.20 mm | 3 walls, 100% | None |
| Hinge pin | **Use 27 mm of 1.75 mm filament instead** (cut flush). The printed pin is a backup — print it flat with a brim if you use it. | — | — | — |

**Combined plate:** `stl/puffpal-v4-combined.stl` lays all parts out on a
202.5 × 97.5 mm plate (fits a 220×220 mm bed), minimum inter-part gap
2.50 mm (measured). Print time estimate: ~6–8 h at 0.20 mm (body dominates).
Filament: ~80 g total (see §5 for the weight build-up).

**Known multi-shell note:** the body STL has 23 shells (18 stages +
2 knuckles + 2 bosses + 1 island) that overlap by 0.5–1.5 mm — the slicer
unions them. One stage (A1c, Y 5..8) has a small separate island (the
lower-left wall, 187 mm³) because the spray tunnel + nozzle pilot cut it
off from the rest of that stage. It is fused to the stages above and below
by 1.0–1.1 mm overlaps and prints as one piece — this is expected, not a
defect. (v4 adds the tunnel cut to A1b too; A1b stays one connected
profile — verified in the shell count.)

**The pilot holes are square, not round.** The faceplate pilots (1.8 mm),
back-plate pilots (1.8 mm), and nozzle pilots (1.6×2.0 mm slots) are cut
in 2D, so they are square slots, not drilled round holes. M2 self-tapping
screws cut their own thread into them — this is intentional and it works,
but do not expect a round hole. If a screw binds, run it in and out once
before final assembly to cut the thread.

**Press-cap dish note:** the finger dish is built as a solid disc plus an
annulus rim (two stacked shells), NOT a subtracted dish — a 3D
coaxial-cylinder dish subtract cracked the STL (20 bad edges). Do not
"clean it up" into a single subtract; the two-shell construction is the
fix.

## 7. Assembly order

1. **Nozzle block first.** Transplant variant (recommended): cut the stem
   block + nozzle out of your stock actuator (keep the stem socket, sump,
   and orifice intact — cut around them, do not cut through them). Test-fit
   it in the pocket (12×12×10 mm); file the pocket or the cut until it
   seats flush with the block's top. Epoxy it in, keeping epoxy out of the
   stem socket and the spray tunnel. Let it cure fully before the next step.
   Printed variant (experimental only): epoxy a 23G blunt needle into the
   Ø0.95 mm pocket, trim it flush with the block's front face. See §8.
2. **Nozzle block into the shell.** Slide the block up into the nozzle
   pocket from below (the pocket is open at the bottom). The block's front
   face (Z=14.5 in body coords) must sit flush against the spray tunnel's
   back wall. Fix with 2× M2×10 self-tapping screws from the left face
   (through the 1.6×2.0 mm pilot slots at Y=2.5 and Y=6.5). If the block
   sits low, glue 1 mm spacer washers to its bottom (print 3, use as needed
   — the pocket is open below, so the spacers need glue).
3. **Hinge.** Place the mouthpiece's hinge arms between the shell knuckles
   (arms at X 4..6 and 22..24, knuckles at X 1..3.5 and 25.5..28). Push a
   27 mm length of 1.75 mm filament through as the pin (or the printed
   pin). Fold/unfold 20 times — it should swing freely with no catch
   (v4 fixes the v3 catch in the first ~8° — if you still feel one, check
   the tunnel is clear of support material). If it binds, ream the Ø2.15 mm
   bores with a 2.2 mm drill bit by hand.
4. **Reed sled.** Solder two wires to the reed switch, slide it into the
   sled's U-pocket (open side toward the can), and slide the sled into the
   reed groove from the top. For a 19 ml can the sled sits at Y 75.1..90.1
   (reed centre ~2 mm below the magnet at rest). For 14/17 ml cans, slide
   it down until the reed is ~2 mm below the magnet at rest, then fix it
   with a drop of CA glue or a foam wedge. Route the wires down the groove
   and into the bay through the wire channel.
5. **Electronics.** Solder the e-ink panel's FPC to the adapter (DESPI-C154
   or direct to the board), the button, the LED (+ 220 Ω resistor), and the
   reed wires to the nRF52840 board. Seat the board in the bay bottom
   (Y 2..36.1) with its USB-C end down — the connector must poke through
   the bottom slot (X 33.2..42.4, Z 30..34.5). Hold the board with a foam
   pad behind it (between the board and the back plate) — there are no
   board screws; the USB slot + foam is the retention. Place the 301230
   cell in the bay (Y 42..72) with a foam pad behind it. Mount the e-ink
   adapter in the bay beside the battery with a foam pad. **The back
   plate's lower span (Y 2..39) has no screw** (the bosses are at Y=39) —
   if it rattles or flexes, add a foam pad between the cell/board and the
   back plate, or a drop of CA at the bottom edge.
6. **Faceplate.** Seat the e-ink panel in its pocket (X 8.8..41.2,
   Y 50.9..88.9, 1.4 mm deep) — centre it in the pocket so the active
   area aligns with the window (X 11.5..38.5, Y 56.4..83.4). Fold the FPC
   down from the panel's bottom edge into the FPC channel (X 17..32,
   below the pocket) and through into the bay. Seat the tactile switch
   in its pocket (Y 43.9..50.4) and the LED in its cavity. Place the
   button cap (hat in the Ø7.7 mm faceplate hole at (21.1, 47.15), stem
   into the switch pocket). Screw the faceplate on with 4× M2×5 (at
   (9,42), (41,42), (9,90), (41,90)).
7. **Back plate.** Screw on with 4× M2×8: (29,39) and (40,39) into the
   bosses, (30,92.5) and (40,92.5) into the top cap.
8. **Can + press cap.** Drop the bare can (stem down) into the well — the
   stem must seat in the nozzle block's socket (transplant: the
   transplanted socket; printed: the Ø3.3 bore). Press the can down by
   hand — you should feel the fire click at ~3.2 mm travel. Seat the
   press cap on the can top (cup over the can, magnet boss in the reed
   groove, key rib in the back keyway). The cap's top sits 1.1 mm below
   the body's top rim (v4 — was 3.1 mm) with the finger dish centred —
   press it with a fingertip through the Ø25 opening.
9. **Test the reed.** With the board powered, press the cap — the reed
   must close once per press (no double-trigger, no miss). If it misses,
   slide the sled 1 mm toward the magnet; if it double-triggers, slide it
   1 mm away. 100 presses, zero missed/double, is the acceptance test.

## 8. Spray-pattern validation (required before real use)

**Transplant variant:** the transplant preserves the stock orifice, sump,
and stem fit — the drug-delivery geometry is unchanged from the stock
actuator. Even so, before trusting it: (1) fit-test with an **empty can**
(water-filled or spent) — 10 actuations, check the plume exits the
mouthpiece cleanly with no leakage at the block/pocket joint; (2) compare
the spray pattern on paper against the stock actuator at the same distance
(same can, same number of puffs) — the patterns should match in shape and
spread; (3) only then use it with a live can, and keep your stock actuator
as the backup for the first week.

**Printed needle-orifice variant (experimental):** this variant **changes
the drug path** — a printed bore + a 23G needle is not the stock orifice,
sump, or stem fit. Do not trust it with real medication until: (1) the
water/empty-can spray-pattern comparison above passes; (2) the plume
geometry (cone angle, droplet size by eye on paper) is close to stock;
(3) the needle is firmly epoxied (pull-test it — a loose needle in the
airway path is a hazard). Even then, it is a prototype — the transplant
is the variant to trust.

## 9. Verification (all geometry, no hardware yet)

Run with `PYTHONPATH=/home/hatch/.local/lib/python3.12/site-packages
python3 jscad/verify-v4.py` (needs trimesh + networkx + rtree):

- **All 14 STLs watertight, 0 bad edges** (checked per shell).
- **Folded bbox 50.0 × 94.0 × 38.0 mm** (1.97" × 3.70" × 1.50") —
  inside the 101.6 × 50.8 mm limit.
- **Void checks pass** (screen pocket, FPC channel, button pocket, can
  bore, cap bore, mouthpiece pocket, spray tunnel, nozzle pocket, bay,
  reed groove) — sampled grids, union semantics across the body's
  overlapping shells. (Note: `trimesh.contains()` on the merged
  multi-shell body mesh gives parity garbage — the verifier tests each
  shell separately and ORs the results. Do not "simplify" that away.)
- **Faceplate window clear** (no triangles inside the window rect) and
  the button/LED hole gap is solid.
- **Hinge sweep collision-free at 0–90°** in 5° steps (mouthpiece
  vertices + triangle centroids vs body shells).
- **Combined plate** 202.5 × 97.5 mm, gaps ≥2.50 mm (fits a 220×220 bed).

## 10. What is NOT yet verified

- **Print test.** No part has been printed. The STLs are geometry-verified
  (see §9) but nothing has been through a printer. First print is a
  fit-test, not a final part.
- **Fit test.** The can, board, panel, cell, reed, and magnet have not been
  test-fitted in a printed shell. The clearances are designed (0.35 mm
  radial on the can bore, 0.3 mm on the cap bore, 0.5 mm on the mouthpiece
  pocket, 0.3 mm/side on the screen pocket) but unproven.
- **FPC exit edge.** The design assumes the panel FPC exits the short
  edge (see §2) — verify on the received panel.
- **Spray-pattern validation.** See §8 — required before real use.
- **Fold-cycle life.** The hinge (printed knuckles + filament pin) has not
  been cycle-tested. 1,000 fold cycles is the acceptance target.
- **Battery life.** Depends on the board's real sleep current (see the BOM
  warning — the SuperMini's sleep current is disputed). Measure it, do not
  assume it.
- **Find My.** Not implemented (see report 13 §1.2 for the MFi gate).
- **Usability.** The §5 audit is a design audit from the CAD. No one has
  held the device; the weight, the one-handed unfold, and the no-detent
  mouthpiece are the things most likely to annoy in real use.

## 11. Files

- `jscad/puffpal-v4.js` — parametric source (all dimensions in `P`)
- `jscad/export-v4.js` — exports all STLs (`node export-v4.js`)
- `jscad/verify-v4.py` — watertight / void / hinge-sweep checks (see §9)
- `jscad/checks.js` — exports body shells + mouthpiece separately
- `stl/` — 11 individual parts + 2 assemblies + combined plate (14 files)
- `renders/` — folded + unfolded PNG previews (`render.py`) and
  three-view SVG orthographic renders (`render_svg.py`)
- `puffpal-v4-bom.md` — Amazon-only BOM, parts only, exact search terms
- `README.md` — regeneration instructions
