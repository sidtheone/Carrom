# Rabbit — Game Loop Analysis

**Audience:** Game developer building a Carrom board game in Godot 4, intermediate level
**Scope:** Section-sized — 6 files, ~680 lines. Direct analysis.
**Calibration:** Medium density — Tiger 4/5, Rat 4/5, Snake 3/5, Ox 3/5

---

## Synthesis

The game loop is clean for its stage: a 4-state machine (PLACE_STRIKER -> AIM -> POWER -> SIMULATION) centralized in a GameManager autoload, with input handling delegated to striker.gd and presentation to hud.gd. Signal-driven, readable, no spaghetti. The architecture is sound for a local 2-player carrom game.

Three things need attention now, one needs a decision, and the rest can wait.

**The simulation stop-detection is fragile.** `_check_simulation_complete()` polls every `_process` frame and resolves the turn the instant all velocities dip below 0.5 cm/s. Physics collisions can momentarily slow pieces below threshold mid-bounce — a piece ricocheting off a wall or another piece passes through a near-zero velocity at the inflection point. This is the highest-risk bug: premature turn resolution mid-simulation would corrupt game state silently. Fix: require N consecutive frames below threshold (a simple counter), or only check after a minimum simulation time.

**Zero-power shots are a free pass.** Releasing the power button immediately fires `_shoot_striker()` with `power = 0.0`, producing `speed = 0.0`. Simulation starts, all pieces are already stopped, `_resolve_turn()` fires instantly. No foul, no penalty — the current player just gets another placement. Depending on intent: either enforce a minimum power, or treat zero-power as cancellation (return to AIM).

**Visibility is load-bearing game state.** `piece.visible` is the canonical "in play" check — used in stop detection, win checking, piece counting, foul handling, and HUD updates. If any future system (animation, VFX, LOD) toggles visibility for rendering reasons, game logic breaks. This is architectural debt — not urgent, but the earlier you add an `is_in_play` flag, the cheaper the migration.

**Encapsulation break in striker.gd:99** — the striker directly calls `GameManager._set_state()` (a private method) to cancel power and return to AIM. This works but creates an implicit state transition outside the state machine owner. Consider exposing a `cancel_power()` method on GameManager instead.

### Findings Summary

**Tiger (Stress Test)** — 2/4 critical

- `_check_simulation_complete` (HIGH) — Single-frame velocity check can false-positive during mid-collision deceleration. Silent corruption.
- `_return_piece_to_center` overlap (MEDIUM) — Spacing formula `2.0 + count * 1.5` with random jitter has no collision check against existing center pieces. Can stack returned pieces.
- Negative scores (LOW) — `_return_opponent_pieces` subtracts points with no floor. Score can go negative. May be intentional.
- Camera during P2 turn (LOW) — Camera flips 180 degrees but input mapping only negates `world_x` for placement. Aim angle negation relies on direction flip in `_shoot_striker` line 176. Works but the inversion logic is spread across 3 locations.

**Rat (Consequences)** — 2/4 notable

- Zero-power shot (HIGH) — `power=0.0` -> `speed=0.0` -> instant simulation resolve -> free turn. Unintentional pass mechanic.
- Striker foul overrides queen evaluation (MEDIUM) — `_resolve_turn` returns early on striker foul (line 214), never evaluating queen coverage. If striker + queen are both pocketed in same shot, queen state is silently orphaned: `queen_pocketed_by` stays set but `_handle_foul` doesn't address it.
- Silent striker script failure (LOW) — `load("res://scripts/striker.gd")` at board.gd:296 — if load fails, striker has no input handling. No error, no fallback.
- Audio player pool exhaustion (LOW) — 8 pooled AudioStreamPlayers. A fast multi-piece collision chain could exhaust the pool, interrupting the oldest sound. Acceptable for now.

**Snake (Scope)** — 2/3 cuttable

- Debug print statements everywhere (MEDIUM) — 15+ print() calls across game_manager.gd and board.gd. Not behind a debug flag. Will spam the console in release.
- `_sim_frame_count` / `_sim_log_interval` infra (LOW) — Simulation logging scaffolding. Useful during physics tuning, removable after.
- `_physics_process` in game_manager.gd (LOW) — Only logs first 3 frames of simulation. Could be folded into `_log_simulation_frame` or removed.

**Ox (First Principles)** — 2/3 architectural

- Visibility as game state (MEDIUM) — `piece.visible` doubles as the "is this piece in play" flag across 6+ callsites. Conflates rendering with game logic.
- Private method called externally (MEDIUM) — striker.gd:99 calls `GameManager._set_state()` directly. Breaks encapsulation of the state machine.
- Mouse-only input (LOW) — All input is mouse-based. No abstraction layer for touch/gamepad. Fine for now, expensive to retrofit later.

### Action Items

1. **Fix simulation stop-detection.** Add a consecutive-frame counter (e.g., 10 frames below threshold) before resolving. This is the only finding that can silently corrupt a game in progress.
2. **Decide on zero-power behavior.** Either enforce `MIN_POWER` (e.g., 0.5) or treat release-at-zero as cancel-back-to-AIM. Current behavior is a loophole.
3. **Audit striker-foul + queen interaction.** Determine correct carrom rules: if striker is pocketed on the same shot that pockets the queen, does the queen return to center? If so, `_handle_foul` needs to reset `queen_pocketed_by`.
4. **Add `cancel_power()` to GameManager.** Replace the direct `_set_state` call in striker.gd:99 with a public method. Small change, prevents future state-transition bugs.
5. **Gate debug prints behind a flag.** A `const DEBUG := true` check or Godot's `OS.is_debug_build()` wrapper. Low effort, cleans up release output.

### Coverage Gaps

- **Monkey / Hostile Input** — What happens with extremely rapid clicks during state transitions? Could rapid left-click during SIMULATION skip ahead?
- **Dragon / Temporal Analysis** — How does this architecture scale to online multiplayer? The autoload singleton pattern and frame-based polling would need significant rework.
- **Rat / Feedback Loop** — What are the downstream effects of the physics revamp (noted in memory) on the stop-detection threshold? The 0.5 cm/s threshold may need recalibration.

---

*Full raw outputs below.*

---

## Raw Outputs

### Tiger — Stress Test (4 findings)

**Attack angle: State machine boundary conditions and physics edge cases**

#### Finding 1: Premature simulation termination (HIGH confidence: 85%)

**Target:** `game_manager.gd:196-205` — `_check_simulation_complete()`

The stop-detection logic checks every `_process` frame whether all pieces have velocity below `STOP_THRESHOLD` (0.5 cm/s). In physics simulation, rigid body collisions involve momentary deceleration — a piece hitting a wall decelerates to near-zero before bouncing back. During this inflection point, velocity can dip below 0.5 cm/s for a single frame.

**Attack scenario:** Striker hits a cluster near a wall. Multiple pieces bounce. During the collision resolution (which spans multiple physics frames but is checked in `_process`), pieces momentarily slow below threshold. `_check_simulation_complete` fires. Turn resolves. Pieces that would have continued moving are now frozen mid-trajectory because `_resolve_turn` changes state away from SIMULATION.

**Why this matters:** The turn resolution logic (`_resolve_turn`) makes irreversible decisions — returning pieces, switching players, checking wins. A false-positive stop detection causes these to fire on incomplete board state.

**Mitigation:** Require N consecutive frames (10-15) below threshold before declaring simulation complete. This is standard practice in physics-based games.

#### Finding 2: Returned piece overlap (MEDIUM confidence: 70%)

**Target:** `game_manager.gd:259-268` — `_return_piece_to_center()`

Spacing formula: `dist = 2.0 + _return_count * 1.5` with `angle = _return_count * PI * 2.0 / 3.0 + randf_range(-0.3, 0.3)`.

For `_return_count = 0`: dist = 2.0 cm from center.
Piece radius = 1.8 cm.

If there are already pieces near the center (e.g., from a previous foul return, or pieces that never moved from initial spawn), the returned piece lands on top of them. No overlap detection, no nudging. Physics will eventually push them apart (gravity_scale=0 but collision response is active), but the impulse could send pieces flying unpredictably.

**Attack scenario:** Player 1 fouls. Their piece returns to center. Queen is still at (0, 0.2, 0) from spawn. Returned piece lands at ~2cm from center — overlapping with queen (radius 1.8cm, so pieces overlap if centers are < 3.6cm apart). Physics explosion.

#### Finding 3: Negative scores (LOW confidence: 60%)

**Target:** `game_manager.gd:271-279` — `_return_opponent_pieces()`

Line 278: `scores[current_player - 1] -= points` when returning opponent pieces pocketed this turn. No floor check. If a player repeatedly pockets opponent pieces (earning points on pocket, then losing them on return), the subtraction could drive score negative — though the earn-then-subtract nets to zero per returned piece. However, if the scoring logic changes or edge cases arise, the lack of a floor is a latent risk.

Actually, re-examining: `on_piece_pocketed` adds points immediately (line 358), and `_return_opponent_pieces` subtracts them back. Net effect is zero for returned pieces. Score only goes negative if there's a bug elsewhere. LOW risk.

#### Finding 4: P2 input inversion spread (LOW confidence: 50%)

**Target:** `striker.gd:62-63`, `game_manager.gd:128`, `game_manager.gd:175-177`

Player 2 input inversion is handled in 3 separate places:
- Placement: `world_x = -world_x` in striker.gd:63
- Striker position: `placement_z` flipped in game_manager.gd:128
- Shoot direction: `direction.x` and `direction.z` negated in game_manager.gd:175-177

This works but the inversion logic is scattered. A change to one location without updating the others would silently break P2 gameplay.


### Rat — Consequence Map (4 findings)

**Mapping: Second-order effects of game loop decisions**

#### Finding 1: Zero-power instant turn (HIGH confidence: 90%)

**Target:** `game_manager.gd:159-191` — `release_power()` / `_shoot_striker()`

**Chain:**
1. Player enters POWER state
2. Player immediately releases mouse (before any charging)
3. `power = 0.0` (initialized at line 152)
4. `speed = power * 22.0 = 0.0`
5. `striker.linear_velocity = direction * 0 = Vector3.ZERO`
6. State changes to SIMULATION
7. Next `_process` frame: all pieces already below threshold
8. `_check_simulation_complete` -> `_resolve_turn`
9. No pieces pocketed, no foul -> `_switch_turn` or `_set_state(PLACE_STRIKER)` depending on `own_piece_pocketed`

**Consequence:** Since `own_piece_pocketed` is reset to `false` at line 189, and nothing was pocketed, `_switch_turn()` fires. The current player effectively "passed" their turn with no penalty. In carrom, you must strike. This is either a missing rule or an exploitable gap.

**Second-order:** If a player is losing, they can stall by zero-power shooting repeatedly, hoping the opponent makes mistakes. No game clock means infinite stalling.

#### Finding 2: Striker foul + queen orphaning (MEDIUM confidence: 75%)

**Target:** `game_manager.gd:208-235` — `_resolve_turn()`

**Chain:**
1. Player pockets queen (queen_pocketed_by = current_player, queen_covered = false)
2. Same shot: striker goes in too
3. `_resolve_turn`: striker_pocketed is true -> `_handle_foul()` -> return
4. `_handle_foul` returns one of the player's pieces, switches turn
5. Queen coverage check (lines 216-224) was NEVER reached
6. `queen_pocketed_by` remains set to the fouling player
7. Next turn: the OTHER player is now current_player
8. On their next `_resolve_turn`, queen coverage check: `queen_pocketed_by == current_player` is FALSE (it's set to the OTHER player)
9. Queen state is orphaned — pocketed_by is set but can never be covered because the covering logic only checks `current_player`

**Consequence:** Queen is permanently in limbo. It's invisible (pocketed) but `queen_pocketed_by` points to a player who can't cover it because covering is only checked on the pocketing player's turn. The queen never returns to play.

**Correct behavior (standard carrom rules):** If the striker is pocketed on the same shot as the queen, the queen should be returned to center.

#### Finding 3: Striker script load failure (LOW confidence: 65%)

**Target:** `board.gd:296-297`

```gdscript
var striker_script: Resource = load("res://scripts/striker.gd")
if striker_script:
    striker.set_script(striker_script)
```

If `load()` returns null (file missing, parse error), the condition silently skips script assignment. The striker becomes a physics body with no input handling. The game enters PLACE_STRIKER state but nothing can advance it — soft lock.

**Consequence:** Player sees the game, pieces are placed, but clicking does nothing. No error message, no recovery path.

#### Finding 4: Audio pool saturation (LOW confidence: 50%)

**Target:** `audio_manager.gd:56-66`

8 AudioStreamPlayers in pool. Fast multi-collision chain (striker hits cluster, 5+ pieces hit walls simultaneously) could request 8+ sounds in one frame. Oldest sound gets interrupted.

**Consequence:** Barely perceptible. Carrom collisions are short sounds. Pool of 8 is generous for typical play. Only matters in edge cases with large cluster breaks.


### Snake — Scope Cut (3 findings)

**Question: What can be removed or simplified?**

#### Finding 1: Debug print saturation (MEDIUM — Earned: no)

15+ `print()` calls across game_manager.gd and board.gd:
- `_ready` prints (gm:59, board:276-280, 305-307)
- State transition prints (gm:117)
- Shoot prints (gm:184-185)
- Simulation frame logging (gm:100-101)
- Physics process logging (gm:83-85)
- Pocket prints (implied by state transitions)

None are behind a debug flag. All fire in release builds. The `_log_simulation_frame` function alone fires every 30 frames during simulation.

**Verdict:** Either wrap in `if OS.is_debug_build():` or use Godot's built-in `@tool` / print_debug patterns. Or strip entirely — the git history preserves them.

#### Finding 2: Simulation logging infrastructure (LOW — Earned: no)

`_sim_frame_count`, `_sim_log_interval`, `_log_simulation_frame()` — this is physics debugging scaffolding. Useful during the recent physics revamp, but once tuning is done, it's dead code.

**Verdict:** Keep during active physics tuning. Remove when physics is stable.

#### Finding 3: Redundant `_physics_process` logging (LOW — Earned: no)

`game_manager.gd:82-85` — Logs striker velocity for first 3 frames of simulation only. This is a subset of what `_log_simulation_frame` already logs. Redundant.

**Verdict:** Remove. `_log_simulation_frame` covers this.


### Ox — First Principles (3 findings)

**Question: Is the architecture appropriate for what this is?**

#### Finding 1: Visibility as canonical game state (MEDIUM confidence: 75%)

`piece.visible` is checked in:
- `_check_simulation_complete` (gm:200) — stop detection
- `_handle_foul` (gm:250) — finding pieces to return
- `_return_queen_to_center` (gm:289) — queen state check
- `_check_win` (gm:302-303) — win condition
- `_update_piece_counts` (hud:120) — HUD display
- `_log_simulation_frame` (gm:94) — debug logging

**Principle:** Game state should be authoritative, not derived from rendering state. `visible = false` means "this piece is pocketed" — but it ALSO means "don't render this." If you ever need to hide a piece visually without removing it from play (e.g., animation, pocket entry effect, slow-motion replay), the conflation breaks.

**Counter-argument:** For a 2-player local game at this stage, the conflation is practical. Adding `is_pocketed: bool` metadata to each piece and keeping it in sync with visibility is more code for no current benefit.

**Verdict:** Architectural debt. Note it, don't fix it yet unless you're adding visual effects on pocketing.

#### Finding 2: State machine encapsulation break (MEDIUM confidence: 80%)

`striker.gd:99`: `GameManager._set_state(GameManager.State.AIM)`

This is a direct call to a private method (`_` prefix convention) from an external script. The state machine's transitions should be owned by GameManager. Every other transition goes through a public method (`confirm_placement`, `confirm_aim`, `release_power`) — except this one.

**Principle:** A state machine's transitions should have a single owner. External actors request transitions; the owner validates and executes them.

**Fix:** Add `func cancel_power() -> void:` to GameManager that does the AIM transition internally. One-line method, preserves encapsulation.

#### Finding 3: Mouse-only input binding (LOW confidence: 55%)

All input in striker.gd is `InputEventMouseMotion` and `InputEventMouseButton`. No Godot Input Actions used for the core gameplay loop (only `reset_board` uses an action at gm:63).

**Principle:** Godot's Input Map system exists to decouple logical actions from physical inputs. Using raw mouse events ties the game to mouse input permanently. Adding touch or gamepad later means rewriting striker.gd.

**Counter-argument:** For a mouse-aimed billiards-style game, mouse input IS the game feel. Abstracting it through Input Actions would lose the direct mouse-position-to-world-position mapping that makes placement and aiming work. This is a legitimate design choice, not an oversight.

**Verdict:** Acceptable for mouse-targeted gameplay. Only becomes debt if you want mobile/gamepad support.
