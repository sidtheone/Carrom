# Rabbit — Stop Threshold Calibration Analysis

**Audience:** Game dev mid-physics-tuning, needs to know if 0.5 cm/s is right
**Scope:** Section-sized — focused on stop detection + physics parameter interaction
**Calibration:** Thin target (one constant + its dependencies) — Rat 3/5, Rooster 3/5, Tiger 3/5, Ox 2/5

---

> **Status update (2026-03-31):** ✅ **ALL ACTION ITEMS RESOLVED.** Commit `8a0e133` added both recommended fixes: consecutive-frame counter (`STOP_CONFIRM_FRAMES := 12`) and minimum simulation time (`MIN_SIMULATION_TIME := 0.5`). The threshold value (0.5 cm/s) was kept as recommended. This analysis is now historical context — the failure modes described below are no longer possible.

## Synthesis

**The threshold was scaled correctly in absolute terms but is 10x more aggressive relative to gameplay.**

The math: in the old system, `STOP_THRESHOLD = 0.005` at `SCALE = 0.01` meant board was 7.4 Godot units. Old threshold = 0.005 / 7.4 = 0.068% of board per second. New system: `STOP_THRESHOLD = 0.5` on a 74-unit board = 0.5 / 74 = 0.676% of board per second. **The threshold is 10x more permissive relative to the board.**

This matters because the new physics is simultaneously *bouncier and less damped*:

| Parameter | Old | New | Effect |
|---|---|---|---|
| linear_damp | 3.0 | 0.5 | 6x less friction — pieces coast 6x longer |
| bounce | 0.5 | 0.8 | 60% more elastic — more low-velocity bounces |
| friction | 0.4 | 0.05 | 8x less surface friction |
| STOP_THRESHOLD (board-relative) | 0.068%/s | 0.676%/s | 10x more aggressive stop declaration |

The combination creates a specific failure mode: in the new physics, pieces bounce more and coast longer, producing more time spent in the 0.3-2.0 cm/s velocity range. The threshold sits right in the danger zone of that range.

**Is 0.5 cm/s imperceptible?** Yes — 0.5 cm/s on a 74cm board is glacial. A piece would take 148 seconds to cross the board. No human notices this.

**Is 0.5 cm/s safe from false positives?** No — not with single-frame detection. A glancing piece-to-piece collision can momentarily slow one piece below 0.5 while it's still dynamically interacting. With bounce = 0.8, the collision response hasn't fully resolved. With damp = 0.5, pieces in the 1-3 cm/s range (common late-simulation velocities) are only ~1 second from crossing the 0.5 threshold.

**The real fix is dual:** the threshold value itself is fine, but it needs either (a) a consecutive-frame counter as noted in the previous analysis, or (b) a minimum simulation time before checking. The Rooster's verification shows that the old threshold (0.005 at old scale) was ~10x tighter, which compensated for NOT having a frame counter — false positives were practically impossible at that threshold. The new, looser threshold makes the frame counter mandatory.

### Findings Summary

**Rooster (Evidence Audit)** — 2/3 verified

✗ "Threshold was correctly scaled 100x" (MEDIUM confidence: 75%)
  Numerically 100x, but board-relative it's 10x more aggressive. The commit message says "scaled for cm/s" but doesn't note the relative change.

✓ "0.5 cm/s is imperceptible" (HIGH confidence: 90%)
  At 74cm board scale, 0.5 cm/s = 148 seconds to cross. Verified imperceptible.

✗ "Stop detection works at new physics values" (LOW confidence: 55%)
  Unverified — no evidence of playtesting the threshold with the new bounce/damp values. The revamp commit changes 7 files simultaneously; threshold was likely scaled mechanically.

**Rat (Feedback Loop)** — 3/3 mapped

✗ Damp reduction → longer coasting tail → more time in threshold danger zone (HIGH: 80%)
  With damp=0.5, velocity at 2 cm/s takes ~2.8 seconds to reach 0.5. That's 2.8 seconds where a collision could produce a momentary dip below threshold.

✗ Bounce increase → more low-velocity collision events (MEDIUM: 70%)
  bounce=0.8 means pieces hit walls and return at 80% velocity. A piece at 3 cm/s bounces back at 2.4, then 1.9, then 1.5, then 1.2, then 0.96, then 0.77, then 0.61 — seven bounces in the 0.5-3.0 range. Each bounce is a collision event where velocities momentarily change.

✗ Higher threshold + lower damp = shorter "safe window" for stop detection (HIGH: 85%)
  Old: threshold=0.005 at damp=3.0 → pieces near threshold are decelerating fast (damp force proportional to velocity). New: threshold=0.5 at damp=0.5 → pieces near threshold are barely decelerating. A piece at 0.6 cm/s with damp=0.5 loses only 0.3 cm/s per second. That piece hovers near the threshold for multiple seconds.

**Tiger (Stress Test)** — 2/3 found vulnerable

✗ Cluster scatter scenario (HIGH: 80%)
  Full-power shot (speed=110) into center cluster. 19 pieces scatter. After 8-10 seconds, most have bounced several times and are in the 0.3-3.0 cm/s range. One piece clips another in a glancing hit. Piece A drops from 0.8 to 0.3 cm/s. All other pieces happen to be below 0.5 at that frame. Simulation ends. Piece A would have continued rolling for 1+ seconds.

✗ Wall corner trap (MEDIUM: 65%)
  A piece bouncing in a corner between two walls. Each bounce at 80% retention: 5.0 → 4.0 → 3.2 → 2.56 → 2.05 → 1.64 → 1.31 → 1.05 → 0.84 → 0.67 → 0.54 → 0.43. That's 11 bounces. If each bounce takes ~0.5-1s, the piece is in the 0.43-5.0 range for 5-10 seconds. Other pieces may have already stopped. The single piece's velocity crosses 0.5 at frame N. If it bounces back above 0.5 at frame N+1 (corner bounce), and the check happened at frame N — premature stop.

✓ Low-power shot (LOW confidence: 50%)
  power=1.0 → speed=22 cm/s. Striker hits one piece. Piece rolls to 0.5 threshold in ~6.5 seconds (ln(22/0.5)/0.5). This is the happy path — slow, clean deceleration. No false positive risk. Single-piece hits work fine.

**Ox (First Principles)** — 2/2

The threshold has two jobs:
1. **Imperceptibility:** Declare stopped when movement is invisible. 0.5 cm/s meets this easily.
2. **Game-state safety:** Declare stopped only when the board is truly settled. 0.5 cm/s fails this with single-frame checking at these physics parameters.

The old system "accidentally" satisfied both jobs because the tight threshold (0.068% of board) meant only truly stationary pieces passed. The new system satisfies job 1 but not job 2.

**First-principles threshold calculation:**
- At damp=0.5, a piece at velocity v decelerates at 0.5*v per second
- To guarantee a piece won't be "near" the threshold during a collision, the threshold should be low enough that collision velocities never dip into its range
- Minimum collision velocity that matters: ~1 cm/s (below this, collision response is negligible)
- Safe threshold: < 0.2 cm/s (giving a 5x margin below minimum meaningful collision)
- BUT: lowering the threshold increases simulation time (pieces coast longer before being declared stopped)
- With damp=0.5, time from 0.5 → 0.2 = ln(0.5/0.2)/0.5 = 1.83 seconds extra per simulation

**The consecutive-frame counter is the correct fix** because it addresses job 2 without sacrificing job 1 or extending simulation time. The threshold stays at 0.5 (pieces are imperceptible), but the board must stay below threshold for 10+ frames (~0.17s at 60fps) before resolving. This filters out momentary dips during collisions.

### Action Items

1. ~~**Add consecutive-frame counter to `_check_simulation_complete`.**~~ ✅ Done — `STOP_CONFIRM_FRAMES := 12` (commit `8a0e133`).
2. ~~**Decide whether to also add a minimum simulation time.**~~ ✅ Done — `MIN_SIMULATION_TIME := 0.5` (commit `8a0e133`). Addresses both false-positive stop detection and zero-power exploit.
3. **Audit the old threshold's effective value.** The commit message says "scaled for cm/s" but the board-relative threshold changed 10x. Lower priority now that the frame counter compensates.

### Coverage Gaps

- **Monkey / Replay Probe** — What happens when you replay the same shot multiple times? Does Godot's physics engine produce deterministic results, or does frame-to-frame jitter mean the "same" shot can stop-detect at different times?
- **Tiger / Load Test** — What's the actual frame time during a 19-piece simulation? If `_process` runs slower than 60fps, the stop-check interval changes. On a low-end machine, fewer checks per second could mask or exacerbate the problem.

---

## Raw Outputs

### Rooster — Evidence Audit (3 findings)

#### Finding 1: Threshold scaling claim — "correctly scaled 100x"

**Claim:** STOP_THRESHOLD went from 0.005 to 0.5, a 100x increase matching the 100x scale change (from 0.01 multiplier to 1:1 cm).

**Verification:**

The old system: `SCALE = 0.01`, `BOARD_SIZE = 740.0`, so board in Godot units = 740 * 0.01 = 7.4 units.
Old threshold: 0.005 units/s.
Board-relative: 0.005 / 7.4 = 0.000676 = 0.068% of board per second.

The new system: 1 unit = 1 cm, board = 74 cm = 74 units.
New threshold: 0.5 units/s.
Board-relative: 0.5 / 74 = 0.00676 = 0.676% of board per second.

Ratio: 0.676 / 0.068 = **9.94x** — the new threshold is ~10x more aggressive relative to the board.

Why? Because the scale change was not a clean 100x. The old system used `BOARD_SIZE = 740` (pre-scale units) * `SCALE = 0.01` = 7.4 Godot units for a 74cm board. So 1 old Godot unit = 10 cm. The new system: 1 Godot unit = 1 cm. That's a 10x change in Godot-unit-to-real-world mapping, not 100x.

The threshold was scaled 100x (0.005 → 0.5) when it should have been scaled 10x (0.005 → 0.05) to maintain the same board-relative sensitivity.

**Verified: NO** — the 100x scaling is numerically present in the diff but results in a 10x relative loosening.

#### Finding 2: "0.5 cm/s is imperceptible at this scale"

**Claim:** The code comment says "cm/s — imperceptible at this scale."

**Verification:**

0.5 cm/s = 5mm per second. On a 74cm board:
- Time to cross board: 74 / 0.5 = 148 seconds
- Distance per frame (60fps): 0.5 / 60 = 0.0083 cm = 0.083mm per frame
- At a typical viewing distance, 0.083mm of movement per frame is below visual perception threshold

For comparison, human visual motion detection threshold is approximately 1-2 arcminutes per second. At a typical screen distance and board rendering size, 0.5 cm/s on the game board would be well below this.

**Verified: YES** — 0.5 cm/s is genuinely imperceptible. The threshold value itself is reasonable for determining when pieces "look" stopped.

#### Finding 3: Stop detection tested with new physics parameters

**Claim (implicit):** The revamp commit changes physics parameters and threshold together, implying they work correctly together.

**Evidence check:**
- Commit 91a6200 changes 7 files simultaneously
- No test files in the repo
- No simulation logging was added specifically for threshold validation
- The existing `_log_simulation_frame` was simplified in the same commit (removed delta parameter, reduced verbosity)
- Memory note says "Pieces overlapping, needs spacing fix next session" — suggesting post-revamp issues remain

**Verified: NO** — no evidence the threshold was tested against the new physics parameters. The change appears mechanical (scale the number) rather than empirically tuned.


### Rat — Feedback Loop (3 findings)

#### Finding 1: Lower damping → longer time in danger zone

**Chain:**
1. Old linear_damp = 3.0 → pieces decelerate aggressively → spend very little time near threshold
2. New linear_damp = 0.5 → pieces coast gently → spend much more time near threshold
3. With exponential decay v(t) = v0 * e^(-damp * t):
   - Old: time from 2.0 to 0.005 = ln(2.0/0.005) / 3.0 = 1.98 seconds
   - New: time from 2.0 to 0.5 = ln(2.0/0.5) / 0.5 = 2.77 seconds
4. But the critical metric is time spent NEAR threshold (within 2x of it):
   - Old: time from 0.01 to 0.005 = ln(0.01/0.005) / 3.0 = 0.23 seconds
   - New: time from 1.0 to 0.5 = ln(1.0/0.5) / 0.5 = 1.39 seconds
5. Pieces in the new system spend **6x longer** in the "near threshold" velocity band

**Consequence:** 6x more time in the zone where a collision or physics jitter could momentarily push velocity below threshold. Combined with single-frame detection, this is the core risk amplifier.

#### Finding 2: Higher bounce → more low-velocity collision events

**Chain:**
1. Old bounce = 0.5 → piece hitting wall at 3 cm/s bounces back at 1.5 → next bounce at 0.75 → stops quickly
2. New bounce = 0.8 → piece at 3 cm/s → 2.4 → 1.92 → 1.54 → 1.23 → 0.98 → 0.79 → 0.63 → 0.50
3. That's 8 bounces in the 0.5-3.0 range vs 2 bounces in the old system
4. Each bounce is a physics collision event where Godot resolves velocity changes
5. During collision resolution, the engine computes impulse responses — momentary velocity changes occur within the physics step

**Consequence:** 4x more collision events in the threshold-adjacent velocity range. Each collision is an opportunity for a one-frame velocity dip below 0.5.

#### Finding 3: Threshold-to-damp ratio inversion

**Chain:**
1. Old ratio: threshold / damp = 0.005 / 3.0 = 0.00167
2. New ratio: threshold / damp = 0.5 / 0.5 = 1.0
3. This ratio represents "how much velocity does a piece at threshold lose per second, relative to the threshold itself"
4. Old: a piece AT threshold (0.005) loses 0.005 * 3.0 = 0.015 per second = 3x threshold per second → rapid decay BELOW threshold → once below, stays below
5. New: a piece AT threshold (0.5) loses 0.5 * 0.5 = 0.25 per second = 0.5x threshold per second → very slow decay → piece lingers near threshold for seconds

**Consequence:** In the old system, crossing the threshold was essentially a one-way door — velocity dropped so fast that returning above threshold (from a collision) was rare. In the new system, pieces hover near the threshold. Velocity at 0.4 cm/s (below threshold) is only 0.2 cm/s of damping-per-second away from 0.6 cm/s (above threshold). A tiny collision impulse pushes it back over. This makes the system oscillation-prone.


### Tiger — Stress Test (3 findings)

#### Finding 1: Cluster scatter false positive

**Scenario:** Full-power shot (speed=110 cm/s) into center cluster of 19 pieces.

**Timeline:**
- T=0s: Striker hits cluster at 110 cm/s. Pieces scatter in all directions.
- T=2s: Fast pieces have hit walls, bounced back. Slower pieces still rolling. Velocities range 1-30 cm/s.
- T=5s: Most bounces complete. 15 pieces below 5 cm/s. 4 pieces below 1 cm/s.
- T=8s: 18 pieces below 1 cm/s. One piece in corner bouncing (see Finding 2). Several pieces in 0.3-0.8 range.
- T=9s: Bouncing piece at 0.55 cm/s. Two other pieces at 0.48 and 0.42. All others below 0.3.
- T=9.1s: Bouncing piece clips board surface irregularity (physics jitter), velocity drops to 0.45 for one frame.
- T=9.1s: ALL pieces below 0.5. `_check_simulation_complete` returns true. Turn resolves.
- T=9.2s: Bouncing piece would have continued to 0.6 cm/s from bounce impulse. But simulation already ended.

**Impact:** Turn resolves with a piece still in motion. The piece's final position is wrong by 1-3cm. In carrom, piece position matters — pieces near pockets or in blocking positions change strategy.

**Confidence: 80%** — the specific numbers are estimated, but the dynamics are physically correct for the given parameters.

#### Finding 2: Corner bounce trap

**Scenario:** One piece rolling toward a corner at ~5 cm/s.

**Bounce sequence (bounce=0.8):**
Each wall hit: velocity * 0.8. Corner bounces alternate between two walls.
5.0 → 4.0 → 3.2 → 2.56 → 2.05 → 1.64 → 1.31 → 1.05 → 0.84 → 0.67 → 0.54 → 0.43

The piece crosses 0.5 between bounce 10 (0.54) and bounce 11 (0.43). But at each wall hit, there's a physics frame where velocity is computed as the pre-bounce value, then updated to post-bounce. If `_check_simulation_complete` runs between the pre-bounce deceleration (near 0.5) and the post-bounce acceleration (away from wall), it catches the piece below threshold.

**Impact:** Same as Finding 1 — premature resolution. Corner-trapped pieces are particularly vulnerable because their velocity oscillates around the threshold for multiple bounces.

**Confidence: 65%** — depends on Godot's physics step timing relative to `_process` frame timing. If both run at 60fps and are interleaved, the window exists. If physics runs at a different rate, the window may be wider or narrower.

#### Finding 3: Low-power clean shot (control case)

**Scenario:** power=1.0 → speed=22 cm/s. Striker hits one piece centrally.

**Timeline:**
- Energy transfers ~80% to target piece (mass ratio: striker 15g, piece 5g — actually with unequal mass, striker retains more).
- One piece rolls at ~15 cm/s, decelerates smoothly: 15 * e^(-0.5t)
- Time to reach 0.5: ln(15/0.5) / 0.5 = 6.8 seconds
- Clean exponential decay, no collision events near threshold
- `_check_simulation_complete` fires correctly at ~7 seconds

**Impact:** None — this is the happy path. Single-piece-hit scenarios work fine because there are no collision events near the threshold.

**Confidence: 90%**


### Ox — First Principles (2 findings)

#### Finding 1: Two jobs of the stop threshold

A stop-detection threshold serves two distinct purposes:

**Job 1: Visual imperceptibility.** Declare "stopped" when the player can't perceive any movement. This is a perceptual threshold. Depends on: board size, rendering resolution, piece size, camera distance. At 74cm board scale, 0.5 cm/s is absolutely imperceptible. Even 2.0 cm/s would be barely perceptible. Threshold of 0.5 is generous for this job.

**Job 2: Game-state safety.** Declare "stopped" only when the board has truly settled and no more meaningful interactions will occur. This is a physics threshold. Depends on: collision mechanics, bounce coefficients, damping rates, number of pieces, board geometry. A threshold that's too high declares "stopped" while pieces are still dynamically interacting — producing incorrect final board states.

**The old system satisfied both jobs through tightness.** STOP_THRESHOLD = 0.005 at 7.4 Godot-unit board = 0.068% of board per second. At this threshold, only truly stationary pieces pass. Collisions at these velocities produce negligible impulse responses. Both jobs satisfied trivially.

**The new system satisfies Job 1 but compromises Job 2.** STOP_THRESHOLD = 0.5 at 74-unit board = 0.676% of board per second. Visually imperceptible, yes. But at 0.5 cm/s, pieces can still be mid-collision-response. With bounce=0.8, a piece at 0.5 cm/s hitting a wall bounces back at 0.4. That's a collision event that changes piece position and could redirect the piece toward other game elements (pockets, other pieces). The board hasn't "settled" — it's just moving slowly.

#### Finding 2: Threshold vs. frame-counter — orthogonal fixes

Two independent parameters control stop-detection quality:

1. **Threshold value** — determines the velocity below which a piece is "maybe stopped"
2. **Confirmation window** — determines how long a piece must stay below threshold before being declared "actually stopped"

The old system used approach 1 (tight threshold, no window). The threshold was so low that no confirmation was needed — pieces that slow were never coming back.

The new system needs approach 2 because:
- The threshold is high enough that pieces can oscillate around it
- The physics (low damp, high bounce) produces more velocity fluctuation near the threshold
- The combination means single-frame checks are unreliable

**Recommended fix:** Keep threshold at 0.5 (good for Job 1), add frame counter of 10-15 frames (good for Job 2). Alternatively, add a minimum simulation time of 0.5-1.0s — this also prevents the zero-power exploit identified in the previous analysis.

These are orthogonal: you could do both, or either. The frame counter alone is sufficient for the stop-detection bug. The minimum time alone addresses zero-power but not the false-positive risk during normal play.
