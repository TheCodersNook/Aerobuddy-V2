# CAD — AeroBuddy v2 (PuffPal v4)

All 14 v4 STLs, ASCII (every slicer reads them, and they diff on GitHub).
All watertight, verified with trimesh. Folded assembly: 50.0 × 94.0 ×
38.0 mm (1.97" × 3.70" × 1.50") — fits a 4" × 2" box.

## Parts

| File | What it is |
|---|---|
| `puffpal-v4-body.stl` | Main shell — can bore, screen pocket, battery + board bays |
| `puffpal-v4-faceplate.stl` | Front faceplate with the 1.54" screen window |
| `puffpal-v4-backplate.stl` | Back plate with lanyard hole |
| `puffpal-v4-press-cap.stl` | Finger press cap with dish (the fire button) |
| `puffpal-v4-mouthpiece.stl` | Folding mouthpiece, flattened oval — swings flush into the body |
| `puffpal-v4-hinge-pin.stl` | Hinge pin (or cut a 27 mm length of 1.75 mm filament) |
| `puffpal-v4-nozzle-transplant.stl` | Nozzle block, transplant variant — **recommended** (cut the stem block + nozzle out of your stock actuator and epoxy it in; stock drug-delivery geometry preserved) |
| `puffpal-v4-nozzle-printed.stl` | Nozzle block, needle-orifice variant — **experimental only**, needs spray-pattern validation before real use |
| `puffpal-v4-reed-sled.stl` | Reed switch sled (dose counting) |
| `puffpal-v4-spacer-1mm.stl` | 1 mm spacer |
| `puffpal-v4-button-cap.stl` | Button cap |
| `puffpal-v4-assembly-folded.stl` | Full assembly, folded (reference) |
| `puffpal-v4-assembly-unfolded.stl` | Full assembly, unfolded (reference) |
| `puffpal-v4-combined.stl` | All parts on one 202.5 × 97.5 mm plate (fits a 220×220 bed) |

## Print

PETG recommended. Print the combined plate, or the individual parts.
See `../docs/build-notes.md` for settings, assembly order, and the
spray-pattern validation you must do before real use.

**Measure your can with calipers before printing either nozzle block**
(§4 of the build notes) — the socket/stem fit is not tolerant of
guesses, and these STLs are fixed exports: there is no parametric
source in this repo.
