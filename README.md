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
foundation checks. The assigned drawing, filling, clipping and curve modules are
not implemented yet. See [workfile.md](workfile.md) for the assignments and workflow.

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

These commands cover the current foundation only. Extend the build configuration
and documented commands when the assigned algorithm modules are added.

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
| Bresenham line drawing and line clipping | Assigned; not implemented |
| Scanline polygon filling | Assigned; not implemented |
| Polygon clipping | Assigned; not implemented |
| Midpoint circle and ellipse drawing | Assigned; not implemented |
| Cubic Bézier evaluation and rendering | Assigned; not implemented |
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
- The initial foundation builds without warnings and its checks pass using Apple
  Clang on macOS with the SDK flag described above. CMake is not installed here;
  CMake and other-platform builds need verification on machines with those tools.
