# Kitenge Pattern Designer and Virtual Dress Preview

A computer graphics semester project for designing geometric Kitenge-inspired
fabric patterns and previewing them on a simplified dress. The planned final
application includes selectable motif shapes and color combinations, a color
grading index for comparing palettes, and a dress preview. Live camera alignment
is an advanced goal developed after the graphics foundation works.

## Current checkpoint

CP1 implements project-related graphics algorithms in pure C++ before the later
transformation, animation and OpenGL checkpoints. The shared foundation is ready:
a pixel canvas, common geometry types, PPM image export, a diagnostic image and
foundation checks. Line drawing and clipping, polygon filling, polygon
clipping, circle and ellipse drawing, and Bézier evaluation and rendering
are implemented.
See [workfile.md](workfile.md) for the assignments and workflow.
The source commits for each feature remain in the merge history. Eric waived
review for the foundation and the deadline integration of PRs #5 and #6.

## Group members and responsibilities

| Student number | Member | Initial CP1 responsibility |
| --- | --- | --- |
| 166263 | Waruhiu Jeremy Kang'ethe | Lines and line clipping |
| 166390 | Muthii Eric Macharia | Shared foundation, build setup and integration |
| 168656 | Deborah Rehana | Polygon representation and scanline filling |
| 166914 | Murega Kelvin Mutwiri | Polygon clipping |
| 161492 | Sayialel Eric Lenina | Circle and ellipse drawing |
| 167144 | Andrew Karanja Gathirwa | Bézier curves |

## Technologies and tools

- C++17 and its standard library; no external graphics or vision dependency in CP1.
- CMake 3.20 or newer for the portable build configuration, or direct compiler commands.
- Git and GitHub for individual contributions, issues, branches and pull requests.
- PPM images for pure C++ raster output; an image viewer displays the generated files.

## Compile and run with CMake

Run these commands from the repository root with a C++17 compiler installed:

```sh
cmake -S . -B build
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

On macOS or Linux with a single-configuration generator:

```sh
./build/kitenge_cp1 outputs/foundation.ppm
```

With a Visual Studio multi-configuration build on Windows PowerShell:

```powershell
.\build\Debug\kitenge_cp1.exe outputs/foundation.ppm
```

If a different CMake generator changes the executable location, use its build
output directory. Passing an image path is optional; the default is
`outputs/foundation.ppm`. Run from the repository root for that default location.

## Compile directly on macOS or Linux

This route works without CMake using Clang or GCC with C++17 support:

```sh
mkdir -p build
c++ -std=c++17 -Wall -Wextra -Wpedantic -Iinclude src/canvas.cpp src/ppm.cpp src/main.cpp -o build/kitenge_cp1
c++ -std=c++17 -Wall -Wextra -Wpedantic -Iinclude src/canvas.cpp src/ppm.cpp tests/foundation_tests.cpp -o build/foundation_tests
./build/foundation_tests build/test-output/foundation.ppm
./build/kitenge_cp1 outputs/foundation.ppm
```

Polygon clipping checks and its before-and-after demonstration:

```sh
c++ -std=c++17 -Wall -Wextra -Wpedantic -Iinclude src/polygon_clip.cpp tests/polygon_clip_tests.cpp -o build/polygon_clip_tests
c++ -std=c++17 -Wall -Wextra -Wpedantic -Iinclude src/canvas.cpp src/ppm.cpp src/polygon_clip.cpp src/polygon_clip_demo.cpp -o build/polygon_clip_demo
./build/polygon_clip_tests
./build/polygon_clip_demo outputs/polygon-clipping.ppm
```

Line drawing and clipping checks and their before-and-after demonstration:

```sh
c++ -std=c++17 -Wall -Wextra -Wpedantic -Iinclude src/canvas.cpp src/lines.cpp tests/line_tests.cpp -o build/line_tests
c++ -std=c++17 -Wall -Wextra -Wpedantic -Iinclude src/canvas.cpp src/ppm.cpp src/lines.cpp src/lines_demo.cpp -o build/lines_demo
./build/line_tests
./build/lines_demo outputs/lines-clipping.ppm
```

Circle and ellipse checks and decorative swatch demonstration:

```sh
c++ -std=c++17 -Wall -Wextra -Wpedantic -Iinclude src/canvas.cpp src/circles.cpp tests/circle_tests.cpp -o build/circle_tests
c++ -std=c++17 -Wall -Wextra -Wpedantic -Iinclude src/canvas.cpp src/ppm.cpp src/circles.cpp src/circles_demo.cpp -o build/circles_demo
./build/circle_tests
./build/circles_demo outputs/circles-ellipses.ppm
```

Bézier evaluation, raster rendering and fabric-motif demonstration:

```sh
c++ -std=c++17 -Wall -Wextra -Wpedantic -Iinclude src/canvas.cpp src/lines.cpp src/curves.cpp tests/curve_tests.cpp -o build/curve_tests
c++ -std=c++17 -Wall -Wextra -Wpedantic -Iinclude src/canvas.cpp src/ppm.cpp src/lines.cpp src/curves.cpp src/bezier_demo.cpp -o build/bezier_demo
./build/curve_tests
./build/bezier_demo outputs/bezier-curves.ppm
```

The same commands work on Windows with MSYS2 UCRT64 `g++` from Git Bash.
The CMake build includes all implemented modules and their checks.

On the current macOS machine, the compiler defaults to an SDK that its linker
cannot read. Both direct builds were verified by adding this flag to the compiler
commands above:

```text
-isysroot /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk
```

That existing SDK points to macOS 26.5. This is a local toolchain workaround;
teammates with a consistent installation should use the normal commands. A CMake
build on this machine may need the equivalent configuration option
`-DCMAKE_OSX_SYSROOT=/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk`.

## Graphics implementation progress

| Component | Status |
| --- | --- |
| Common points, RGB colors, polygons and rectangle types | Implemented |
| Pixel buffer and bounds-safe pixel writes | Implemented |
| Binary PPM image export | Implemented |
| Bresenham line drawing and Cohen-Sutherland line clipping | Implemented on `feat/cp1-lines`; see [lines.md](docs/algorithms/lines.md) |
| Scanline polygon filling | Implemented; see [polygon_fill.md](docs/algorithms/polygon_fill.md) |
| Sutherland-Hodgman polygon clipping against convex boundaries | Implemented on `feat/cp1-polygon-clipping`; see [polygon-clipping.md](docs/algorithms/polygon-clipping.md) |
| Midpoint circle and ellipse drawing | Implemented on `feat/cp1-circles-ellipses`; see [circles-ellipses.md](docs/algorithms/circles-ellipses.md) |
| Cubic Bézier evaluation and raster rendering | Implemented; see [bezier.md](docs/algorithms/bezier.md) and [evidence](docs/evidence/bezier-curves.png) |
| Integrated fabric swatch and simplified dress | Planned after module integration |

The foundation diagnostic is a grid of RGB color blocks written pixel by pixel.
It verifies the raster output path and does not claim to demonstrate the assigned
class algorithms.

## Repository structure

```text
.github/                 Pull request and checkpoint issue templates
include/kitenge/         Shared C++ types and public interfaces
src/                    Implementations and current diagnostic entry point
tests/                  Foundation checks and future algorithm checks
docs/evidence/          Selected screenshots and evidence descriptions
outputs/                Regenerable image output, ignored except its README
CMakeLists.txt          Build configuration
workfile.md             Temporary CP1 collaboration plan
```

## Known limitations

- Current output is static; there is no live window, dress scene or camera input yet.
- Pattern generation, palette comparison, transformations and OpenGL integration
  are not part of the implemented foundation.
- PPM display support depends on the viewer. A presentation copy may be converted
  to PNG without changing the pure C++ rendering implementation.
- CP1 uses a simplified 2D dress; future previews show style and pattern placement,
  not verified garment measurements or physically simulated cloth.
- The complete CP1 module build passes all six CTest checks using Apple Clang
  and the documented macOS SDK selection. Earlier Windows 11 verification used
  g++ 15.2 (MSYS2 UCRT64) and CMake 4.4.4 on a polygon clipping branch;
  the current integrated build still needs Windows and Linux verification.
- CMake verification used an isolated tool installation inside ignored `build/`
  rather than a system installation. Teammates need CMake on their own PATH to
  use the standard CMake commands, or can use the direct compiler route.
