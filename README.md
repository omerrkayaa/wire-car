# 4x4 WireCar (Tel Araba 2.0)

A drivable **wireframe car** built with OpenGL and GLUT. The body, wheels, axles, steering and driveshaft use the required wire geometry; solid shading, paint colours, lighting and headlights are added as extras. You can drive the car around a grid ground plane and switch between four-wheel and rear-wheel drive.

Project report: [RAPOR.md](RAPOR.md). Screenshots: [figures/](figures/) (`fig01`–`fig10`).

## Versions

| Tag | Description |
|-----|-------------|
| [`v1.0`](../../tree/v1.0) | Animated wire car with a second preview window |
| [`v1.1`](../../tree/v1.1) | Body colours, paint finishes, studio lighting, headlights |
| [`v2.0-4x4`](../../tree/v2.0-4x4) | Driving model, Ackermann steering, switchable 4×4 (this version) |

## Requirements

| Platform | Packages |
|----------|----------|
| **macOS** | Xcode Command Line Tools (OpenGL + GLUT included) |
| **Linux** | `g++`, `libgl1`, `libglu1`, `freeglut` or `glut` |
| **Windows** | MinGW-w64 + FreeGLUT, or Visual Studio (`wire_car.sln`, uses the `nupengl` NuGet package) |

## Build and run

```bash
# Linux (default)
make
./wire_car

# macOS
make PLATFORM=mac
./wire_car

# Windows (MinGW)
make PLATFORM=win
wire_car.exe
```

Clean: `make clean`

## Controls

| Key | Action |
|-----|--------|
| **W** / **↑** | Accelerate / brake when reversing |
| **S** / **↓** | Brake / reverse |
| **A** / **D** or **←** / **→** | Steer left / right |
| **1** – **7** | Camera preset (7 = follow camera, default) |
| **X** | 4×4 on / off |
| **L** or **9** | Toggle solid coating + lighting ↔ wire only |
| **C** | Next body colour |
| **V** | Next paint finish (gloss, matte, metallic) |
| **F** | Headlights on / off |
| **H** | Show / hide control panel |
| **Right click** | Menu: cameras, body colour, finish, headlights, mode |
| **ESC** | Quit |

## Project layout

```
wire-car/
├── wire_car/
│   ├── main.cpp       # Source (single file)
│   └── wire_car.vcxproj
├── wire_car.sln       # Visual Studio solution
├── Makefile           # Linux / macOS / Windows (MinGW) build
├── RAPOR.md           # Project report
├── README.md          # This file
└── figures/           # Screenshots of version 2.0 (fig01–fig10)
```

## What's new in 2.0

- The car actually moves: time-based acceleration, braking, reversing and rolling resistance
- Steering limit drops from 30° to 12° as speed increases
- Ackermann steering: inner and outer front wheels turn by different angles
- Turning follows a kinematic bicycle model
- 4×4 mode (**X**): front driveshaft and gears turn only in 4×4; drivetrain colours change
- Large grid ground plane instead of the lane road
- Cameras follow the car; smooth follow camera is the default
- Single window (the preview sub-window from 1.x was removed)

## License

Computer graphics coursework project.
