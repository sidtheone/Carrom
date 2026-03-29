# Input Handling

## Mouse

| State | Button | Action |
|-------|--------|--------|
| Place Striker | Left click | Position striker in zone (x: -123 to +123) |
| Place Striker | Right click | Confirm → go to Direction state |
| Direction | Left hold | Go to Power state, start bar sound |
| Power | Left release | Fire striker with current power |

## Mouse Motion
- **Direction state:** Passive mouse X → angle (-75° to +75°)
- **Formula:** `angle = -(mouseX - screenW/2) / 300.0 * 75`

## Keyboard
- **'r':** Reset board (only in Place Striker state)
- **ESC:** Exit game

## Coordinate Conversion
```
screen → world:
  x = (screenX - w/2) * 370/186.0
  y = fixed at -145 * 370/186.0
```
