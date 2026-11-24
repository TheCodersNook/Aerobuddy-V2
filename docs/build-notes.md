# AeroBuddy v5 — Build Notes

*(Developed as PuffPal — this is the v5 hardware, PuffPal v5 lineage.
Internal file names keep the `puffpal-v5` prefix.)*

**Personal prototype, not a medical device.** This is a replacement actuator
for your own albuterol inhaler, built for prototyping. It is not tested,
certified, or approved as a medical device. Do not give it to anyone else,
do not sell it, and do not trust it with a real dose until you have validated
it as described in §8.

## 1. What v5 is, and what changed vs v4

v5 is the v4 replacement-actuator design with the parts count cut from 11
to 7, a simpler dose sensor, and a cleaner, rounder shell. Same architecture,
same folded size, same screen, same board, same battery.

| | v4 | v5 |
|---|---|---|
| Printed parts | 11 | **7** (body, faceplate, backplate, mouthpiece, press cap, nozzle-transplant, hinge pin) |
| Folded size | 50.0 × 94.0 × 38.0 mm | **50.0 × 94.0 × 38.0 mm (unchanged)** |
| Unfolded size | 50.0 × 94.0 × 61.5 mm | **50.0 × 94.0 × 61.5 mm (unchanged)** |
| Fits a 4" × 2" box? | Yes — 3.70" × 1.97" | **Yes — 3.70" × 1.97"** |
| Dose sensing | Reed switch + magnet (reed sled + magnet boss) | **6×6×5 mm microswitch (same part as the UI button)** |
| Nozzle | Transplant (recommended) + printed needle-orifice (experimental) | **Transplant only** |
| Button | Printed button cap over the tactile switch | **Switch pressed directly through the faceplate — no cap** |
| Press cap | Finger dish on top | **Finger dish + cam rib (drives the microswitch) + finger scoop in the top rim** |
| Shell finish | Flat rims, square faceplate edge, square mouthpiece tip | **Rounded top/bottom rims (stepped round-overs), chamfered faceplate edge, rounded mouthpiece tip; shell corner radius stays 8 mm** |
| Colour scheme | Any colour / matte two-tone | **White shell + red accents (mouthpiece, press cap) + red cross on the backplate** |

**What was cut, and why:**

- **Reed sled + magnet boss + reed switch + magnet** — dose sensing is now
  a 6×6×5 mm tactile microswitch in a pocket in the front bore wall, driven
  by a cam rib on the press cap. One cheap part, no magnet to align, no
  sled to position per can size, and the switch is the same part as the UI
  button (buy one pack). See §7 step 4 for the mount.
- **1 mm spacer shim** — gone; the nozzle pocket no longer takes one.
- **Printed needle-orifice nozzle** — gone; transplant only. The needle
  variant changed the drug path and was experimental-only — keeping one
  nozzle variant keeps the build simpler and the drug-delivery geometry
  stock.
- **Button cap** — gone; the tactile button is pressed directly through
  the faceplate hole.

**What was KEPT as a separate part:** the **hinge pin**. A snap-fit pin
would weaken the hinge — the pin stays a separate printed part (or a
27 mm length of 1.75 mm filament).

**What did NOT change:** can well, nozzle transplant block, hinge,
mouthpiece geometry (apart from the rounded tip), back plate, board/battery
bay, USB slot, screw positions, 1.54" screen + pocket, FPC channel. If you
printed v4 parts, the body, faceplate, and press cap need reprinting for
v5 (the microswitch pocket, cam rib, scoop, and round-overs are in those
parts); the backplate, mouthpiece, nozzle-transplant block, and hinge pin
are the same parts apart from the v5 finish (rounded mouthpiece tip, cross
pocket on the backplate).

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

## 3. De-vape choices (unchanged from v4, plus the v5 colour scheme)

- **Pebble/slab, not a tube.** 50 mm wide × 38 mm thick with 8 mm corner
  radii — a small power bank, not a vape pen. Flat top and bottom, no
  cylindrical body anywhere. v5's rounded rims make it softer still.
- **Mouthpiece folds flush.** Folded, the mouthpiece's front face is at Z=0,
  exactly flush with the faceplate — no protruding tube at all.
- **Flattened oval mouthpiece.** Unfolded it is 20×9 mm outside, 16×6 mm
  channel — the flattened-oval cross-section of an inhaler mouthpiece, not
  a round vape tip. The tip is rounded in v5.
- **Screen on the broad flat face.** The 27×27 mm e-ink window is on the
  50 mm-wide face, not on a narrow edge.
- **No pocket clip.** A lanyard hole (4 mm square) in the back plate instead.
- **White shell + red accents, red cross on the back.** The white body with
  a red mouthpiece, red press cap, and a red cross on the backplate reads
  as medical, not vape. No branding, nothing that glows except the small LED.

## 4. Measurements to verify BEFORE printing the nozzle block

These dimensions are **estimated** in the research file. Measure your own
can with calipers before printing the nozzle block. The rest of the body is tolerant of
±0.5 mm; the nozzle block is not.

| Dimension | Estimated value | What to measure on your can |
|---|---|---|
| `valve_stack_h` | 12.0 mm | Base of can → ferrule face (the valve stack height) |
| `stem_protrusion` | 10.0 mm | Ferrule face → stem tip |
| `stem_socket_h` | 6.5 mm | Socket depth — verify the stem seats fully without bottoming |
| `fire_stroke` | ~3.2 mm | How far the can travels from rest to fire (press slowly, feel the click) |
| `can_od` | 22.1 mm | Can body diameter (verify — generics vary) |
| `stem_od` | 3.16 mm | Stem diameter (Aptar DF30 RCU nominal) |

Also measure: the ferrule OD (assumed Ø20 — the relief is Ø20.5, so you
have 0.25 mm/side), and your transplant cut (the pocket is 12×12×10 mm
default — **measure your cut**; if it differs, file the pocket or the
block to fit).
If any measured value differs by more than 0.5 mm from the estimate, do
not print the nozzle block as-is — the socket/stem fit is not tolerant
of guesses. The STLs here are fixed exports (no parametric source in
this repo).

## 5. Usability audit (design audit — no user testing was done)

This is a design audit against the CAD, not user-test results. Nobody has
held this device yet. What the geometry says:

**Press cap + finger scoop — improved in v5.** The fire force is set by the
can's own valve spring (same as a stock inhaler — the cap is just a bigger
button), and the travel is the can's ~3.2 mm fire stroke. The cap top sits
1.1 mm below the body rim with a Ø16 × 0.7 mm finger dish, so the pad
meets the fingertip at the rim and the dish centres it. The Ø24.4 pad
spreads the load over more fingertip area than the stock can's bare top,
so the same force feels slightly lighter. **New in v5:** a concave finger
scoop is printed into the top-front rim above the can bore, so the
fingertip finds the cap naturally. One-handed press (grip the 50×38 body,
press with the index finger or thumb) is feasible — the body is wide
enough to palm.

**Microswitch dose sensing — simpler than the reed, and the margin is
designed in.** The cam rib on the press cap (ramp + dwell flat) closes the
switch ~1.4 mm into the ~3.2 mm fire stroke — 1.1 mm of margin against the
2.5 mm minimum stroke — and the dwell flat holds it closed without
blocking the stroke. No magnet alignment, no per-can-size sled position.
The switch actuation still needs checking on the real print (see §10).

**Button + LED — reachable.** The button sits at Y 47.15 on the front
face, mid-device — a thumb of the holding hand reaches it without shifting
grip. **In v5 there is no printed button cap** — you press the 6×6×5
tactile switch directly through the faceplate hole. The LED is 6.4 mm to
its right, visible while pressing. The v3 hole-overlap defect stays fixed.

**Mouthpiece — shape is right, edges need finishing.** The flattened
oval (20×9 mm outside, 16×6 mm channel, 28 mm long) matches a stock
inhaler mouthpiece profile and gives an adequate lip seal. Two notes:
(1) the tip is rounded in v5 but still rigid PETG with printed layer
edges — **sand the tip edges smooth** (220-grit, then wash) before first
use, or the rim will feel sharp on the lips; (2) there is **no detent**
at either end of the swing — the mouthpiece is held folded/unfolded by
hinge friction only, so it can flop open in a bag. A detent bump is a
future-revision item, not in v5.

**Screen readability — the big win (unchanged from v4).** 27×27 mm active
at 200×200 (0.135 mm/px, 188 dpi). Sensible minimum sizes at arm's length:
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

**Weight — honest number: ~115–120 g (v4 estimate, carried over — the v5
parts count is lower but the shell volume is nearly the same).** Computed,
not weighed: printed parts ~86 cm³ solid volume at PETG 1.27 g/cm³,
derated for 25% infill + 4 walls ≈ **~79 g**; canister ≈ 25 g (estimate —
weigh yours); panel 2.1 g (datasheet); board ≈ 3 g, battery ≈ 3 g,
adapter ≈ 2 g, screws/switches/LED/wire ≈ 4 g. A stock inhaler is ~50 g,
so this is roughly **2.3× a stock inhaler** — a pocket brick, not a pocket
afterthought. The weight is the price of the screen, the folding mechanism,
and the battery. If that bothers you in use, the only real diet is a smaller
battery or a thinner shell — both are future revisions.

**Anything that would make a real user put it down:** the weight and
38 mm thickness (it will not disappear in a pocket like a stock
inhaler); the mouthpiece flopping open in a bag (no detent); the
2 s full-refresh screen feeling laggy if firmware updates the whole
display per dose; and the one-handed unfold. None of these is a
print-stopper — they are the known costs of this architecture.

## 6. Print settings

**Material: PETG** (see the BOM for why). PLA is fine for a fit-test print
only — it creeps under the can's spring load and softens in heat.

**Colour split:** white — body, faceplate, backplate, hinge pin,
nozzle-transplant block. Red — mouthpiece, press cap. (The red cross is
either a red-filament swap at the cross layer on the white backplate, or
red paint in the pocket — see §7 step 6.)

| Part | Orientation | Layer height | Walls / infill | Supports |
|---|---|---|---|---|
| Body | Standing on its bottom (Y=0 down) | 0.20 mm | 4 walls, 25% | Tree supports (bay ceiling, pocket ceilings, tunnel). Remove through the back opening. |
| Faceplate | Flat, front (Z=0) down | 0.20 mm | 4 walls, 25% | None |
| Back plate | Flat, Z=33 down | 0.20 mm | 4 walls, 25% | None |
| Mouthpiece | Standing on hinge end | 0.20 mm | 4 walls, 25% | Tree (hinge arms only) |
| Press cap | Open face up (cup opening up) | 0.20 mm | 4 walls, 25% | None |
| Nozzle block (transplant) | Standing, socket/pocket up | **0.12 mm** | 4 walls, **100%** | None (the Ø4 tunnel bridges) |
| Hinge pin | **Use 27 mm of 1.75 mm filament instead** (cut flush). The printed pin is a backup — print it flat with a brim if you use it. | — | — | — |

**Combined plate:** `cad/puffpal-v5-combined.stl` lays all parts out on one
plate (fits a 220×220 mm bed). Print time estimate: ~6–8 h at 0.20 mm
(body dominates). Filament: ~80 g total (see §5 for the weight build-up).
Note: the combined plate is single-colour — for the white/red scheme,
print the mouthpiece and press cap separately in red, or split the plate
by colour in the slicer.

**Known multi-shell note (carried from v4):** the body STL has multiple
overlapping shells that the slicer unions. One stage (A1c, Y 5..8) has a
small separate island (the lower-left wall) because the spray tunnel +
nozzle pilot cut it off from the rest of that stage. It is fused to the
stages above and below by ~1 mm overlaps and prints as one piece — this
is expected, not a defect.

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

1. **Nozzle block first (transplant only).** Cut the stem block + nozzle
   out of your stock actuator (keep the stem socket, sump, and orifice
   intact — cut around them, do not cut through them). Test-fit it in the
   pocket (12×12×10 mm); file the pocket or the cut until it seats flush
   with the block's top. Epoxy it in, keeping epoxy out of the stem socket
   and the spray tunnel. Let it cure fully before the next step.
2. **Nozzle block into the shell.** Slide the block up into the nozzle
   pocket from below (the pocket is open at the bottom). The block's front
   face (Z=14.5 in body coords) must sit flush against the spray tunnel's
   back wall. Fix with 2× M2×10 self-tapping screws from the left face
   (through the 1.6×2.0 mm pilot slots at Y=2.5 and Y=6.5).
3. **Hinge.** Place the mouthpiece's hinge arms between the shell knuckles
   (arms at X 4..6 and 22..24, knuckles at X 1..3.5 and 25.5..28). Push a
   27 mm length of 1.75 mm filament through as the pin (or the printed
   pin). Fold/unfold 20 times — it should swing freely with no catch
   (v4 fixed the v3 catch in the first ~8° — if you still feel one, check
   the tunnel is clear of support material). If it binds, ream the Ø2.15 mm
   bores with a 2.2 mm drill bit by hand.
4. **Microswitch (dose sensor).** Solder two wires to a 6×6×5 mm tactile
   switch (the same part as the UI button). Seat the switch in the pocket
   in the **front bore wall**, **plunger facing +Z toward the can** — the
   cam rib on the press cap pushes the plunger as the cap travels down.
   Route the wires through the **channel in the wall** into the electronics
   bay, and solder them to **D6 + GND** on the board. The switch closes
   ~1.4 mm into the ~3.2 mm fire stroke (1.1 mm margin vs the 2.5 mm
   minimum stroke); the dwell flat on the cam rib holds it closed without
   blocking the stroke. Before closing the shell, press the cap by hand
   and listen/feel for the switch click — it should click once, part-way
   through the stroke, before the can fires.
5. **Electronics.** Solder the e-ink panel's FPC to the adapter (DESPI-C154
   or direct to the board), the UI button, the LED (+ 220 Ω resistor), and
   the microswitch wires (D6 + GND) to the nRF52840 board. Seat the board
   in the bay bottom (Y 2..36.1) with its USB-C end down — the connector
   must poke through the bottom slot (X 33.2..42.4, Z 30..34.5). Hold the
   board with a foam pad behind it (between the board and the back plate)
   — there are no board screws; the USB slot + foam is the retention.
   Place the 301230 cell in the bay (Y 42..72) with a foam pad behind it.
   Mount the e-ink adapter in the bay beside the battery with a foam pad.
   **The back plate's lower span (Y 2..39) has no screw** (the bosses are
   at Y=39) — if it rattles or flexes, add a foam pad between the cell/board
   and the back plate, or a drop of CA at the bottom edge.
6. **Red cross on the backplate.** The backplate has a **15 mm Greek-cross
   pocket, 1 mm deep**, on its outer face. Fill it one of two ways:
   (a) **filament swap** — pause the print (or use a filament-change
   command) at the cross layer and print the cross in red PETG, then
   switch back to white; or (b) **paint** — print the backplate all white,
   then fill the pocket with red acrylic or enamel paint, wipe the surface
   flush, and let it dry. Either way the cross sits flush with the white
   surface. — The red cross emblem is a protected symbol under the Geneva
   Conventions for official medical use — fine for a personal prototype,
   but a product sold later would need a different medical mark (e.g. a
   green cross).
7. **Faceplate.** Seat the e-ink panel in its pocket (X 8.8..41.2,
   Y 50.9..88.9, 1.4 mm deep) — centre it in the pocket so the active
   area aligns with the window (X 11.5..38.5, Y 56.4..83.4). Fold the FPC
   down from the panel's bottom edge into the FPC channel (X 17..32,
   below the pocket) and through into the bay. Seat the UI tactile switch
   in its pocket (Y 43.9..50.4) and the LED in its cavity. **There is no
   button cap in v5** — the switch is pressed directly through the Ø7.7 mm
   faceplate hole at (21.1, 47.15). Screw the faceplate on with 4× M2×5 (at
   (9,42), (41,42), (9,90), (41,90)).
8. **Back plate.** Screw on with 4× M2×8: (29,39) and (40,39) into the
   bosses, (30,92.5) and (40,92.5) into the top cap.
9. **Can + press cap.** Drop the bare can (stem down) into the well — the
   stem must seat in the nozzle block's socket (the transplanted socket).
   Press the can down by hand — you should feel the fire click at ~3.2 mm
   travel. Seat the press cap on the can top (cup over the can, cam rib
   facing the microswitch pocket, key rib in the back keyway). The cap's
   top sits 1.1 mm below the body's top rim with the finger dish centred —
   press it with a fingertip through the Ø25 opening, guided by the finger
   scoop in the top rim.
10. **Test the microswitch.** With the board powered, press the cap — the
    switch must close once per press (no double-trigger, no miss). If it
    misses, check the cam rib actually reaches the plunger (the switch
    must sit fully in its pocket, plunger facing the can); if it
    double-triggers, check the dwell flat holds the plunger through the
    rest of the stroke. 100 presses, zero missed/double, is the acceptance
    test.

## 8. Spray-pattern validation (required before real use)

**Transplant variant (the only v5 variant):** the transplant preserves the
stock orifice, sump, and stem fit — the drug-delivery geometry is
unchanged from the stock actuator. Even so, before trusting it: (1)
fit-test with an **empty can** (water-filled or spent) — 10 actuations,
check the plume exits the mouthpiece cleanly with no leakage at the
block/pocket joint; (2) compare the spray pattern on paper against the
stock actuator at the same distance (same can, same number of puffs) —
the patterns should match in shape and spread; (3) only then use it with
a live can, and keep your stock actuator as the backup for the first week.

## 9. Verification (all geometry, no hardware yet)

Verified in this pass with trimesh (watertight + bounding-box checks on
the v5 STLs):

- **All 10 v5 STLs watertight** (7 parts + folded/unfolded assemblies +
  combined plate).
- **Folded bbox 50.0 × 94.0 × 38.0 mm** (1.97" × 3.70" × 1.50") —
  inside the 101.6 × 50.8 mm limit.
- **Unfolded bbox 50.0 × 94.0 × 61.5 mm.**
- **Combined plate 141.5 × 92.0 mm** — fits a 220×220 mm bed.

**Re-run for v5 (boolean point probes on the unioned body solid, 16/16
pass):** microswitch pocket + wire channel voids present with solid walls
around them, cam groove void, finger-scoop void with 2 mm of wall to the
bore, top/bottom round-over insets, top and faceplate pilot holes, can
bore, bay. **Cam-rib probes 6/6** (ramp + dwell faces in the right places)
and **cross-pocket probes 8/8** (pocket 1 mm deep, floor solid, corners
solid). **Interference checks (boolean):** press cap vs body 0.0 mm³ at
rest AND at full 3.2 mm press; mouthpiece vs body 0.0 mm³ folded.

**Not re-run for v5 yet:** the 0–90° hinge-sweep check (mouthpiece
rotation through its full arc — only the folded position was
interference-checked). The hinge geometry is unchanged from v4, where
the sweep passed, but re-run it before the first print.

## 10. What is NOT yet verified

- **Print test.** No part has been printed. The STLs are geometry-verified
  (see §9) but nothing has been through a printer. First print is a
  fit-test, not a final part.
- **Fit test.** The can, board, panel, cell, and microswitch have not been
  test-fitted in a printed shell. The clearances are designed (0.35 mm
  radial on the can bore, 0.3 mm on the cap bore, 0.5 mm on the mouthpiece
  pocket, 0.3 mm/side on the screen pocket) but unproven.
- **Microswitch actuation on the real print.** The 1.4 mm close point and
  1.1 mm margin are designed values — the cam rib, pocket depth, and
  plunger travel need confirming on a printed body with a real switch
  (see §7 step 10).
- **FPC exit edge.** The design assumes the panel FPC exits the short
  edge (see §2) — verify on the received panel.
- **Spray-pattern validation.** See §8 — required before real use.
- **Fold-cycle life.** The hinge (printed knuckles + filament pin) has not
  been cycle-tested. 1,000 fold cycles is the acceptance target.
- **Battery life / sleep current.** Depends on the board's real sleep
  current (see the BOM warning — the SuperMini's sleep current is disputed).
  Measure it, do not assume it.
- **Find My (OpenHaystack).** Implemented in firmware and compile-checked,
  but no Apple device has heard it advertise yet — bench verification
  (scanner check, 15-min rotation, first report) is still required. See
  `firmware/OPENHAYSTACK.md` §6. Unofficial DIY route, not MFi; locations
  appear in the fetch tool, never in Apple's Find My app.
- **Usability.** The §5 audit is a design audit from the CAD. No one has
  held the device; the weight, the one-handed unfold, and the no-detent
  mouthpiece are the things most likely to annoy in real use.

## 11. Files

- `cad/` — 10 STLs: 7 individual parts + folded/unfolded assemblies + combined print plate
- `images/` — renders of the v5 geometry
- `docs/BOM.md` — Amazon-only BOM, parts only, exact search terms
- `docs/build-notes.md` — this file
- `firmware/` — PlatformIO + Arduino firmware (XIAO nRF52840 + 1.54" e-ink),
  incl. the OpenHaystack beacon (`OPENHAYSTACK.md`, `tools/haystack_keys.py`)
- `README.md` — project overview
