# Audio System

## Tech: OpenAL + ALUT
- 5 buffers, 5 sources
- Listener at origin, facing -Z

## Sound Events

| # | File | Trigger | Gain | Loop |
|---|------|---------|------|------|
| 0 | carrom_carrommen_cd.wav | Piece-piece collision | 0.3 (dynamic) | No |
| 1 | carrom_striker_wall.wav | Striker-wall collision | 0.7 (dynamic) | No |
| 2 | carrom_carrommen_wall.wav | Piece-wall collision | 0.4 | No |
| 3 | carrom_pot_sound.wav | Piece enters pocket | 0.7 | No |
| 4 | carrom_power_bar.wav | Power bar active | 0.7 | Yes |

## Dynamic Volume
Collision sounds scale inversely with velocity difference:
```
gain = 0.5 / velocity_difference.magnitude()
```
Faster collisions → louder sounds.
