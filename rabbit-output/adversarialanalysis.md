# Adversarial Game Loop Review

Fresh review of the Carrom game loop, independent of prior Rabbit analysis.

---

## CRITICAL — Will corrupt games

### 1. Single-frame stop detection
**File:** `game_manager.gd:196-205`

`_check_simulation_complete()` runs every `_process` frame and resolves the instant all velocities dip below 0.5. During wall/piece collisions, velocity passes through zero at the inflection point. One unlucky frame = premature turn resolution on incomplete physics. Board state gets permanently corrupted — pieces frozen mid-flight, wrong pocketing decisions.

### 2. Queen orphaning on striker foul
**File:** `game_manager.gd:211-214`

If striker + queen are both pocketed on the same shot, `_resolve_turn` hits the `striker_pocketed` early return at line 214 and never reaches the queen coverage logic (line 216). `queen_pocketed_by` stays set to the fouling player but `_handle_foul` doesn't touch it. The opponent can never "cover" it because coverage only checks `current_player == queen_pocketed_by`. Queen is gone forever.

### 3. Pocket re-entry / double-pocket
**Files:** `board.gd:155-157`, `game_manager.gd:327-368`

`_on_pocket_body_entered` fires on Area3D overlap. A piece sliding along a pocket edge could trigger `body_entered` multiple times before it's teleported to `(0, -100, 0)`. There's no guard — `on_piece_pocketed` will run again, double-counting points, appending the piece to `pocketed_this_turn` twice, and potentially returning it twice on foul. The striker path has `striker_pocketed = true` which is idempotent, but the piece path adds points and appends unconditionally.

---

## HIGH — Exploitable or wrong behavior

### 4. Zero-power shot = free pass
**File:** `game_manager.gd:159-191`

Click-release instantly in POWER state → `power = 0.0` → `speed = 0.0` → SIMULATION starts → next `_process` everything is already stopped → `_resolve_turn` fires. No pieces moved, no foul. `_switch_turn()` gives the opponent the turn. Player can intentionally "pass" with no penalty. Carrom has no passing.

### 5. Striker not in `pieces` array
**File:** `game_manager.gd:196-205`

`_check_simulation_complete` iterates `pieces` then checks `striker` separately. But `pieces` is populated in `_spawn_pieces` and the striker is spawned via `_spawn_striker` — never added to `pieces`. This happens to work because of the separate striker check on line 202, but every other loop over `pieces` (logging, win check, HUD counts) silently excludes the striker. If any future code assumes `pieces` is "all rigid bodies on the board," it'll miss the striker.

### 6. `_return_count` state leak across fouls
**File:** `game_manager.gd:209, 257-268`

`_return_count` is reset at the top of `_resolve_turn` (line 209). But `_handle_foul` also calls `_return_piece_to_center`, which increments it. If a future code path calls `_return_piece_to_center` outside `_resolve_turn`, the counter won't be reset, and pieces will spawn at increasingly large offsets.

### 7. Race between `_process` and pocket signals
**File:** `game_manager.gd:67-79, 327`

`on_piece_pocketed` modifies `pocketed_this_turn`, `striker_pocketed`, `own_piece_pocketed`, and scores. These are read by `_resolve_turn`, called from `_process`. If a piece enters a pocket on the same frame that stop detection triggers, the order matters: does `body_entered` fire before or after `_process`? In Godot, `body_entered` signals from physics fire during physics step, before `_process`. This should be safe — but if physics runs multiple sub-steps, a piece could be pocketed on a sub-step after the `_process` that called `_resolve_turn`. Edge case but not guarded.

---

## MEDIUM — Incorrect rules / UX bugs

### 8. Win check requires queen pocketed by winner
**File:** `game_manager.gd:311-316`

Lines 311-314: `black_remaining == 0 and queen_covered and queen_pocketed_by == 1`. This means P1 can only win if P1 pocketed the queen. But in real carrom, any player can pocket the queen — what matters is covering. If P2 pockets the queen and P1 covers it on the next shot, P1 should still be able to win eventually. The current logic permanently blocks that path.

### 9. Opponent piece return subtracts from wrong player
**File:** `game_manager.gd:271-279`

`scores[current_player - 1] -= points` — subtracts from the current player's score when returning opponent pieces. But the current player *earned* those points by pocketing them (line 358). So the net is zero. However, the score visually jumps up on pocket, then back down on turn resolution. This creates a confusing UI — score shows +20 during simulation, then drops back. The earn-then-subtract should happen atomically, or opponent pieces shouldn't award points at all.

### 10. Camera doesn't update on turn change
**File:** `camera_controller.gd:58-59`

`_on_turn_changed` is a no-op (`pass`). The camera transitions on `state_changed`, which fires when `_set_state(PLACE_STRIKER)` is called from `_switch_turn`. But `_switch_turn` changes `current_player` *before* calling `_set_state`, so `_transition_to` sees the new player. This works — but only because of execution order. If `turn_changed` and `state_changed` are ever reordered, camera breaks.

### 11. Striker gets wrong piece color metadata
**File:** `board.gd:284-289`

Striker is created with `PieceColor.BLACK`. It's never checked by game logic (the striker path returns early in `on_piece_pocketed`), but if any code ever iterates `pieces` looking for BLACK pieces and the striker is added to `pieces`, the striker would be counted as a black game piece.

---

## LOW — Latent issues

### 12. `_return_piece_to_center` can overlap existing pieces
No collision check, just a formula with random jitter.

### 13. `_handle_foul` only returns ONE piece
The `break` at line 252 stops after the first. Standard carrom rules vary, but some variants return one piece + queen.

### 14. No input blocking during SIMULATION
`striker._unhandled_input` skips non-matching states, but mouse clicks during simulation are silently consumed. If the state machine ever has a bug that leaves it in SIMULATION with a stopped board, the game soft-locks with no feedback.

### 15. `get_tree().reload_current_scene()` for reset
Blows away everything including the autoload singleton state. Works because `_ready` reinits, but `GameManager` members like `pieces` array, `striker` ref, and `queen` ref point at now-freed nodes until `board._ready` runs again. Brief window where GameManager holds dangling refs.

---

## Priority

**Top 3 to fix first:** #1 (stop detection), #2 (queen orphan), #3 (double pocket). Everything else is annoying but survivable.
