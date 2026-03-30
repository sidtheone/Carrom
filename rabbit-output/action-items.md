# Carrom Game Loop — Action Items

Consolidated from 8 independent analyses. 25 unique issues.
Updated: 2026-03-30

---

## Fix Now (crash/corruption — game is unplayable without these)

- [x] **Clear `pieces[]` on reload.** Added `GameManager.pieces.clear()` + null striker/queen at top of `board.gd._ready()`. *(Fixed 2026-03-30)*

- [x] **Add consecutive-frame stop counter.** 12-frame confirm counter + 0.5s min simulation time + moved to `_physics_process`. *(Fixed 2026-03-30)*

- [x] **Guard `on_piece_pocketed` for idempotency.** Added `if not body.visible: return` guard at top. *(Fixed 2026-03-30)*

---

## Fix Before Playtesting (scoring is wrong)

- [ ] **Subtract score on foul piece return.** `_handle_foul()` at `game_manager.gd:237` returns a piece but keeps its score. Add score subtraction matching what `_return_opponent_pieces` does. *(World1, Monkey, Rabbit — 4 sources)*

- [ ] **Subtract queen score on return.** `_return_queen_to_center()` at `game_manager.gd:288` resets `queen_pocketed_by` but not `scores[]`. Subtract `SCORE_QUEEN` (50). Queen is RED, excluded from opponent-return color check. *(World1-Moon, 90% confidence)*

- [ ] **Handle queen on striker foul.** `_resolve_turn()` returns early at line 214 on striker foul, never reaching queen logic. If `queen_pocketed_by == current_player`, return queen to center inside `_handle_foul`. *(Rabbit-Rat, Adversarial — 3 sources)*

- [ ] **Block pocket events during resolution.** Add `var _resolving: bool = false` flag. Set true at top of `_resolve_turn`, false at end. Guard `on_piece_pocketed` with `if _resolving: return`. Returned pieces drifting into pockets corrupt mid-resolution state. *(Monkey, World1, Adversarial — 3 sources)*

---

## Fix After Playtesting (decide if these matter in practice)

- [ ] **Fix d2 spacing constant.** `board.gd:221` — change `d2 = 2.0 * r - 0.4` to `2.0 * r + 0.1`. Six outer-ring pieces have sub-collision-diameter spacing. Known from project memory. *(World2-Hermit, memory)*

- [ ] **Add minimum power threshold.** Reject `release_power()` when `power < 0.3` or add minimum simulation time (0.5s). Instant click-release wastes turn silently. *(5 sources unanimous)*

- [ ] **Add overlap check to `_return_piece_to_center`.** Inner offset (2.0 cm) can overlap queen at origin. Use spatial query or scan for free space before placement. *(4 sources)*

- [ ] **Fix win condition rules.** `game_manager.gd:311` requires `queen_pocketed_by == winner`. Standard carrom: any player can pocket queen, what matters is covering. Decide: standard rules or house rules? *(Adversarial, Self-Audit)*

- [ ] **Expose `cancel_power()` on GameManager.** Replace direct `GameManager._set_state()` call at `striker.gd:99` with a public method. *(5 sources)*

- [ ] **Add simulation timeout.** After ~15s, force-zero all velocities and resolve. Prevents infinite hang from physics glitches. *(2 sources)*

---

## Decide Later (UX/polish, won't know priority until playing)

- [ ] **Assess damping model.** `linear_damp=0.5` produces exponential decay, 11s settling. Real carrom: 2-4s. Options: increase damp to ~2.0 (quick), or implement Coulomb friction via `_integrate_forces` (correct). *(World2-Hermit, Stop Threshold)*

- [ ] **Add AIM→PLACEMENT back-navigation.** Right-click cancels POWER→AIM but nothing cancels AIM→PLACEMENT. *(World2-Fool)*

- [ ] **Fix input mapping.** `striker.gd:61` maps full screen width to 74cm but valid range is ±12.3cm. Only 33% of screen active. Use raycasting instead. *(World2-Fool, Monkey)*

- [ ] **Add negative score floor.** `game_manager.gd:278` — `scores[]` can go negative. Add `max(0, ...)` or decide if negative scores are intentional. *(Monkey, Rabbit)*

- [ ] **Fix collision sound for gentle hits.** `board.gd:376` — instance_id dedup means the piece with lower ID checks its own velocity, not relative. Glancing blows can be silent. *(World2-HP)*

- [ ] **Add stalemate/forfeit/timeout.** Queen limbo + no stalemate detection = possible deadlock. No pause either. *(Monkey)*

- [ ] **Fix score UX jumps.** Points added on pocket, subtracted on resolution. Player sees temporary spike. Either defer scoring to resolution or don't display during simulation. *(Adversarial, Self-Audit)*

- [ ] **Decouple visibility from game state.** `piece.visible` is "in play" flag across 6+ callsites. Add `is_pocketed` metadata before adding visual effects. *(Rabbit-Ox)*

- [ ] **Gate debug prints.** 15+ `print()` calls with no debug flag. Wrap in `OS.is_debug_build()` or strip. *(Rabbit-Snake)*

- [ ] **Increase audio pool or add priority.** 8 players, break shots generate 10+ collisions. Most dramatic moment sounds worst. *(Monkey, Rabbit)*

- [ ] **Remove dead camera callback.** `camera_controller.gd:58` — `_on_turn_changed` is `pass`. Remove handler and signal connection. *(World1-Tower)*

- [ ] **Fix striker BLACK metadata.** `board.gd:289` — striker created with `PieceColor.BLACK`. Safe today (not in `pieces[]`), latent if arrays merge. *(Monkey, Adversarial)*

- [ ] **Fix inconsistent Y positions.** Pieces spawn at Y=0.1, return at Y=0.2. Negligible impact but inconsistent. *(World2-Hermit)*
