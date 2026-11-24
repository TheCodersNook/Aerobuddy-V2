# Verify PuffPal v4 STLs: watertight per shell, bbox, pocket voids, hinge sweep.
# NOTE: the body STL is 23 OVERLAPPING shells, so trimesh contains() on the
# merged mesh gives parity garbage. Every containment test below checks each
# shell separately and ORs the results (union semantics).
# Usage: PYTHONPATH=/home/hatch/.local/lib/python3.12/site-packages python3 jscad/verify-v4.py
import os
import numpy as np, trimesh

HERE = os.path.dirname(os.path.abspath(__file__))
STL = os.path.join(HERE, '..', 'stl')
FILES = ['puffpal-v4-body.stl', 'puffpal-v4-faceplate.stl', 'puffpal-v4-backplate.stl',
         'puffpal-v4-mouthpiece.stl', 'puffpal-v4-press-cap.stl',
         'puffpal-v4-nozzle-transplant.stl', 'puffpal-v4-nozzle-printed.stl',
         'puffpal-v4-reed-sled.stl', 'puffpal-v4-spacer-1mm.stl',
         'puffpal-v4-button-cap.stl', 'puffpal-v4-hinge-pin.stl',
         'puffpal-v4-assembly-folded.stl', 'puffpal-v4-assembly-unfolded.stl',
         'puffpal-v4-combined.stl']

print('== watertight / bad edges (per shell) ==')
all_ok = True
for f in FILES:
    m = trimesh.load(os.path.join(STL, f))
    bad = 0; nwt = 0
    for p in m.split(only_watertight=False):
        _, counts = np.unique(p.edges_sorted, axis=0, return_counts=True)
        bad += int((counts != 2).sum())
        nwt += 0 if p.is_watertight else 1
    ok = (bad == 0 and nwt == 0)
    all_ok &= ok
    print(f'{f:38s} bad_edges={bad:3d} non_watertight={nwt} {"OK" if ok else "FAIL"}')
print('ALL WATERTIGHT:', all_ok)

print('== folded assembly bbox ==')
asm = trimesh.load(os.path.join(STL, 'puffpal-v4-assembly-folded.stl'))
size = asm.bounds[1] - asm.bounds[0]
print('size mm:', np.round(size, 2), 'inches:', np.round(size / 25.4, 3),
      'limit 101.6 x 50.8:', 'PASS' if size[1] <= 101.6 and size[0] <= 50.8 else 'FAIL')

body = trimesh.load(os.path.join(STL, 'puffpal-v4-body.stl'))
shells = body.split(only_watertight=False)

def inside_any(pts):
    out = np.zeros(len(pts), dtype=bool)
    for s in shells:
        out |= s.contains(pts)
    return out

def void_free(name, x0, x1, y0, y1, z0, z1, n=6):
    xs = np.linspace(x0, x1, n); ys = np.linspace(y0, y1, n); zs = np.linspace(z0, z1, max(2, n // 2))
    pts = np.array([[x, y, z] for x in xs for y in ys for z in zs])
    k = int(inside_any(pts).sum())
    print(f'{name:28s} samples={len(pts)} inside_solid={k}', 'PASS' if k == 0 else 'FAIL')
    return k == 0

print('== void checks (union semantics) ==')
# NB test boxes stay strictly inside round/odd voids: cap bore is a circle
# (box corners would be wall), the bay box avoids the screw bosses at (29,39)/(40,39).
void_free('screen pocket', 9.5, 40.5, 51.5, 88.3, 2.2, 3.2)
void_free('FPC channel', 18, 31, 44.5, 49.8, 2.2, 5.2)
void_free('button pocket', 18.5, 23.7, 44.4, 49.9, 2.2, 6.8)
void_free('can bore Y=50', 8, 20, 49.9, 50.1, 17.5, 29.5)
void_free('cap bore Y=80', 7, 21, 79.9, 80.1, 16.5, 30.5)
void_free('mouthpiece pocket', 4.5, 24.5, 5, 37.5, 2.5, 8.5)
void_free('spray tunnel', 4.5, 23.5, 7, 17, 2.5, 14.0)
void_free('nozzle pocket', 4.5, 23.5, 0.5, 17.5, 15, 32)
# bay check excludes the two screw-boss cylinders (intentional solid in the bay)
_bx = np.linspace(32, 46, 6); _by = np.linspace(5, 88, 6); _bz = np.linspace(6, 36, 3)
_bpts = np.array([[x, y, z] for x in _bx for y in _by for z in _bz])
_keep = np.array([min(np.hypot(x - 29, y - 39), np.hypot(x - 40, y - 39)) > 2.6
                  for x, y, z in _bpts])
print('bay (bosses excluded) inside_solid =', int(inside_any(_bpts[_keep]).sum()),
      'PASS' if inside_any(_bpts[_keep]).sum() == 0 else 'FAIL')
void_free('reed groove', 11.5, 16.5, 25, 90, 6, 11.5)
solid_pts = np.array([[40, 50, 3.5], [25, 50, 20], [45, 93, 20], [5, 50, 36]])
print('known-solid points inside:', inside_any(solid_pts).tolist(), '(expect all True)')

fp = trimesh.load(os.path.join(STL, 'puffpal-v4-faceplate.stl'))
c = fp.triangles.mean(axis=1)
in_win = (c[:, 0] > 12) & (c[:, 0] < 38) & (c[:, 1] > 57) & (c[:, 1] < 83)
print('== faceplate window == interior triangles:', int(in_win.sum()),
      'PASS' if in_win.sum() == 0 else 'FAIL')
mid = np.array([[25.4, 47.15, 1.0]])
print('gap between button+LED holes solid:', bool(fp.contains(mid)[0]), '(expect True)')

mouth = trimesh.load(os.path.join(STL, 'puffpal-v4-mouthpiece.stl'))
hinge = np.array([14.0, 10.0, 4.5])
print('== hinge sweep (mouthpiece vs body, 0-90 deg) ==')
worst = 0
for deg in range(0, 95, 5):
    th = np.radians(-deg)
    c_, s_ = np.cos(th), np.sin(th)
    R = np.array([[1, 0, 0], [0, c_, -s_], [0, s_, c_]])
    allp = np.vstack([(R @ mouth.vertices.T).T + hinge,
                      (R @ mouth.triangles.mean(axis=1).T).T + hinge])
    n_in = int(inside_any(allp).sum())
    worst = max(worst, n_in)
    print(f'  {deg:2d} deg: points inside body = {n_in}')
print('hinge sweep:', 'PASS (collision-free)' if worst == 0 else f'FAIL ({worst} intrusions)')

print('== volumes (solid mm^3) ==')
tot = 0.0
for f in FILES[:11]:
    m = trimesh.load(os.path.join(STL, f))
    v = sum(p.volume for p in m.split(only_watertight=False) if p.is_watertight)
    tot += v
    print(f'  {f:38s} {v:9.1f} mm^3')
print(f'  printed total (both nozzle variants incl.) {tot:9.1f} mm^3')
