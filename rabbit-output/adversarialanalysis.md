# Adversarial Game Loop Review

Fresh review of the Carrom game loop, independent of prior Rabbit analysis.

> **Status update (2026-03-31):** 3 of 3 CRITICAL issues resolved in commit `8a0e133`. Several HIGH/MEDIUM issues also resolved by aim redesign (`483f75f`). See status tags below.

---

## CRITICAL — Will corrupt games

### 1. Single-frame stop detection ✅ FIXED
**File:** `game_manager.gd` | **Fixed in:** commit `8a0e133`

~~`_check_simulation_complete()` runs every `_process` frame and resolves the instant all velocities dip below 0.5.~~ Now uses `STOP_CONFIRM_FRAMES := 12` consecutive frames below threshold + `MIN_SIMULATION_TIME := 0.5` seconds. Moved to `_physics_process` for frame-accurate detection.

### 2. Queen orphaning on striker foul ✅ FIXED
**File:** `game_manager.gd` | **Fixed in:** commit `8a0e133`

~~`queen_pocketed_by` stays set to the fouling player but `_handle_foul` doesn't touch it.~~ Now resets `queen_pocketed_by = -1` when queen is not covered, preventing orphan state.

### 3. Pocket re-entry / double-pocket ✅ FIXED
**Files:** `board.gd`, `game_manager.gd` | **Fixed in:** commit `8a0e133`

~~No guard — `on_piece_pocketed` will run again, double-counting points.~~ Idempotency guard added: `if not body.visible: return` prevents re-processing already-pocketed pieces.

---

## HIGH — Exploitable or wrong behavior

### 4. Zero-power shot = free pass ✅ FIXED
**File:** `game_manager.gd` | **Fixed in:** commit `8a0e133`

~~Click-release instantly in POWER state → instant resolve → free pass.~~ `MIN_SIMULATION_TIME := 0.5` prevents instant resolve even with `power = 0.0`.

### 5. Striker not in `pieces` array — BY DESIGN
**File:** `game_manager.gd`

~~Concern about striker exclusion from `pieces` array.~~ **Assessed as intentional** — striker has different collision layer (4 vs 2), different mass (15g vs 5g), different lifecycle (never permanently pocketed). The separate handling is by design, not accident.

### 6. `_return_count` state leak across fouls
**File:** `game_manager.gd` — ⚠️ **STILL OPEN** (low priority)

`_return_count` is reset at the top of `_resolve_turn`. If a future code path calls `_return_piece_to_center` outside `_resolve_turn`, the counter won't be reset. Latent risk.

### 7. Race between `_process` and pocket signals
**File:** `game_manager.gd` — ⚠️ **STILL OPEN** (low priority)

Stop detection now runs in `_physics_process` (not `_process`), which reduces but doesn't fully eliminate the race window. The consecutive-frame counter also makes this much harder to trigger in practice.

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

## Priority (Updated 2026-03-31)

~~**Top 3 to fix first:** #1 (stop detection), #2 (queen orphan), #3 (double pocket).~~ ✅ All 3 critical issues resolved.

**Remaining open items (by priority):**
1. #8 — Win condition requires queen pocketed by winner (wrong carrom rules)
2. #9 — Score UX jumps during simulation (cosmetic)
3. #6 — `_return_count` state leak (latent)
4. #7 — Physics/process race (mitigated by frame counter)
5. #10-15 — Low-priority / by-design items
