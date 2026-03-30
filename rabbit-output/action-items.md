# Carrom Game Loop — Action Items

## Critical (game-breaking)

- **Fix simulation stop-detection.** Add a consecutive-frame counter (10-15 frames / ~0.2s) to `_check_simulation_complete` in `game_manager.gd:196`. Single-frame velocity check false-positives during mid-collision deceleration, silently corrupting board state. Threshold value (0.5) stays — the counter is the fix.

- **Fix striker-foul + queen orphaning.** In `_resolve_turn` (game_manager.gd:208), striker foul returns early at line 214 without evaluating queen state. If both striker and queen are pocketed same shot, queen is permanently stuck invisible with no path to recovery. `_handle_foul` must reset `queen_pocketed_by` and return queen to center.

## High (gameplay bugs)

- **Block zero-power shots.** Releasing power at 0.0 fires a speed=0 shot, instant simulation resolve, free turn pass with no penalty. Either enforce `MIN_POWER` (~0.5) or treat zero-power release as cancel-back-to-AIM. Consider a minimum simulation time (0.5-1.0s) which fixes this AND helps stop-detection.

- **Decide on stop threshold value.** The revamp scaled threshold 100x (0.005 → 0.5) but board-relative it's 10x looser than the original. If old value was tuned, board-equivalent is 0.05 cm/s, not 0.5. Trade-off: 0.05 adds ~1.8s to simulation tails but is more faithful. With the frame counter fix, 0.5 is acceptable.

## Medium (code quality)

- **Expose `cancel_power()` on GameManager.** Replace direct `GameManager._set_state()` call in `striker.gd:99` with a public method. Preserves state machine encapsulation.

- **Add overlap check to `_return_piece_to_center`.** Spacing formula (`2.0 + count * 1.5` cm) with random jitter can place returned pieces on top of existing center pieces. Physics explosion possible.

- **Gate debug prints behind a flag.** 15+ `print()` calls across game_manager.gd and board.gd with no debug guard. Use `OS.is_debug_build()` or a `const DEBUG` flag. Remove redundant `_physics_process` logging (subset of `_log_simulation_frame`).

## Low (architectural debt, no rush)

- **Decouple visibility from game state.** `piece.visible` is the canonical "in play" check across 6+ callsites. Add `is_pocketed` metadata to pieces before adding any visual effects on pocketing.

- **Consider input abstraction.** All input is raw `InputEventMouse*` in striker.gd. Fine for mouse-only, expensive to retrofit for touch/gamepad later.
