# 2D transformations with homogeneous matrices

Owner: Waruhiu Jeremy Kang'ethe, 166263.

This module turns the studio's transform settings (translation, rotation,
scale, reflection and a pivot) into one 3 x 3 matrix and applies it to motif
geometry. Kelvin's pattern engine applies it to every tile of the fabric and
dress. Lenina's animation and Andrew's controls change the settings it reads.
The maths depends only on `kitenge/types.hpp`. It has no Canvas, PPM, GLFW or
OpenGL code, so the headless renderer and the desktop window run the same
calculation.

```cpp
// include/kitenge/transforms.hpp
struct Mat3 { std::array<std::array<double, 3>, 3> m; /* identity by default */ };
Mat3 multiply(const Mat3& left, const Mat3& right) noexcept;   // also operator*
Point2D transformPoint(const Mat3& matrix, Point2D point);

Mat3 translationMatrix(Point2D offset);          // or (dx, dy)
Mat3 rotationMatrix(double angleRadians);
Mat3 scalingMatrix(double sx, double sy);        // independent, both > 0
Mat3 reflectionMatrix(bool reflectX, bool reflectY) noexcept;

struct TransformParams { Point2D translation; double angleRadians; double scale;
                         Point2D pivot; bool reflectX; bool reflectY; };
Mat3 pivotTransformMatrix(const TransformParams&); // T(p) R F S T(-p)
Mat3 transformMatrix(const TransformParams&);      // T(t) T(p) R F S T(-p)

Polygon transformPolygon(const Mat3&, const Polygon&);                         // new copy
std::vector<Point2D> transformPath(const Mat3&, const std::vector<Point2D>&); // new copy
```

## Why homogeneous coordinates

Rotation, scaling and reflection are linear maps: each is a 2 x 2 matrix `L`,
and `p' = L p`. Linear maps always send the origin to the origin. Translation
moves the origin, so no 2 x 2 matrix can represent it, and it has to be added
separately as `p' = L p + t`. A chain of such steps then mixes products and sums,
and the order is easy to get wrong.

Homogeneous coordinates carry each point as the column vector `(x, y, 1)`. The
extra `1` lets a 3 x 3 matrix add a constant:

```text
| a  b  tx |   | x |   | a*x + b*y + tx |
| c  d  ty | * | y | = | c*x + d*y + ty |
| 0  0  1  |   | 1 |   |       1        |
```

With this form, translation is a matrix too. All four transforms then compose by
matrix multiplication alone, and any sequence of them reduces to a single
matrix. The pattern engine builds that matrix once per tile and applies it to
every vertex.

## Conventions

- **Row-major, column vectors.** `m[row][column]`, and `p' = M * p`.
- **Order.** In `A * B` the right-hand matrix `B` acts on the point first. Matrix
  multiplication is not commutative, so the order of factors matters.
- **Screen y points down.** The origin is at the top left, x goes right and
  y goes down. The rotation matrix is the standard mathematical one:

  ```text
  R(θ) = | cos θ  -sin θ  0 |
         | sin θ   cos θ  0 |
         |   0       0    1 |
  ```

  It turns `(1, 0)` into `(cos θ, sin θ)`. For `θ = 90°` that is `(0, 1)`, which
  points **down** on this canvas. So a positive angle turns right towards down,
  which appears **clockwise** on screen, although the same matrix is
  counterclockwise on a y-up maths graph. The matrix is unchanged; only the
  display axis is flipped. To rotate counterclockwise on screen, use a negative
  angle.
- **Radians.** The maths uses radians. A UI showing degrees converts with
  `radians = degrees * π / 180`.
- **Doubles throughout.** Geometry is rounded only when it is rasterized.

## The four transforms

| Transform | Matrix (top two rows) | Effect on `(x, y)` |
| --- | --- | --- |
| `translationMatrix(tx, ty)` | `[1 0 tx; 0 1 ty]` | `(x + tx, y + ty)` |
| `rotationMatrix(θ)` | `[cos -sin 0; sin cos 0]` | rotation about the origin |
| `scalingMatrix(sx, sy)` | `[sx 0 0; 0 sy 0]` | `(sx x, sy y)`, independent factors |
| `reflectionMatrix(rx, ry)` | `[±1 0 0; 0 ±1 0]` | negates x if `rx`, y if `ry` |

### Reflection naming

The two reflections are easy to confuse, so here is exactly what each flag does:

| Flag | Calculation | Mirror line | On screen |
| --- | --- | --- | --- |
| `reflectX` | negates **x**: `(x, y) -> (-x, y)` | the **Y-axis** (vertical) | left-right flip |
| `reflectY` | negates **y**: `(x, y) -> (x, -y)` | the **X-axis** (horizontal) | top-bottom flip |

Each flag is named after the coordinate it negates, not after the axis it
mirrors across. Reflecting *across the x-axis* negates **y**, so that is
`reflectY`. The planned CP3 key legend in `docs/team/README.md` reads
"X / Y: reflect across the x-axis / y-axis", which pairs the names the other
way round. Whoever maps those keys must therefore choose deliberately: key X
labelled "across the x-axis" needs `reflectY`. Alternatively the label could
describe the flag ("mirror x" / left-right flip). Both flags together equal a
180-degree rotation. Applying either flag twice gives the identity.

### Positive scale only

`scalingMatrix` and `TransformParams::scale` reject zero, negative and
non-finite values. A zero scale collapses the motif to a point or line, which
cannot be undone. A negative scale is a hidden reflection: it would duplicate
the reflection flags and make the UI state ambiguous (it would allow both
`scale = -1` and `reflectX = reflectY = true` for the same result). Mirroring
always goes through the explicit flags.

## Pivot composition

Rotation, reflection and scaling all keep **only the origin** fixed. Applied
directly, a motif whose pivot is not at the origin would swing around the
origin and move away. The fix is a sandwich: move the pivot to the origin,
apply the transform there, then move it back.

```text
p_out = T(translation) * T(pivot) * R(angle) * F(reflect) * S(scale) * T(-pivot) * p
         6               5          4          3            2          1  <- acts first
```

1. `T(-pivot)` puts the pivot at the origin.
2. `S(scale)` scales about it.
3. `F(reflect)` mirrors about it.
4. `R(angle)` rotates about it.
5. `T(pivot)` puts the pivot back.
6. `T(translation)` moves the result. Only this step moves the pivot.

Steps 1 to 5 form `pivotTransformMatrix`. Its matrix sends the pivot to itself,
and the tests check this for every combination of angle, scale and flags.
`transformMatrix` adds step 6.

The reflection acts before the rotation. `S` and `F` are both diagonal, so they
commute, but `R` and `F` do not. Because the mirror is applied first, a positive
angle still turns the mirrored motif clockwise on screen. The tests check that
rotating first and then reflecting gives a different point.

## Worked example (hand calculated, asserted in the tests)

Settings: `translation = (10, 5)`, `angle = 90°`, `scale = 2`,
`pivot = (1, 1)`, `reflectX = true`, `reflectY = false`. The point is `p = (3, 2)`.

Step by step, rightmost factor first:

| Step | Operation | Calculation | Result |
| --- | --- | --- | --- |
| 1 | `T(-pivot)` | `(3 - 1, 2 - 1)` | `(2, 1)` |
| 2 | `S(2)` | `(2*2, 2*1)` | `(4, 2)` |
| 3 | `F(reflectX)` | `(-4, 2)` | `(-4, 2)` |
| 4 | `R(90°)`, cos = 0, sin = 1 | `(0*(-4) - 1*2, 1*(-4) + 0*2)` | `(-2, -4)` |
| 5 | `T(pivot)` | `(-2 + 1, -4 + 1)` | `(-1, -3)` |
| 6 | `T(translation)` | `(-1 + 10, -3 + 5)` | `(9, 2)` |

The same result as a single matrix. The linear part is

```text
L = R * F * S = | 0 -1 | * | -1 0 | * | 2 0 |  =  |  0 -2 |
                | 1  0 |   |  0 1 |   | 0 2 |     | -2  0 |
```

The translation column is `translation + pivot - L * pivot
= (10, 5) + (1, 1) - (-2, -2) = (13, 8)`, so

```text
M = |  0 -2 13 |      M * (3, 2, 1) = (0*3 - 2*2 + 13, -2*3 + 0*2 + 8, 1)
    | -2  0  8 |                    = (9, 2, 1)
    |  0  0  1 |
```

Check: the pivot `(1, 1)` maps to `(-2 + 13, -2 + 8) = (11, 6)`. That is the
pivot plus the translation, as expected. `testWorkedExample` in
`tests/transform_tests.cpp` asserts the matrix, each of the six intermediate
points and the final point.

## Use by the pattern engine

`TransformParams` matches `PatternTransform` in Kelvin's
`include/kitenge/pattern.hpp` field for field: the same names, types, order and
defaults. His header describes that struct as an interim input, to be replaced
by this one or aliased to it, for example:

```cpp
using PatternTransform = TransformParams;
```

His engine computes `T(origin + translation + tileOffset) * localTransform`.
Translations simply add, so the same placement with this module is

```cpp
const Mat3 local = transformMatrix(settings.transform);       // includes translation
const Mat3 tile  = translationMatrix(origin.x + column * pitch.x,
                                     origin.y + row * pitch.y) * local;
const Polygon placed = transformPolygon(tile, motifContour);  // fresh copy per tile
```

or, keeping his split, `translationMatrix(origin + translation + tileOffset) *
pivotTransformMatrix(settings.transform)`.

**Verification against the engine.** His private `localTransform` and its
`Affine` helpers were taken verbatim from `src/pattern.cpp` on
`feat/cp2-pattern-engine` (commit `1c58454`; unchanged on
`feat/cp3-pattern-model-integration`). They were compiled in a scratch program
next to this module and compared over 200,000 random settings (angles ±20 rad,
scales 0.05 to 20, both flags, coordinates ±300). Results:

- All six matrix entries were bit-for-bit identical to `pivotTransformMatrix`.
- The tile points differed by at most 9e-15 relative to his `patternPoint`
  formula. That difference comes only from the order in which the translations
  are summed.
- His helpers also compiled unchanged inside `namespace kitenge` with
  `PatternTransform` aliased to `TransformParams`. They gave identical results
  and no name clashes. The public builders are named `translationMatrix`,
  `rotationMatrix` and so on so that they do not collide with his private
  `translation`, `multiply` and `apply` helpers during the switch-over.

The scratch program is not part of the repository. The committed tests check
the same composition against an independent closed form,
`p' = L (p - c) + c + t`.

## Never mutating the source

`transformPolygon` and `transformPath` take the source by `const&` and return a
new container. The engine transforms a fresh copy of the same catalog motif for
every tile. If the functions edited the motif in place, tile 2 would start from
tile 1's already-transformed vertices and every tile would compound the
previous one's transform. The same rule protects animation: Lenina derives each
frame's settings from the base settings and time, and the geometry is always
re-transformed from the untouched local motif.

The tests check that source vertices are bit-for-bit unchanged:

- after a single transform,
- after a loop that places four tiles (each tile equals the local result plus
  its own offset only),
- after a call rejected because of a NaN vertex.

A reflection reverses a contour's winding. The vertex order is kept, and both
the even-odd scanline filler and the convex clipper accept either winding.
Stroked paths, open or closed, are transformed point by point. An affine map
sends straight segments to straight segments, so this transforms the whole
stroke. Curves must be sampled to polylines first. A transformed circle can
become an ellipse that is rotated or non-uniformly scaled, which the CP1
midpoint rasterizers do not draw, so the circle is approximated by its sampled
polyline.

## Validation

Every rejection throws `std::invalid_argument`, so callers handle one exception
type.

| Input | Rejected when |
| --- | --- |
| translation, pivot, angle, point | any coordinate is NaN or ±infinity |
| `scale`, `sx`, `sy` | NaN, ±infinity, zero (including -0.0) or negative |
| a matrix passed to `transformPoint`, `transformPolygon` or `transformPath` | any entry is non-finite, even for empty input |
| a result | finite inputs overflow, for example a scale of 1e300 times a pivot of 1e300 |
| homogeneous `w` | zero (the point has no Cartesian position) |

A polygon with one bad vertex returns nothing: the function throws before it
returns a partial copy. `multiply` and `reflectionMatrix` cannot fail and are
`noexcept`. A non-finite product is caught when the matrix is used.
The composition functions check their result directly.

## Floating-point tolerance

`std::cos(π/2)` is about `6.1e-17`, not `0`, because π cannot be stored
exactly. A 90-degree rotation of `(1, 0)` therefore gives `(6.1e-17, 1)`.
The tests compare coordinates with an absolute tolerance of `1e-9` and assert
explicitly that `cos(π/2) != 0.0`, which documents why the tolerance is needed.
The source-unchanged checks are the only exact comparisons, because the source
must be untouched bit for bit.

## Tests

`tests/transform_tests.cpp` (CTest name `transforms`) links only the
transforms library. It covers:

- identity: the default matrix, `I*M = M*I = M`, and the default parameters
- a hand-computed 3 x 3 product, and that matrix products do not commute
- translation `(2,3)` then `(4,-1)` totalling `(6,2)`
- a 90-degree rotation taking `(1,0)` to `(0,1)` and `(0,1)` to `(-1,0)`
- independent x and y scale factors, including about a pivot
- each reflection, both together equalling a 180-degree rotation, and
  reflecting twice giving the identity (also about a pivot)
- pivot invariance over 180 combinations of pivot, angle, scale and flags
- inverse rotation, with `R(-θ)` equal to the transpose of `R(θ)`
- translate-then-rotate `(0,11)` against rotate-then-translate `(10,1)`, and
  reflect-before-rotate
- the composite matrix against applying each factor in turn, and against the
  closed form
- the worked example above
- source vertices unchanged, and no compounding across tiles
- empty input
- rejection of NaN, ±infinity, zero, negative, overflow and `w = 0` cases,
  with valid extremes still accepted
- the drop-in contract: field types, defaults and aggregate field order,
  checked with `static_assert`

## Demo

```sh
cmake -S . -B build-core -DKITENGE_BUILD_DESKTOP=OFF -DCMAKE_BUILD_TYPE=Debug
cmake --build build-core --parallel
./build-core/transforms_demo outputs/transforms.ppm
```

The default output path is `outputs/transforms.ppm`. The demo draws only with the
CP1 modules: polygons and the label font use the scanline filler, and outlines,
strokes, grid and axes use Bresenham lines.

The motif is a filled **"F"** with an open zigzag stroke. The F is concave and
has no mirror or rotational symmetry, so every reflection and rotation gives a
visibly different shape. A symmetric motif would look the same after mirroring
and would prove nothing. The pivot sits inside the F's foot, away from the motif
centre, so pivot handling is visible.

Each panel shows:

- a faint grid (10 local units per cell),
- a grey ghost of the untransformed motif,
- the transformed copy,
- the images of the local +x (red) and +y (green) axes at the pivot,
- the pivot (white),
- the pivot's moved image (yellow), where it moved.

The axes show what the matrix did to the frame: their turn is the rotation,
their lengths are the scale, and a swapped handedness is a reflection. The
mapping from motif units to pixels is itself a `Mat3`
(`T(panel centre) * S(2.3, 2.3)`).

| Panel | Matrix | What to look for |
| --- | --- | --- |
| Original | identity | reference |
| Translate | `T(12,-8)` | everything shifts, including the pivot (yellow) |
| Rotate +30° | pivot sandwich, `R(30°)` | turns **clockwise** about the white pivot, which stays put |
| Scale 1.6 x 0.6 | `T(p) S(1.6,0.6) T(-p)` | wider and flatter; pivot fixed |
| reflectX | `F(true,false)` about the pivot | left-right flip across the dashed vertical line; red axis now points left |
| reflectY | `F(false,true)` about the pivot | upside down across the dashed horizontal line; green axis now points up |
| Combined | `T(8,-6) T(p) R(30°) F(x) S(0.8) T(-p)` | all six factors in the shared order |
| Order | coral `T(18,0)*R(90°)`, blue `R(90°)*T(18,0)` | same two factors, different order, different place |

## Challenges recorded

- **Overflow that cancels.** The first overflow test used a translation and
  pivot of 1.5e308 with a scale of 1, expecting the composite to overflow. It
  did not. In the pivot sandwich the translation column is
  `pivot - scale * pivot`, which is exactly 0 when the scale is 1, so the large
  values cancelled. The tests now use cases that really overflow:
  `scale * pivot = 1e300 * 1e300` inside the sandwich, and a finite sandwich
  (`0.75e308`) whose final translation pushes it past the largest double.
- **Interim types in the pattern engine.** The pattern engine was written
  before this module and has its own private `Affine` and `PatternTransform`.
  Rather than invent a different parameter struct, this module matches his
  field for field. Its public names avoid his private helper names, so
  switching over is an alias rather than a rewrite.

## Limitations

- `TransformParams::scale` is uniform, as in the shared contract and
  `PatternTransform`. Non-uniform scaling exists as `scalingMatrix(sx, sy)` but
  is not part of the parameter struct. Adding it would change the shared
  contract and needs agreement with Eric, Kelvin and Andrew.
- No general matrix inverse is provided. Inverses are exercised for rotation
  only. Undoing settings is done by recomputing from the base settings, never by
  inverting accumulated geometry.
- Upper limits on scale and tile density belong to the pattern engine
  (`kMinPatternScale`, `kMaxPatternScale`) and the controls, not to this module,
  which only requires a positive finite scale.
- This module was built and tested with GCC 13.3 (`-Wall -Wextra -Wpedantic`,
  also checked with `-Wconversion -Wsign-conversion -Wshadow`) on Ubuntu 24.04
  under WSL. It was not compiled with MSVC `/W4` here.
