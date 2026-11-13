# Pattern generation: repetition and garment clipping

Owner: Murega Kelvin Mutwiri, 166914. Branches: `feat/cp2-pattern-engine`
(engine) and `feat/cp3-pattern-model-integration` (desktop integration, below).

The pattern engine turns one motif into a repeating fabric and fits that fabric
inside a convex region: the fabric preview rectangle, or the bodice or skirt
panel of the mannequin. The same function draws all three, so the swatch and the
dress always show the same design.

```cpp
// include/kitenge/pattern.hpp
void renderPattern(Canvas& canvas, const MotifShape& motif,
                   const std::vector<Color>& palette,
                   const PatternSettings& settings, const Polygon& destination);
```

The renderer is Pure C++. It writes into a `Canvas` and has no GLFW or OpenGL
dependency, so the headless renderer and the desktop window produce the same
pixels for the same state.

## Inputs

| Input | Meaning |
| --- | --- |
| `motif` | Filled contours and stroked polylines in local coordinates centred on `(0,0)`, each with a palette role. Fill contours must be simple (no self-crossings, no holes) and may be concave. |
| `palette` | Colours indexed by role. Role `0` is the background that fills the destination first. |
| `settings.transform` | Translation, angle (radians), scale, pivot and two reflection flags. |
| `settings.tilePitch` | Horizontal and vertical distance between neighbouring tiles. |
| `settings.origin` | Canvas position of tile `(0,0)`. |
| `settings.alternateTiles` | Swap roles 1 and 2 on odd tiles for a checkerboard of colours. |
| `destination` | Convex polygon to fill, in either winding. |

Coordinates follow the shared contract: origin at the top left, x to the right,
y downwards. Every intermediate value is a `double`; values are rounded only when
a pixel is written.

## 1. Transforming a tile: matrix order

Each tile gets a fresh copy of the motif. A motif-local point `p` is mapped to
the canvas with the agreed order (column vectors, rightmost operation first):

```text
p_canvas = T(origin + translation + tileOffset) * T(pivot) * R(angle) * F(reflection) * S(scale) * T(-pivot) * p
tileOffset = (column * pitch.x, row * pitch.y)
```

Reading right to left:

1. `T(-pivot)` moves the pivot to the local origin.
2. `S(scale)` scales uniformly.
3. `F` mirrors: `reflectX` maps `x -> -x`, `reflectY` maps `y -> -y`.
4. `R(angle)` rotates by `[[cos, -sin], [sin, cos]]`. Because y points down on
   the canvas, a positive angle turns `+x` towards `+y`, which looks clockwise.
5. `T(pivot)` moves the pivot back, so it stays fixed under steps 2–4.
6. `T(origin + translation + tileOffset)` places the tile.

The tile offset is added **after** the local transform. Rotating a motif
therefore turns every copy in place; the grid itself does not rotate. The
pattern translation shifts the pattern origin, never the dress.
Starting every tile from the untouched local motif means the transform is never
applied twice to already-transformed geometry.

`patternPoint(settings, column, row, p)` exposes exactly this mapping for tests.
Internally a small 2×3 affine type composes the matrices once per render. When
Jeremy's shared `Mat3` lands it replaces that private type; the order does not
change.

## 2. Which tiles to draw: a conservative tile range

Using the untransformed motif size is not enough. After a rotation by 45° or a
scale of 1.8, a copy whose centre is outside the destination can still reach
into it. The engine computes the real extent instead:

1. Transform every fill vertex and stroke point of the motif with the local
   transform (steps 1–5 above, without the tile offset). Their bounding box
   `[lx0, lx1] × [ly0, ly1]` is the exact extent of any one tile relative to its
   origin, because an affine map sends polygon vertices to polygon vertices.
2. Tile `c` covers `x` from `O.x + c·pitch.x + lx0` to `O.x + c·pitch.x + lx1`.
   It can meet a destination spanning `[dx0, dx1]` only if

   ```text
   ceil((dx0 - O.x - lx1) / pitch.x) <= c <= floor((dx1 - O.x - lx0) / pitch.x)
   ```

   and likewise for rows. The destination bounds are widened by one pixel to
   cover rounding at the raster boundary.
3. The tile count is checked in `double` before any loop runs. More than
   `kMaxPatternTiles` (20 000) tiles is rejected, which keeps the work bounded.

The test `conservative tile range` checks the guarantee directly. It scans a
four-tile ring around the computed range, for several rotations, a scale of
1.7, an off-centre pivot, a translation and a reflection, and confirms that
none of those tiles reaches the panel.

## 3. Clipping

Each tile in the range is checked against the destination:

- **Bounding box outside the destination's box:** skipped.
- **All four corners of the tile's bounding box inside the convex destination:**
  then the whole tile is inside (a convex region contains the convex hull of
  its points), so it is drawn unclipped. Most tiles take this fast path.
- **Otherwise:** the tile is clipped geometrically.

### Filled contours: Sutherland–Hodgman, with triangulation for concave motifs

Filled pieces use the CP1 Sutherland–Hodgman clipper (`clipPolygonToConvex`).
It is exact for a convex subject. For a **concave** subject that the boundary
cuts into separate parts, it returns one polygon whose parts are joined by
zero-width "bridge" edges running along the boundary. The CP1 fill validator
correctly rejects such a polygon as self-touching, so the whole motif would
silently disappear.

The test `concave motif at slanted edge` reproduces this. A U shape whose base
lies outside the panel splits into two arms. Naive clipping gives an invalid
polygon. The engine keeps both arms, 150 px² each.

The engine avoids the problem by decomposition:

1. When the motif is prepared, a convex contour is kept as it is. A concave
   contour is split into triangles by **ear clipping**
   (`triangulateSimplePolygon`). A vertex is an *ear tip* when it turns the same
   way as the polygon and no other vertex lies inside or on the triangle it forms
   with its neighbours. Cutting off ears one at a time leaves `n − 2`
   triangles that exactly cover the polygon. Collinear vertices are dropped
   first because they add no area.
2. Each convex part is transformed and clipped. Convex ∩ convex is convex, so
   every clipped piece is a single simple convex polygon. It always passes
   `isValidPolygon`, and it can never break into disconnected pieces. Disconnected
   parts of the motif arrive as separate pieces automatically.
3. Triangulation happens once per render in local coordinates. Affine maps
   keep triangles as triangles, so the same split is valid for every tile and
   under reflection.

Neighbouring triangles share edges. The scanline filler's half-open
pixel-centre rule (`[xa, xb)` on each row) gives each pixel centre to exactly
one of them, so a split leaves no gaps and no doubled pixels.

### Strokes: Cyrus–Beck segment clipping

Stroked paths, including Bézier curves sampled with Andrew's
`sampleCubicBezier`, are clipped segment by segment against the convex
destination (`clipSegmentToConvex`). The CP1 line clipper (`clipLineToRect`)
only handles axis-aligned rectangles. The dress panels have slanted sides, so
the engine uses the parametric Cyrus–Beck method:

- For segment `P(t) = P0 + t (P1 − P0)` and boundary edge `A → B`, the signed
  inside distance `f(t) = s · cross(B − A, P(t) − A)` is linear in `t`. Here
  `s` is the boundary winding from the shoelace sign, as in the CP1 clipper.
- Each edge keeps the values of `t` where `f(t) ≥ 0`: if `f` increases, `t` must
  be at least the root; if it decreases, at most the root. A segment parallel to
  an edge is either wholly kept or wholly rejected.
- The kept interval `[tEnter, tLeave]` is the visible part. If it is empty,
  nothing is drawn. Endpoints stay as doubles and are rounded only when the
  CP1 Bresenham routine draws them.

## 4. Bounds-safe writes versus geometric clipping

These are different guarantees:

- **Bounds-safe writes:** `Canvas::setPixel` ignores coordinates outside the
  image. This only stops memory errors at the canvas edge. It knows nothing
  about the dress, so without real clipping a motif would paint over the arms,
  the background and the other panel.
- **Geometric clipping:** cuts the shapes themselves at the destination
  boundary, in `double` precision, before any pixel exists. It keeps the
  geometry correct (exact areas, intersection points on the slanted edges) and
  saves work, because invisible parts are never rasterized.
- **Pixel mask (last step):** rounding a clipped stroke endpoint, or Bresenham
  stepping, can still land on a pixel whose centre is just outside a slanted
  edge. The renderer therefore draws into an off-screen layer the size of the
  destination's bounding box. It then copies only the pixels that the scanline
  filler assigns to the destination polygon, which is exactly how the panel
  itself would be filled. This is the "panel mask" option from the guide, used
  together with geometric clipping, not instead of it.

The test `no pixels outside destination` renders rotated, reflected,
checkerboarded patterns into three convex shapes, including one that extends
past the canvas. It checks that every pixel outside the destination is
untouched and that every pixel inside it is filled.

## 5. One pattern across both dress panels

The bodice and skirt are separate convex panels meeting at the waist (`y = 377`).
The garment hook renders both with **one** `PatternSettings` value and one
shared origin (`kGarmentPatternOrigin`), so tile `(c, r)` lands at the same
canvas position whichever panel it is clipped to. A motif crossing the waist is
cut into a bodice part and a skirt part that join exactly.

`continuity across the waist` checks this pixel for pixel. It renders the two
panels separately, then renders one rectangle covering the whole dress, and
checks that every dress pixel matches.

The fabric preview uses the same motif, palette and settings with its own origin
(the centre of the swatch). The garment view zooms the whole repeat by
`kGarmentPatternZoom = 0.5` (scale, pitch and translation together) so that
several repeats fit on the small mannequin. The design is the same; only the
viewing size differs. With a non-zero pivot this zoom also shifts the repeat's
phase by a constant. That shift is identical in both panels, so continuity at the
waist is unaffected.

## 6. Studio integration

`makePatternHooks(provider)` returns `StudioHooks` with `fabric` and `garment`
set. `studio_render` and the desktop app (`kitenge_studio`) both use them. The
shell still draws the head, limbs, outlines and belt; the hooks only write
inside the fabric area and the two panels. A test checks every other pixel of
the scene stays unchanged.

The panel polygons and palettes now come from `studioFabricArea()`,
`studioBodice()`, `studioSkirt()` and `studioPalette()` in `studio.hpp`. The
outlines and the fills therefore use one copy of the geometry instead of two.

`provider` is where Jeremy's transformation settings and Lenina's animated
parameters plug in once `StudioState` carries them. Without it, the hooks use
`defaultStudioPatternSettings()`, which reproduces the starter look: pitch
64 × 70 and alternating colours.

## Validation and limits

All of these throw `std::invalid_argument`, before anything is drawn:

- Tile pitch must be finite and ≥ `kMinTilePitch` (4 px) on both axes.
- Scale must be finite and within `[kMinPatternScale, kMaxPatternScale]` =
  `[0.05, 20]`.
- Angle, translation, pivot and origin must be finite.
- More than `kMaxPatternTiles` = 20 000 tiles is rejected.
- The destination must be convex with non-zero area.
- Fill contours must be simple polygons, and strokes need at least two finite
  points.
- Every role, including roles swapped on odd tiles, must index the palette, and
  the palette must not be empty.

The UI should clamp its scale and spacing controls to these limits.

## Performance

Measured by `pattern_demo` (Release build, MinGW g++ 15.2, Windows 11), timing
one `renderPattern` call with the concave cross plus a 24-segment Bézier stroke:

| Case | Tiles | Clipped tiles | Pieces | Time |
| --- | --- | --- | --- | --- |
| Fabric preview, default pitch 64 × 70 | 77 | 32 | 1 961 | ≈ 5 ms |
| Fabric preview, pitch 8 × 8, scale 0.15, rotated | 4 617 | 272 | 114 053 | ≈ 45 ms |
| Skirt, same dense settings at garment zoom | 2 350 | 897 | 40 369 | ≈ 25 ms |

Default settings render well within a frame. The densest accepted settings take
tens of milliseconds, which is acceptable for redraws after a control change.
If continuous animation at that density is required, raise `kMinTilePitch` or
cache the clipped geometry per settings change before adding finer density
controls. Debug builds are about four times slower.

## Tests

`tests/pattern_tests.cpp` (CTest name `pattern`) covers:

- matrix order and pivot behaviour, including the on-screen rotation direction
- rejection of invalid pitches, scales, angles, concave destinations,
  self-intersecting motifs, missing palette roles and over-dense settings, with
  no drawing on rejection
- triangulation of concave and collinear polygons (area kept, `n − 2`
  triangles)
- Cyrus–Beck clipping on a slanted panel (crossing, parallel, on-edge and
  reversed segments)
- motifs fully inside, fully outside and crossing an edge, with exact kept
  areas
- rotated and reflected edge tiles
- conservative tile ranges under rotation, scale, pivot, translation and
  reflection
- a concave motif split by the boundary, plus a pixel comparison against a
  masked reference on slanted edges
- a stroked Bézier curve near a slanted edge
- no pattern pixels outside the destination, and none missing inside
- continuity across the waist
- studio hooks: motif, palette and transform changes reach both views, shell
  pixels are untouched, and rendering is deterministic

## Reproducing the evidence

```sh
cmake -S . -B build-core -DKITENGE_BUILD_DESKTOP=OFF -DCMAKE_BUILD_TYPE=Debug
cmake --build build-core --parallel
ctest --test-dir build-core --output-on-failure
./build-core/pattern_demo outputs
./build-core/studio_render outputs/studio.ppm
```

See `docs/evidence/README.md` for the images.

## Known limitations and integration notes

- `PatternTransform` and `MotifShape` are interim inputs. The shared contracts
  give the transform type to Jeremy and the motif and palette data to Deborah,
  and neither was on `main` when this was written. Both structures follow the
  contract field for field and should be replaced by, or aliased to, the shared
  types when they merge. Eric coordinates that header change.
- `starterMotifShape()` reproduces the three starter shapes as local data until
  Deborah's motif module replaces it.
- Strokes are one pixel wide (CP1 Bresenham).
- The desktop was checked with scripted key events and frame captures (see CP3
  below). Physical key presses still need a manual check in the running window.

## CP3: the pattern in the desktop studio

Branch: `feat/cp3-pattern-model-integration`.

### Both views update immediately

The desktop redraws whenever a key changes the state. The hooks read the motif,
palette and pattern settings on every render, so nothing is cached between
frames that could go stale. The desktop smoke test (`desktop_smoke`) now records
the frame before the scripted M and P presses and the frame right after them.
It fails unless the fabric preview, the bodice and the skirt have all changed in
that very next frame. The headless `studio hooks` test checks the same for
motif, palette and transform changes.

Transformation controls belong to Andrew (input) and Jeremy (state), so no keys
were added here. For verification, both programs accept the same scene options:

```text
--motif N  --palette N  --angle DEGREES  --scale S  --pitch X Y
--translate X Y  --reflect-x  --reflect-y
```

`--angle` is given in degrees and converted with `degreesToRadians`, following
the contract (radians in the maths, degrees for people). Every option is
validated so that both views stay within the engine's limits; the garment view
works at half scale, so `--scale` must be at least 0.1 and `--pitch` at least 8.

### The renderer stays independent of OpenGL

`kitenge_pattern` links only the Pure C++ studio core. No pattern or studio file
includes a GL, GLEW or GLFW header, and the `build-core` tree builds and passes
all of its tests with `KITENGE_BUILD_DESKTOP=OFF`. OpenGL only displays the
finished canvas as a texture.

### Desktop result equals headless result

`ppm_compare` compares two PPM images pixel for pixel and can write a
difference image. With `-DKITENGE_GUI_TESTS=ON`, CTest runs
`tests/desktop_matches_headless.cmake` for three scenes:

1. `kitenge_studio --capture … --exit-after-capture <scene>` reads the OpenGL
   back buffer after real presentation.
2. `studio_render … <scene>` renders the same scene without a window.
3. `ppm_compare` requires the two images to be identical.

| Scene | Options | Result |
| --- | --- | --- |
| default | none | identical, 912 000 pixels |
| transformed | `--motif 2 --palette 1 --angle 20 --scale 0.9` | identical |
| reflected | `--motif 1 --palette 2 --angle -35 --scale 1.3 --pitch 52 60 --translate 17 -9 --reflect-x` | identical |

Measured on Windows 11 with Intel Iris Xe Graphics (OpenGL 3.3, GLFW and GLEW
from MSYS2 UCRT64). The texture uses nearest filtering and the window opens at
the canvas size, so every canvas pixel reaches exactly one screen pixel. As a
check that the comparison can fail, comparing the default scene with the
transformed one reports 319 142 differing pixels.

### Profiling before adding density controls

`pattern_demo` also times whole studio frames (`renderStudio` with the hooks,
Release build, average of 20 frames):

| Frame | Time |
| --- | --- |
| Starter scene, no pattern hooks (shell only) | ≈ 4 ms |
| Pattern hooks, default settings | ≈ 9 ms |
| Pattern hooks, densest accepted options (pitch 8, scale 0.1, rotated) | ≈ 36 ms |

The desktop only redraws when the state changes, or every frame while
animation plays. Default settings fit comfortably in a 60 Hz frame (16.7 ms).
The densest settings take about 36 ms, so a redraw after a key press is still
instant, but continuous animation at that density would run near 28 fps.

Recommendation for the controls: keep the current bounds (`kMinTilePitch`,
`kMinPatternScale`, `kMaxPatternTiles`) and do not add unbounded density.
If animation must reach 60 fps at high density, cache the clipped geometry per
settings value, or raise the minimum pitch, before widening the range.
