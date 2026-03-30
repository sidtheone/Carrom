# Carrom Game Loop — Consolidated Review

**Sources:** 8 independent analyses
- World Spread 1: Death + Tower + Moon
- World Spread 2: Hermit + Fool + High Priestess
- Monkey Chaos Check (21 findings)
- Rabbit Game Loop Analysis (Tiger, Rat, Snake, Ox)
- Rabbit Stop Threshold Analysis (Rooster, Rat, Tiger, Ox)
- Rabbit Action Items
- Adversarial Game Loop Review (15 findings)
- Rabbit Self-Audit (meta-analysis of framework gaps)

**Cross-referencing method:** Issues found by multiple independent passes get higher confidence. Issues found by only one pass are flagged as single-source.

**Date:** 2026-03-30

---

## P0 — Crash / Corruption

### 1. `pieces[]` array never cleared on scene reload
**Sources:** Monkey (Findings 3, 13, 15), Adversarial #15 (partial)
**Confidence:** 95%
**Files:** `board.gd:271`, `game_manager.gd:40`

GameManager is an autoload (persists across scene changes). `board.gd._ready()` appends 19 pieces to `GameManager.pieces` but never clears it. On restart (R key, Restart button, or Menu->Play), the array doubles with 19 freed-node references. Next `_check_simulation_complete` iterates freed nodes -> **instant crash**.

**Why others missed it:** All other passes analyzed steady-state game loop, not lifecycle transitions. This is the most critical find across all reviews.

**Fix:** Add `GameManager.pieces.clear()` at the top of `board.gd._ready()` or `start_game()`.

---

### 2. Premature stop detection (single-frame velocity check)
**Sources:** Rabbit-Tiger, Rabbit-Stop-Threshold (entire analysis), World1-Death #2, Adversarial #1 — **4 sources, highest cross-source agreement**
**Confidence:** 95%
**File:** `game_manager.gd:196-205`

`_check_simulation_complete()` resolves the turn the instant ALL velocities dip below 0.5. With bounce=0.8 and damp=0.5, pieces oscillate near threshold for seconds. A glancing collision can momentarily drop velocity below 0.5 for a single frame, triggering premature resolution mid-simulation.

The stop threshold analysis proved the threshold is **10x more aggressive** relative to the board than the pre-revamp value (0.676% vs 0.068% of board/second), AND runs in `_process` instead of `_physics_process`.

**Fix:** Add consecutive-frame counter (10-15 frames below threshold before resolving). Optionally add minimum simulation time (0.5s) which also kills the zero-power exploit.

---

### 3. Double-pocket scoring (no idempotency guard)
**Sources:** Adversarial #3, Rabbit Self-Audit (confirmed miss) — **single-source + meta-confirmation**
**Confidence:** 85%
**Files:** `board.gd:155-157`, `game_manager.gd:327-368`

`_on_pocket_body_entered` fires on Area3D overlap. A piece sliding along a pocket edge can trigger `body_entered` multiple times before the teleport to `(0, -100, 0)` takes effect (physics updates are deferred). `on_piece_pocketed` has no guard — it adds points, appends to `pocketed_this_turn`, and sets flags unconditionally on every call.

**Consequence:** Points double-counted, `pocketed_this_turn` gets duplicate entries, pieces could be returned twice on foul.

**Fix:** Guard `on_piece_pocketed` with `if body in pocketed_this_turn: return` or `if not body.visible: return` at the top.

---

### 4. Pocket events fire during turn resolution, corrupting state
**Sources:** Monkey #5, World1-Death #6, Adversarial #7 (partial) — **3 sources**
**Confidence:** 88%
**File:** `game_manager.gd:208-268`, `board.gd:155-157`

`_resolve_turn()` calls `_return_piece_to_center()` which unfreezes pieces. Physics can push returned pieces into pocket Area3Ds. `on_piece_pocketed` fires mid-resolution, mutating `scores`, `pocketed_this_turn`, `own_piece_pocketed`, `queen_pocketed_by` — all actively being read by `_resolve_turn`.

**Fix:** Set a flag to ignore pocket events during resolution, or freeze returned pieces until the next SIMULATION.

---

## P1 — Score / Rule Bugs

### 5. Foul doesn't deduct score for returned piece
**Sources:** World1-Death #5, World1-Moon #1, Monkey #7, Rabbit action items — **4 sources, unanimous**
**Confidence:** 95%
**File:** `game_manager.gd:237-253`

`_handle_foul()` returns one pocketed piece to center but never subtracts its score. Player keeps points for a piece that's back on the board. Exploitable: pocket 3 pieces (+30) and striker, lose 1 piece back, keep +20.

---

### 6. Queen return doesn't subtract 50 points
**Sources:** World1-Moon #2 — **single-source, 90% confidence**
**File:** `game_manager.gd:288-294`

`_return_queen_to_center()` resets `queen_pocketed_by` but never touches `scores[]`. Queen is RED, excluded from `_return_opponent_pieces()` color check. 50 free points every time.

---

### 7. Striker foul + queen = queen orphaned permanently
**Sources:** Rabbit-Rat #2, Adversarial #2, Rabbit action items — **3 sources**
**Confidence:** 92%
**File:** `game_manager.gd:208-214`

If striker and queen are both pocketed in the same shot, `_resolve_turn` returns early at the striker foul check (line 214), never evaluating queen coverage. `queen_pocketed_by` stays set but `_handle_foul` doesn't address it. Queen stuck in limbo — invisible, uncoverable.

**Fix:** `_handle_foul` must also return the queen to center if `queen_pocketed_by == current_player`.

---

### 8. Win condition requires queen pocketed by winner (wrong carrom rules)
**Sources:** Adversarial #8, Rabbit Self-Audit (confirmed miss) — **single-source + meta-confirmation**
**Confidence:** 80%
**File:** `game_manager.gd:311-316`

`_check_win` requires `queen_pocketed_by == 1` for P1 to win. In standard carrom, any player can pocket the queen — what matters is covering. If P2 pockets the queen and P1 covers on the next shot, P1 should still be able to win by clearing all their pieces. Current code permanently blocks that path.

**Why others missed it:** No analysis validated game rules against actual carrom rules. The self-audit identified this as a framework gap: "No animal has domain knowledge or seeks it."

---

### 9. Queen limbo -> stalemate with no detection
**Sources:** Monkey #9, Monkey #21, Monkey #19 — **single-source cluster**
**Confidence:** 88%
**File:** `game_manager.gd:297-317`

If a player pockets only the queen (no own piece), turn switches. The queen CAN be covered on a future turn. But if that never happens, neither player can win (`_check_win` requires `queen_covered`). No stalemate detection, no timeout, no forfeit mechanism.

---

## P2 — Gameplay / Physics

### 10. Initial piece layout: `d2 = 3.2 < 3.6` (collision diameter)
**Sources:** World2-Hermit #2, project memory — **confirmed by memory**
**Confidence:** 90%
**File:** `board.gd:221`

Six outer-ring pieces use `d2 = 2.0 * r - 0.4 = 3.2 cm`, less than `2 * MEN_RADIUS = 3.6 cm`. Tightest pairwise gaps are 0.09 cm. Project memory confirms: "Pieces overlapping, needs spacing fix."

**Fix:** Change `d2` to `2.0 * r + 0.1` (matching `d`).

---

### 11. Returned pieces overlap existing pieces and drift unmonitored
**Sources:** World1-Death #3, World1-Death #6, Rabbit-Tiger #2, Monkey #1 — **4 sources**
**Confidence:** 90%
**File:** `game_manager.gd:259-268`

`_return_piece_to_center()` places at `2.0 + count * 1.5` cm from center with no spatial check. Inner offset (2.0 cm) overlaps queen at origin. Pieces are unfrozen and can drift — but state has already moved to PLACE_STRIKER. Nobody monitors them.

---

### 12. Zero-power shot wastes turn silently
**Sources:** World1-Death #1, World2-Fool #3, Rabbit-Rat #1, Adversarial #4, Monkey #11 — **5 sources, unanimous**
**Confidence:** 95%
**File:** `game_manager.gd:159-191`

Instant click-release fires with power=0, speed=0. Simulation resolves immediately. Turn wasted with no feedback. Most common accidental first interaction.

**Fix:** Minimum power threshold (0.3-0.5) or minimum simulation time.

---

### 13. Exponential damping model -> 11s settling time
**Sources:** World2-Hermit #1, Rabbit-Stop-Threshold — **2 sources**
**Confidence:** 80%
**File:** `board.gd:20` (`PIECE_LINEAR_DAMP = 0.5`)

`linear_damp = 0.5` with `gravity_scale = 0` produces exponential decay instead of Coulomb friction. Pieces decelerate fast at high speed, coast forever at low speed. Real carrom: 2-4s settling. This game: ~11s.

---

## P3 — UX / Architecture

### 14. Score UX jumps during simulation
**Sources:** Adversarial #9, Rabbit Self-Audit (confirmed miss) — **2 sources**
**Confidence:** 75%
**File:** `game_manager.gd:358, 278`

Points added immediately on pocket (line 358), subtracted back on resolution for opponent pieces (line 278). During 1-3s simulation, player sees score spike then drop. Net zero on final state but visually jarring.

---

### 15. Aspect-ratio dependent input mapping
**Sources:** World2-Fool #1, Monkey #2 — **2 sources**
**Confidence:** 80%
**File:** `striker.gd:61`

Maps full screen width to 74 cm, but valid placement is +/-12.3 cm. Only ~33% of screen produces movement. Worse on ultrawide.

---

### 16. No AIM->PLACEMENT back-navigation
**Sources:** World2-Fool #2 — **single-source**
**Confidence:** 75%
**File:** `striker.gd:71-82`

Right-click cancels POWER->AIM but nothing cancels AIM->PLACEMENT. Misplaced striker commits the player.

---

### 17. Private `_set_state` called externally
**Sources:** World1-Tower #1, World2-HP #1, Rabbit-Ox #2, Adversarial #10 (partial), Monkey #18 — **5 sources**
**Confidence:** 90%
**File:** `striker.gd:99`

Breaks state machine encapsulation. If `_set_state` gains validation, this call bypasses it.

**Fix:** Expose `cancel_power()` on GameManager.

---

### 18. Collision sound silent on gentle/glancing hits
**Sources:** World2-HP #2 — **single-source**
**Confidence:** 70%
**File:** `board.gd:365-377`

Instance_id dedup means the piece with lower ID checks *its own* velocity (not relative). For glancing blows, the reporting piece may be below the 5.0 cm/s audio threshold.

---

### 19. Negative scores possible, no floor
**Sources:** Monkey #14, Rabbit-Tiger #3 — **2 sources**
**Confidence:** 75%
**File:** `game_manager.gd:278`

`scores[current_player - 1] -= points` with no `max(0, ...)`. HUD shows negative scores.

---

### 20. Signal ordering fragility in `_switch_turn`
**Sources:** World2-HP #3, Adversarial #10 — **2 sources (Rabbit Self-Audit rates this as "overstated")**
**Confidence:** 60%
**File:** `game_manager.gd:282-285`

`current_player` must be set before `_set_state()` for camera to pick the right preset. Self-audit says this is standard pattern, not accidental. Fragile but not buggy.

---

### 21. Visibility as canonical game state
**Sources:** Rabbit-Ox #1 — **single-source**
**Confidence:** 75%
**File:** 6+ callsites across game_manager.gd, hud.gd

`piece.visible` doubles as "in play" flag. Conflates rendering with game logic. Add `is_pocketed` metadata before adding visual effects.

---

### 22. No pause/forfeit/timeout/stalemate detection
**Sources:** Monkey #19, Adversarial #14 (partial) — **2 sources**
**Confidence:** 95%
**Files:** All scripts

No way to end a game except winning, R-key reset, or Menu. Combined with queen limbo (#9), game can deadlock.

---

### 23. Debug prints everywhere (15+)
**Sources:** Rabbit-Snake #1 — **single-source**
**File:** game_manager.gd, board.gd

Not behind a debug flag. Will spam console in release.

---

### 24. Audio pool exhaustion on break shots
**Sources:** Monkey #8, Rabbit-Rat #4 — **2 sources**
**Confidence:** 75%
**File:** `audio_manager.gd:56-66`

8 pooled players. Break shot generates 10+ collision pairs. Sounds interrupt each other during the most satisfying moment.

---

### 25. Striker has BLACK piece color metadata
**Sources:** Monkey #10, Adversarial #11 — **2 sources**
**Confidence:** 85%
**File:** `board.gd:289`

Striker created with `PieceColor.BLACK`. Safe by accident (not in `pieces[]`). Latent if arrays are ever merged.

---

## Issues Verified as Correct (Survived)

These were stress-tested across multiple analyses and held:

| Issue | Sources | Verdict |
|-------|---------|---------|
| Physics math (max speed, travel distance) | Monkey #4 | Plausible, no tunneling |
| Camera signal ordering | Monkey #6, World2-HP #3, Adversarial #10, Self-Audit | Correct by design, not accident |
| Pocket collision layers | Monkey #20, World1-Death #4 | Properly configured |
| Explicit damping (not reliant on defaults) | Monkey #16 | Defensive coding |
| Queen coverage across turns | World1-Death #4 | Order-independent, correct |
| Striker not in pieces[] | Adversarial #5, Self-Audit | Intentional separation, not accident |

---

## Source Coverage Matrix

| # | Issue | W1 | W2 | Monkey | Rabbit | Threshold | Adversarial | Self-Audit |
|---|-------|----|----|--------|--------|-----------|-------------|------------|
| 1 | Pieces array crash | | | **X** | | | x | |
| 2 | Stop detection | X | | | **X** | **X** | X | |
| 3 | Double-pocket | | | | | | **X** | X |
| 4 | Pocket during resolution | X | | **X** | | | x | |
| 5 | Foul score inflation | **X** | | X | X | | | |
| 6 | Queen score kept | **X** | | | | | | |
| 7 | Striker+queen orphan | | | | **X** | | **X** | |
| 8 | Wrong win rules | | | | | | **X** | **X** |
| 9 | Queen stalemate | | | **X** | | | | |
| 10 | d2 layout overlap | | **X** | | | | | |
| 11 | Return piece overlap | **X** | | X | X | | x | |
| 12 | Zero-power shot | X | X | X | **X** | | X | |
| 13 | Exponential damping | | **X** | | | X | | |
| 14 | Score UX jumps | | | | | | **X** | X |
| 15 | Input mapping | | **X** | X | | | | |
| 16 | No AIM back-nav | | **X** | | | | | |
| 17 | Private _set_state | X | X | X | **X** | | X | |
| 18 | Sound dedup bug | | **X** | | | | | |
| 19 | Negative scores | | | X | X | | | |
| 20 | Signal ordering | | X | X | | | X | X |
| 21 | Visibility as state | | | | **X** | | | |
| 22 | No pause/forfeit | | | **X** | | | x | |
| 23 | Debug prints | | | | **X** | | | |
| 24 | Audio pool | | | X | X | | | |
| 25 | Striker BLACK meta | | | X | | | X | |

**X** = first to find, x = also found or partially found

---

## Unique Contributions Per Source

| Source | Unique finds (not caught by others) |
|--------|-------------------------------------|
| World Spread 1 | Queen return keeps 50 points (#6) |
| World Spread 2 | d2 layout overlap (#10), no AIM back-nav (#16), sound dedup (#18) |
| Monkey | Pieces array crash (#1), queen stalemate (#9), no pause/forfeit (#22) |
| Rabbit | Visibility as state (#21), debug prints (#23) |
| Adversarial | Double-pocket (#3), wrong win rules (#8), score UX jumps (#14) |
| Self-Audit | Framework gap analysis (no new bugs, but explained WHY bugs were missed) |

**Key insight:** No single analysis found more than 60% of total issues. The lifecycle crash (#1), the idempotency bug (#3), and the wrong domain rules (#8) each required a different analytical lens.

---

## Prioritized Fix Order

| Priority | Issue | Fix Complexity | Sources |
|----------|-------|---------------|---------|
| 1 | `pieces.clear()` on reload (#1) | 1 line | 1 source, 95% confidence |
| 2 | Consecutive-frame stop counter (#2) | ~10 lines | 4 sources |
| 3 | Double-pocket idempotency guard (#3) | 2 lines | 2 sources |
| 4 | Foul score subtraction (#5) + queen score (#6) | ~5 lines each | 4 sources + 1 source |
| 5 | Striker-foul queen handling (#7) | ~5 lines | 3 sources |
| 6 | Pocket events during resolution guard (#4) | ~5 lines | 3 sources |
| 7 | Fix d2 spacing constant (#10) | 1 line | confirmed by memory |
| 8 | Minimum power threshold (#12) | ~3 lines | 5 sources |
| 9 | Win condition rules (#8) | ~5 lines | 2 sources |
| 10 | Overlap check on piece returns (#11) | ~10 lines | 4 sources |
| 11 | Expose cancel_power() (#17) | ~5 lines | 5 sources |
| 12 | Add simulation timeout | ~5 lines | 2 sources |

---

## Meta-Observations (from Self-Audit)

The self-audit identified three structural analysis gaps that explain WHY certain bugs were missed:

1. **No cross-file signal tracing.** The double-pocket bug (#3) lives in the seam between board.gd and game_manager.gd. Every analysis that stayed within file boundaries missed it.

2. **No domain rule validation.** The wrong win condition (#8) is syntactically perfect code implementing incorrect carrom rules. Every analysis checked code quality, not rule correctness.

3. **Data-state vs experiential correctness.** The score UX jump (#14) is "net zero" on final state — but the player sees a confusing spike during simulation. Analyses that stopped at data correctness missed the UX consequence.
