# XXI The World — Game Loop Deep Reading

**Target:** Carrom game loop — second pass for missed issues
**Type:** Broad review (complementing previous Death + Tower + Moon spread)
**Density:** Medium
**Spread:** Hermit + Fool + High Priestess
**Date:** 2026-03-30
**Prerequisite:** Read `tarot-output/gameloop-reading.md` first (Death + Tower + Moon findings)

---

## Synthesis

The first spread found accounting bugs and physics-return issues. This spread finds what's fragile, confusing, or subtly off.

The deepest finding is in the physics model. Pieces use `linear_damp = 0.5` with `gravity_scale = 0` as a proxy for board friction. This produces exponential velocity decay instead of the constant deceleration that real surface friction (Coulomb friction) produces. Pieces decelerate aggressively at high speed but coast indefinitely at low speed. A full-power shot takes ~11 seconds to settle. Real carrom: 2-4 seconds.

The initial piece layout uses `d2 = 3.2 cm` for six outer-ring pieces, which is less than the collision diameter (3.6 cm). While 2D ring geometry means adjacent pieces clear by ~0.09 cm, this is within physics engine tolerances. Project memory confirms this is a known issue.

For player experience: input mapping is aspect-ratio dependent, there's no back-navigation from AIM, and zero-power shots are allowed.

The most subtle finding: collision audio deduplication can produce silence for gentle hits.

---

## Tension Map

### Physics fidelity vs implementation simplicity
**Hermit** says: Exponential damping is wrong for surface friction. 3x settling time, wrong speed curve.
**Fool** says: Players see "slow" not "wrong model." Crank the constant or fix the model — same UX outcome.
**The tradeoff:** Custom Coulomb friction is more work but correct. Higher linear_damp is one-line but keeps wrong curve.

### Tight initial layout vs. physics self-correction
**Hermit** says: `d2 = 3.2 < 3.6` creates borderline overlap. 0.09 cm gap.
**Fool** says: Frame-1 explosion from overlap resolution looks like a bug.
**The tradeoff:** Change d2 constant (simple) or restructure layout formula (thorough).

### Agreements
- Zero-power shot (Fool + Death previous)
- State machine has no guardrails (High Priestess + Tower previous)

### Blind Spots
- Collision sound dedup ordering (High Priestess only)

---

## Action Items

1. Fix `d2` constant in `_spawn_pieces()` — root cause of initial piece overlap
2. Assess damping model — quick fix (increase constant) or proper fix (Coulomb friction)
3. Add minimum power threshold — prevent zero-power shots
4. Trace collision sound path for gentle hits — verify audio dedup issue
5. Add AIM→PLACE_STRIKER back-navigation

## Coverage Gaps

- Magician: Godot 4's `_integrate_forces()` for proper friction model
- Death / Replay Probe: Compound foul score inflation across multiple turns
- Devil: Whether 2D-in-3D approach (gravity=0, Y-lock) is the right architectural call

---

## Raw Card Outputs

### Hermit IX — 3 Findings

#### Finding 1: Exponential damping model produces unrealistic settling times

**Confidence:** 80

Pieces use `linear_damp = 0.5` (board.gd:20) with `gravity_scale = 0.0` (board.gd:313) and `axis_lock_linear_y = true` (board.gd:319). Since pieces don't press against the board (no gravity, Y-locked), surface friction doesn't apply. The only speed reduction comes from `linear_damp`, which Godot applies as exponential decay:

```
v(t) = v0 * e^(-damp * t)
```

With damp=0.5 and max speed 110 cm/s (power 5 * 22):
- 1 second: 66.7 cm/s
- 3 seconds: 24.5 cm/s
- 5 seconds: 9.1 cm/s
- 8 seconds: 2.0 cm/s
- 10.8 seconds: 0.5 cm/s (STOP_THRESHOLD)

Real carrom friction (Coulomb model) produces constant deceleration:
```
v(t) = v0 - μ * g * t
```

With typical carrom powder friction (μ ≈ 0.1), a 110 cm/s shot stops in:
```
t = v0 / (μ * g) = 110 / (0.1 * 981) ≈ 1.1 seconds
```

Even with lower friction (μ ≈ 0.03, well-powdered board):
```
t = 110 / (0.03 * 981) ≈ 3.7 seconds
```

The exponential model takes 11 seconds. The linear model takes 1-4 seconds. The difference is the long tail — exponential decay asymptotically approaches zero, hence the need for `STOP_THRESHOLD` as a cutoff.

**Gameplay impact:**
- High-speed shots feel snappy (exponential decay is steep at high v)
- Low-speed coasting feels floaty (exponential tail is shallow near threshold)
- Total simulation time is 3-10x longer than real carrom

**Possible fixes:**
1. Increase `linear_damp` to ~2.0 (halves settling time, keeps wrong curve)
2. Override `_integrate_forces()` to apply constant deceleration force
3. Use `PhysicsServer3D.body_add_constant_force()` per piece

---

#### Finding 2: Piece layout spacing constant d2 creates sub-collision-diameter gaps

**Confidence:** 90

In `_spawn_pieces()` (board.gd:218-221):
```gdscript
var r := MEN_RADIUS  # 1.8 cm
var d := 2.0 * r + 0.1  # 3.7 cm — 0.1 cm gap between pieces
var d2 := 2.0 * r - 0.4  # 3.2 cm — 0.4 cm OVERLAP
```

Two pieces with radius 1.8 cm need center-to-center distance >= 3.6 cm to not overlap. `d2 = 3.2 < 3.6`.

Six outer-ring pieces use `d2` for their radial positioning (lines 234, 237, 238, 240, 241, 243). These pieces sit at radius `2 * d2 = 6.4 cm` from center. Because they're on a ring, the actual center-to-center distances between adjacent pieces are larger than `d2` itself.

**Tightest pairs verified:**
- (0, 3.7) to (3.2, 5.543): 3.69 cm — gap of 0.09 cm
- (3.204, 1.85) to (3.2, 5.543): 3.69 cm — gap of 0.09 cm
- (3.2, 5.543) to (0, 7.4): 3.70 cm — gap of 0.10 cm

All pairs clear the 3.6 cm minimum, but by less than 0.1 cm. Physics engine collision detection tolerances, floating-point rounding, and the fact that CylinderShape3D collision isn't perfectly circular at coarse segment counts could turn these 0.09 cm gaps into overlaps.

**Confirmed by project memory:** "Physics revamp in progress — Pieces overlapping, needs spacing fix next session."

**Fix:** Change `d2` to match `d`: `var d2 := 2.0 * r + 0.1` (or use a single consistent spacing constant).

---

#### Finding 3: Inconsistent Y positions between initial spawn and piece returns

**Confidence:** 50

Pieces spawn at `MEN_HEIGHT * 0.5 = 0.1 cm` (board.gd:269):
```gdscript
piece.position = Vector3(pos_x, MEN_HEIGHT * 0.5, pos_z)
```

Pieces are returned to `Y = 0.2 cm` (game_manager.gd:263):
```gdscript
piece.global_position = Vector3(0, 0.2, 0) + offset
```

Striker placement also uses `Y = 0.2` (game_manager.gd:129).

With `axis_lock_linear_y = true`, pieces stay at whatever Y they're placed at. A returned piece at Y=0.2 has its CylinderShape3D (height 0.2) spanning Y=0.1 to Y=0.3. An original piece at Y=0.1 spans Y=0.0 to Y=0.2. They overlap in the Y=0.1-0.2 range, so collisions work. But the collision normal has a slight Y component that gets discarded by the axis lock.

**Impact:** Negligible in practice. The 0.1 cm offset produces an imperceptible angle in collision normals. But the inconsistency is a latent bug — if the Y offset were larger, collisions between original and returned pieces would behave differently.

---

### Fool 0 — 3 Findings

#### Finding 1: Placement input mapping is aspect-ratio dependent

**Confidence:** 80

striker.gd line 61:
```gdscript
var world_x: float = (mouse_x - screen_w / 2.0) / screen_w * 74.0
```

This maps the full screen width to 74 cm (the board width). But the valid placement range is only ±12.3 cm (game_manager.gd:22-23), clamped in `place_striker_at()`.

**At 1920x1080 (16:9):**
- Full screen maps to ±37 cm
- Valid range ±12.3 cm = 33.2% of screen width
- Only the middle ~637 pixels of 1920 produce movement
- 641 pixels on each side are dead zones (clamped)

**At 2560x1080 (ultrawide 21:9):**
- Full screen maps to ±37 cm (same formula)
- Valid range still ±12.3 cm = 33.2% of 2560 = ~851 pixels
- ~855 pixels dead zone on each side

**At 1024x768 (4:3):**
- Same formula: 33.2% active = ~340 pixels active
- ~342 pixels dead zone each side

A first-time player moves the mouse expecting the striker to track. On the outer 2/3 of the screen, nothing happens. The striker "sticks" at the placement limit.

The root cause: the mapping should use the camera's projection to map screen coordinates to world coordinates (raycasting), not a fixed linear formula. The camera angle (not perfectly top-down) adds further inaccuracy.

---

#### Finding 2: No back-navigation from AIM to PLACE_STRIKER

**Confidence:** 75

State transitions:
```
PLACE_STRIKER → (left click) → AIM → (left click) → POWER → (release) → SIMULATION
                                                     ↑ (right click) ↓
                                                     AIM ←←←←←←←←←←←←
```

Right-click in POWER cancels back to AIM (striker.gd:95-99). But there is no equivalent back-navigation from AIM to PLACE_STRIKER. Once the player confirms placement (left click), they're committed.

A first-time player who clicks to confirm placement but realizes the striker is in the wrong spot has no recourse. They must aim and shoot from the bad position, potentially wasting the turn.

**The asymmetry is confusing:** right-click works as "cancel" in POWER but does nothing in AIM. A consistent pattern would be: right-click always goes back one step.

---

#### Finding 3: Zero-power shot has no minimum threshold or feedback

**Confidence:** 85

In `_handle_power()` (striker.gd:85):
- Left mouse down: `GameManager.is_charging = true`
- Left mouse up: `GameManager.release_power()`

If the player clicks and releases quickly (< 1 frame), `_process()` may not have incremented power at all. `power` remains 0.0. `release_power()` fires. `_shoot_striker()` applies velocity `0 * 22 = 0 cm/s`. Simulation starts and immediately resolves (all pieces stationary).

The turn is wasted with no visual or audio feedback that anything went wrong. The power bar showed 0%, the striker didn't move, and the state cycles through SIMULATION → PLACE_STRIKER (opponent's turn) in one frame.

This is especially problematic for first interaction — a player's natural instinct is to click (expecting a UI response), then figure out the mechanic. The first click wastes their turn.

**Fix options:**
1. Minimum power threshold: if power < 0.3, don't fire (return to POWER state)
2. Require a minimum hold time before arming the release
3. Show "too weak!" feedback if power is below minimum

---

### High Priestess II — 3 Findings

#### Finding 1: State machine accepts any transition without validation

**Confidence:** 75

`_set_state()` (game_manager.gd:114):
```gdscript
func _set_state(new_state: State) -> void:
    current_state = new_state
    state_changed.emit(new_state)
```

No validation of the transition. All of these would succeed silently:
- `_set_state(State.SIMULATION)` from PLACE_STRIKER (skip aim and power)
- `_set_state(State.PLACE_STRIKER)` from SIMULATION (skip turn resolution)
- `_set_state(State.POWER)` from PLACE_STRIKER (skip aim)

Currently, only legal transitions are called because each caller checks its own preconditions (e.g., `if current_state != State.AIM: return` in `confirm_aim()`). But `_set_state` itself is unguarded.

The specific risk: striker.gd:99 calls `GameManager._set_state(GameManager.State.AIM)` directly — a private function called from outside, bypassing any future validation. If `_set_state` gained a transition table, this call would bypass it.

**Implicit contract:** "Only call _set_state with a valid next state from the current state." Enforced by convention, not code.

---

#### Finding 2: Collision sound dedup produces silence for gentle striker-piece hits

**Confidence:** 70

board.gd:365-377 — collision sound logic:
```gdscript
func _on_piece_collision(other: Node, piece: RigidBody3D) -> void:
    var vel := piece.linear_velocity.length()
    if vel < 5.0:
        return
    # ...
    elif other is RigidBody3D:
        if piece.get_instance_id() < other.get_instance_id():
            AudioManager.play_collision_sound(...)
```

Deduplication: only the piece with the lower instance_id plays the sound (avoids double-play since both pieces fire `body_entered`).

Instance IDs are assigned in creation order. Pieces are created in `_spawn_pieces()` (board.gd:254-275). Striker is created in `_spawn_striker()` (board.gd:283). Therefore: all 19 pieces have lower instance_ids than the striker.

When a piece and striker collide:
- piece.get_instance_id() < striker.get_instance_id() → TRUE
- The PIECE's callback runs the sound logic
- Sound threshold checks PIECE's velocity (not striker's, not relative)

For a gentle striker hit (striker at 8 cm/s):
- Post-collision: striker slows to ~4 cm/s, piece gains ~4 cm/s (approximate, equal masses would be different but striker is 3x heavier)
- With mass ratio 15:5 (3:1), piece gains more momentum: piece velocity ≈ 2 * 15/(15+5) * 8 = 12 cm/s (elastic collision formula)
- Piece velocity 12 > 5.0 → sound plays ✓

For a very gentle hit (striker at 6 cm/s):
- Piece velocity ≈ 2 * 15/20 * 6 = 9 cm/s
- Piece velocity 9 > 5.0 → sound plays ✓

For a glancing blow (striker at 10 cm/s, 60° angle):
- Normal component: 10 * cos(60°) = 5 cm/s
- Piece gains ~7.5 cm/s in normal direction, tangential unchanged
- Piece total velocity depends on geometry... but normal component transferred is small
- Could result in piece velocity < 5.0 → NO sound ✗

**Impact:** Glancing collisions between striker and pieces at moderate speeds may produce no sound. The effect is more pronounced for piece-piece collisions where the impacting piece slows below 5 cm/s on contact.

**Root cause:** Velocity threshold should use relative velocity (or contact impulse), not the reporting piece's absolute velocity.

---

#### Finding 3: Signal emission ordering in _switch_turn creates fragile camera dependency

**Confidence:** 80

game_manager.gd:282-285:
```gdscript
func _switch_turn() -> void:
    current_player = 2 if current_player == 1 else 1  # Line 283
    turn_changed.emit(current_player)                   # Line 284
    _set_state(State.PLACE_STRIKER)                     # Line 285
```

`_set_state` emits `state_changed`. Camera's `_on_state_changed` reads `GameManager.current_player` to select the preset (camera_controller.gd:55, 63).

**Dependency:** `current_player` MUST be updated (line 283) BEFORE `_set_state` (line 285) fires `state_changed`. If these lines were reordered — say, emit turn_changed last for "cleaner" signal ordering — the camera would use the wrong player's preset on every turn switch.

This ordering dependency is:
- Undocumented
- Not enforced by any assertion or contract
- Would fail silently (camera shows wrong angle, no error, no crash)
- Easy to break during a refactor

**Additionally:** `turn_changed` fires (line 284) before `state_changed` (line 285). If any `turn_changed` subscriber reads `current_state`, it's still SIMULATION (not yet PLACE_STRIKER). Currently no subscriber does this, but it's another implicit assumption.

**Mitigation:** Pass player and state as signal parameters instead of relying on global state reads. Or document the ordering requirement with a comment.
