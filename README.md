# WireCar 1.1 (Tel Araba)

An animated **wireframe car** built with OpenGL and GLUT. The body, wheels, axles, steering and driveshaft use the required wire geometry; solid shading, lighting and a second preview window are added as extras. Version 1.1 adds body colours, paint finishes and headlights.

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
└── figures/           # Report figures (fig01–fig16)
```

## Features

**Required**

- `glutWireCube` — body  
- `glutWireTorus` — wheels and steering wheel  
- Wire lines — axles, driveshaft, steering column and rack  

**Extras**

- `glutSolidCube` / `glutSolidTorus` hybrid coating  
- Key and fill lights (`GL_LIGHT0`, `GL_LIGHT1`) and material properties  
- Two headlight spotlights (`GL_LIGHT2`, `GL_LIGHT3`)  
- Semi-transparent windows  
- Second GLUT sub-window (independent camera)  
- Wheel spin, scrolling road lines, steering animation (~60 FPS)

## What's new in 1.1

- 6 body colours (olive green, racing red, night blue, pearl white, carbon black, copper orange)
- Gloss, matte and metallic paint finishes
- One paint material shared by the body, roof, hood and bumpers
- Studio lighting: warm key light and cool fill light
- Two OpenGL spotlight headlights that can be switched on and off
- Right-click menu with submenus for camera, colour and finish
- Help panel shows the selected colour, finish and headlight state

The screenshots in `figures/` were taken with version 1.0.

## License

Computer graphics coursework project.
