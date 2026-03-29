# Physics Engine

## Core Algorithm
- **Method:** Spring-based collision (Hooke's Law: F = -kx)
- **Substeps:** 20 per frame for numerical stability
- **Spring constant (k):** 0.9
- **Friction (U_FRICTION):** 0.005
- **Velocity damping:** 0.99 per frame
- **Stop threshold:** velocity < 0.0005

## Object Properties

| Object | Radius | Mass | Height |
|--------|--------|------|--------|
| Carrom men | 18 | 12 | 2 |
| Striker | 22 | 18 | - |

## Board Dimensions
- **Surface:** 740 x 740
- **Boundary height:** 70, breadth: 80
- **Pocket radius:** 28 (4 corners)

## Collision Types
1. **Piece-to-piece:** Sphere-to-sphere (distance < r1 + r2)
2. **Wall:** AABB (axis-aligned bounding box)
3. **Pocket:** Distance-based (magnitude < pocket radius)

## Physics Loop
```
Timer(10ms) → carromEngine() {
  for 20 substeps:
    detect collisions (piece-piece, wall, pocket)
    apply spring forces
    apply friction
    apply damping (0.99)
    update positions
  check if all stopped → change state
}
```
