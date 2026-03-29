# Engine Trace: Release Trigger (Striker Fire)

Complete trace of what happens before, during, and after the player releases
the mouse button to fire the striker. Every function call, signal emission,
and side effect is documented with file:line references.

---

## Timeline Overview

```
 BEFORE RELEASE                    RELEASE MOMENT                AFTER RELEASE
 ──────────────                    ──────────────                ─────────────
 POWER state active                Mouse button UP event         SIMULATION state
 is_charging = true                ├─ stop charging              ├─ Physics engine runs
 power increasing each frame       ├─ stop audio                 ├─ Collisions detected
 power bar filling                 ├─ unfreeze striker           ├─ Pocket detection
 aim indicator visible             ├─ compute direction          ├─ Sounds play
                                   ├─ apply impulse              ├─ Pieces decelerate
                                   ├─ clear turn tracking        └─ All stop → resolve turn
                                   └─ emit state_changed
```

---

## BEFORE RELEASE — Power Charging

### Entry into POWER state

The player clicked to confirm aim. This triggered:

```
striker.gd:83   → event.pressed (mouse DOWN in AIM state)
striker.gd:84   → GameManager.confirm_aim()
game_manager.gd:120   power = 0.0
game_manager.gd:121   is_charging = false        ← NOT charging yet
game_manager.gd:122   _set_state(State.POWER)
game_manager.gd:85      current_state = POWER
game_manager.gd:86      state_changed.emit(POWER)
                           ├─ camera_controller.gd:62  → tween to POWER preset
                           ├─ hud.gd:59                → state_label = "Power (hold... release!)"
                           ├─ hud.gd:60                → placement_indicator hidden
                           └─ striker.gd:37             → aim_indicator stays visible
```

At this point the mouse button from the aim-confirm click is still held.
When the player releases it, `_handle_power` sees `not event.pressed` but
`is_charging` is `false`, so nothing fires. This prevents the zero-power bug.

### Player clicks and holds (new click)

```
striker.gd:88   → InputEventMouseButton, MOUSE_BUTTON_LEFT
striker.gd:89   → event.pressed = true (mouse DOWN)
striker.gd:91   → GameManager.is_charging = true    ← charging begins
striker.gd:92   → AudioManager.play_power_bar()
audio_manager.gd:78   → _power_bar_player.play()   ← looping charge sound starts
```

### Every frame while held (charging loop)

```
game_manager.gd:63   _process(delta) called by engine
game_manager.gd:67     match current_state → State.POWER
game_manager.gd:69       is_charging == true
game_manager.gd:70       power = minf(power + 3.0 * delta, 5.0)
                          ← power increases by 3.0 units/sec, capped at 5.0

hud.gd:50        _process(delta) called by engine
hud.gd:51          current_state == POWER
hud.gd:52          power_bar.value = (power / 5.0) * 100.0   ← bar fills
hud.gd:53          power_bar.visible = true
```

**State at this moment:**
| Variable | Value | Location |
|----------|-------|----------|
| `current_state` | `POWER` | game_manager.gd:33 |
| `is_charging` | `true` | game_manager.gd:38 |
| `power` | `0.0 → 5.0` (increasing) | game_manager.gd:37 |
| `aim_angle` | `-75.0 to +75.0` (frozen) | game_manager.gd:36 |
| `striker.freeze` | `true` | set in place_striker_at |
| `striker.linear_velocity` | `(0, 0, 0)` | frozen body |
| `aim_indicator.visible` | `true` | striker.gd:38 |
| `power_bar.visible` | `true` | hud.gd:53 |
| Audio | power_bar.wav looping | audio_manager.gd |

---

## RELEASE MOMENT — The Trigger

Player releases left mouse button. Single input event propagates through
Godot's input system → `_unhandled_input` on striker.

### Step 1: Input captured by striker

```
striker.gd:43   _unhandled_input(event)
striker.gd:44     game_active == true → continue
striker.gd:47     match current_state → State.POWER
striker.gd:53       _handle_power(event)
striker.gd:88         event is InputEventMouseButton ✓
striker.gd:88         button_index == MOUSE_BUTTON_LEFT ✓
striker.gd:93         event.pressed == false (mouse UP)
striker.gd:95           is_charging == true ✓
striker.gd:96           → GameManager.release_power()
```

### Step 2: Stop charging, stop audio

```
game_manager.gd:128  release_power()
game_manager.gd:129    current_state == POWER ✓ (guard passes)
game_manager.gd:131    is_charging = false              ← charging stops
game_manager.gd:132    AudioManager.stop_power_bar()
audio_manager.gd:84      _power_bar_player.stop()       ← sound stops immediately
game_manager.gd:133    _shoot_striker()                  ← fire!
```

### Step 3: Unfreeze striker and apply impulse

```
game_manager.gd:136  _shoot_striker()
game_manager.gd:137    striker == null? → no, continue
game_manager.gd:139    striker.freeze = false             ← physics enabled

game_manager.gd:141    angle_rad = deg_to_rad(aim_angle)
                        ← e.g. aim_angle = 15.0° → angle_rad = 0.2618

game_manager.gd:142    direction = Vector3(sin(0.2618), 0.0, cos(0.2618))
                        ← direction = Vector3(0.2588, 0.0, 0.9659)

game_manager.gd:143    if current_player == 2:
game_manager.gd:144      direction.z = -direction.z      ← flip for P2

game_manager.gd:145    impulse = direction * power * 2.0
                        ← e.g. power = 3.5:
                           impulse = (0.2588, 0, 0.9659) * 3.5 * 2.0
                                   = (1.812, 0, 6.761)

game_manager.gd:146    striker.apply_central_impulse(impulse)
                        ← Godot physics engine receives impulse
                           striker begins moving this physics frame
```

**Direction by player:**
```
Player 1 (at Z = -2.9):  direction.z = +cos(angle)   → shoots toward +Z (center)
Player 2 (at Z = +2.9):  direction.z = -cos(angle)   → shoots toward -Z (center)
Angle = 0°:  straight ahead         (sin=0, cos=1)
Angle = +75°: hard right for P1     (sin=0.97, cos=0.26)
Angle = -75°: hard left for P1      (sin=-0.97, cos=0.26)
```

**Impulse magnitude examples:**
| Power | Multiplier | Result (straight shot) | Feel |
|-------|-----------|----------------------|------|
| 0.5 | × 2.0 | 1.0 | Gentle tap |
| 2.0 | × 2.0 | 4.0 | Medium shot |
| 3.5 | × 2.0 | 7.0 | Strong shot |
| 5.0 | × 2.0 | 10.0 | Maximum power |

### Step 4: Reset turn-tracking flags

```
game_manager.gd:148    pocketed_this_turn.clear()       ← empty array
game_manager.gd:149    striker_pocketed = false          ← no foul yet
game_manager.gd:150    own_piece_pocketed = false        ← no extra turn yet
```

### Step 5: Transition to SIMULATION

```
game_manager.gd:151    _set_state(State.SIMULATION)
game_manager.gd:85       current_state = SIMULATION
game_manager.gd:86       state_changed.emit(SIMULATION)
```

**Signal receivers fire:**
```
camera_controller.gd:62   _on_state_changed(SIMULATION)
camera_controller.gd:63     _transition_to(SIMULATION, current_player)
camera_controller.gd:83       tween: position → (0, 7.0, 2.0), rotation → (-70°, 0°, 0°)
                               ← camera pulls up to wide overhead view over 0.5s

hud.gd:59                _on_state_changed(SIMULATION)
hud.gd:60                  state_label.text = "Simulating..."
hud.gd:61                  placement_indicator.visible = false

hud.gd:50                _process (next frame)
hud.gd:54                  current_state != POWER → power_bar.visible = false
                           ← power bar hides

striker.gd:33             _on_state_changed(SIMULATION)
striker.gd:40               _aim_indicator.visible = false
                            ← aim cone disappears
```

---

## AFTER RELEASE — Simulation Phase

### Physics loop (every physics frame, ~60Hz)

Godot's physics engine handles all movement automatically:

```
For each RigidBody3D (striker + 19 pieces):
  1. Apply forces (none — gravity = 0, no external forces)
  2. Integrate velocity → update position
  3. Detect collisions (piece↔piece, piece↔wall, body↔pocket)
  4. Resolve collisions (bounce via PhysicsMaterial)
  5. Apply damping:
       linear_velocity *= (1.0 - linear_damp * delta)
       ← linear_damp = 3.0 for pieces, slows them over time
```

### Collision detection — Piece hits piece

When two RigidBody3D objects overlap, Godot resolves the collision using
their PhysicsMaterial (bounce=0.5, friction=0.4). Additionally, the
`body_entered` signal fires for sound:

```
board.gd:358     body.body_entered connected to _on_piece_collision.bind(body)

board.gd:363     _on_piece_collision(other, piece)
board.gd:364       vel = piece.linear_velocity.length()
board.gd:365       vel < 0.05? → skip (too quiet)
board.gd:367       other is StaticBody3D?
board.gd:368         YES (wall):
board.gd:370           piece == striker?
board.gd:371             → AudioManager.play_collision_sound(STRIKER_WALL, vel)
board.gd:373             → AudioManager.play_collision_sound(PIECE_WALL, vel)
board.gd:374       other is RigidBody3D?
board.gd:376           piece.get_instance_id() < other.get_instance_id()?
                        ← only one side triggers sound (prevents double-play)
board.gd:377             → AudioManager.play_collision_sound(PIECE_COLLISION, vel)

audio_manager.gd:69   play_collision_sound(sfx, velocity)
audio_manager.gd:71     gain = DEFAULT_GAINS[sfx]         ← e.g. 0.3 for piece-piece
audio_manager.gd:73     gain = clamp(0.3 * velocity * 0.5, 0.1, 1.0)
                         ← at vel=2.0: gain = 0.3, at vel=5.0: gain = 0.75
audio_manager.gd:74     play_sound(sfx, gain)
audio_manager.gd:57       find free AudioStreamPlayer from pool of 8
audio_manager.gd:60       player.volume_db = linear_to_db(gain)
audio_manager.gd:61       player.play()                    ← sound plays
```

### Pocket detection — Piece enters pocket

Pockets are Area3D nodes. When a RigidBody3D enters the trigger zone:

```
board.gd:148     area.body_entered.connect(_on_pocket_body_entered)

board.gd:153     _on_pocket_body_entered(body)
board.gd:154       body is RigidBody3D? ✓
board.gd:155       → GameManager.on_piece_pocketed(body)

game_manager.gd:288  on_piece_pocketed(body)
game_manager.gd:289    game_active? ✓
game_manager.gd:292    AudioManager.play_sound(POT, 0.7)    ← pocket sound

  IF body == striker:
game_manager.gd:295      striker_pocketed = true             ← foul flag set
game_manager.gd:296      velocity zeroed
game_manager.gd:299      return                              ← striker stays visible

  IF body is a piece:
game_manager.gd:301    body.visible = false                  ← piece disappears
game_manager.gd:302    body.freeze = true                    ← stop physics
game_manager.gd:303    body.global_position = (0, -10, 0)   ← move off-board
game_manager.gd:304    pocketed_this_turn.append(body)       ← track for turn resolution

game_manager.gd:306    piece_color = body.get_meta("color")  ← BLACK/WHITE/RED

game_manager.gd:310    Scoring:
                          BLACK → +10 pts
                          WHITE → +20 pts
                          RED   → +50 pts
game_manager.gd:318    scores[current_player - 1] += points
game_manager.gd:319    score_updated.emit(...)               ← HUD updates score
game_manager.gd:320    piece_pocketed.emit(...)              ← HUD updates piece count

                        Signal receivers:
                          hud.gd:68  → _update_piece_counts()
                          hud.gd:69  → _update_queen_status()

game_manager.gd:323    Own piece check:
                          P1 pocketed BLACK? → own_piece_pocketed = true
                          P2 pocketed WHITE? → own_piece_pocketed = true

game_manager.gd:328    Queen check:
                          RED pocketed? → queen_pocketed_by = current_player
                                          queen_covered = false
```

### Stop detection — Polling every frame

```
game_manager.gd:63   _process(delta)
game_manager.gd:67     match → State.SIMULATION
game_manager.gd:72       _check_simulation_complete()

game_manager.gd:156  _check_simulation_complete()
game_manager.gd:160    for each piece in pieces[]:
game_manager.gd:161      if piece.visible AND velocity.length() > 0.005:
game_manager.gd:162        return                ← still moving, check next frame
game_manager.gd:163    if striker.velocity.length() > 0.005:
game_manager.gd:164      return                  ← still moving
game_manager.gd:166    → _resolve_turn()          ← ALL stopped
```

### Turn resolution — Decision tree

```
game_manager.gd:169  _resolve_turn()

  ┌─ FOUL CHECK ─────────────────────────────────────────────┐
  │ game_manager.gd:171  striker_pocketed?                    │
  │   YES →                                                   │
  │     :172  foul_committed.emit("Striker pocketed!")        │
  │             └─ hud.gd:75  foul_label shows, fades in 2s  │
  │     :173  _handle_foul()                                  │
  │             :204  striker velocity zeroed                  │
  │             :206  striker.freeze = true                    │
  │             :207  striker.visible = true                   │
  │             :211  find one pocketed own-color piece        │
  │             :216    _return_piece_to_center(piece)         │
  │                       :223  piece → center + random offset│
  │                       :227  piece.visible = true           │
  │             :218  _switch_turn()                           │
  │                     :242  current_player flips 1↔2        │
  │                     :243  turn_changed.emit()             │
  │                     :244  _set_state(PLACE_STRIKER)       │
  │     :174  return                                          │
  └───────────────────────────────────────────────────────────┘

  ┌─ QUEEN COVER CHECK ──────────────────────────────────────┐
  │ game_manager.gd:177  queen_pocketed_by == current_player  │
  │                       AND NOT queen_covered?              │
  │   YES →                                                   │
  │     :178  own_piece_pocketed?                             │
  │       YES → :179  queen_covered = true   ← queen safe    │
  │       NO  →                                               │
  │         :182  foul_committed.emit("Queen not covered!")   │
  │         :183  _return_queen_to_center()                   │
  │                 :249  queen → (0, 0.02, 0)               │
  │                 :253  queen.visible = true                │
  │         :184  queen_pocketed_by = -1                      │
  │         :185  _switch_turn()                              │
  │         :186  return                                      │
  └───────────────────────────────────────────────────────────┘

  ┌─ OPPONENT PIECE RETURN ──────────────────────────────────┐
  │ game_manager.gd:189  _return_opponent_pieces()            │
  │   :231  opponent_color = WHITE if P1, BLACK if P2        │
  │   :232  for each piece in pocketed_this_turn:            │
  │   :234    if piece.color == opponent_color:               │
  │   :235      _return_piece_to_center(piece) ← back on board│
  │   :237      scores[player] -= points       ← undo score  │
  │   :238      score_updated.emit()           ← HUD updates │
  └───────────────────────────────────────────────────────────┘

  ┌─ WIN CHECK ──────────────────────────────────────────────┐
  │ game_manager.gd:192  _check_win()                        │
  │   :260  count visible BLACK pieces remaining             │
  │   :261  count visible WHITE pieces remaining             │
  │   :272  P1 wins if: black_remaining == 0                 │
  │                      AND queen_covered                    │
  │                      AND queen_pocketed_by == 1           │
  │   :276  P2 wins if: white_remaining == 0                 │
  │                      AND queen_covered                    │
  │                      AND queen_pocketed_by == 2           │
  │   WIN →                                                   │
  │     :282  game_active = false                             │
  │     :283  game_over.emit(winner)                          │
  │             └─ hud.gd:85  game_over_panel visible        │
  │                           "Player X Wins!"               │
  │                           Restart / Menu buttons active   │
  │     return                                                │
  └───────────────────────────────────────────────────────────┘

  ┌─ EXTRA TURN OR SWITCH ───────────────────────────────────┐
  │ game_manager.gd:196  own_piece_pocketed?                  │
  │   YES →                                                   │
  │     :197  _set_state(PLACE_STRIKER)  ← same player again │
  │   NO  →                                                   │
  │     :199  _switch_turn()             ← other player      │
  │             :242  current_player flips 1↔2               │
  │             :243  turn_changed.emit()                    │
  │                     └─ hud.gd:63 → _update_turn()        │
  │             :244  _set_state(PLACE_STRIKER)              │
  │                     └─ state_changed.emit()              │
  │                         ├─ camera tweens to placement    │
  │                         ├─ hud: "Place Striker (click)"  │
  │                         └─ aim indicator hides           │
  └───────────────────────────────────────────────────────────┘
```

---

## State Snapshot: Before vs After

| Property | Before Release | After Release (settled) |
|----------|---------------|------------------------|
| `current_state` | POWER | PLACE_STRIKER |
| `is_charging` | true | false |
| `power` | 0.0–5.0 | 0.0 (next turn) |
| `striker.freeze` | true | true (re-frozen for placement) |
| `striker.linear_velocity` | (0,0,0) | (0,0,0) (stopped) |
| `current_player` | N | N or flipped (depends on pocketed) |
| `pocketed_this_turn` | (previous) | [] (cleared on fire) |
| `striker_pocketed` | (previous) | false |
| `own_piece_pocketed` | (previous) | false (or true if pocketed) |
| Power bar | visible, filling | hidden |
| Aim indicator | visible | hidden |
| Camera | AIM/POWER preset | PLACE_STRIKER preset |
| Audio | power_bar.wav looping | silent (or collision sounds) |

---

## Data Flow Diagram

```
                    Mouse UP
                       │
                       ▼
               ┌──────────────┐
               │  striker.gd  │
               │ _handle_power│
               └──────┬───────┘
                      │ GameManager.release_power()
                      ▼
          ┌───────────────────────┐
          │    game_manager.gd    │
          │    release_power()    │
          │  ┌─────────────────┐  │
          │  │ is_charging=false│  │
          │  └─────────────────┘  │
          │           │           │
          │           ▼           │
          │  ┌─────────────────┐  │         ┌─────────────────┐
          │  │AudioManager.stop│──┼────────►│ audio_manager.gd│
          │  │ _power_bar()   │  │         │ _power_bar_player│
          │  └─────────────────┘  │         │    .stop()      │
          │           │           │         └─────────────────┘
          │           ▼           │
          │  ┌─────────────────┐  │
          │  │ _shoot_striker()│  │
          │  │                 │  │
          │  │ freeze = false  │  │
          │  │ compute impulse │  │
          │  │ apply_impulse() │──┼────────► Godot Physics Engine
          │  │                 │  │              │
          │  │ clear flags     │  │              ▼
          │  └─────────────────┘  │         Striker moves
          │           │           │         Collisions happen
          │           ▼           │              │
          │  ┌─────────────────┐  │              │
          │  │_set_state(SIM)  │  │              ▼
          │  │                 │  │    ┌──────────────────┐
          │  │state_changed    │──┼──► │ board.gd         │
          │  │  .emit()        │  │    │ pocket detection  │
          │  └─────────────────┘  │    │ collision sounds  │
          │           │           │    └────────┬─────────┘
          └───────────┼───────────┘             │
                      │                         │ on_piece_pocketed()
          ┌───────────▼───────────┐             │
          │ camera_controller.gd  │    ┌────────▼─────────┐
          │ tween to SIM preset   │    │ game_manager.gd  │
          └───────────────────────┘    │ scoring, flags   │
                                       └────────┬─────────┘
          ┌───────────────────────┐             │ score_updated.emit()
          │      hud.gd           │◄────────────┘ piece_pocketed.emit()
          │ "Simulating..."       │
          │ power_bar hidden      │
          │ scores update         │
          │ piece counts update   │
          └───────────────────────┘
                      │
                      │  ... frames pass, pieces decelerate ...
                      │
                      ▼
          ┌───────────────────────┐
          │ _check_simulation_    │
          │   complete()          │
          │ all velocities < 0.005│
          └───────────┬───────────┘
                      │
                      ▼
          ┌───────────────────────┐
          │   _resolve_turn()     │
          │                       │
          │ foul? → penalty       │
          │ queen? → cover check  │
          │ opponent? → return    │
          │ win? → game over      │
          │ own piece? → extra    │
          │ else → switch turn    │
          └───────────┬───────────┘
                      │
                      ▼
              PLACE_STRIKER
             (next turn begins)
```
