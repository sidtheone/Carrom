# Game States & Flow

## States
```
STATE 0: PLACE STRIKER  (Orthographic 2D top-down)
    ↓ [Right-click after placing]
STATE 2: AIM DIRECTION  (Perspective 3D, camera at 0,-2,6)
    ↓ [Left-click hold]
STATE 3: SET POWER      (Perspective 3D, camera at 0,-1,10)
    ↓ [Left-click release]
STATE 1: SIMULATION     (Perspective 3D, camera at 0,-2,8)
    ↓ [All pieces stop]
STATE 0: PLACE STRIKER  (loop)
```

## State Details

### STATE 0 — Place Striker
- **View:** glOrtho (2D top-down)
- **Input:** Left-click to place in yellow zone, Right-click to confirm
- **Constraints:** x ∈ [-123, +123], y fixed at -145 (scaled)
- **Visual:** Wire cube shows placement zone

### STATE 2 — Aim Direction
- **View:** Perspective 65°, gluLookAt(0,-2,6)
- **Input:** Mouse X movement → angle (-75° to +75°)
- **Visual:** Cone on striker shows direction
- **Formula:** `angle = -(mouseX - screenW/2) / 300 * 75`

### STATE 3 — Set Power
- **View:** Perspective 65°, gluLookAt(0,-1,10)
- **Input:** Hold left button (power bar fills), release to fire
- **Visual:** Vertical rectangle grows (0→5 units, cycles)
- **Audio:** Looping power bar sound

### STATE 1 — Simulation
- **View:** Perspective 65°, gluLookAt(0,-2,8)
- **Input:** None (automatic)
- **Physics:** Runs until all pieces have velocity < 0.0005
