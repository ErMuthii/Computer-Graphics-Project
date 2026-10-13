# Pure C++ rendering foundation

Owner: Muthii Eric Macharia, 166390.

The foundation gives each graphics algorithm the same pixel surface and geometry
types. It uses C++17 and the standard library. CMake configures compilation and
checks; it does not draw the images or replace a graphics algorithm.

## Coordinates and pixel storage

The canvas origin is the top-left corner. A pixel at column `x` and row `y` has
the index `y * width + x` in the one-dimensional pixel array. Each row contains
`width` RGB colors, and rows are stored top to bottom. Integer `Point` values
identify pixel coordinates. `Point2D` uses doubles for curve evaluation and
clipping intersections before rounding for rasterization.

`setPixel` ignores coordinates outside the valid range so that a drawing algorithm
cannot access invalid memory. This is a storage safeguard; geometric clipping
still needs its own algorithm. `pixelAt` rejects an invalid read with an exception.
Canvas dimensions must be positive, and the constructor checks that the requested
pixel count fits the pixel container's size limit before allocating.

## Image export

`writePpm` creates a binary P6 PPM file. Its header states the image dimensions
and the maximum channel value, 255. Every pixel is then written as three bytes in
red, green, blue order. The payload size is `width * height * 3` bytes. Export does
not depend on the in-memory padding of the `Color` struct.

The writer creates the output's parent directory when needed and reports errors
opening or writing the file. A separate image viewer displays the saved file.
An optional PNG conversion for documentation does not change the C++ renderer.

## What the checks demonstrate

`foundation_tests` checks invalid dimensions, valid writes, ignored writes at all
four out-of-bounds edges, rejected invalid reads, and clearing the canvas. It also
reads an exported two-by-two PPM file and checks the header, exact RGB bytes and
row order against an expected image.

`kitenge_cp1` currently generates a color grid as a pixel diagnostic. It confirms
that the shared surface and export work. It is not the final fabric swatch, dress,
or evidence that the assigned class algorithms have been implemented.

## How teammates use the foundation

An algorithm takes a `Canvas&` and calls `setPixel` with coordinates calculated by
the algorithm. Keep geometry calculations independent of PPM export. The assigned
members own their line, polygon, clipping, circle, ellipse and curve modules; the
integration task will compose their completed modules into project-related scenes.

See the root README for build, run and test commands, and `workfile.md` for proposed
algorithm signatures, assignments and the collaboration workflow.
