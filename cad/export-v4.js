// Export PuffPal v4 parts + combined plate + assemblies to STL.
// Usage: node export-v4.js   (writes ../stl/*.stl)
const fs = require('fs')
const path = require('path')
const { translate, rotateX, rotateZ } = require('@jscad/modeling').transforms
const { measureAggregateBoundingBox } = require('@jscad/modeling').measurements
const { serialize } = require('@jscad/stl-serializer')
const M = require('./puffpal-v4.js')

const OUT = path.join(__dirname, '..', 'stl')
fs.mkdirSync(OUT, { recursive: true })

function writeSTL (name, geoms) {
  const out = serialize({ binary: true }, geoms)
  const buf = Buffer.concat(out.map(s => Buffer.isBuffer(s) ? s : Buffer.from(s)))
  fs.writeFileSync(path.join(OUT, name), buf)
  const bb = measureAggregateBoundingBox(geoms)
  const size = bb[1].map((v, i) => (v - bb[0][i]).toFixed(1))
  console.log(name.padEnd(38), 'shells=' + String(geoms.length).padStart(2), 'bbox=' + size.join(' x ') + ' mm')
  return buf.length
}

// ---- individual parts (as modeled; print orientation noted in build notes) ----
writeSTL('puffpal-v4-body.stl', M.buildBody())
writeSTL('puffpal-v4-faceplate.stl', M.buildFaceplate())
writeSTL('puffpal-v4-backplate.stl', M.buildBackplate())
writeSTL('puffpal-v4-mouthpiece.stl', M.buildMouthpiece())
writeSTL('puffpal-v4-press-cap.stl', M.buildPressCap())
writeSTL('puffpal-v4-nozzle-transplant.stl', M.buildNozzleTransplant())
writeSTL('puffpal-v4-nozzle-printed.stl', M.buildNozzlePrinted())
writeSTL('puffpal-v4-reed-sled.stl', M.buildReedSled())
writeSTL('puffpal-v4-spacer-1mm.stl', M.buildSpacer())
writeSTL('puffpal-v4-button-cap.stl', M.buildButtonCap())
writeSTL('puffpal-v4-hinge-pin.stl', M.buildHingePin())

// ---- assemblies (folded + unfolded) ----
writeSTL('puffpal-v4-assembly-folded.stl', M.assemblyFolded())
writeSTL('puffpal-v4-assembly-unfolded.stl', M.assemblyUnfolded())

// ---- combined print plate (220 x 220 bed, >=2 mm gaps) ----
// Bed: X right, Y back, Z up. Parts rotated into print orientation.
const plate = []
const add = (geoms, fn) => { for (const g of geoms) plate.push(fn(g)) }
// body: standing on bottom (Y=0 down). rotateX(90): (x,y,z)->(x,-z,y)
add(M.buildBody(), g => translate([0, 38, 0], rotateX(Math.PI / 2, g)))
// faceplate: flat as modeled (Z = thickness). X 2..48 -> 60..106, Y 39..93 -> 0..54
add(M.buildFaceplate(), g => translate([58, -39, 0], g))
// back plate: flat as modeled, bottom (Z=33) on bed. X 27..48.5 -> 120..141.5, Y 2..94 -> 0..92
add(M.buildBackplate(), g => translate([93, -2, -33], g))
// mouthpiece: standing on hinge end. rotateX(90): local Y -> bed Z
add(M.buildMouthpiece(), g => translate([10, 49.5, 4.5], rotateX(Math.PI / 2, g)))
// press cap: open face up. rotateX(-90): local Y -> bed -Z, then up 12.6 (v4 cap height)
add(M.buildPressCap(), g => translate([42.2, 57.2, 12.6], rotateX(-Math.PI / 2, g)))
// nozzle blocks: standing, socket up. rotateX(90)
add(M.buildNozzleTransplant(), g => translate([70, 69, 0], rotateX(Math.PI / 2, g)))
add(M.buildNozzlePrinted(), g => translate([95, 69, 0], rotateX(Math.PI / 2, g)))
// reed sled: flat, length along bed X. rotateZ(-90): (x,y,z)->(y,-x,z)
add(M.buildReedSled(), g => translate([150, 71.5, 1.15], rotateZ(-Math.PI / 2, g)))
// spacers x3: flat as modeled (annulus in XY, Z 0..1)
for (const cx of [153, 173, 193]) add(M.buildSpacer(), g => translate([cx, 88.75, 0], g))
// button cap: lying flat. rotateX(90): axis Z -> bed -Y
add(M.buildButtonCap(), g => translate([198.75, 67, 3.75], rotateX(Math.PI / 2, g)))
// hinge pin: lying flat as modeled (axis X)
add(M.buildHingePin(), g => translate([13.5, 75.9, 0.9], g))
writeSTL('puffpal-v4-combined.stl', plate)
console.log('done.')
