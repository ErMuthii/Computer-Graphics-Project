# Desktop starter

This is Eric's application and integration scaffold. It keeps the CP1 algorithms
in Pure C++ and presents their RGB output using a textured OpenGL quad.
The fixed motifs and static mannequin are starter content, not completed CP2
transformation/animation modules or a claim of other members' contributions.

## Build and launch on macOS

Install the command line tools (`xcode-select --install`) if needed, then:

```sh
brew install cmake glfw
cmake -S . -B build-desktop -DKITENGE_BUILD_DESKTOP=ON -DCMAKE_BUILD_TYPE=Debug -DCMAKE_OSX_SYSROOT=/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk
cmake --build build-desktop --parallel
ctest --test-dir build-desktop --output-on-failure
./build-desktop/kitenge_studio
```

The SDK flag addresses the previously documented toolchain mismatch on Eric's
machine. Omit it on machines with a consistent default SDK.

## Windows and Linux

On Windows, use one consistent MSYS2 UCRT64 environment and install
`mingw-w64-ucrt-x86_64-toolchain`, `mingw-w64-ucrt-x86_64-cmake`,
`mingw-w64-ucrt-x86_64-ninja`, `mingw-w64-ucrt-x86_64-glfw` and
`mingw-w64-ucrt-x86_64-glew`. Configure with `-G Ninja` in the UCRT64 terminal.
On Ubuntu, install `build-essential cmake libglfw3-dev libglew-dev libgl1-mesa-dev`.
Both use:

```sh
cmake -S . -B build-desktop -DKITENGE_BUILD_DESKTOP=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build-desktop --parallel
ctest --test-dir build-desktop --output-on-failure
```

Launch `build-desktop/kitenge_studio` (or `.exe` on Windows). Those platform
instructions are provided for teammates; validation on Eric's Mac does not imply
that Windows/Linux builds have been tested.

OpenGL 3.3 core is required. macOS requests a forward-compatible context, following
[GLFW's context guidance](https://www.glfw.org/docs/3.3.3/context_guide.html).
GLEW loads modern GL entry points on Windows/Linux; macOS uses its framework.
Dependencies are discovered locally. CMake does not download packages.

## Controls and output

| Key | Action |
| --- | --- |
| M | Cycle diamond, triangle and cross placeholders |
| P | Cycle three palettes, updating fabric and dress together |
| R | Reset selections and the animation clock |
| S | Save the complete C++ canvas to `outputs/studio.ppm` |
| Esc | Close the window |

Resize preserves the canvas aspect ratio with letterboxing. Retina framebuffer
sizes are handled separately from the window size. This scaffold uses a fixed
1200 x 760 logical canvas. Controls are keyboard based, not clickable buttons.
Space/playback is intentionally unbound until the team supplies an animation.

Run from the repository root for the default export location. Change it with
`--export /absolute/path/image.ppm`. PPM is the existing project's output format.

To prove the actual OpenGL display path:

```sh
./build-desktop/kitenge_studio --smoke-test --capture outputs/desktop-frame.ppm
```

This opens a real window, compiles/links shaders, uploads the canvas, exercises
selection/reset callbacks, resizes, reads back the back buffer and exits after
six frames. It needs a graphical login session. Callback checks use scripted
events; they do not simulate physical keyboard input.
Without `--smoke-test`, `--capture` saves once after the third frame and stays open.
Failures in shader compilation, linking, window creation and GL operations return
nonzero and report the cause in the terminal. A failed user export reports an
error and leaves the application running.

## Pattern scene options and desktop/headless comparison

The fabric and dress interiors come from the pattern engine
(`docs/algorithms/pattern-generation.md`). `kitenge_studio` and `studio_render`
accept the same scene options, so both can be run at identical state and
settings:

```sh
./build-desktop/kitenge_studio --motif 2 --palette 1 --angle 20 --scale 0.9
./build-core/studio_render outputs/studio.ppm --motif 2 --palette 1 --angle 20 --scale 0.9
```

Options: `--motif N`, `--palette N`, `--angle DEGREES`, `--scale S`,
`--pitch X Y`, `--translate X Y`, `--reflect-x`, `--reflect-y`. They are for
verification and demonstrations, not a replacement for the planned controls.
`--exit-after-capture` closes the window after `--capture` has saved its frame.

`ppm_compare expected.ppm actual.ppm [difference.ppm]` reports whether two
images are pixel-identical. In a graphical session, configure the desktop build
with `-DKITENGE_GUI_TESTS=ON` to add CTest checks that open real windows:
`desktop_smoke`, and three `desktop_matches_headless_*` scenes that capture
the OpenGL back buffer and compare it with `studio_render`.

## Pure C++ path

The default build still needs no OpenGL or GLFW:

```sh
cmake -S . -B build-core -DKITENGE_BUILD_DESKTOP=OFF
cmake --build build-core --parallel
ctest --test-dir build-core --output-on-failure
./build-core/studio_render outputs/studio.ppm
```

All old CP1 executable names and test targets remain available. `kitenge_cp1`
continues to produce its original foundation diagnostic.

## Team integration contract

`studio.hpp` owns `StudioState`, `StudioAction`, `StudioHooks`, and the logical
canvas size. `studio.cpp` owns the current scaffold composition. Neither includes
an OpenGL or GLFW header. `desktop_main.cpp` owns windowing, input, texture upload
and presentation. `studio_render` proves that the scene also runs without a GUI.

The data flow is input -> state -> Pure C++ canvas -> RGB texture -> OpenGL window.
The OpenGL shaders display the canvas; they do not replace the CP1 algorithms.

| Owner | Integration boundary |
| --- | --- |
| Jeremy | Add a separate Pure C++ matrix module, then extend scene/state with transformation parameters |
| Deborah | Replace fixed `motif()` and palette definitions with reusable motif data; refine mannequin geometry |
| Kelvin | Replace `StudioHooks::fabric` and `garment` with the reusable pattern/garment renderer |
| Lenina | Supply `StudioHooks::animate(state, deltaSeconds)` and extend state with animated parameters |
| Andrew | Extend actions and input mapping for the real selection/parameter controls |
| Eric | Maintain shell, OpenGL presenter, build, integration and shared contracts |

Hooks default to a fixed starter pattern. Assign them on `App::hooks` when modules
are ready. `fabric` owns the interior fabric view (x=52..696, y=202..648).
`garment` owns the dress interior. The starter bodice is
`(897,278),(1003,278),(985,377),(915,377)` and skirt is
`(915,377),(985,377),(1041,574),(859,574)`. Both are convex.
The shell draws panel outlines and belt after the garment hook.

`advanceStudio` passes time only when playing. The desktop caps elapsed time at
0.1 seconds after a stall; offline CP2 frame generation can provide exact steps.
The starter does not yet rotate/scale motifs or animate the dress. Implement those
as team-owned modules, keeping the maths independent of OpenGL.

Coordinate convention: top-left origin, x right, y down. Keep clipping and geometry
in doubles; rasterize only at the Canvas boundary. Preserve existing public
interfaces unless the group agrees on a change. Keep future tests headless where
possible and verify the desktop separately in a real graphical session.
