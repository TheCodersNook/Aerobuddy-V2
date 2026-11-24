# Dose-logic simulation results

Scripted run of `dose_logic_sim.py` — mirrors the firmware
state machine in `src/main.cpp` (400 ms debounce, day
counter, view cycling, long-press reset with confirm).

**10/10 cases passed.**

- PASS — 1. switch bounce burst (12 edges / 33 ms)
- PASS — 2. rapid double-press (200 ms vs 600 ms)
- PASS — 3. normal day (4 puffs, hourly)
- PASS — 4. midnight rollover (uptime days)
- PASS — 5. long-press reset + confirm timeout
- PASS — 6. heavy-use day (12 puffs)
- PASS — 7. empty canister (200 + 5 presses)
- PASS — 8. button view cycling (4 taps)
- PASS — 9. SETTIME midnight split (UTC days)
- PASS — 10. 7-day week totals

Proven: no double-counts (bounce + rapid double-press),
no missed doses (normal / heavy-use / empty-canister days),
correct daily + weekly totals, correct reset behaviour
(confirm resets, timeout cancels, lifetime total kept).
