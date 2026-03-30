# Rabbit Self-Audit — Framework Gap Analysis

**Audience:** Skill author evaluating structural blind spots in the Rabbit framework
**Scope:** Section-sized — meta-analysis of one prior Rabbit run vs. independent adversarial review
**Calibration:** Rooster 4/5, Ox 3/5

---

## Synthesis

The adversarial review found 3 bugs that Rabbit's prior game loop analysis missed. Two of those misses point to structural gaps in how the animals are defined. The third is an execution gap in an existing animal.

### Confirmed Misses

**1. Double-pocket scoring (Critical)**
`board.gd:155` fires `body_entered` → `game_manager.gd:327` handles with no idempotency guard. A piece sliding along a pocket edge can trigger the signal multiple times before teleport to (0, -100, 0). Points double-counted, `pocketed_this_turn` gets duplicate entries, pieces could be returned twice on foul.

**Why Rabbit missed it:** No animal traced the `body_entered.connect(_on_pocket_body_entered)` chain from board.gd into game_manager.gd. Tiger analyzed `on_piece_pocketed` in isolation. Rat analyzed `_resolve_turn` in isolation. The bug lives in the seam.

**2. Win condition encodes wrong carrom rules (Medium)**
`_check_win` at game_manager.gd:311 requires `queen_pocketed_by == 1` for P1 to win. In standard carrom, any player can pocket the queen — what matters is covering. If P2 pockets queen and P1 covers on next shot, P1 should still be able to win by clearing all their pieces. Current code permanently blocks that path.

**Why Rabbit missed it:** No animal validates domain rules. Ox asked "is the architecture appropriate?" and "is the state machine well-structured?" but never asked "do these rules match actual carrom?" The code is syntactically perfect and architecturally sound — just wrong.

**3. Score UX jumps during simulation (Low-Medium)**
Points are added immediately on pocket (line 358), then subtracted back on turn resolution for opponent pieces (line 278). During the 1-3s simulation window, the player sees their score spike then drop. Rabbit noted the earn-then-subtract and concluded "net zero" — technically correct on final state, but didn't trace the player's visual experience over time.

**Why Rabbit missed it:** Rat traced consequences of the subtraction but stopped at data correctness ("net zero"). Didn't extend to "what does the player see between pocket and turn resolution?"

### Overclaimed Misses

**Camera execution order (claimed as "works by accident")**
`_switch_turn` sets `current_player` before calling `_set_state(PLACE_STRIKER)`. Camera reads `current_player` inside `state_changed` handler. The adversarial review called this "works by execution order accident." This is overstated — setting state before announcing it is the standard pattern. The `turn_changed` handler being a no-op is unused code, not fragile coupling.

**Striker not in `pieces` array (claimed as "works by accident")**
The striker is intentionally separate — different collision layer (4 vs 2), different mass (15g vs 5g), different lifecycle (never permanently pocketed, returns on foul). Every loop over `pieces` correctly excludes it because the striker isn't a game piece. The separate check in `_check_simulation_complete` is by design, not by accident.

### Root Cause Analysis

The confirmed misses cluster into two framework-level gaps:

**Gap 1: No cross-file signal tracing.**
Current animals analyze code within files. Tiger stress-tests functions. Rat traces consequences of decisions. Neither systematically follows `.connect()` chains across files and asks:
- Can this signal fire more than once per logical event?
- What are the ordering guarantees between this signal and other callbacks?
- Is the handler idempotent?
- What's the contract between emitter and receiver?

The double-pocket bug lives entirely in this gap. `board.gd` creates the Area3D and connects `body_entered`. `game_manager.gd` handles it without guarding against re-entry. No animal looked at the boundary.

**Gap 2: No domain rule validation.**
Current animals check internal consistency, architectural soundness, and edge-case behavior. None ask "does this code implement the rules of the thing it claims to be?" This requires either domain knowledge or the instinct to look up the actual rules. The win condition bug is the result — valid code implementing wrong carrom.

**Not a framework gap (execution gaps):**
- Idempotency checking fits under Tiger — "what if this fires twice?" is a stress test.
- Temporal UX reasoning fits under Rat — tracing what the user *sees over time* is a consequence map.
- These weren't applied in the prior run but don't need new animals to cover them.

---

## Action Items

1. **Add cross-boundary signal tracing to Tiger or Rat.** Every `.connect()` is a contract boundary. The animal that encounters it should trace both sides: emitter behavior (multiplicity, timing) and handler assumptions (idempotency, ordering). This is the highest-value addition — event-driven architectures put their worst bugs in signal seams.

2. **Add domain rule validation as an Ox technique.** "Does this code implement the rules it claims to?" One question, applicable to any domain-specific logic (game rules, business rules, protocol specs). Ox already asks "is the architecture appropriate?" — extend to "is the logic correct for the domain?"

3. **Sharpen Rat's temporal lens.** Rat should trace what the user *experiences over time*, not just final data state. The earn-then-subtract pattern is exactly Rat's territory — it just stopped at "net zero" instead of asking "but what does the player see during the 1-3s simulation window?"

4. **Consider adding idempotency as a Tiger checklist item.** For every callback/handler: "What happens if this fires twice? Three times? On the same frame?" Standard adversarial technique for event-driven code that Tiger should apply systematically.

---

## Coverage Gaps

- **Monkey / Hostile Input** — Could verify the double-pocket by simulating rapid pocket-edge sliding
- **Dog / Drift Detection** — Could check whether original design docs specified correct carrom rules vs. house rules
- **Dragon / Temporal Analysis** — Could assess whether fixing the win condition now vs. later affects save-game compatibility or scoring redesign options

---

## Raw Outputs

### Rooster — Verification (5 claims checked)

**Verification target: Adversarial review's claims about Rabbit's missed bugs**

#### Claim 1: Double-pocket scoring — "Rabbit missed this"
**Verdict: Verified.**

Evidence: Rabbit's Tiger analysis examined `_check_simulation_complete` (game_manager.gd:196-205), `_return_piece_to_center` (259-268), and `_resolve_turn` (208-235). Rabbit's Rat examined `release_power`/`_shoot_striker` (159-191) and `_resolve_turn` again.

Neither Tiger nor Rat examined `_on_pocket_body_entered` in board.gd or traced the `body_entered.connect()` chain. The word "pocket" appears in Rat Finding 2 (striker foul + queen) but only in the context of what happens after pocketing, not whether the pocketing signal itself can fire multiple times.

The double-pocket bug: `board.gd:150` connects `area.body_entered` to `_on_pocket_body_entered`. `_on_pocket_body_entered` (line 155-157) calls `GameManager.on_piece_pocketed(body)` with no guard. `on_piece_pocketed` (game_manager.gd:327-368) adds points, appends to `pocketed_this_turn`, and sets flags — all without checking whether the piece was already processed. A piece sliding along a pocket rim could trigger `body_entered` multiple times before `body.global_position = Vector3(0, -100, 0)` takes effect (physics updates are deferred).

Rabbit's analysis never mentions this. **Confirmed miss.**

#### Claim 2: Win condition wrong carrom rules — "Rabbit missed this"
**Verdict: Verified.**

Evidence: Rabbit's output mentions `_check_win` only in the Tiger Finding 1 context ("Turn resolution logic makes irreversible decisions — returning pieces, switching players, checking wins"). The actual win condition logic (game_manager.gd:311-316) was never analyzed by any animal.

The code: `if black_remaining == 0 and queen_covered and queen_pocketed_by == 1: _end_game(1)`. This requires P1 to have been the one who pocketed the queen. In standard carrom rules, the queen can be pocketed by either player — what matters is that the pocketing player covers it on the same turn. The `queen_pocketed_by` tracking is correct for coverage purposes but incorrectly reused as a win condition gate.

No Rabbit animal checked game rules against carrom rules. **Confirmed miss.**

#### Claim 3: Score UX jumps — "Rabbit missed this"
**Verdict: Verified, severity debatable.**

Evidence: Rabbit's Rat Finding 1 discusses score subtraction in `_return_opponent_pieces`: "Line 278: `scores[current_player - 1] -= points` when returning opponent pieces pocketed this turn. No floor check." Rabbit's Tiger Finding 3 also touches scoring: "If a player repeatedly pockets opponent pieces (earning points on pocket, then losing them on return), the subtraction could drive score negative."

Both analyses focus on the data correctness of the final score, not the player's visual experience during simulation. The earn (line 358) happens during physics simulation when the pocket signal fires. The subtract (line 278) happens during `_resolve_turn` after simulation ends. During the 1-3 second gap, the HUD shows inflated scores.

Rabbit saw the mechanism but evaluated only the endpoint. **Confirmed miss**, though severity is cosmetic.

#### Claim 4: Camera works "by accident" via execution order
**Verdict: Overstated.**

The adversarial review claims the camera only works because `current_player` is set before `_set_state` fires in `_switch_turn` (line 282-285). But this ordering is not accidental — `_switch_turn` sets the player, then transitions state. The camera's `_on_state_changed` reading `GameManager.current_player` is standard signal-driven code. The `turn_changed` handler being a no-op is unused code, not evidence of fragility.

If someone reordered `_switch_turn` to emit `state_changed` before setting `current_player`, the camera would break — but that's true of any signal-driven system. The ordering is correct by design. **Not a valid miss.**

#### Claim 5: Striker not in `pieces` "by accident"
**Verdict: Overstated.**

The striker has collision_layer=4 (pieces have layer=2), mass=15g (pieces have 5g), and is handled separately in `on_piece_pocketed` (line 333 early return), `_check_simulation_complete` (separate check at line 202), and `_handle_foul` (repositioned, not returned via `_return_piece_to_center`). This is intentional separation, not an accident.

The adversarial review's concern about "future code assuming `pieces` is all rigid bodies" is speculative. The current design clearly treats striker as a distinct entity. **Not a valid miss.**


### Ox — Root Cause Analysis (3 findings)

**Question: Why did Rabbit's framework miss these specific bugs?**

#### Finding 1: Animals analyze files, not flows (HIGH confidence: 85%)

Every Rabbit animal in the prior run received the game loop code and analyzed it per-function or per-file. Tiger found edge cases in `_check_simulation_complete`, `_return_piece_to_center`, scoring. Rat found consequence chains in `release_power` → `_shoot_striker` → simulation → resolve. Both worked within the boundaries of individual functions or within a single file's logic.

The double-pocket bug requires following a chain: `board.gd` creates Area3D → connects `body_entered` signal → `_on_pocket_body_entered` calls `GameManager.on_piece_pocketed()` → handler in `game_manager.gd` has no guard. This is a 2-file, 3-hop chain. No animal was tasked with "trace every signal connection and verify the contract at each boundary."

**Root cause:** The animal framework defines analysis targets as "the code" — not as "the data/signal flow." Adding explicit flow-tracing as a technique would catch this class of bug.

#### Finding 2: No animal has domain knowledge or seeks it (MEDIUM confidence: 75%)

The win condition bug is valid GDScript. The state machine is well-structured. The architecture is appropriate. Every question Rabbit's animals ask is about the code — its structure, its edge cases, its consequences. None ask "is this the right behavior for carrom?"

This isn't about the LLM lacking carrom knowledge. It's about no animal being prompted to compare implemented behavior against domain rules. Ox asks "is the architecture appropriate for what this is?" — close, but "appropriate architecture" and "correct rules" are different questions.

**Root cause:** Domain rule validation is not a technique in any animal's toolkit. It would fit naturally in Ox ("first principles includes: do the rules match reality?") or as a standalone check.

#### Finding 3: Rat's "net zero" conclusion reveals a stopping-too-early pattern (MEDIUM confidence: 70%)

Rat traced the earn-then-subtract chain and correctly identified the net-zero outcome. But Rat's job is consequence mapping — and the consequence of earning points during simulation then subtracting them 1-3 seconds later is a visible score jump. Rat stopped at data correctness instead of extending to experiential correctness.

This isn't a framework gap — it's a calibration issue. Rat's consequence tracing should extend to "what does the user perceive?" not just "what's the final data state?" For game code specifically, the player's experience IS the consequence.

**Root cause:** Rat's stopping criterion is data-state correctness, not user-experience correctness. For game code (and most UI code), these diverge.
