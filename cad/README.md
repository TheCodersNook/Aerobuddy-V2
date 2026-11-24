# CAD — AeroBuddy v2 (PuffPal v4)

Parametric JSCAD source. Every dimension lives in the `P` object at the top
of `puffpal-v4.js` (and the `STAGES` table for the shell).

| File | What it is |
|---|---|
| `puffpal-v4.js` | Parametric source for all parts |
| `export-v4.js` | Exports all STLs (`node export-v4.js` → `../stl/`, binary) |
| `verify-v4.py` | Watertight / void / hinge-sweep checks (needs trimesh + networkx + rtree) |
| `checks.js` | Exports body shells + mouthpiece separately (collision/clearance checks) |

## Regenerating

Requires Node.js plus `@jscad/modeling` 2.x and `@jscad/stl-serializer`
(the original build used modeling 2.13.0 / stl-serializer 2.1.23):

```bash
npm install @jscad/modeling @jscad/stl-serializer
node export-v4.js
```

To change a dimension: edit `P` (or `STAGES`), re-run `node export-v4.js`,
and re-verify with `verify-v4.py` — every STL must be watertight, 0 bad
edges. The STLs in `../stl/` are ASCII exports of these binary outputs
(round-trip verified: identical bounds, volume, and triangle count).

## Design rules (earned the hard way)

- A 3D subtract on an extrusion whose 2D profile contains holes **cracks
  the STL**. So: every shell/back-plate feature is cut in 2D before
  extrusion; annular parts are direct 2D annulus extrusions; 3D subtracts
  are used only on solid profiles (the nozzle blocks).
- A feature stage must be 1.0 mm taller than its feature (0.5 mm per end)
  or neighbouring stages' overlaps fill the feature in.
- `trimesh.contains()` on the merged multi-shell body mesh gives parity
  garbage — containment checks must test each shell separately and OR the
  results (see `verify-v4.py`).
