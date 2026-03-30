# Rabbit — Aim Plan Analysis

**Audience:** Game developer validating a redesign plan before implementing
**Scope:** Section-sized — one plan doc, ~50 lines. Direct analysis.
**Calibration:** Thin target — Tiger 3/5, Rat 3/5, Snake 2/3

---

## Synthesis

The core approach is right — raycast eliminates the P1/P2 flip mess completely, and matching pool game UX is the correct call. One gap will break gameplay if not addressed.

**The plan says raycast "naturally limits to forward hemisphere." It doesn't.** When the mouse is between the striker and the player's own wall, the raycast produces a valid Y=0 hit point — but the direction vector points BACKWARD. In real carrom you can't shoot toward your own side. The current system prevents this with `MAX_AIM_ANGLE = 75°`. The plan removes that constant and adds no replacement guard.

Fix: after computing direction, check that it points toward the opponent's side. Simplest: `if dir.z > 0: return` for P1, `if dir.z < 0: return` for P2. But that reintroduces P1/P2 logic. Better: compute the "forward" vector from the striker's baseline position (`forward.z = -sign(striker.global_position.z)`) and reject aim directions where `dir.dot(forward) < 0`. One line, no player check.

Everything else in the plan is sound. Two minor notes:

**Placement refactor is optional scope.** The plan bundles aim + placement into one change. Placement currently works (with known aspect-ratio issues) and doesn't share the P2 bugs that motivated this redesign. Splitting it out reduces blast radius. But if you want to ship it together, the raycast technique is identical for both — no extra complexity, just more surface area to test.

**12 sphere MeshInstance3Ds work fine** but are heavier than needed. A single `ImmediateMesh` drawing a dashed line is 1 node instead of 12. Either works at this scale — the spheres are more readable code, the ImmediateMesh is cleaner scene tree.

### Findings Summary

**Tiger (Stress Test)** — 1/3 critical

✗ Backward aim unguarded (HIGH confidence: 90%)
  Raycast behind striker produces valid but wrong direction. Player can shoot toward own wall.

✓ Raycast at steep camera angles (LOW confidence: 50%)
  AIM camera at -50° works fine. Edge-of-screen hits are far but direction normalizes correctly.

✓ Viewport coordinate mapping (LOW confidence: 60%)
  `event.position` matches `project_ray_origin` expectations in Godot 4 stretch mode "viewport".

**Rat (Consequences)** — 1/3 notable

✓ Dots as striker children (LOW)
  Move with striker during placement — correct. Hidden on SIMULATION — correct.

✗ Placement raycast changes feel subtly (MEDIUM confidence: 65%)
  At angled cameras, perspective projection means mouse-Y affects world-X of hit point. Minimal at PLACE_STRIKER preset (-85°, nearly top-down) but noticeable if camera changes.

✓ 12 spheres performance (LOW)
  Visual-only MeshInstance3Ds, no collision. Fine for 20-piece carrom game.

**Snake (Scope)** — 1/2 cuttable

✗ Placement refactor bundled with aim (MEDIUM — Earned: debatable)
  Adds surface area. Fixes aspect-ratio bug but that's not the motivating problem. Could ship separately.

✓ Dot count / approach (LOW — Earned: yes)
  12 spheres vs ImmediateMesh is preference, not scope creep. Both work.

### Action Items

1. **Add forward-hemisphere guard.** After computing aim direction from raycast, reject directions that point toward the player's own side. `dir.dot(forward) < 0` where `forward = Vector3(0, 0, -sign(striker.z))`. This replaces `MAX_AIM_ANGLE` with a spatial check. Without this, backward shots are possible.

2. **Decide whether to bundle placement refactor.** The aim redesign works independently. Placement raycast fixes the aspect-ratio bug but increases test surface. Ship together or split?

3. **Decide dot visual approach.** 12 spheres (readable, more nodes) vs ImmediateMesh (1 node, slightly more complex code). No wrong answer at this scale.

### Coverage Gaps

- **Monkey / Hostile Input** — What happens when the raycast hits exactly on the striker position (direction = zero vector)? Division by zero in normalize?
- **Dragon / Temporal** — If the camera presets change in the future (e.g., closer camera, different angle), does the raycast approach degrade?
- **Ox / First Principles** — Is `project_ray_origin` the right Godot API, or should this use `PhysicsRayQueryParameters3D` with the physics server for more accurate board-plane intersection?

---

*Full raw outputs saved above.*
