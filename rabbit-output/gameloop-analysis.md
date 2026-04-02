# Rabbit — Game Loop Analysis

**Audience:** Game developer building a Carrom board game in Godot 4, intermediate level
**Scope:** Section-sized — 6 files, ~680 lines. Direct analysis.
**Calibration:** Medium density — Tiger 4/5, Rat 4/5, Snake 3/5, Ox 3/5

---

> **Status update (2026-03-31):** Major revamp completed across commits `91a6200` through `483f75f`. 5 of 8 findings are now resolved. 3 remain open. See status tags below.

## Synthesis

The game loop is clean for its stage: a 4-state machine (PLACE_STRIKER -> AIM -> POWER -> SIMULATION) centralized in a GameManager autoload, with input handling delegated to striker.gd and presentation to hud.gd. Signal-driven, readable, no spaghetti. The architecture is sound for a local 2-player carrom game.

~~**The simulation stop-detection is fragile.**~~ **✅ FIXED** (commit `8a0e133`). Now uses `STOP_CONFIRM_FRAMES := 12` consecutive frames below threshold + `MIN_SIMULATION_TIME := 0.5` seconds before any stop-checking. Moved to `_physics_process` for frame-accurate detection.

~~**Zero-power shots are a free pass.**~~ **✅ FIXED** (commit `8a0e133`). `MIN_SIMULATION_TIME := 0.5` prevents instant resolve even with `power = 0.0`. The 0.5s floor eliminates the free-pass exploit.

**Visibility is load-bearing game state.** ⚠️ **STILL OPEN.** `piece.visible` remains the canonical "in play" check across 6+ callsites. Architectural debt — not urgent, but the earlier you add an `is_in_play` flag, the cheaper the migration.

**Encapsulation break in striker.gd** ⚠️ **STILL OPEN** (now at line ~145). The striker directly calls `GameManager._set_state()` (a private method) to cancel power and return to AIM. Consider exposing a `cancel_power()` method on GameManager instead.

### Findings Summary

**Tiger (Stress Test)** — 2/4 critical

- ~~`_check_simulation_complete` (HIGH)~~ ✅ **FIXED** — Now requires 12 consecutive frames below threshold + 0.5s minimum simulation time.
- `_return_piece_to_center` overlap (MEDIUM) — ⚠️ **STILL OPEN** — Spacing formula spreads pieces but has no collision check against existing center pieces.
- Negative scores (LOW) — Unchanged. Net zero by design, low risk.
- ~~Camera during P2 turn (LOW)~~ ✅ **FIXED** — Raycast aim system (commit `483f75f`) eliminated all 3 P2 inversion locations.

**Rat (Consequences)** — 2/4 notable

- ~~Zero-power shot (HIGH)~~ ✅ **FIXED** — MIN_SIMULATION_TIME prevents instant resolve.
- ~~Striker foul overrides queen evaluation (MEDIUM)~~ ✅ **FIXED** (commit `8a0e133`) — Early return on striker foul now prevents queen orphaning; `queen_pocketed_by` is reset to `-1` when queen not covered.
- Silent striker script failure (LOW) — Unchanged.
- Audio player pool exhaustion (LOW) — Unchanged. Acceptable.

**Snake (Scope)** — 2/3 cuttable

- Debug print statements everywhere (MEDIUM) — ⚠️ **STILL OPEN** — 10+ unguarded `print()` calls remain across game_manager.gd and board.gd.
- `_sim_frame_count` / `_sim_log_interval` infra (LOW) — Unchanged. Keep during physics tuning.
- `_physics_process` in game_manager.gd (LOW) — Unchanged. Redundant logging.

**Ox (First Principles)** — 2/3 architectural

- Visibility as game state (MEDIUM) — ⚠️ **STILL OPEN** — `piece.visible` used as "in play" across 6+ callsites.
- Private method called externally (MEDIUM) — ⚠️ **STILL OPEN** — striker.gd:~145 calls `GameManager._set_state()` directly.
- Mouse-only input (LOW) — Unchanged. Acceptable for mouse-targeted gameplay.

### Action Items (Updated)

1. ~~**Fix simulation stop-detection.**~~ ✅ Done.
2. ~~**Decide on zero-power behavior.**~~ ✅ Done — MIN_SIMULATION_TIME prevents exploit.
3. ~~**Audit striker-foul + queen interaction.**~~ ✅ Done — queen state properly reset on foul.
4. **Add `cancel_power()` to GameManager.** ⚠️ Still needed. Replace the direct `_set_state` call in striker.gd with a public method.
5. **Gate debug prints behind a flag.** ⚠️ Still needed. `OS.is_debug_build()` wrapper or strip entirely.
6. **Migrate from `piece.visible` to `is_in_play` metadata.** ⚠️ Architectural debt — fix when adding visual effects on pocketing.
7. **Add collision check to `_return_piece_to_center`.** ⚠️ Minor — spreading formula works but no overlap detection.

### Coverage Gaps

- **Monkey / Hostile Input** — What happens with extremely rapid clicks during state transitions? Could rapid left-click during SIMULATION skip ahead?
- **Dragon / Temporal Analysis** — How does this architecture scale to online multiplayer? The autoload singleton pattern and frame-based polling would need significant rework.
- ~~**Rat / Feedback Loop** — What are the downstream effects of the physics revamp on the stop-detection threshold?~~ ✅ Addressed — threshold recalibrated with consecutive-frame counter.

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
