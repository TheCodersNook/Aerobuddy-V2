#!/usr/bin/env python3
"""AeroBuddy v2 (PuffPal v4) — tolerance Monte Carlo.

Fits checked:
  1. Can in bore:   can OD nominal 22.10 mm, bore 22.80 mm (radial 0.35 mm
                    per side, v4 build notes §10). Jam = diametral
                    clearance <= 0 (can OD >= bore).
  2. Stem in socket: stem OD 3.16 mm (Aptar DF30 nominal), socket bore
                    3.30 mm (v4 printed-nozzle variant; the transplant
                    variant keeps the stock socket, so this applies to
                    the printed variant only). Too tight (fires on
                    insertion / won't seat) = clearance <= 0.02 mm.
                    Too loose (leak path) = clearance >= 0.30 mm.

Distributions (all stated assumptions -- no measured data exists yet):
  - Can OD: Normal(22.10, 0.10) mm. Generics vary; +/-0.2 mm (2 sigma)
    is a conservative reading of "generics vary" (v3 notes) for a
    drawn-aluminium can. Truncated to [21.8, 22.4].
  - FDM print tolerance on bore diameters: Normal(0, 0.10) mm applied
    to the bore (holes print small on FDM, so also run a -0.10 mm mean
    bias case). Sigma 0.10 mm is typical for a tuned 0.20 mm-layer FDM
    printer on a 20-25 mm bore; 0.12 mm layer height on the nozzle block
    is a bit better, so socket sigma = 0.08 mm.
  - Stem OD: Normal(3.16, 0.03) mm (Aptar DF30 moulded stem; +/-0.06 mm
    at 2 sigma is a typical moulded-plastic tolerance).
  - Socket bore: nominal 3.30 mm + Normal(0, 0.08) mm print error.

N = 100,000 samples (task asked >= 10,000).
"""
import numpy as np

rng = np.random.default_rng(20261123)
N = 100_000

# --- Can in bore ---
can_od = np.clip(rng.normal(22.10, 0.10, N), 21.8, 22.4)
bore_print_err = rng.normal(0.0, 0.10, N)      # unbiased printer
bore = 22.80 + bore_print_err
clear_can = bore - can_od                       # diametral clearance
p_jam = np.mean(clear_can <= 0)
p_tight_can = np.mean(clear_can < 0.20)         # <0.1 mm/side: stiff slide
lo, hi = np.percentile(clear_can, [2.5, 97.5])

# Bias case: FDM holes print ~0.1 mm small on average
bore_bias = 22.80 + rng.normal(-0.10, 0.10, N)
clear_bias = bore_bias - can_od
p_jam_bias = np.mean(clear_bias <= 0)
lo_b, hi_b = np.percentile(clear_bias, [2.5, 97.5])

# --- Stem in socket (printed nozzle variant only) ---
stem_od = np.clip(rng.normal(3.16, 0.03, N), 3.07, 3.25)
socket = 3.30 + rng.normal(0.0, 0.08, N)
clear_stem = socket - stem_od
p_tight = np.mean(clear_stem <= 0.02)   # won't seat / fires on insertion
p_loose = np.mean(clear_stem >= 0.30)   # leak path
lo_s, hi_s = np.percentile(clear_stem, [2.5, 97.5])

# Bias case for socket too
socket_bias = 3.30 + rng.normal(-0.10, 0.08, N)
clear_sb = socket_bias - stem_od
p_tight_b = np.mean(clear_sb <= 0.02)
p_loose_b = np.mean(clear_sb >= 0.30)

print(f"N = {N:,}")
print()
print("CAN IN BORE (nominal clearance 0.70 mm diametral / 0.35 mm radial)")
print(f"  P(jam, clearance <= 0):        {p_jam*100:.3f}%")
print(f"  P(stiff slide, clearance <0.2): {p_tight_can*100:.2f}%")
print(f"  95% clearance band:            [{lo:.2f}, {hi:.2f}] mm")
print(f"  With -0.10 mm FDM hole bias:   P(jam) = {p_jam_bias*100:.3f}%, "
      f"95% band [{lo_b:.2f}, {hi_b:.2f}] mm")
print()
print("STEM IN SOCKET (printed nozzle variant; nominal clearance 0.14 mm)")
print(f"  P(too tight, clearance <=0.02): {p_tight*100:.2f}%")
print(f"  P(too loose, clearance >=0.30): {p_loose*100:.3f}%")
print(f"  P(any fail):                    {(p_tight+p_loose)*100:.2f}%")
print(f"  95% clearance band:             [{lo_s:.2f}, {hi_s:.2f}] mm")
print(f"  With -0.10 mm FDM hole bias:   P(too tight) = {p_tight_b*100:.2f}%, "
      f"P(too loose) = {p_loose_b*100:.3f}%")
print()
print("Note: transplant variant keeps the STOCK stem socket, so the stem")
print("Monte Carlo does not apply to it -- only the printed needle-orifice")
print("variant. The can-bore result applies to both variants.")
