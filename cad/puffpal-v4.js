// PuffPal v4 — folding replacement-actuator inhaler companion (pebble/slab).
// Hand-written parametric JSCAD (v2.13). Units: mm.
// Axes: X = width 0..50, Y = height 0..94, Z = thickness 0..38 (Z=0 front face).
// Can axis vertical (Y) at (X=14, Z=23.5), stem DOWN into the nozzle block.
// v4 vs v3: 1.54" e-ink (GDEH0154D67, 31.8 x 37.32) replaces the 0.97" panel;
// button + LED move below the screen (Y ~47); press-cap pad raised + dished.
// DESIGN RULE (proven Nov 23 by bisection, boolexp*.js): a 3D subtract on an
// extrusion whose 2D profile contains holes CRACKS the STL (12-84 boundary
// edges, independent of the cutter). So EVERY shell/back-plate feature is cut
// in 2D before extrusion; stages split at feature edges. 3D subtracts are used
// ONLY on solid profiles (nozzle blocks) and solid primitives, which test
// watertight. Coaxial cylinder bores are never subtracted — annular parts are
// direct 2D annulus extrusions (a subtracted coaxial bore cracks: 48-96 edges).
// SOURCES: /tmp/puffpal-v2/puffpal/research.md + modeler_brief.md (v2 SCAD).
// ESTIMATED dims (user must measure their own can before printing the nozzle
// block): valve_stack_h, stem_protrusion, fire_stroke. See build notes.
const { cuboid, cylinder, circle, rectangle, roundedRectangle, polygon } = require('@jscad/modeling').primitives
const { subtract, union, intersect } = require('@jscad/modeling').booleans
const { extrudeLinear } = require('@jscad/modeling').extrusions
const { translate, rotateX, rotateY, rotateZ } = require('@jscad/modeling').transforms
const { measureBoundingBox } = require('@jscad/modeling').measurements

// ---------------- parameters ----------------
const P = {
  // can / valve (research.md)
  can_od: 22.1, can_body_h_19ml: 58.8,
  valve_stack_h: 12.0,          // ESTIMATED — measure base->ferrule face
  stem_od: 3.16, stem_protrusion: 10.0, // ESTIMATED — measure ferrule->stem tip
  stem_socket_h: 6.5, fire_stroke: 3.2, // ESTIMATED ~2.5-3.5
  can_bore: 22.8, cap_bore: 25.0, stem_bore_d: 3.3,
  // body envelope — folded target <= 101.6 x 50.8 (4" x 2")
  body_w: 50, body_h: 94, body_t: 38, corner_r: 8,
  face_t: 2,                    // faceplate thickness (Z 0..2); shell Z 2..38
  can_cx: 14, can_cz: 23.5,     // can axis
  // nozzle block (both variants share the outer envelope)
  nb_w: 20, nb_d: 18, nb_h: 18, nb_r: 1.5,
  nb_pocket_w: 20.5, nb_pocket_d: 18.5, nb_pocket_h: 18.5,
  orifice_y: 10,                // orifice axis height from block bottom
  sump_d: 3.3, sump_h: 2.0,     // sump = bottom 2 mm of the dia 3.3 bore (17.1 mm3, in the 12-20 band)
  needle_pocket_d: 0.95,        // 23G blunt needle (OD 0.64) epoxied in
  transplant_x: 12, transplant_y: 12, transplant_z: 10, // pocket; MEASURE cut
  // spray tunnel (shell): X 3.5..24.5, Y 5..18, Z 2..14.5 (21 wide so the
  // 20 mm mouthpiece stub sweeps through it with 0.5 mm/side clearance)
  // mouthpiece pocket (shell front): X 3.5..25.5, Z 2..9, Y 0..38.5
  // electronics bay: X 28.5..47.5, Z 5..38 (open at back), Y 2..91.5
  bay_x0: 28.5, bay_x1: 47.5, bay_z0: 5,
  // reed groove in well front wall: X 10.8..17.2, Z 5.5..bore+0.5, Y 19..94
  // keyway groove in well back wall: X 12.5..15.5, Z 35.5..37.2, Y 60..94
  // hinge axis (mouthpiece): Y=10, Z=4.5; shell knuckles X 1..3.5, 25.5..28
  hinge_y: 10, hinge_z: 4.5, hinge_r: 3, hinge_hole_d: 2.15, hinge_pin_d: 1.8,
  // mouthpiece: flattened oval tube, outer 20 x 9, channel 16 x 6, len 28 + 4.5 stub
  mouth_w: 20, mouth_t: 9, mouth_ch_w: 16, mouth_ch_t: 6, mouth_len: 28, mouth_stub: 4.5,
  // e-ink GDEH0154D67 (or GDEY0154D67 successor, same outline): 31.8 x 37.32
  // panel, active 27 x 27, 24-pin FPC 12.5 wide exits the panel's bottom edge.
  // Portrait: narrow side across X. Pocket X 8.8..41.2, Y 50.9..88.9 (38.0 tall).
  epd_w: 31.8, epd_h: 37.32, epd_act_w: 27, epd_act_h: 27,
  screen_x0: 8.8, screen_y0: 50.9, // panel pocket X 8.8..41.2, Y 50.9..88.9
  screen_cx: 25, screen_cy: 69.9,  // panel centre (window centre)
  // button + LED moved below the screen (v3 had them at Y 65, inside the screen)
  button_cx: 21.1, button_cy: 47.15, led_cx: 27.5, led_cy: 47.15,
  // Press cap: cup ID 22.4, wall 1.0 -> OD 24.4, cup 9 + raised pad 3.6 (top 1.1 below rim) + finger dish
  // magnet 6x2 N52 in cap front wall; reed 2x14 glass in sled
  // fasteners: M2 screws (faceplate x5, backplate x8, nozzle x10 self-tap)
  // back-plate screws: bosses at (29,39) and (40,39); top cap at (30,92.5),(40,92.5)
  // board: SuperMini nRF52840 at bay bottom, Y 2..36.1, USB end down (slot in A0)
  // battery: 301230 cell, Y 42..72 in bay
}

// ---------------- 2D helpers (profiles live in X-Z plane: 2D-x = body X, 2D-y = body Z) ----------------
function rect2 (x0, x1, y0, y1) { return rectangle({ size: [x1 - x0, y1 - y0], center: [(x0 + x1) / 2, (y0 + y1) / 2] }) }
function circ2 (cx, cy, r, seg) { return circle({ radius: r, center: [cx, cy], segments: seg || 48 }) }
function shellOutline () {
  // rounded rect, X 0..50, Z 2..38 (faceplate occupies Z 0..2)
  return roundedRectangle({ size: [P.body_w, P.body_t - P.face_t], roundRadius: P.corner_r, center: [P.body_w / 2, P.face_t + (P.body_t - P.face_t) / 2] })
}
// Extrude a 2D X-Z profile along body Y from y0 to y1.
function stageXZ (profile2d, y0, y1) {
  let g = extrudeLinear({ height: y1 - y0 }, profile2d)
  g = rotateX(Math.PI / 2, g) // (x,y,z)->(x,-z,y): extrusion -> -Y, profile-Y -> +Z
  return translate([0, y1, 0], g)
}
// 3D helpers (only used on solid targets — see header rule)
function box3 (cx, cy, cz, sx, sy, sz) { return translate([cx, cy, cz], cuboid({ size: [sx, sy, sz] })) }
function box3c (x0, x1, y0, y1, z0, z1) { return box3((x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2, x1 - x0, y1 - y0, z1 - z0) }
function cylZ (cx, cy, cz, r, h, seg) { return translate([cx, cy, cz], cylinder({ radius: r, height: h, segments: seg || 24 })) }
function cylX (cx, cy, cz, r, h, seg) { let g = cylinder({ radius: r, height: h, segments: seg || 24 }); g = rotateY(Math.PI / 2, g); return translate([cx, cy, cz], g) }
function cylY (cx, cy, cz, r, h, seg) { let g = cylinder({ radius: r, height: h, segments: seg || 24 }); g = rotateX(Math.PI / 2, g); return translate([cx, cy, cz], g) }

// ---------------- main shell: 18 stacked 2D-cut extrusions, 0.5 mm overlaps ----------------
// 2D void shapes (X-Z plane)
function nozzlePocket2d () { return rect2(P.can_cx - P.nb_pocket_w / 2, P.can_cx + P.nb_pocket_w / 2, P.can_cz - P.nb_pocket_d / 2, P.can_cz + P.nb_pocket_d / 2) }
function mouthPocket2d () { return rect2(3.5, 25.5, 2, 9) }
function tunnel2d () { return rect2(3.5, 24.5, 2, 14.5) }
function bay2d () { return rect2(P.bay_x0, P.bay_x1, P.bay_z0, P.body_t) }
function bore2d (r) { return circ2(P.can_cx, P.can_cz, r, 64) }
function reedGroove2d (boreFront) { return rect2(10.8, 17.2, 5.5, boreFront + 0.5) } // 6.4 wide: reed sled + magnet boss ride here
function keyway2d () { return rect2(12.5, 15.5, 35.5, 37.2) }
function usbSlot2d () { return rect2(33.2, 42.4, 30.0, 34.5) } // USB-C plug slot in bottom wall (A0)
function ferrule2d () { return circ2(P.can_cx, P.can_cz, 10.25, 48) } // dia 20.5 relief for the 20 mm ferrule (A3b)
// Feature cuts (2D, confined to their stage by the stage table)
function nozzlePilot2d () { return rect2(0, 10, 22.7, 24.3) } // M2 screw slot from left face
function pilotSq2d (x) { return rect2(x - 0.9, x + 0.9, 2, 5) } // faceplate M2 pilot, 1.8 sq
function buttonPocket2d () { return rect2(18, 24.2, 2, 7) } // tactile switch 6.2 x 6.2 x 5
function wireChannel2d () { return rect2(17, 28.5, 2, 5.5) } // to bay (breaks Z=5 wall by 0.5)
function ledRect2d () { return rect2(25.8, 29.2, 2, 6) } // LED cavity (faceplate has the round hole); moved right in v4 so the LED hole no longer overlaps the button hole (v3 defect: centres 4.9 apart, radii sum 5.45)
function screenPocket2d () { return rect2(8.8, 41.2, 2, 3.4) } // 1.54" panel pocket 32.4 x 38.0 x 1.4 (panel 31.8 x 37.32 x 1.05)
function fpcSlot2d () { return rect2(17, 32, 2, 5.5) } // FPC channel below the panel (12.5 mm FPC + margin); merges with the wire channel, breaks the Z=5 wall into the bay at X 28.5..32
function topPilot2d (x) { return rect2(x - 0.9, x + 0.9, 30, 38) } // back-plate M2 pilot, thru back face

// Base profile for a stage spanning [y0, y1]. Stages never straddle a transition
// except at the 0.5 mm overlaps, where the larger void wins in the union.
function baseProfile (y0, y1) {
  const cuts = []
  if (y0 >= 19) cuts.push(bore2d(y0 >= 60 ? P.cap_bore / 2 : P.can_bore / 2)) // bore starts at B; 25.0 above Y=60
  if (y0 >= 1 && y1 <= 92) cuts.push(bay2d()) // bay Y 1..91.5 (not A0, not E)
  if (y1 <= 19.5) cuts.push(nozzlePocket2d()) // nozzle pocket Y 0..18.5
  if (y1 <= 39) cuts.push(mouthPocket2d()) // mouthpiece pocket Y 0..38.5
  if (y0 >= 3.4 && y1 <= 18.5) cuts.push(tunnel2d()) // spray tunnel Y 4..18 (A1b+A1c+A2+A3a; cut in all four so no overlap fills it — v4 fix: v3 started the tunnel at A1c (Y 6.1) and the mouthpiece stub tip caught on the solid A1b lip in the first ~8 deg of unfold)
  if (y0 >= 19) cuts.push(reedGroove2d(y0 >= 60 ? 11.0 : 12.1)) // reed groove Y 19..94
  if (y0 >= 60) cuts.push(keyway2d()) // keyway Y 60..94
  if (y0 === 0) cuts.push(usbSlot2d()) // USB slot in bottom wall (A0)
  return subtract(shellOutline(), ...cuts)
}

// [y0, y1, extra 2D cuts] — every joint overlaps 0.5-1.5 mm.
// NOTE: a feature cut in one stage is FILLED by its neighbours over the
// overlap, so a feature stage must be 1.0 mm taller than its feature (0.5 per
// end) or the feature comes out short — e.g. D1b is 7.5 tall for a 6.5 button
// pocket, D4a+D4b span 50.4..89.4 for a 38.0 screen pocket.
const STAGES = [
  [0, 2.5, []],                                        // A0: USB slot, no bay
  [1.0, 4.0, [nozzlePilot2d()]],                       // A1a: nozzle screw 1 (slot Y 1.5..3.5)
  [3.4, 6.1, []],                                      // A1b
  [5.0, 8.0, [nozzlePilot2d()]],                       // A1c: nozzle screw 2 (slot Y 5.5..7.5)
  [7, 13.5, []],                                       // A2: spray tunnel
  [13, 18, []],                                        // A3a (ends at the ferrule's bottom — no lip)
  [17.5, 20, [ferrule2d()]],                           // A3b: ferrule relief (Y 18..19.5)
  [19, 33.5, []],                                      // B
  [33, 39, []],                                        // C1
  [38.5, 41.6, []],                                    // C2a
  [40.6, 43.9, [pilotSq2d(9), pilotSq2d(41)]],         // C2b: faceplate pilots (slot Y 41.1..43.4)
  [43.4, 50.9, [buttonPocket2d(), wireChannel2d(), ledRect2d(), fpcSlot2d()]], // D1b: button pocket (Y 43.9..50.4, 6.5 tall) + FPC channel below the panel
  [50.4, 60.5, [screenPocket2d()]],                   // D4a: screen pocket, lower half (can bore — below Y=60)
  [60, 89.4, [screenPocket2d()]],                     // D4b: screen pocket, upper half (cap bore + keyway — above Y=60); split at 60 because baseProfile picks the bore from y0
  [88.9, 90.5, []],                                    // D5
  [88.5, 91.5, [pilotSq2d(9), pilotSq2d(41)]],         // D6: faceplate pilots (slot Y 89..91)
  [90.5, 92, []],                                      // D7
  [91, 94, [topPilot2d(30), topPilot2d(40)]]           // E: top cap + back-plate pilots
]

// Hinge knuckle: direct 2D annulus (outer r=3, bore r=1.075) extruded along X.
// 2D centre (-hinge_z, hinge_y): rotateY(+90) maps 2D-x -> body -Z, 2D-y -> body Y.
function hingeKnuckle (x0) {
  let ann = subtract(circ2(-P.hinge_z, P.hinge_y, P.hinge_r, 32), circ2(-P.hinge_z, P.hinge_y, P.hinge_hole_d / 2, 20))
  let g = extrudeLinear({ height: 2.5 }, ann)
  g = rotateY(Math.PI / 2, g)
  return translate([x0, 0, 0], g)
}
// Back-plate screw boss: direct 2D annulus (outer r=2.5, pilot r=0.9), Z 4..36.
function screwBoss (bx, by) {
  let ann = subtract(circ2(bx, by, 2.5, 24), circ2(bx, by, 0.9, 16))
  return translate([0, 0, 4], extrudeLinear({ height: 32 }, ann))
}

function buildBody () {
  const parts = STAGES.map(([y0, y1, cuts]) => {
    const prof = cuts.length ? subtract(baseProfile(y0, y1), ...cuts) : baseProfile(y0, y1)
    return stageXZ(prof, y0, y1)
  })
  parts.push(hingeKnuckle(1), hingeKnuckle(25.5)) // shell knuckles X 1..3.5, 25.5..28
  parts.push(screwBoss(29, 39), screwBoss(40, 39)) // back-plate bosses (between board and battery)
  return parts
}

// ---------------- faceplate (front, Z 0..2, Y 39..93, X 2..48) ----------------
// 2D profile in X-Y, extruded 2 mm in Z. Screen window = active area;
// button hole dia 7.7 (recessed cap); LED hole dia 3.2; M2 holes dia 2.2.
function buildFaceplate () {
  let prof = roundedRectangle({ size: [46, 54], roundRadius: 6, center: [25, 66] })
  const winX0 = P.screen_cx - P.epd_act_w / 2, winX1 = P.screen_cx + P.epd_act_w / 2
  // Window centred on the screen pocket (Y 50.9..88.9 — D4a/D4b's range minus
  // the 0.5 mm overlaps), so a panel centred in the pocket aligns with it.
  const winY0 = P.screen_cy - P.epd_act_h / 2, winY1 = P.screen_cy + P.epd_act_h / 2
  prof = subtract(prof,
    rect2(winX0, winX1, winY0, winY1),
    circ2(P.button_cx, P.button_cy, 3.85, 32),   // button hole dia 7.7
    circ2(P.led_cx, P.led_cy, 1.6, 20),          // LED hole dia 3.2 (6.4 from button centre — no overlap, v3 defect fixed)
    circ2(9, 42, 1.1, 16), circ2(41, 42, 1.1, 16),
    circ2(9, 90, 1.1, 16), circ2(41, 90, 1.1, 16) // M2 clearance dia 2.2
  )
  return [extrudeLinear({ height: 2 }, prof)]
}

// ---------------- back plate (bay cover, follows shell's rounded back) ----------------
// Profile in X-Z: bay region clipped to the shell outline, extruded along Y.
// Split into Y-stages so the screw holes are 2D cuts (2.2 mm square — an M2
// screw self-taps into them; the plate is 5 mm PETG, no pilot needed).
// Holes at (29,39),(40,39) [into bosses] and (30,92.5),(40,92.5) [into top cap];
// lanyard hole 4 mm square at (37.5,86). Plate spans Y 2..94 (covers the
// relocated board at Y 2..36.1). Right-hand screws at X=40 (not 44/46) so the
// holes stay in the flat region, clear of the shell's corner arc.
function buildBackplate () {
  const prof = intersect(shellOutline(), rect2(27, 48.5, 33, 38.5))
  const hole = (x) => rect2(x - 1.1, x + 1.1, 33, 38.5)
  const stages = [
    [2, 38.4, []],
    [37.4, 40.6, [hole(29), hole(40)]],        // boss screws (holes Y 37.9..40.1)
    [39.6, 84.5, []],
    [83.5, 88.5, [rect2(35.5, 39.5, 33, 38.5)]], // lanyard hole (Y 84..88)
    [87.5, 91.9, []],
    [90.9, 94, [hole(30), hole(40)]]           // top-cap screws (holes Y 91.4..94)
  ]
  return stages.map(([y0, y1, cuts]) => stageXZ(cuts.length ? subtract(prof, ...cuts) : prof, y0, y1))
}

// ---------------- mouthpiece (local frame: hinge axis at origin, tube along +Y) ----------------
// Flattened oval tube: outer 20 x 9 (r=4), channel 16 x 6 (r=3), local Y -4.5..28.
// Hinge arms: discs r=3.5 with 2.15 mm bore, at local X +/-8..10 (2 mm plates).
function buildMouthpiece () {
  const parts = []
  let tubeProf = roundedRectangle({ size: [P.mouth_w, P.mouth_t], roundRadius: 4, center: [0, 0] })
  tubeProf = subtract(tubeProf, roundedRectangle({ size: [P.mouth_ch_w, P.mouth_ch_t], roundRadius: 2.9, center: [0, 0] }))
  let tube = extrudeLinear({ height: P.mouth_len + P.mouth_stub }, tubeProf)
  tube = rotateX(-Math.PI / 2, tube) // (x,y,z)->(x,z,-y): extrusion -> +Y, profile-Y -> -Z
  tube = translate([0, -P.mouth_stub, 0], tube) // local Y -4.5..28
  parts.push(tube)
  // hinge arms (discs in local Y-Z plane, 2 mm thick in X)
  for (const sx of [-1, 1]) {
    let arm = circ2(0, 0, 3.5, 32)
    arm = subtract(arm, circ2(0, 0, P.hinge_hole_d / 2, 20))
    let a3 = extrudeLinear({ height: 2 }, arm) // disc in XY, extruded in Z
    a3 = rotateY(Math.PI / 2, a3)              // (x,y,z)->(z,y,-x): thickness -> X, disc -> Y-Z
    a3 = translate([sx > 0 ? 8 : -10, 0, 0], a3) // local X 8..10 / -10..-8
    parts.push(a3)
  }
  return parts
}
// Mouthpiece placement: folded (tube up along body +Y) or unfolded (tube out +Z).
function mouthpieceFolded () {
  return buildMouthpiece().map(g => translate([P.can_cx, P.hinge_y, P.hinge_z], g))
}
function mouthpieceUnfolded () {
  // rotate -90 about X: local +Y (tube) -> body -Z (forward out of the face)
  return buildMouthpiece().map(g => translate([P.can_cx, P.hinge_y, P.hinge_z], rotateX(-Math.PI / 2, g)))
}

// ---------------- press cap (local frame: cup bottom Y=0, axis at origin) ----------------
// Cup: annulus OD 24.4 / ID 22.4, Y 0..9 (wall is only 1.0 mm, so the 2 mm magnet
// does NOT sit in the wall — it sits in a boss on the front outer wall, which
// rides in the widened reed groove). Finger pad disc Y 9..10.6; key rib at back.
function buildPressCap () {
  const parts = []
  const annulus = subtract(circ2(0, 0, 12.2, 64), circ2(0, 0, 11.2, 64))
  parts.push(stageXZ(annulus, 0, 9))
  // v4: finger pad raised (top 1.1 below the body rim instead of 3.1) with a
  // dia 16 x 0.7 mm finger dish. Built as two stacked extrusions (solid disc
  // + annulus rim) — a 3D coaxial-cylinder dish subtract CRACKS the STL
  // (20 bad edges), same failure as the v3 coaxial-bore rule.
  parts.push(cylY(0, 10.2, 0, 12.2, 3.4, 64)) // pad base disc, Y 8.5..11.9
  const rim = subtract(circ2(0, 0, 12.2, 64), circ2(0, 0, 8, 48))
  parts.push(stageXZ(rim, 11.7, 12.6)) // dish rim, Y 11.7..12.6 (0.2 overlap)
  parts.push(box3c(-1, 1, 0, 9, 11.2, 13.4)) // key rib (back), 1 mm into wall
  // magnet boss: 6.2 x 6.2, proud 2.2 of the front wall (local Z -14.4..-11.4,
  // 0.8 into the wall), Y 1.2..7.4, with a 6 x 6.2 x 2.0 pocket for the 6x2 N52
  // disc (open at front)
  let bossProf = rect2(-3.1, 3.1, -14.4, -11.4)
  bossProf = subtract(bossProf, rect2(-3, 3, -14.4, -12.4))
  parts.push(stageXZ(bossProf, 1.2, 7.4))
  return parts
}
// Press cap rest position: bottom on can top. Can top Y = nb_h + (stem_protrusion - stem_socket_h) + can_body_h
function canTopY () { return P.nb_h + (P.stem_protrusion - P.stem_socket_h) + P.can_body_h_19ml }
function pressCapPlaced () {
  return buildPressCap().map(g => translate([P.can_cx, canTopY(), P.can_cz], g))
}

// ---------------- nozzle blocks (local frame: bottom Y=0, centered X=0, Z=0) ----------------
// Shared outer: 20 x 18 x 18 rounded rect (r=1.5). Placed at body (14, 0, 23.5).
// Screw holes dia 1.8 (M2 self-tap) along X at (Y=2.5, Z=0) and (Y=6.5, Z=0).
// NOTE: the outer profile is SOLID (no 2D holes), so 3D subtracts here are safe
// (proven watertight in isolation). Cutters protrude past every face they exit.
function nozzleOuter () {
  let prof = roundedRectangle({ size: [P.nb_w, P.nb_d], roundRadius: P.nb_r, center: [0, 0] })
  return stageXZ(prof, 0, P.nb_h)
}
function nozzleScrews (g) {
  return subtract(g, cylX(-5.5, 2.5, 0, 0.9, 11, 16), cylX(-5.5, 6.5, 0, 0.9, 11, 16))
}
// TRANSPLANT (preferred): pocket 12x12x10 from top (Y 8..18), spray tunnel
// dia 4 at Y=10.5 from pocket front to block front (Z -10..-5). NO separate
// stem hole — the pocket IS the stem passage (a stem hole ending at the pocket
// floor would be geometrically redundant and its cap coplanar with the floor).
// The tunnel (Y 8.5..12.5) clears the pocket floor (Y=8) by 0.5 mm.
function buildNozzleTransplant () {
  let g = nozzleOuter()
  g = subtract(g,
    box3c(-6, 6, 8, 18.5, -6, 6),      // transplant pocket (measure your cut!)
    cylZ(0, 10.5, -7.5, 2, 5, 24)      // spray tunnel dia 4, Z -10..-5
  )
  return [nozzleScrews(g)]
}
// PRINTED needle-orifice (EXPERIMENTAL — changes the drug path; validate first):
// Single dia 3.3 bore (Y 9.5..18.5): the stem socket is its top 6.5 mm
// (Y 11.5..18, slip fit on the 3.16 mm stem) and the sump is its bottom 2 mm
// (Y 9.5..11.5, 17.1 mm3, in the 12-20 band). One diameter on purpose: a
// stepped coaxial socket+sump cracks in JSCAD's boolean (64 boundary edges at
// the step — the same coaxial-cylinder failure as a subtracted bore). Needle
// pocket dia 0.95 at Y=10, Z -10..-1 (epoxy a 23G blunt needle, trim flush).
function buildNozzlePrinted () {
  let g = nozzleOuter()
  g = subtract(g,
    cylY(0, 14, 0, P.stem_bore_d / 2, 9, 24),   // stem bore dia 3.3, Y 9.5..18.5
    cylZ(0, 10, -5.5, P.needle_pocket_d / 2, 9, 12) // needle pocket dia 0.95, Z -10..-1
  )
  return [nozzleScrews(g)]
}

// ---------------- reed sled (local: X -3..3, Z -1.15..1.15, Y 0..15) ----------------
// 6.0 mm wide to ride the widened reed groove without slop. U-profile in X-Z
// (reed pocket 2.4 wide x 2.0 deep, open toward +Z = toward can), extruded 15 mm.
// Glass reed 2 x 14 mm.
function buildReedSled () {
  let prof = rect2(-3, 3, -1.15, 1.15)
  prof = subtract(prof, rect2(-1.2, 1.2, -0.85, 1.15))
  return [stageXZ(prof, 0, 15)]
}
// Sled placement for a 19 ml can: reed centre ~2 mm below magnet at rest.
// Magnet centre body Y = canTopY + 4.3 = 84.6 -> reed centre 82.6 -> sled Y 75.1..90.1.
// Sled Z centre 6.85 (Z 5.7..8.0); magnet face at 9.1 -> 1.1 mm clearance.
function reedSledPlaced () {
  return buildReedSled().map(g => translate([P.can_cx, 75.1, 6.85], g))
}

// ---------------- spacer washer (dia 17.5, hole dia 6, 1 mm) ----------------
function buildSpacer () {
  let prof = subtract(circ2(0, 0, 8.75, 48), circ2(0, 0, 3, 24))
  return [extrudeLinear({ height: 1 }, prof)]
}

// ---------------- button cap (axis Z: hat dia 7.5 x 1.2 + stem dia 3.8 x 5.8) ----------------
function buildButtonCap () {
  return [
    cylZ(0, 0, 0.6, 3.75, 1.2, 32),  // hat, Z 0..1.2
    cylZ(0, 0, 3.85, 1.9, 6.3, 20)   // stem, Z 0.7..7 (0.5 overlap into hat)
  ]
}

// ---------------- hinge pin (dia 1.8 x 27, axis X) ----------------
function buildHingePin () {
  return [cylX(0, 0, 0, P.hinge_pin_d / 2, 27, 16)]
}

// ---------------- assemblies ----------------
function nozzleTransplantPlaced () {
  return buildNozzleTransplant().map(g => translate([P.can_cx, 0, P.can_cz], g))
}
function hingePinPlaced () {
  return buildHingePin().map(g => translate([14.5, P.hinge_y, P.hinge_z], g))
}
function buttonCapPlaced () {
  return buildButtonCap().map(g => translate([P.button_cx, P.button_cy, 0], g))
}
function assemblyFolded () {
  return [
    ...buildBody(), ...buildFaceplate(), ...buildBackplate(),
    ...nozzleTransplantPlaced(), ...pressCapPlaced(), ...mouthpieceFolded(),
    ...hingePinPlaced(), ...reedSledPlaced(), ...buttonCapPlaced()
  ]
}
function assemblyUnfolded () {
  return [
    ...buildBody(), ...buildFaceplate(), ...buildBackplate(),
    ...nozzleTransplantPlaced(), ...pressCapPlaced(), ...mouthpieceUnfolded(),
    ...hingePinPlaced(), ...reedSledPlaced(), ...buttonCapPlaced()
  ]
}

module.exports = {
  P, buildBody, buildFaceplate, buildBackplate, buildMouthpiece,
  mouthpieceFolded, mouthpieceUnfolded, buildPressCap, pressCapPlaced,
  buildNozzleTransplant, buildNozzlePrinted, nozzleTransplantPlaced,
  buildReedSled, reedSledPlaced, buildSpacer, buildButtonCap, buttonCapPlaced,
  buildHingePin, hingePinPlaced, assemblyFolded, assemblyUnfolded, canTopY
}
