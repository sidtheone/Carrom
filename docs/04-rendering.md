# Rendering & Visuals

## Pipeline
```
Clear buffers → Set projection → Set camera → Draw board → Draw pieces → Draw UI → Swap buffers
```

## Geometry (All Procedural via GLU)
- Board: solid cubes
- Carrom men: cylinder + disk caps
- Striker: cylinder + disk
- Direction indicator: cone
- Pockets: black disks
- Center circle: ring
- Power bar: solid cube (dynamic height)

## Materials & Colors

| Object | Color (RGB) | Shininess |
|--------|------------|-----------|
| Board surface | (1.0, 0.9, 0.0) yellow | High |
| Boundaries | (1.0, 0.9, 0.0) yellow | Low |
| Pockets/rings | (0, 0, 0) black | None |
| Striker | (0, 0, 0.6) dark blue | - |
| Black carrom | (0.01, 0.01, 0.01) | - |
| Red carrom | (1.0, 0, 0) | - |
| White carrom | (0.7, 0.7, 0.7) | - |

## Lighting
- Single light at (5, 5, 10)
- GL_SMOOTH shading
- Ambient material: (0.5, 0.5, 0.5)
- Diffuse + Specular enabled

## Camera System
| State | Projection | Camera Position |
|-------|-----------|-----------------|
| Place striker | Orthographic | Top-down |
| Direction | Perspective 65° | (0, -2, 6) |
| Power | Perspective 65° | (0, -1, 10) |
| Simulation | Perspective 65° | (0, -2, 8) |

## Scale
- Board: 0.01× in display
- Pieces: radius/100, height/100
- Positions: /100
