# WireCar (Tel Araba)

An animated **wireframe car** built with OpenGL and GLUT. The body, wheels, axles, steering and driveshaft use the required wire geometry; solid shading, lighting and a second preview window are added as extras.

Project report: [RAPOR.md](RAPOR.md). Screenshots: [figures/](figures/) (`fig01`–`fig16`).

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
| **A** / **S** or **←** / **→** | Steer front wheels left / right |
| **1** – **7** | Main window camera preset |
| **Q**, **W**, **E**, **R**, **T** | Sub-window (top-right) camera preset |
| **L** or **9** | Toggle solid coating + lighting ↔ wire only |
| **H** | Show / hide control panel |
| **Right click** | Camera menu |
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
└── figures/           # Report figures (fig01–fig16)
```

## Features

**Required**

- `glutWireCube` — body  
- `glutWireTorus` — wheels and steering wheel  
- Wire lines — axles, driveshaft, steering column and rack  

**Extras**

- `glutSolidCube` / `glutSolidTorus` hybrid coating  
- `GL_LIGHT0` and material properties  
- Semi-transparent windows  
- Second GLUT sub-window (independent camera)  
- Wheel spin, scrolling road lines, steering animation (~60 FPS)

## Body color

Edit `BODY_COLOR_R`, `BODY_COLOR_G` and `BODY_COLOR_B` (0–255) at the top of `wire_car/main.cpp` (default: 133, 149, 2).

## License

Computer graphics coursework project.
