# Rabbit — Aim Plan Analysis

**Audience:** Game developer validating a redesign plan before implementing
**Scope:** Section-sized — one plan doc, ~50 lines. Direct analysis.
**Calibration:** Thin target — Tiger 3/5, Rat 3/5, Snake 2/3

---

> **Status update (2026-03-31):** ✅ **IMPLEMENTED** in commit `483f75f`. Raycast aim system shipped with dotted line + sphere collisions. The backward aim guard and placement refactor were both included. This analysis is now historical context.

## Synthesis

The core approach is right — raycast eliminates the P1/P2 flip mess completely, and matching pool game UX is the correct call. ~~One gap will break gameplay if not addressed.~~

~~**The plan says raycast "naturally limits to forward hemisphere." It doesn't.**~~ ✅ **ADDRESSED** — The implemented aim system uses raycast point-to-aim with sphere collisions, and the P1/P2 direction flip logic was fully eliminated (3 locations → 0).

~~**Placement refactor is optional scope.**~~ ✅ **SHIPPED** — Bundled with aim redesign as planned.

**12 sphere MeshInstance3Ds work fine** — this approach was used in the implementation. Readable code, acceptable at this scale.

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

1. ~~**Add forward-hemisphere guard.**~~ ✅ Done — implemented in commit `483f75f`.
2. ~~**Decide whether to bundle placement refactor.**~~ ✅ Done — shipped together.
3. ~~**Decide dot visual approach.**~~ ✅ Done — 12 spheres approach used.

### Coverage Gaps

- **Monkey / Hostile Input** — What happens when the raycast hits exactly on the striker position (direction = zero vector)? Division by zero in normalize?
- **Dragon / Temporal** — If the camera presets change in the future (e.g., closer camera, different angle), does the raycast approach degrade?
- **Ox / First Principles** — Is `project_ray_origin` the right Godot API, or should this use `PhysicsRayQueryParameters3D` with the physics server for more accurate board-plane intersection?

---

*Full raw outputs saved above.*
