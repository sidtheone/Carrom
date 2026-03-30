# XXI The World — Game Loop Reading

**Target:** Carrom game loop (6 GDScript files, ~1200 lines)
**Type:** Codebase / architecture
**Density:** Medium
**Spread:** Death + Tower + Moon
**Date:** 2026-03-30

---

## Synthesis

The game loop's state machine is clean — 4 states, clear transitions, correct physics. The architecture is appropriate for this scale. But the turn resolution logic has **score accounting bugs** that compound across foul handling and queen returns.

When the striker is pocketed (foul), one of the current player's previously-pocketed pieces is returned via `_handle_foul()`. But the score awarded when that piece was originally pocketed is never subtracted. The player profits from a foul. Similarly, when the queen is returned for not being covered, the 50-point queen score remains on the scoreboard. `_return_queen_to_center()` resets `queen_pocketed_by` but never touches `scores[]`.

A secondary cluster of issues lives in piece-return physics. `_return_piece_to_center()` places pieces at calculated offsets but doesn't check for existing pieces at those positions. The inner offset (2.0 cm) is inside the initial piece layout ring (3.7 cm), making overlap with the queen or nearby pieces possible during mid-game returns. Additionally, returned pieces are unfrozen and potentially moving while the state machine has already transitioned to `PLACE_STRIKER`.

---

## Tension Map

### Singleton coupling vs. practical simplicity
**Tower** says: Striker.gd directly mutates `GameManager.is_charging`, `GameManager.power`, and calls the private `GameManager._set_state()` (line 99). The state machine has uncontrolled entry points.
**Moon** says: The coupling works fine for every happy path. The risk is dark paths — a future change that adds a state or validation check to `_set_state` won't automatically apply when striker.gd bypasses it.
**The tradeoff:** Refactoring to signal-based input would add complexity for theoretical safety. The private-function call is the only actually dangerous coupling. Fix that one line.

### Stop detection timing
**Death** says: `_check_simulation_complete()` runs in `_process()` (variable framerate), reading physics velocities that are updated in `_physics_process()` (fixed rate). Architecturally wrong — could read stale data.
**Tower** says: It works in practice because Godot exposes last-step velocity, and the threshold is conservative (0.5 cm/s). Moving to `_physics_process` is trivial and correct.
**The tradeoff:** Low risk, easy fix. Just move it.

### Agreements
- Score accounting is broken during fouls — Death and Moon both independently found the bugs
- Returned pieces can overlap existing pieces — Death found the offset math, Moon found the consequence

### Blind Spots
- No simulation timeout — Moon found this. Low probability but no safety net.

---

## Action Items

1. **Fix foul score accounting.** `_handle_foul()` must subtract the score for the returned piece. `_return_queen_to_center()` must subtract SCORE_QUEEN.
2. **Add overlap check to `_return_piece_to_center()`.** Verify no existing visible piece occupies the target position within a collision radius.
3. **Decide whether returned pieces should block the next turn.** Currently, state transitions to PLACE_STRIKER while returned pieces may still be moving.
4. **Move `_check_simulation_complete()` to `_physics_process()`.** Trivial change, architecturally correct.
5. **Add a simulation timeout.** After ~15 seconds, force-stop all pieces and resolve the turn.

## Coverage Gaps

- **Hermit / Deep Dive** — The piece layout math uses mixed spacing constants (`d` vs `d2`) that could cause initial overlaps.
- **Magician / Existing Resources** — Godot 4 has `ShapeCast3D` and `PhysicsServer3D.space_get_direct_state()` for overlap queries on return-to-center.
- **Fool / Fresh Eyes** — Screen-to-world mapping for placement/aiming uses linear mouse X interpolation, which may feel off at non-standard aspect ratios.

---

## Raw Card Outputs

### Death XIII — 6 Findings

#### Finding 1: Zero-power shot creates instant turn resolution
- **Arcanum:** Hostile Input
- **Confidence:** 90
- **Survived:** yes

If `power == 0.0` when released, `_shoot_striker()` (game_manager.gd:167) sets `speed = 0 * 22 = 0`. Striker velocity is `Vector3.ZERO`. State transitions to SIMULATION. On the very next `_process` frame, `_check_simulation_complete()` sees all velocities below threshold and fires `_resolve_turn()` immediately. The turn is wasted (no foul, just a skip). There's no minimum power check — a player can accidentally (or intentionally) fire a zero-power shot.

**Impact:** Gameplay UX issue, not a crash. Player loses their turn silently.

---

#### Finding 2: Stop detection runs in `_process`, not `_physics_process`
- **Arcanum:** State Violation
- **Confidence:** 70
- **Survived:** yes (practically)

`_check_simulation_complete()` is called from `_process()` (game_manager.gd:79), which runs at variable framerate. It reads `linear_velocity.length()` from RigidBody3D nodes whose velocities are updated in `_physics_process()` (fixed rate, typically 60fps).

In Godot 4, physics bodies expose their last-step velocity to `_process`, so the values aren't garbage. But if `_process` runs faster than physics (high FPS), consecutive checks read the same stale velocity. If `_process` runs slower, a stop event could be delayed. The threshold (0.5 cm/s) is conservative enough that this rarely matters in practice.

**Impact:** Architecturally incorrect. Works due to conservative threshold. Easy to fix by moving the check to `_physics_process`.

---

#### Finding 3: Returned pieces can overlap existing pieces near center
- **Arcanum:** Assumption Flip
- **Confidence:** 85
- **Survived:** no

`_return_piece_to_center()` (game_manager.gd:259) calculates placement offsets:
```gdscript
var dist := 2.0 + _return_count * 1.5  # cm offset from center
```

For `_return_count == 0`, the piece is placed 2.0 cm from center. The queen sits at (0, 0.2, 0) — center of the board. Pieces in the inner ring of the initial layout are at ~3.7 cm from center. A returned piece at 2.0 cm could overlap the queen (radius 1.8 cm) if the queen is still on the board.

No spatial query is performed before placement. If an existing piece occupies the target position, Godot's physics will attempt separation, potentially causing pieces to fly apart (physics explosion).

**Impact:** Mid-game fouls where pieces are returned can cause visible physics glitches. More likely as the game progresses and pieces cluster near center.

---

#### Finding 4: Queen coverage logic handles pocketing order correctly
- **Arcanum:** Assumption Flip
- **Confidence:** 60
- **Survived:** yes

The order in which pieces enter pockets during simulation is non-deterministic (physics-dependent). `on_piece_pocketed()` sets `queen_pocketed_by` and `own_piece_pocketed` as independent flags. `_resolve_turn()` checks both flags together after all motion stops.

Whether the queen enters the pocket before or after the covering piece doesn't matter — both flags are set during simulation and evaluated atomically at resolution time. The logic is correct.

---

#### Finding 5: Foul handling keeps score for all pocketed pieces, only returns one
- **Arcanum:** Delete Probe
- **Confidence:** 80
- **Survived:** no

When a piece is pocketed (`on_piece_pocketed`, game_manager.gd:327), the score is immediately added (line 358). When the striker is pocketed (foul), `_resolve_turn()` calls `_handle_foul()` (line 213) and returns immediately — skipping `_return_opponent_pieces()`.

`_handle_foul()` (line 237) returns ONE of the current player's pocketed pieces to center. But it never subtracts the score for that piece or any other pieces pocketed during the foul turn.

**Scenario:** Player 1 pockets 2 black pieces (+20 points) and the striker in one shot. Foul fires. One black piece is returned. Score remains +20 instead of correcting to +10 (or +0 per strict rules).

**Impact:** Score inflation on fouls. Players can exploit this by pocketing pieces even if they expect to foul.

---

#### Finding 6: Returned pieces can be moving while state is already PLACE_STRIKER
- **Arcanum:** State Violation
- **Confidence:** 75
- **Survived:** no

In `_resolve_turn()`:
1. `_return_opponent_pieces()` (line 226) unfreezes returned pieces (`piece.freeze = false`, line 267)
2. `_check_win()` (line 228) runs
3. State transitions to `PLACE_STRIKER` (line 232 or via `_switch_turn()`)

Returned pieces are now unfrozen and may have velocity from physics overlap resolution. But the state machine is already in PLACE_STRIKER. No code waits for returned pieces to settle. The player can place and shoot the striker while penalty pieces are sliding.

**Impact:** Visual glitch and potential for the player to hit a moving returned piece, creating unpredictable physics.

---

### Tower XVI — 3 Findings

#### Finding 1: Uncontrolled singleton mutation
- **Confidence:** 85
- **Load-bearing:** The singleton pattern is fine; the uncontrolled access is not.

Every script directly reads and writes GameManager properties:
- `striker.gd:90` — `GameManager.is_charging = true` (direct state mutation)
- `striker.gd:97-98` — `GameManager.is_charging = false; GameManager.power = 0.0`
- `striker.gd:99` — `GameManager._set_state(GameManager.State.AIM)` (calling private function)

The `_set_state` call is the most concerning. The `_` prefix convention marks it as private/internal. External callers bypass any validation that might be added later. If `_set_state` gains a guard clause (e.g., "can't go to AIM from SIMULATION"), striker.gd would bypass it.

**Recommendation:** Add a public `cancel_power()` function to GameManager that striker.gd calls instead of directly manipulating state. This centralizes the state machine transitions.

---

#### Finding 2: Board as builder + event router
- **Confidence:** 50
- **Load-bearing:** yes, appropriate for scale

Board.gd creates all game objects (pieces, striker, walls, pockets) and assigns references to GameManager (lines 271, 304). It also routes pocket collision signals to GameManager (line 157).

This is standard Godot architecture — the scene script builds the scene and connects signals. The board doesn't own the objects after creation; GameManager owns the game logic. The split is reasonable.

No structural issue found.

---

#### Finding 3: Dead code in camera_controller.gd
- **Confidence:** 90
- **Tear down:** yes

`_on_turn_changed` (camera_controller.gd:58-59) is connected to `GameManager.turn_changed` but the body is just `pass`. Camera transitions happen via `state_changed` + `current_player` lookup.

The ordering dependency is subtle: `_switch_turn()` must set `current_player` before calling `_set_state()`. Currently correct (game_manager.gd:283-285), but the empty callback gives a false impression that turn changes are handled separately.

**Recommendation:** Remove the dead callback and its signal connection.

---

### Moon XVIII — 3 Findings

#### Finding 1: Foul penalty doesn't subtract score for returned piece
- **Confidence:** 90

`_handle_foul()` (game_manager.gd:237) finds a hidden piece of the current player's color and calls `_return_piece_to_center()`. It never adjusts the score.

When a piece was pocketed, `on_piece_pocketed()` (line 358) added its score immediately: `scores[current_player - 1] += points`. When the piece is returned as a foul penalty, the score should be subtracted. It isn't.

**Code path:**
1. Player 1 pockets black piece → score += 10
2. Player 1 pockets striker → foul
3. `_handle_foul()` returns one black piece to center → score unchanged (should be -= 10)
4. Player 1 keeps 10 points for a pocketed piece that's back on the board

---

#### Finding 2: Queen return doesn't subtract 50-point score
- **Confidence:** 90

When the queen is pocketed but not covered:
1. `on_piece_pocketed()` adds 50 points (SCORE_QUEEN) to the current player
2. `_resolve_turn()` line 219-224: queen not covered → `_return_queen_to_center()` + `queen_pocketed_by = -1` + `_switch_turn()`
3. `_return_queen_to_center()` (line 288) moves the queen back and makes it visible. Does NOT touch scores.

The queen is RED, so it's excluded from `_return_opponent_pieces()` which only handles BLACK and WHITE. No code path subtracts the 50 points.

**Code path:**
1. Player 1 pockets queen → score += 50
2. Player 1 doesn't cover (no own piece pocketed)
3. Queen returned to center → score unchanged (should be -= 50)
4. Player 1 keeps 50 points for a queen that's back on the board

---

#### Finding 3: No simulation timeout
- **Confidence:** 65

`_check_simulation_complete()` polls every frame with no upper bound. With `linear_damp = 0.5`, maximum theoretical settling time from max velocity (110 cm/s) to threshold (0.5 cm/s) is:

```
t = ln(110/0.5) / 0.5 ≈ 10.8 seconds
```

This assumes clean exponential decay. If a piece gets caught in a physics glitch (vibrating against a wall corner, stuck between two objects with high bounce), the velocity may oscillate above threshold indefinitely.

**Impact:** Infinite hang in SIMULATION state. No timeout, no escape (except the reset_board action which reloads the scene). Low probability with current physics values but no safety net.

**Recommendation:** Add a frame/time counter. After 15 seconds or 900 physics frames, force-zero all velocities and resolve.
