# Carrom Board 3D

A 3D carrom board game built with Godot 4.6. Two-player local multiplayer with procedurally generated board, physics-based gameplay, and full carrom rules including queen mechanics.

No textures or external assets — the entire board, pieces, and UI are generated procedurally.

## Requirements

- [Godot 4.6](https://godotengine.org/download) (Forward Plus renderer)
- Desktop OS (Windows / macOS / Linux)

## How to Run

**From Godot Editor:**
1. Open Godot 4.6
2. Import `godot/project.godot`
3. Press F5

**From Command Line:**
```bash
# macOS
/Applications/Godot.app/Contents/MacOS/Godot --path godot/

# Linux
godot --path godot/

# Windows
godot.exe --path godot/
```

## Controls

| State | Action | Input |
|-------|--------|-------|
| Place Striker | Move striker left/right | Mouse X |
| Place Striker | Confirm position | Left click |
| Aim | Adjust angle | Mouse X |
| Aim | Confirm angle | Left click |
| Power | Charge power | Hold (auto-charges) |
| Power | Fire striker | Release click |
| Any | Reset board | R key |

## Game Rules

- **Player 1** plays Black pieces, **Player 2** plays White
- Pocket your own color for an **extra turn** (10 pts black, 20 pts white)
- Pocket opponent's color — pieces return, points deducted
- **Queen** (red, 50 pts) must be "covered" by pocketing your own piece in the same turn
- **Striker pocketed** = foul: one pocketed piece returns, lose turn
- **Win** by pocketing all 9 of your pieces and covering the queen

## Project Structure

```
godot/
├── project.godot
├── autoload/
│   ├── game_manager.gd          # State machine, rules, scoring
│   └── audio_manager.gd         # Sound pool, collision audio
├── scenes/
│   ├── game/
│   │   ├── main.tscn            # Game scene
│   │   ├── board.gd             # Procedural board, pieces, pockets
│   │   ├── striker.gd           # Mouse input, aim indicator
│   │   └── camera_controller.gd # Camera presets + tweens
│   └── ui/
│       ├── main_menu.tscn/.gd   # Play / Settings / Quit
│       ├── hud.tscn/.gd         # Scores, power bar, game over
│       └── settings.tscn/.gd    # Audio toggle
└── assets/audio/                # Collision and pocket sounds
```

## Architecture

- **GameManager** (autoload singleton) — central state machine: `PLACE → AIM → POWER → SIMULATION`
- **Signal-based communication** — all scripts react to GameManager signals
- **Procedural generation** — board, walls, pockets, pieces, and markings created at runtime
- **Physics** — real carrom parameters (1:1 cm scale, mass ratios, damping, bounce)

## Documentation

- [`docs/GAME_DOCUMENTATION.md`](docs/GAME_DOCUMENTATION.md) — complete reference (architecture, testing plan, physics config, signal flow)
- [`docs/engine.md`](docs/engine.md) — engine trace from striker release through simulation stop

## License

MIT
