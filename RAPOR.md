# WireCar: An Animated Wireframe Automobile in OpenGL and GLUT

**Subtitle:** Computer Graphics Project Report — Mandatory Primitives, Hybrid Shading, and Dual-Viewport Interaction

**Author:** Ömer Kaya  
**Date:** June 2026 (version 1.0)  
**Repository:** WireCar (`wire_car/main.cpp`, `Makefile`, `figures/`)

---

## Abstract

This report documents *Tel Araba* (WireCar), an interactive three-dimensional car model written in C-style C++ with the OpenGL fixed-function pipeline and GLUT. The mandatory geometry is built from `glutWireCube` for the chassis, `glutWireTorus` for the wheels and the steering wheel, and line segments for the axles, driveshafts and steering linkage. On top of the wire model the program adds solid surfaces with material properties, semi-transparent cabin glass, a second preview sub-window with five camera presets, seven main-window cameras, keyboard and menu control, and continuous animation of wheel rotation, road markings and the steerable front wheels. The car sits near the world origin with **Z** as forward/backward and **Y** as vertical, which keeps camera placement and the drivetrain layout simple. Sixteen screenshots show both rendering modes from several angles.

---

## 1. Introduction

Wireframe modelling is a common first project in computer graphics courses because it shows scene structure, transformation hierarchies and projection without needing textures or complex shading. The assignment was to build a recognisable object from GLUT primitives, animate it, and optionally add lighting and extra views.

WireCar is a car that should read as a car while still showing its mechanical parts (axles, centre differential, steering rack). The goals were:

1. **Compliance** — use the required GLUT wire primitives for the body, wheels and steering wheel; draw axles and driveshafts as lines.
2. **Interaction** — steer the front wheels and the steering wheel together, switch cameras and toggle display options.
3. **Presentation** — run at about 60 frames per second and document solid and wireframe modes with screenshots.
4. **Bonus** — add solid geometry and lighting over the wire model, a second viewport and transparent windows.

All code is in one file ([`wire_car/main.cpp`](wire_car/main.cpp)), built with a small [`Makefile`](Makefile) or the Visual Studio solution [`wire_car.sln`](wire_car.sln). Physics, networking and texture mapping are out of scope.

---

## 2. Background

### 2.1 Fixed-function OpenGL and GLUT

The code uses classic OpenGL 1.x immediate mode: matrix stacks (`glPushMatrix` / `glPopMatrix`), `gluLookAt` for the cameras, `gluPerspective` for projection, and GLUT's built-in shapes (`glutWireCube`, `glutSolidCube`, `glutWireTorus` and others). GLUT also provides the event loop, timers, sub-windows, bitmap fonts and popup menus, so almost no platform-specific window code is needed.

On macOS the source includes `<GLUT/glut.h>`; on Linux and Windows it includes `<GL/glut.h>` (FreeGLUT on Windows). The choice is made with an `#ifdef __APPLE__` block at the top of [`main.cpp`](wire_car/main.cpp).

### 2.2 Wireframe versus hybrid rendering

Pure wireframe rendering shows only edges and makes it easy to check that the required `glutWire*` calls are used. In hybrid mode the solid fills are drawn first with lighting enabled, then the same wire outlines are drawn on top, so the model is still clearly built from wire primitives but gains depth from shading. The `solidLightEnabled` flag switches between the two modes.

### 2.3 Second viewport

A small preview window, as in CAD or game editors, helps keep track of the car's orientation while the main window uses perspective cameras. The sub-window has its own `subCameraMode`, separate from the main window's `cameraMode`.

---

## 3. Methodology

### 3.1 Development tools

| Component | Choice |
|-----------|--------|
| Language | C++ (written in C style, single file) |
| Compiler | `g++ -Wall -O2` (Makefile) or MSVC (Visual Studio solution) |
| Graphics | OpenGL 1.x, GLU quadrics |
| Toolkit | GLUT / FreeGLUT |
| Math | `<math.h>` (`cosf`, `sinf`, `fabsf`) |

### 3.2 Build and run

The [`Makefile`](Makefile) supports Linux (default), macOS (`PLATFORM=mac`) and Windows MinGW (`PLATFORM=win`). On macOS:

```bash
make PLATFORM=mac
./wire_car
```

macOS builds link `-framework OpenGL -framework GLUT`; Linux builds link `-lGL -lGLU -lglut -lm`.

### 3.3 Coordinate system and scale

The car is centred near **(0, 0, 0)**. **Z** points to the front of the car, **Y** is up and **X** is left–right. The main constants are:

- Front axle at `FRONT_AXLE_Z = 0.62`, rear axle at `REAR_AXLE_Z = -0.62`
- Wheel lateral offset `WHEEL_X = 0.62`
- Axle height `AXLE_Y = -0.34`

### 3.4 Rendering steps per frame

Each display callback:

1. Clears the colour and depth buffers.
2. Loads the model-view matrix and applies the active camera preset (`gluLookAt`).
3. Draws the road (always wire, lighting off).
4. Draws the car in a fixed order (body, lights, bumpers, 4×4 drivetrain, steering, wheels, and the windows last so the glass overlaps correctly).
5. In the main window only, draws the 2D help panel in screen space.

A timer runs every **16 ms** (about 60 Hz). It updates the steering angle, `wheelAngle` and `roadOffset`, then redraws **both** windows through `requestRedrawBoth()`.

### 3.5 Screenshots

The figures in Section 6 were captured while running the program. The help panel is visible in some of them.

---

## 4. System Design

### 4.1 Overall structure

```mermaid
flowchart TB
    main[main glutMainLoop]
    timer[timer 16ms]
    key[keyDown keyUp special]
    dispMain[displayMain]
    dispSub[displaySub]
    drawCar[drawCar]
    main --> timer
    main --> key
    timer --> updateSteer[updateSteering]
    timer --> wheelAnim[wheelAngle roadOffset]
    timer --> dispMain
    timer --> dispSub
    dispMain --> setCam[setCamera mainCameras]
    dispSub --> setSubCam[setSubCamera subCameras]
    dispMain --> drawRoad --> drawCar
    dispSub --> drawRoad --> drawCar
    drawCar --> body[drawBody]
    drawCar --> lights[drawLights]
    drawCar --> bumpers[drawBumpers]
    drawCar --> drivetrain[drawSelectable4x4]
    drawCar --> steer[drawSteeringSystem]
    drawCar --> wheels[drawWheel x4]
    drawCar --> windows[drawWindows]
```

### 4.2 Functions

| Function | What it does |
|----------|--------------|
| `initShared` | Depth test, `GL_LIGHT0`, `GL_NORMALIZE`, quadric object |
| `setMaterial` | Ambient, diffuse, specular and shininess for solid parts |
| `drawBody` | Scaled `glutWireCube` + cabin/hood lines; `glutSolidCube` in hybrid mode |
| `drawWheel` | `glutWireTorus`, optional `glutSolidTorus`, spoke lines, steering and spin |
| `drawSelectable4x4` | Axles, shafts, wire boxes, gear symbols |
| `drawSteeringSystem` | Steering wheel torus, column, rack to the front axle |
| `drawWindows` | Blended quads with blue wire frames |
| `drawLights` / `drawBumpers` | Headlights, taillights, bumpers (solid + wire) |
| `drawRoad` | Road edges and moving centre dashes |
| `drawHelpPanel` | On-screen help (main window only) |
| `updateSteering` | Updates `steerAngle` from the pressed keys |

### 4.3 Windows

`main()` creates the main window (950×700) and then a 240×180 sub-window, which is moved to the top-right corner when the main window is resized:

```cpp
int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(950, 700);

    mainWindow = glutCreateWindow("TelAraba [H panel | ok/A-S direksiyon | L isik]");
    initShared();
    glutDisplayFunc(displayMain);
    // ... reshape, keyboard, menu ...

    subWindow = glutCreateSubWindow(mainWindow, 680, 10, 240, 180);
    initShared();
    glutDisplayFunc(displaySub);
    // ... same keyboard handlers ...

    glutTimerFunc(16, timer, 0);
    glutMainLoop();
    return 0;
}
```

Both windows use the same keyboard callbacks, so the controls work whichever window has focus.

### 4.4 Camera presets

**Main window (keys 1–7, right-click menu):**

| Index | Key | Label |
|-------|-----|-------|
| 0 | 1 | Perspective view |
| 1 | 2 | Bird's-eye view |
| 2 | 3 | Right side view |
| 3 | 4 | Rear-left view |
| 4 | 5 | Front view |
| 5 | 6 | Rear view |
| 6 | 7 | Follow camera |

**Sub-window (keys Q–T):**

| Index | Key | Label |
|-------|-----|-------|
| 0 | Q | Full top |
| 1 | W | Front-top diagonal |
| 2 | E | Rear-top diagonal |
| 3 | R | Left-top side |
| 4 | T | Close top zoom |

---

## 5. Implementation

### 5.1 Requirements

| Requirement | Where | Main API |
|-------------|-------|----------|
| Wire body | `drawBody` | `glutWireCube` |
| Wire wheels | `drawWheel` | `glutWireTorus` |
| Wire steering wheel | `drawSteeringSystem` | `glutWireTorus` |
| Axles & driveshaft | `drawSelectable4x4` | `GL_LINES`, `drawWireBox` |
| Steering column & rack | `drawSteeringSystem` | `GL_LINES` |
| Solid coating (bonus) | same functions when `solidLightEnabled` is set | `glutSolidCube`, `glutSolidTorus`, `gluCylinder` |
| Lighting (bonus) | `initShared`, `setMaterial` | `GL_LIGHT0` |
| Second window (bonus) | `glutCreateSubWindow` | separate `displaySub` |

### 5.2 Body and colour

The chassis is a `glutWireCube` scaled by **(1.10, 0.42, 1.80)**. In hybrid mode a `glutSolidCube` with the same scale is drawn underneath.

The body colour is set with `BODY_COLOR_R/G/B` at the top of the file: **(133, 149, 2)**, an olive green, converted to 0–1 floats for OpenGL. The solid body uses specular 0.45 and shininess 28. Cabin outlines, the hood and window frames are drawn with `GL_LINE_LOOP` and `GL_LINES` in `drawBody` and `drawWindows`.

### 5.3 Wheel transformations

For each wheel the code moves to the wheel position, turns the front wheels by the steering angle, rotates the torus onto a horizontal axis, then spins it around **Z** by `wheelAngle`:

```cpp
static void drawWheel(float x, float y, float z, int frontWheel)
{
    glPushMatrix();
    glTranslatef(x, y, z);

    if (frontWheel)
        glRotatef(steerAngle, 0.0f, 1.0f, 0.0f);

    glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
    glRotatef(wheelAngle, 0.0f, 0.0f, 1.0f);
    // ...
```

Spoke lines are drawn after the torus so the rotation is visible in wire mode too.

### 5.4 Drivetrain and steering

`drawSelectable4x4` draws a simplified four-wheel-drive layout: front and rear axle lines, shafts to a centre gear, wire boxes for the differentials, and `drawWireGear` for the gear symbols. The drivetrain uses thick white lines (`glLineWidth(2.5f)`) to separate it from the body.

The steering wheel uses the same `steerAngle` as the front wheels. Its torus is tilted 65° about **X** and rotated about **Z** by `steerAngle`. Rack lines connect the column to the ends of the front axle at **±WHEEL_X**.

### 5.5 Draw order

The windows are drawn last so the transparent glass appears over the body:

```cpp
static void drawCar(void)
{
    drawBody();
    drawLights();
    drawBumpers();
    drawSelectable4x4();
    drawSteeringSystem();

    drawWheel(WHEEL_X, AXLE_Y, FRONT_AXLE_Z, 1);
    drawWheel(-WHEEL_X, AXLE_Y, FRONT_AXLE_Z, 1);
    drawWheel(WHEEL_X, AXLE_Y, REAR_AXLE_Z, 0);
    drawWheel(-WHEEL_X, AXLE_Y, REAR_AXLE_Z, 0);

    drawWindows();
}
```

### 5.6 Lighting

`initShared` turns on depth testing and `GL_NORMALIZE` (needed because the solid parts are scaled) and places a single light, `GL_LIGHT0`, at **(3, 5, -4)** with a low ambient and strong diffuse and specular values. `setMaterial` sets the ambient term to 25% of the diffuse colour. Wire parts turn lighting off with `glDisable(GL_LIGHTING)` so that `glColor3f` sets the line colour directly.

### 5.7 Animation and steering

`updateSteering` changes `steerAngle` by 2° per frame while **A** or **S** (or the arrow keys) is held, limited to **±30°**. When the key is released the angle returns to zero by 1.5° per frame. The timer adds 3° to `wheelAngle` and moves `roadOffset` forward for the centre-line dashes. The car itself stays in place; the movement comes from the road and the wheels.

### 5.8 User interface

- **Help panel** (`H`): lists the controls and shows the current mode.
- **Right-click menu**: the seven main cameras and exit.
- **Wire/solid toggle** (`L` or `9`): flips `solidLightEnabled` and redraws both windows.

---

## 6. Results and Discussion

### 6.1 Rendering modes

| Mode | Flag | Appearance |
|------|------|------------|
| Hybrid solid + lighting | `solidLightEnabled = 1` | Olive-green filled body, shaded tyres, headlight and taillight solids, wire edges on top |
| Wire only | `solidLightEnabled = 0` | Coloured wire body without fill; drivetrain and wheels stay visible |

The help panel shows the active mode in Turkish: *Kaplama + isik ACIK* or *Tel (isik kapali)*.

### 6.2 Two viewports

Every screenshot includes the sub-window. Because `subCameraMode` is independent, the inset can show the top view while the main window uses a different camera.

### 6.3 Solid + lighting (Figures 1–8)

![Figure 1: Rear-left view with control panel](./figures/fig01.jpeg)

*Figure 1: View from behind and to the side, hybrid mode, help panel visible. The sub-window shows the top view.*

![Figure 2: Rear-left perspective](./figures/fig02.jpeg)

*Figure 2: View from behind the other side, without the help panel. Road edges (white), red centre dashes and the cabin wire frame are visible.*

![Figure 3: Front view](./figures/fig03.jpeg)

*Figure 3: Front view. Round headlights and blue window outlines.*

![Figure 4: Rear view](./figures/fig04.jpeg)

*Figure 4: Rear view. Taillights and bumper.*

![Figure 5: Rear view variant](./figures/fig05.jpeg)

*Figure 5: Higher view from behind.*

![Figure 6: Side view](./figures/fig06.jpeg)

*Figure 6: Side view.*

![Figure 7: Top view](./figures/fig07.jpeg)

*Figure 7: Top view (bird's-eye).*

![Figure 8: Top view variant](./figures/fig08.jpeg)

*Figure 8: Top view, second capture.*

### 6.4 Wire only (Figures 9–16)

![Figure 9: Wire perspective front-left](./figures/fig09.jpeg)

*Figure 9: Wire-only mode (`L`/`9`), view from behind and to the side. `glutWireCube` body, white drivetrain and `glutWireTorus` wheels.*

![Figure 10: Wire side view](./figures/fig10.jpeg)

*Figure 10: Wire mode, side view. Cabin and hood lines.*

![Figure 11: Wire top view](./figures/fig11.jpeg)

*Figure 11: Wire mode, top view. Full 4×4 layout and the steering wheel torus.*

![Figure 12: Wire top view - steering left](./figures/fig12.jpeg)

*Figure 12: Wire mode, top view while steering. The front wheels are turned.*

![Figure 13: Wire rear view](./figures/fig13.jpeg)

*Figure 13: Wire mode, view along the length of the car.*

![Figure 14: Wire rear view 2](./figures/fig14.jpeg)

*Figure 14: Wire mode, view along the length of the car, second capture.*

![Figure 15: Wire rear view 3](./figures/fig15.jpeg)

*Figure 15: Wire mode, view along the length of the car. Axle and differential boxes are visible.*

![Figure 16: Wire rear view 4](./figures/fig16.jpeg)

*Figure 16: Wire mode, higher view from behind.*

### 6.5 Discussion

The required wire primitives stay visible when the solid layer is on, and wire-only mode keeps the animation and steering, so the bonus features do not replace the required geometry.

Limitations: the road is flat, there is no collision or physics, the car does not actually move (only the road and wheels are animated), and the bitmap help text does not scale on high-DPI screens.

---

## 7. Conclusion

WireCar implements an animated, steerable car with OpenGL and GLUT in a single source file. All required wire parts (body cube, torus wheels, torus steering wheel, axle and driveshaft lines) are present and can be checked in the wire-only screenshots. The bonus parts are the solid layer with lighting, transparent windows, two synchronised viewports, seven main and five sub-window cameras, and the on-screen help.

Possible next steps: selectable body colours, a textured road, letting the car actually drive along the road, suspension movement, and moving to modern shader-based OpenGL while keeping the wireframe requirement.

---

## References

1. Shreiner, D., et al. *OpenGL Programming Guide: The Official Guide to Learning OpenGL* (OpenGL Red Book). Addison-Wesley.
2. Kilgard, M. J. *The OpenGL Utility Toolkit (GLUT) Programming Interface*. Silicon Graphics, Inc.
3. Khronos Group. OpenGL 1.x Specification and Reference Pages. https://www.khronos.org/opengl/
4. FreeGLUT Project Documentation. http://freeglut.sourceforge.net/

---

## Appendix A: Controls

| Input | Action |
|-------|--------|
| **A** / **Left arrow** | Steer front wheels left (max 30°) |
| **S** / **Right arrow** | Steer front wheels right (max 30°) |
| **1** – **7** | Main window camera |
| **Q**, **W**, **E**, **R**, **T** | Sub-window camera |
| **L** or **9** | Solid + lighting / wire only |
| **H** | Show / hide help panel |
| **Right click** | Camera menu (main cameras + exit) |
| **ESC** | Exit |

---

## Appendix B: Requirement Checklist

| Item | Status | Evidence |
|------|--------|----------|
| `glutWireCube` body | Done | `drawBody`, Figures 9–16 |
| `glutWireTorus` wheels | Done | `drawWheel`, all figures |
| `glutWireTorus` steering wheel | Done | `drawSteeringSystem`, Figures 11–12 |
| Axle & driveshaft lines | Done | `drawSelectable4x4`, Figures 9–16 |
| Steering column & rack | Done | `drawSteeringSystem`, Figure 12 |
| Solid + lighting bonus | Done | Figures 1–8, `initShared` |
| Second viewport bonus | Done | Inset in all figures, `subWindow` |
| Animation | Done | Timer, road offset, wheel spin |
| Multiple cameras | Done | Keys 1–7, Q–T, menu |
