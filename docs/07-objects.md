# Game Objects & Architecture

## Class Hierarchy
```
Object (base)
├── CarromBoard
├── CarromMen (9 black + 1 red + 8 white)
├── BoardSurface
├── Boundary
└── Pocket
```

## Object Properties
```cpp
class Object {
  Vector3D position;
  Vector3D velocity;
  float radius;
  float mass;
  float height;
  bool active;  // false = pocketed
}
```

## Initial Setup (18 pieces + 1 striker)

### Carrom Men Layout
- **Center:** Red queen at (0, 0)
- **Inner ring (6):** Around center at radius ~36
- **Outer ring (12):** Around center at larger radius
- Alternating black/white placement

### Striker
- **Start position:** (0, -290, 0)
- **Placement zone:** y = -145 (scaled), x = [-123, +123]

## Missing Game Logic
- No scoring system
- No turn management (player 1 / player 2)
- No win/loss conditions
- No piece ownership tracking
- No foul detection (striker pocketed)
- No queen covering rules
