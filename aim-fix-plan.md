# Redesign Aim System — Point-to-Aim with Dotted Line

## Context
The current aim system maps horizontal mouse movement to an angle, with P2 direction flips scattered across 3 files. It's unintuitive, aspect-ratio dependent, and the P2 logic has been a recurring source of bugs.

**New design:** Raycast from camera through mouse position onto the board plane. Direction = striker → hit point. This inherently handles P2 because the camera is already rotated 180° — the raycast maps screen coordinates to the correct world position for both players automatically. Zero P1/P2 special-casing needed.

## Files to Modify

### `autoload/game_manager.gd`
- Replace `aim_angle: float` with `aim_direction: Vector3 = Vector3(0, 0, -1)`
- Replace `set_aim_angle(angle_deg)` with `set_aim_direction(dir: Vector3)` — stores the normalized XZ direction
- `_shoot_striker()`: use `aim_direction` directly instead of computing from angle. **Remove the P2 direction flip entirely.**
- Remove `MAX_AIM_ANGLE` constant (no longer needed — raycast naturally limits to forward hemisphere)

### `scenes/game/striker.gd` — Major rewrite of aim/visual sections
- **Aim input** (`_handle_aim`):
  - Get camera via `get_viewport().get_camera_3d()`
  - `camera.project_ray_origin(mouse_pos)` + `camera.project_ray_normal(mouse_pos)`
  - Intersect ray with Y=0 plane: `t = -origin.y / normal.y`, `hit = origin + normal * t`
  - Direction = `(hit - global_position)` projected to XZ, normalized
  - **Forward-hemisphere guard:** reject backward aims with `dir.dot(Vector3(0, 0, -sign(global_position.z))) < 0`
  - **Zero-direction guard:** if `dir.length() < 0.1`, ignore (mouse on striker = zero vector = crash)
  - Call `GameManager.set_aim_direction(direction)`
  - Update dots visual

- **Placement input** (`_handle_placement`):
  - Also use raycast for X position (fixes the aspect-ratio bug — Finding #15)
  - `hit.x` from the same raycast technique → `GameManager.place_striker_at(hit.x)`
  - Remove `world_x` screen-width mapping and P2 `world_x = -world_x` flip

- **Visual** — replace cone with dotted line:
  - `_create_aim_dots()`: create 12 small sphere MeshInstance3Ds (radius 0.4cm), semi-transparent white, as children of the striker
  - `_update_aim_dots(dir)`: position each dot along `dir`, spacing 4cm apart, opacity fading from 0.8 to 0.1
  - Show/hide on state change (same as current cone logic)

- **Remove**: `_create_aim_indicator`, `_update_aim_visual`, all `flip` logic

### `scenes/ui/hud.gd`
- Update AIM state text from "Aim (move mouse, click)" to "Aim (point & click)"

## Key Benefit
**Zero P1/P2 special-casing.** The camera projection handles player perspective inherently. No direction flips in `_shoot_striker`, no angle negation in `_handle_aim`, no `world_x = -world_x` in placement. One code path for both players.

## Verification
1. P1: move mouse around board → dotted line follows, striker shoots where dots point
2. P2: same behavior, no inversion bugs, feels identical to P1
3. Aim line fades with distance, looks clean
4. Power charge still works (hold-release unchanged)
5. Right-click cancel (POWER → AIM) still works
6. Placement also works via raycast for both players
