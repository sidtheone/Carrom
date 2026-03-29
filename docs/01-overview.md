# Carrom Board 3D - Overview

## Tech Stack
- **Language:** C++ (Visual Studio 2008)
- **Graphics:** OpenGL (GLUT)
- **Audio:** OpenAL + ALUT
- **Platform:** Windows (Win32)

## Status
- Core physics & rendering: Working
- Game logic (scoring, turns, win): NOT implemented
- Multiplayer: NOT implemented
- AI opponent: NOT implemented

## File Structure
```
├── main.cpp           → Entry point, GLUT init
├── display.cpp        → Rendering & viewport
├── model.cpp          → Board & object init
├── interface.cpp      → Mouse/keyboard input
├── variables.cpp      → Global variable defs
├── engine.h           → Physics & collisions
├── object.h           → Game object classes
├── sound.h            → OpenAL audio system
├── vector3D.h         → 3D vector math
├── *.wav (5 files)    → Audio assets (~607KB)
└── ReadME version 0.2.pdf → User manual
```
