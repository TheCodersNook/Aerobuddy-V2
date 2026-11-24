#!/usr/bin/env python3
"""AeroBuddy v2 (PuffPal v4) — structural sanity, ANALYTIC hand-calcs.

NOT FEA. Closed-form beam / shear / bearing checks with published PETG
properties, to size the safety margins and name what needs a real test.

Geometry (v4 build notes + v3 lineage):
  - Hinge: 2 shell knuckles (X 1..3.5 and 25.5..28 => each 2.5 mm wide),
    mouthpiece arms 2 mm wide each, pin = 1.75 mm filament (PETG) or
    printed 1.8 mm pin, bore 2.15 mm. Knuckle OD ~4 mm (bore 2.15 + wall).
    Mouthpiece length 28 mm (lever arm for fold force).
  - Press cap: 1 mm wall (PETG), pad dia 24.4 mm, finger dish.
  - Thin walls: faceplate ~1.4 mm pocket floor, back plate ~2 mm,
    shell walls ~2 mm (4 walls at 0.45 mm line width ~1.8 mm).
  - Can valve spring / fire force: pMDI actuation force is typically
    15-30 N (literature: ~20 N typical for HFA pMDI). Use 25 N design.

PETG (printed, FDM) properties used:
  - Tensile strength XY ~45 MPa, Z (layer adhesion) ~25 MPa (conservative;
    published FDM PETG Z-strength 20-30 MPa).
  - Shear strength ~25 MPa (approx 0.55 x tensile).
  - Design allowable: 1/3 of Z-strength for sustained/cyclic load = 8 MPa
    (fatigue + print variability knockdown), 1/2 for one-off peak = 12 MPa.
"""
import math

# --- 1. Hinge knuckle: cantilever / bearing during fold ---
# Fold force: user pushes mouthpiece tip with ~5 N (generous; friction only).
# Worst case: tip snagged, user yanks with 15 N at 28 mm lever.
F_fold = 15.0          # N, worst-case yank
L_lever = 28.0         # mm
M_hinge = F_fold * L_lever   # N-mm total moment at hinge
# Moment resisted by 2 knuckles; each knuckle is a cantilever lug.
# Bearing stress on pin: pin dia 1.75 mm, bearing length per knuckle 2.5 mm.
d_pin = 1.75
b_knuckle = 2.5
# Pin shear: double shear across 2 knuckles + 2 arms; force per shear plane
# ~ F_fold/2 per side. Shear area = pi*d^2/4.
A_pin = math.pi * d_pin**2 / 4
tau_pin = (F_fold / 2) / A_pin          # MPa (N/mm2), per shear plane
# Knuckle bearing: force per knuckle F/2 over projected area d*b.
sigma_bearing = (F_fold / 2) / (d_pin * b_knuckle)
# Knuckle root bending: the 28 mm lever moment is transmitted through the
# PIN as bearing force, NOT as bending at the lug root. The lug root sees
# the pin force (F/2 per knuckle) acting at the pin-centre-to-root arm
# (~2 mm, the knuckle radius). M_root = (F/2) * 2 mm.
M_root = (F_fold / 2) * 2.0
Z_lug = 2.5 * 1.8**2 / 6
sigma_lug_full = M_root / Z_lug

# --- 2. Press force vs PETG layer adhesion (thin walls) ---
# Fire force 25 N through press cap (1 mm wall) into can top.
F_fire = 25.0
# Cap wall: cylinder dia 24.4 mm, wall 1 mm, load carried in compression
# across the wall annulus. Area = pi*D*t.
A_cap = math.pi * 24.4 * 1.0
sigma_cap = F_fire / A_cap
# Can bore wall: the can slides in a 22.8 mm bore; side load from a
# tilted press ~ 25 N x 0.1 (10% off-axis) = 2.5 N on the bore wall over
# a contact patch ~5 mm x 10 mm.
sigma_bore = 2.5 / 50.0
# Faceplate pocket floor (1.4 mm) under button press ~5 N: the floor is
# a local pocket bounded by walls ~10 mm apart, backed by the body --
# use the pocket span (10 mm), NOT the 50 mm plate width (that strip
# model gives 19 MPa and is over-conservative: the plate is not a free
# 50 mm beam, it is screwed to the body at 4 points and pocketed).
sigma_plate = 3 * 5.0 * 10 / (2 * 10 * 1.4**2)

# --- 3. Press-cap load path / nozzle block screws ---
# 2x M2 self-tapping screws hold the nozzle block; can spring pushes the
# block down with the fire force (25 N) shared by 2 screws in shear.
A_screw = math.pi * 2.0**2 / 4   # M2 core ~1.6 mm, use 1.6
A_screw = math.pi * 1.6**2 / 4
tau_screw = (F_fire / 2) / A_screw
# Pull-out: thread engagement ~4 mm in PETG, pull-out area = pi*d*L.
tau_pullout = (F_fire / 2) / (math.pi * 2.0 * 4.0)

ALLOW_PEAK = 12.0   # MPa, one-off peak allowable (Z-strength / 2)
ALLOW_CYCLIC = 8.0  # MPa, cyclic allowable (Z-strength / 3)

def margin(stress, allow):
    return allow / stress if stress > 0 else float("inf")

print("ANALYTIC HAND-CALCS ONLY -- NOT FEA. PETG Z-strength 25 MPa assumed;")
print("allowables: 12 MPa peak, 8 MPa cyclic (knockdowns for print variance).")
print()
rows = [
    ("Hinge pin shear (15 N yank)", tau_pin, ALLOW_PEAK),
    ("Knuckle bearing on pin", sigma_bearing, ALLOW_PEAK),
    ("Knuckle root bending (15 N yank)", sigma_lug_full, ALLOW_PEAK),
    ("Press-cap wall compression (25 N)", sigma_cap, ALLOW_CYCLIC),
    ("Can-bore wall, off-axis press", sigma_bore, ALLOW_CYCLIC),
    ("Faceplate pocket floor, button 5 N", sigma_plate, ALLOW_CYCLIC),
    ("Nozzle-block screw shear (25 N / 2)", tau_screw, ALLOW_PEAK),
    ("Nozzle-block screw pull-out", tau_pullout, ALLOW_CYCLIC),
]
print(f"{'Check':<42} {'Stress':>8} {'Allow':>7} {'Margin':>7}")
for name, s, a in rows:
    print(f"{name:<42} {s:>7.2f} MPa {a:>5.1f} MPa {margin(s, a):>6.1f}x")
print()
print("Weakest link: knuckle root bending, 1.1x at a 15 N yank -- but that")
print("is a worst-case yank; at a realistic 5 N fold force the margin is")
print("3.2x. Next thinnest: nozzle-block screw shear (1.9x at 25 N fire")
print("force; conservative -- the block also bears on the pocket floor)")
print("and faceplate pocket floor (2.1x). All other margins >3.8x.")
print("Press path margins are otherwise >10x: the can valve spring (25 N)")
print("is small against PETG section sizes. The real risks are FATIGUE")
print("and CREEP, not one-off yield -- hence PETG (not PLA) and the")
print("1,000-fold + 100-press acceptance tests in the build notes.")
