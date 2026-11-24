# CAD — AeroBuddy v5 (PuffPal v5)

All 10 v5 STLs, ASCII (every slicer reads them, and they diff on GitHub).
All watertight, verified with trimesh. Folded assembly: 50.0 × 94.0 ×
38.0 mm (1.97" × 3.70" × 1.50") — fits a 4" × 2" box.

## Parts (7 printed parts, down from 11 in v4)

| File | What it is |
|---|---|
| `puffpal-v5-body.stl` | Main shell — can bore, screen pocket, battery + board bays, finger scoop on the top rim, dose-switch pocket in the bore wall |
| `puffpal-v5-faceplate.stl` | Front faceplate with the 1.54" screen window, button + LED holes (button pressed directly through the hole — no cap) |
| `puffpal-v5-backplate.stl` | Back plate with lanyard hole + 15 mm red-cross pocket (fill with red paint or a red filament swap) |
| `puffpal-v5-press-cap.stl` | Finger press cap with dish and cam rib (drives the dose microswitch) — print in red |
| `puffpal-v5-mouthpiece.stl` | Folding mouthpiece, flattened oval with rounded tip — swings flush into the body; print in red |
| `puffpal-v5-hinge-pin.stl` | Hinge pin (or cut a 27 mm length of 1.75 mm filament) |
| `puffpal-v5-nozzle-transplant.stl` | Nozzle block, transplant variant — **the only nozzle in v5** (cut the stem block + nozzle out of your stock actuator and epoxy it in; stock drug-delivery geometry preserved) |
| `puffpal-v5-assembly-folded.stl` | Full assembly, folded (reference) |
| `puffpal-v5-assembly-unfolded.stl` | Full assembly, unfolded (reference) |
| `puffpal-v5-combined.stl` | All parts on one 141.5 × 92.0 mm plate (fits a 220×220 bed) |

## Print

PETG recommended, white + red (mouthpiece and press cap in red, the
rest white). Print the combined plate, or the individual parts.
See `../docs/build-notes.md` for settings, assembly order, and the
spray-pattern validation you must do before real use.

**Measure your can with calipers before printing the nozzle block**
(§4 of the build notes) — the socket/stem fit is not tolerant of
guesses, and these STLs are fixed exports: there is no parametric
source in this repo.
