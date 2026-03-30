# 猴 Monkey — Chaos Check

**Target:** Carrom game loop (all 6 GDScript files)
**Date:** 2026-03-30
**VALUES.md:** Not found at repo root

---

## Finding 1 — Assumption Flip

**Target:** `game_manager.gd:200` — `_check_simulation_complete()` uses `piece.visible` as proxy for "still in play"
**Confidence:** 92
**Impact:** breaks-build
**Survived:** no

### Observation

The simulation-complete check iterates `pieces` and skips any piece where `piece.visible == false`. The assumption: invisible = pocketed = irrelevant. But `_return_piece_to_center` (line 266) sets `piece.visible = true` and `piece.freeze = false`. If a piece is returned to center during `_resolve_turn` and gets a tiny physics nudge from an overlapping piece, its velocity could exceed `STOP_THRESHOLD` (0.5 cm/s). But `_resolve_turn` runs after simulation ends — so those returned pieces never get re-checked. They drift with nobody watching, because the state machine has already moved to `PLACE_STRIKER`.

### Consequence Chain

1. Foul occurs, `_handle_foul()` calls `_return_piece_to_center` which unfreezes the piece
2. State transitions to `PLACE_STRIKER` — `_process` no longer runs `_check_simulation_complete`
3. Returned piece overlaps another piece at center, physics pushes them apart
4. Pieces drift across the board with nobody stopping them
5. Next shot fires into a board with pieces still moving from the previous resolution
6. Those moving pieces could cross pocket Area3D triggers, getting pocketed during the *next* player's turn with incorrect attribution

---

## Finding 2 — Hostile Input

**Target:** `striker.gd:61` — mouse-to-world mapping for placement
**Confidence:** 88
**Impact:** nice-to-have
**Survived:** no

### Observation

The placement mapping is `world_x = (mouse_x - screen_w / 2.0) / screen_w * 74.0`. This maps the full screen width to 74 cm. But `GameManager.place_striker_at` clamps to `[-12.3, 12.3]` — only 33% of the board width. So only the center third of the screen actually moves the striker. The outer two-thirds of screen drag produces clamped, dead input.

### Consequence Chain

1. Player moves mouse to left edge — striker clamps at -12.3
2. Player thinks input is broken because ~67% of screen width is unresponsive
3. UX feels sluggish/broken, especially for Player 2 where `world_x = -world_x` inverts it
4. No crash, but a real playability issue

---

## Finding 3 — Existence Question (CRITICAL)

**Target:** `game_manager.gd:40-42` — `pieces` array and `striker` var populated externally by `board.gd`
**Confidence:** 95
**Impact:** breaks-build
**Survived:** no

### Observation

`GameManager` is an autoload that initializes `pieces = []` and `striker = null`. These are populated by `board.gd:271` (`GameManager.pieces.append(piece)`) and `board.gd:304` (`GameManager.striker = striker`). But `board.gd:36` calls `GameManager.start_game()` immediately after spawning. If the scene is reloaded (via `get_tree().reload_current_scene()` on line 64 or HUD restart on line 96), the autoload persists but the scene is rebuilt. `board.gd._ready()` appends to `GameManager.pieces` again — **but never clears it first**. After one restart, `GameManager.pieces` contains 38 entries (19 stale + 19 fresh). The stale entries are freed nodes.

### Consequence Chain

1. Player presses 'R' to reset or clicks Restart
2. Scene reloads, old nodes are freed
3. `board.gd._ready()` appends 19 new pieces to the existing array of 19 stale references
4. `_check_simulation_complete()` iterates all 38 entries, accessing freed objects
5. Accessing `piece.visible` or `piece.linear_velocity` on a freed node = **crash** (invalid instance)
6. Same issue with `striker` — old ref is freed, new ref replaces it (this one is fine since it's a simple assignment, not append)

---

## Finding 4 — Scale Shift

**Target:** `game_manager.gd:182` — `speed = power * 22.0` with `MAX_POWER = 5.0`
**Confidence:** 85
**Impact:** nice-to-have
**Survived:** yes

### Observation

Maximum speed = 5.0 * 22.0 = 110 cm/s. With `linear_damp = 0.5`, max travel = speed/damp = 220 cm. Board diagonal is ~105 cm. A full-power shot crosses the board approximately 3 times before stopping. This is reasonable for carrom where a hard flick can bounce multiple times. The damping model means the striker decelerates exponentially, which is adequate for gameplay.

### Consequence Chain

The math checks out. Max power produces ~3 board-widths of travel which is physically plausible. The 110 cm/s max speed with continuous CD enabled should prevent tunneling at this scale. **This assumption survived the stress test.**

---

## Finding 5 — Time Travel

**Target:** `game_manager.gd:327-368` — `on_piece_pocketed` can fire during resolution phase
**Confidence:** 90
**Impact:** breaks-build
**Survived:** no

### Observation

`on_piece_pocketed` is called by `board.gd:157` via `area.body_entered` signal from pocket Area3Ds. This is a physics callback that fires whenever a RigidBody3D enters the pocket area — including during `_resolve_turn` when pieces are teleported back to center via `_return_piece_to_center`. Pieces returned with `_return_count > 0` get offsets and are unfrozen. Multiple returned pieces could scatter and drift into pockets.

More critically: `on_piece_pocketed` modifies `scores`, `pocketed_this_turn`, `own_piece_pocketed`, `queen_pocketed_by` — all of which `_resolve_turn` is actively reading and branching on. A pocket event firing mid-resolution corrupts the turn state.

### Consequence Chain

1. Turn ends, `_resolve_turn` begins
2. Foul handling returns a piece to center, unfreezes it
3. Physics tick runs, piece drifts or gets pushed
4. Piece enters pocket Area3D — `on_piece_pocketed` fires
5. `scores[current_player - 1]` gets modified mid-resolution
6. `pocketed_this_turn` gets a new entry that `_return_opponent_pieces` may or may not process
7. State becomes inconsistent — score doesn't match reality, wrong pieces returned

---

## Finding 6 — Cross-Seam Probe

**Target:** `camera_controller.gd:58-59` — `_on_turn_changed` is a no-op
**Confidence:** 80
**Impact:** values-gap
**Survived:** yes

### Observation

The camera subscribes to `turn_changed` on line 50 but the handler does nothing (`pass`). The actual camera transition happens via `_on_state_changed` which fires when the state moves to `PLACE_STRIKER` after a turn switch. Since `_switch_turn` sets `current_player` before calling `_set_state`, the camera correctly picks the new player's preset via the `state_changed` path.

### Consequence Chain

The camera correctly transitions between players despite the dead `_on_turn_changed` handler. Signal ordering is correct. **This survived.**

---

## Finding 7 — Requirement Inversion

**Target:** `game_manager.gd:247-253` — foul penalty only returns ONE piece, no score deduction
**Confidence:** 82
**Impact:** values-gap
**Survived:** no

### Observation

In `_handle_foul` (line 248-253), the fouling player's first invisible piece is returned to the board. But `_handle_foul` does NOT deduct points for the returned piece. The player pocketed pieces (gaining points via `on_piece_pocketed`), then fouled, one piece comes back to the board, but the score stays. Also: `_return_opponent_pieces` on line 271 subtracts points when returning opponent pieces, but `_handle_foul` on line 251 does not subtract when returning the fouling player's own piece.

### Consequence Chain

1. Player 1 pockets 3 own pieces (+30 points) and the striker in the same shot
2. Foul: one piece returns to board, score stays at 30
3. Player 1 effectively got 20 free points (2 pocketed pieces that stayed)
4. Repeat: intentionally pocket striker with own pieces to game the scoring
5. Exploitable strategy makes the score system unreliable

---

## Finding 8 — Delete Probe

**Target:** `audio_manager.gd:63-65` — 8-player audio pool exhaustion
**Confidence:** 75
**Impact:** nice-to-have
**Survived:** no

### Observation

When all 8 AudioStreamPlayers are busy, the fallback (line 64-65) interrupts `_players[0]`. A fast multi-piece collision cascade (break shot hitting packed formation) could exhaust all 8 players. With 19 pieces and a striker, a direct center hit could generate 10+ collision events in one physics frame. The `_on_piece_collision` deduplicates by instance_id comparison (line 376), but each unique pair still fires. A break shot could easily generate `C(5,2) = 10` unique pair collisions in a few frames.

### Consequence Chain

1. Break shot hits packed center formation
2. 10+ collision sounds fire in rapid succession
3. After 8, sounds start interrupting each other
4. Audio becomes garbled/clicking during the most dramatic moment of the game
5. No crash, but the break shot — the most satisfying moment — sounds worst

---

## Finding 9 — Replay Probe

**Target:** `game_manager.gd:362-364` — queen limbo / stalemate potential
**Confidence:** 88
**Impact:** values-gap
**Survived:** no

### Observation

If a player pockets ONLY the queen (no own piece), `own_piece_pocketed` stays false. Turn switches. `queen_pocketed_by` is set but the cover check (line 216) only triggers when `queen_pocketed_by == current_player`. The queen CAN be covered on a future turn if the pocketing player later pockets their own piece. But if that player never pockets another own piece, the game deadlocks — neither player can win because `_check_win` requires `queen_covered and queen_pocketed_by == N`.

### Consequence Chain

1. Player 1 pockets queen only, no own piece — turn switches
2. Queen invisible, `queen_pocketed_by = 1`, `queen_covered = false`
3. If Player 1 never pockets another own piece, queen stays in permanent limbo
4. Neither player can achieve win condition
5. No stalemate detection, no timeout, no forfeit mechanism — game hangs forever

---

## Extended Finding 10

**Technique:** Assumption Flip
**Target:** `board.gd:289` — striker gets `PieceColor.BLACK` metadata
**Confidence:** 93
**Impact:** nice-to-have
**Survived:** no

### Observation

`_spawn_striker` calls `_create_piece` with `GameManager.PieceColor.BLACK` as the color parameter. The striker gets BLACK metadata. In `_handle_foul` (line 247), `var player_color := PieceColor.BLACK if current_player == 1 else PieceColor.WHITE`. If a foul occurs when Player 1 is current, it looks for invisible BLACK pieces to return. The striker is NOT in `GameManager.pieces` (stored separately in `GameManager.striker`), so the foul loop misses it. Safe by accident, not design.

### Consequence Chain

Latent bug. If anyone adds the striker to the pieces array for unified iteration, the foul handler would try to "return the striker to center" as if it were a normal piece. Fragile but not currently broken.

---

## Extended Finding 11

**Technique:** Hostile Input
**Target:** `game_manager.gd:73-74` — power charges with no minimum
**Confidence:** 85
**Impact:** nice-to-have
**Survived:** no

### Observation

Power starts at 0.0 (line 152). If the player clicks and immediately releases, `power` could be 0.0 or a tiny fraction (~0.05 at 60fps). `_shoot_striker` computes `speed = power * 22.0` = ~1.1 cm/s. The `STOP_THRESHOLD` is 0.5 cm/s. The striker barely moves, immediately triggers "all stopped," and `_resolve_turn` fires. Turn wasted silently.

### Consequence Chain

1. Quick click = near-zero power
2. Striker moves 1cm and stops
3. Turn wasted, switches to opponent
4. Feels buggy — no visual feedback that the shot was too weak

---

## Extended Finding 12

**Technique:** Cross-Seam Probe
**Target:** `board.gd:117` — pocket offset `half * 0.95` vs wall at `half`
**Confidence:** 78
**Impact:** nice-to-have
**Survived:** no

### Observation

Pockets at `35.15` cm from center. Inner wall face at `35.8` cm. Pocket sphere radius 2.8 cm extends to `37.95` cm — overlapping the wall by 2.15 cm. Pieces near the corner could simultaneously trigger the pocket Area3D AND bounce off the wall. A piece that barely enters the pocket trigger zone could bounce off the wall and escape, but `on_piece_pocketed` already fired, making the piece invisible and frozen. Ghost pocket.

### Consequence Chain

1. Piece approaches corner at shallow angle
2. Enters pocket Area3D sphere at the edge
3. `body_entered` fires, piece vanishes
4. Player sees the piece disappear near the corner, not visually "in" the pocket
5. Feels unfair — "that wasn't a pocket!" moments

---

## Extended Finding 13

**Technique:** Time Travel
**Target:** `game_manager.gd:64` — `reload_current_scene()` during SIMULATION
**Confidence:** 90
**Impact:** breaks-build
**Survived:** no

### Observation

Pressing 'R' triggers `get_tree().reload_current_scene()` at any time, including during SIMULATION. Combined with Finding 3 (pieces array never cleared), this crashes immediately. The scene reload during physics simulation could also leave Godot's physics server with dangling references.

### Consequence Chain

Duplicate of Finding 3's crash vector via a different entry point.

---

## Extended Finding 14

**Technique:** Requirement Inversion
**Target:** `game_manager.gd:278` — negative scores possible
**Confidence:** 87
**Impact:** values-gap
**Survived:** no

### Observation

In `_return_opponent_pieces` (line 278): `scores[current_player - 1] -= points`. Score can go negative. No `max(0, score)` guard. The scoring flow is: `on_piece_pocketed` adds points immediately during simulation, then `_resolve_turn` subtracts if pieces are returned. Player sees score jump up then drop — visually jarring. And scores below zero display as "P1: -30" with no UI handling.

### Consequence Chain

1. Player pockets opponent piece: +20 during simulation
2. Turn resolves, opponent piece returned: -20
3. If score was already negative from earlier fouls, it goes deeper
4. No floor on scores, confusing UI

---

## Extended Finding 15

**Technique:** Delete Probe
**Target:** `hud.gd:100` — Menu button scene change + autoload persistence
**Confidence:** 70
**Impact:** breaks-build
**Survived:** no

### Observation

`_on_menu_pressed` calls `change_scene_to_file("res://scenes/main_menu.tscn")`. Autoloads persist. `GameManager.pieces` retains stale references. When returning to game via Menu → Play, `board.gd._ready()` appends to stale array. Same crash as Finding 3.

### Consequence Chain

Duplicate crash vector of Finding 3 via the Menu → Play path.

---

## Extended Finding 16

**Technique:** Scale Shift
**Target:** `board.gd:314` + `project.godot:49` — explicit vs default damping
**Confidence:** 72
**Impact:** nice-to-have
**Survived:** yes

### Observation

Piece damping (0.5) is explicitly set on creation (line 314). Project default (0.5) is aligned but not depended on. All RigidBody3D instances have explicit damping. Changing the project default wouldn't affect gameplay.

### Consequence Chain

Good defensive coding. **This survived.**

---

## Extended Finding 17

**Technique:** Replay Probe
**Target:** `game_manager.gd:209` — `_return_count` reset at start of `_resolve_turn`
**Confidence:** 80
**Impact:** nice-to-have
**Survived:** no

### Observation

`_return_count` is an instance variable reset to 0 at line 209. Used by `_return_piece_to_center` for piece offset spacing. Currently safe because it's only called within `_resolve_turn`'s flow. But if called from elsewhere in the future, `_return_count` would have stale values, placing pieces at wrong offsets.

### Consequence Chain

Low-risk today. Higher risk during refactors if `_return_piece_to_center` is called from new code paths.

---

## Extended Finding 18

**Technique:** Hostile Input
**Target:** `striker.gd:95-99` — right-click cancel calls private `_set_state` directly
**Confidence:** 75
**Impact:** nice-to-have
**Survived:** no

### Observation

Right-clicking during POWER state calls `GameManager._set_state(GameManager.State.AIM)` directly (line 99) — a method prefixed with `_` (GDScript private convention). External code reaching into internal state transitions. If `_set_state` ever gains pre/post conditions, this direct call bypasses them.

### Consequence Chain

Functional today. Architectural coupling that becomes a maintenance trap if state machine gains validation.

---

## Extended Finding 19

**Technique:** Existence Question
**Target:** All scripts — no pause/forfeit/draw mechanism
**Confidence:** 95
**Impact:** values-gap
**Survived:** no

### Observation

Searched all scripts for: pause, forfeit, draw, stalemate, timeout, resign. Found nothing. The only ways to end: (a) win condition achieved, (b) press R to reset, (c) click Menu. No timeout per turn, no shot clock, no stalemate detection. Combined with Finding 9 (queen limbo), the game can reach an unwinnable state with no resolution.

### Consequence Chain

1. Queen in limbo + no stalemate detection = deadlock
2. No timeout = players can walk away and the game hangs forever
3. No forfeit = stuck players must force-reset

---

## Extended Finding 20

**Technique:** Cross-Seam Probe
**Target:** `board.gd:148-149` — pocket collision layers
**Confidence:** 82
**Impact:** nice-to-have
**Survived:** yes

### Observation

Pocket Area3Ds: `collision_layer = 8`, `collision_mask = 6` (pieces + striker). For `body_entered` to fire, the Area3D's mask must include the body's layer. Pocket mask 6 includes piece layer 2 and striker layer 4. Physics layers are correctly configured.

### Consequence Chain

Pocket detection works correctly for both pieces and striker. **This survived.**

---

## Extended Finding 21

**Technique:** Assumption Flip
**Target:** `game_manager.gd:311-316` — win condition + queen ownership deadlock
**Confidence:** 83
**Impact:** values-gap
**Survived:** no

### Observation

`_check_win` requires `queen_covered AND queen_pocketed_by == N` for player N to win. Only the player who pocketed the queen can cover it (line 216 checks `queen_pocketed_by == current_player`). This is correct per carrom rules, but combined with no stalemate detection, if Player 1 clears all their pieces without the queen being credited to them, the game deadlocks.

### Consequence Chain

Correct rule implementation. But without stalemate detection, creates unwinnable states that hang forever.

---

## Summary

| # | Technique | Severity | Key Issue |
|---|-----------|----------|-----------|
| 3 | Existence Question | **CRASH** | `pieces` array never cleared on scene reload — freed node access |
| 15 | Delete Probe | **CRASH** | Same crash via Menu → Play path |
| 5 | Time Travel | **CORRUPT** | Pocket events fire during resolution, corrupting turn state |
| 1 | Assumption Flip | **DRIFT** | Returned pieces drift unmonitored after resolution |
| 7 | Req Inversion | **EXPLOIT** | Foul doesn't deduct score for returned piece |
| 14 | Req Inversion | **CONFUSE** | Negative scores possible, no floor |
| 9 | Replay Probe | **STALEMATE** | Queen limbo + no stalemate detection = deadlock |
| 19 | Existence Q. | **DESIGN** | No pause/forfeit/timeout mechanism |
| 4 | Scale Shift | **SOLID** | Physics math checks out |
| 6 | Cross-Seam | **SOLID** | Camera signal ordering correct |
| 16 | Scale Shift | **SOLID** | Explicit damping, not reliant on defaults |
| 20 | Cross-Seam | **SOLID** | Pocket collision layers correct |

**Top priority fix:** Finding 3/15 — add `GameManager.pieces.clear()` at the start of `board.gd._ready()` or `start_game()`. This is a guaranteed crash on every restart.
