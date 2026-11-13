# Motifs, palettes and mannequin

Author: Deborah Rehana (168656). Files: `include/kitenge/motifs.hpp`,
`src/motifs.cpp`, `include/kitenge/mannequin.hpp`, `src/mannequin.cpp`,
`src/motif_demo.cpp`, `tests/motif_tests.cpp`, `tests/mannequin_tests.cpp`.

This module supplies the data that the rest of the studio draws: the motif
catalog, the palettes and the fixed mannequin. It does not tile, transform or
animate anything. Those jobs belong to the pattern engine (Kelvin), the
transform module (Jeremy) and animation (Lenina), which all read this data
without changing it.

## Local motif coordinates

Each motif is described once, in its own local coordinates:

- centred on the origin `(0,0)`;
- inside `[-25, 25]` on both axes (`kMotifHalfExtent`);
- x to the right, y down, the same as the canvas.

A motif has no idea where it will be drawn. For tile `(column, row)` the
pattern engine takes a fresh copy and maps each local point `p` with the
shared order

```text
p_canvas = T(origin + translation + tileOffset) * T(pivot) * R * F * S * T(-pivot) * p
```

Because the motif is centred on `(0,0)`, the default pivot `(0,0)` is its
centre, so rotation, reflection and scaling happen in place and the tile
offset then moves the copy into its cell. The catalog geometry is never
modified, so no tile inherits an earlier tile's transform.

## Motif data

```cpp
struct MotifFill   { Polygon contour; int role; };                // filled, drawn in order
struct MotifStroke { std::vector<Point2D> points; bool closed; int role; };
struct MotifShape  { std::vector<MotifFill> fills; std::vector<MotifStroke> strokes; };
```

These are the structures the pattern engine was written against. They replace
the engine's interim copies, with the same field names.

- Fills are simple polygons with no holes. Concave contours are allowed (the
  cross and the steps) because the scanline filler handles them and the engine
  triangulates concave contours before clipping.
- Fills are drawn in list order, so a later fill sits on top of an earlier
  one. An accent detail is a second fill over the primary shape, not a hole.
- Strokes are polylines, drawn with Bresenham lines after the fills.

## Catalog

| Index | Label | Shape | Roles |
| --- | --- | --- | --- |
| 0 | DIAMOND | Diamond with an inner accent diamond | primary, accent |
| 1 | TRIANGLE | Triangle with an inverted accent triangle | primary, accent |
| 2 | CROSS | Concave 12-vertex cross, accent centre square | primary, accent |
| 3 | STEPS | Concave staircase, accent square; asymmetric in x and y | primary, accent |
| 4 | SUNBURST | Four Bezier petals, sampled disc, sampled ring stroke | accent, primary, accent stroke |

The outlines of DIAMOND, TRIANGLE and CROSS are the starter scene's shapes,
moved out of `studio.cpp` into local coordinates. STEPS is deliberately
asymmetric so that x and y reflections give visibly different results (the
diamond outline, for example, is its own x-mirror). SUNBURST is the round and
curved variant.

`kMotifCount` and the labels are published with the data. The studio sizes its
motif selection from `kMotifCount` and prints `catalogMotifLabel`, so adding a
motif needs no other `%3` or name-array edits.

### Curves and circles are sampled

SUNBURST's petals are two mirrored cubic Bezier curves each, evaluated with
Andrew's CP1 de Casteljau evaluator (`sampleCubicBezier`, 8 segments per
curve) and closed into a polygon. The disc (32 points) and the ring (24
points) are sampled circles.

This is an approximation, chosen on purpose. The CP1 midpoint circle and
ellipse rasterizers draw axis-aligned shapes at whole-pixel radii, so they
cannot represent a circle after a non-uniform scale or a rotated ellipse.
Sampled points go through the same affine matrices as every other vertex.
At the studio's motif sizes the polygon edges are not visible. The midpoint
and Bezier demos remain separate CP1 evidence. Andrew's dedicated Bezier
motif for the controls work can be added to the catalog the same way.

## Palettes and roles

Colours are given by role, not RGB, so any motif works with any palette:

| Role | Constant | Meaning |
| --- | --- | --- |
| 0 | `kRoleBackground` | Fabric ground; the engine fills each panel with it first |
| 1 | `kRolePrimary` | Main motif colour |
| 2 | `kRoleAccent` | Detail colour |

| Index | Label | Background | Primary | Accent |
| --- | --- | --- | --- | --- |
| 0 | EARTH | 242,221,175 | 174,65,32 | 48,81,57 |
| 1 | LAGOON | 228,231,216 | 40,96,110 | 190,113,43 |
| 2 | PLUM | 234,214,207 | 110,56,86 | 55,76,77 |
| 3 | INDIGO | 236,229,212 | 38,52,105 | 214,160,36 |

The first three keep the starter scene's colours. `paletteRoleColors(i)`
returns `{background, primary, accent}`, so a role number indexes the list
directly. That list is what `renderPattern` takes. With the studio's
alternating tiles the engine swaps roles 1 and 2 on odd tiles, so every motif
shows in both colourings. Fabric and dress use the same motif, palette and
role mapping, and a test checks that all three role colours appear in both
views for every motif/palette pair.

Automatic palette scoring is outside the project scope.

## Validation

`isValidMotif` accepts a motif only if:

- it has at least one fill or stroke;
- every fill passes `isValidPolygon` from CP1 polygon fill (at least 3
  vertices, all finite, non-zero shoelace area, no touching non-adjacent
  edges);
- every stroke has at least 2 points (3 if closed);
- every point is finite and inside the `[-25, 25]` extent;
- every role is in `[0, 3)`.

Every catalog lookup (`catalogMotif`, `catalogMotifLabel`, `catalogPalette`,
`catalogPaletteLabel`, `paletteRoleColors`, `paletteRoleColor`) throws
`std::out_of_range` for an index or role outside the catalog. The studio's
`studioPalette` forwards to the catalog and so fails the same way.

## Mannequin and how it defines clipping

`mannequinGeometry()` is the only definition of the static figure: head
ellipse, neck, arms, legs, shoes, waistband, and the two dress panels.

- **Bodice and skirt are separate convex polygons.** Sutherland-Hodgman clips
  against one convex boundary, and a single dress outline (narrow waist, wide
  shoulders and hem) would be concave. Splitting at the waist gives two convex
  trapezoids that share the edge `(915,377)-(985,377)`. `isConvexPolygon`
  checks that every turn has the same sign (collinear vertices ignored).
- **One source for outlines and clipping.** `studioBodice()` and
  `studioSkirt()` return the mannequin's own polygons, and the pattern engine
  clips against them. The outline the viewer sees is therefore exactly the
  clipping boundary, with no duplicated coordinates to drift apart.
- **Only the dress is patterned.** The head and limbs are drawn plain by
  `drawMannequinFigure`, which never paints inside the bodice. The garment is
  then drawn (patterned by the engine, or plain with `drawPlainGarment`), and
  `drawGarmentTrim` draws the outlines and waistband on top.
- **Fixed silhouette.** No control moves or reshapes the mannequin. Pattern
  transforms move the design inside the panels, never the panels.

Moving the mannequin out of `studio.cpp` was checked pixel for pixel. Before
the motif catalog was connected, `studio_render` output for the default scene
and two option scenes was identical to the previous code.

## Polygon filling in this module

Every motif fill, dress panel, arm, leg and the neck is drawn with the CP1
even-odd scanline filler (see [polygon_fill.md](polygon_fill.md)). A pixel is
filled when its centre `(x+0.5, y+0.5)` is inside. Edges count on the
half-open rule, so the shared waist edge belongs to exactly one panel, with no
gap or double-painted row between bodice and skirt.

## Demonstration

`motif_demo [output.ppm]` (default `outputs/motif-catalog.ppm`):

- Left: every motif (columns) in every palette (rows). The grey square is the
  local `[-25, 25]` extent and the grey cross the local origin. Motifs are
  drawn at 2x for visibility; this is placement for display, not a matrix
  transform.
- Right: the unpatterned mannequin, with red markers on the panel vertices
  showing the two convex clipping boundaries.

The integrated result is `studio_render` (or `kitenge_studio`, keys M and P),
where the same catalog drives both the fabric preview and the dress.

## Verify

```sh
cmake -S . -B build-core -DKITENGE_BUILD_DESKTOP=OFF
cmake --build build-core --parallel
ctest --test-dir build-core --output-on-failure -R "motifs|mannequin"
./build-core/motif_demo outputs/motif-catalog.ppm
./build-core/studio_render outputs/studio-sunburst.ppm --motif 4 --palette 3
```

On Windows with Visual Studio, add `--config Debug` to the build, `-C Debug`
to `ctest`, and run executables from `build-core/Debug/`.

- `motifs`: counts and labels match the catalog; labels use only letters the
  studio font can draw; every contour is finite, valid, non-zero area and
  inside the extent; roles are in range; each motif uses primary and accent
  and is centred; motifs are visually distinct; palette roles are distinct and
  consistent across `paletteRoleColors`, `studioPalette` and the engine;
  invalid indices and roles throw `std::out_of_range`; the validator rejects
  bow-ties, zero area, NaN, out-of-extent points, bad roles and short strokes;
  with Jeremy's `reflectionMatrix`, STEPS changes under each reflection,
  reflecting twice restores it, area is kept with winding reversed, and the
  catalog is not modified; every role colour appears on both fabric and dress
  for all 20 motif/palette pairs.
- `mannequin`: both panels are valid and convex; the studio uses the same
  polygon objects; panels share the waist edge without overlap; the
  convexity helper accepts both windings and rejects concave, bow-tie,
  degenerate and NaN input; arms stay outside the bodice; the unpatterned
  render puts cloth, skin, trim and shoes where expected; the figure never
  paints inside the bodice.

## Limitations

- Motifs are a fixed, predefined catalog; there is no motif editor (out of
  scope through CP3).
- Curves and circles are polygon approximations (see above).
- Accent details are drawn over the primary fill, not cut out as holes;
  contours with holes are not supported.
- The mannequin is a fixed 2D silhouette at studio canvas coordinates; it is
  not a fitted or simulated garment.
- `StudioPalette` still names the accent role `secondary`; renaming it touches
  the studio and engine, so it is left for the CP3 clean-up with Eric.
