# Cubic Bézier curves

Andrew Karanja Gathirwa (167144) implemented cubic curve evaluation and sampling
in `b7fc479`. Muthii Eric Macharia (166390) connected those samples to the
merged line rasterizer in `780b631` so the curve can appear on the canvas.

## Inputs and method

Four finite `Point2D` controls define a cubic curve. `evaluateCubicBezier`
returns a point for `t` in `[0, 1]`. De Casteljau's method interpolates each
neighbouring pair three times: four controls become three points, then two,
then one point on the curve. The first and last controls are returned exactly
for `t = 0` and `t = 1`.

`sampleCubicBezier` evaluates `segments + 1` equally spaced values of `t`.
`drawCubicBezier` rounds each sample with `std::lround` and joins neighbouring
samples with Bresenham lines. More segments approximate the continuous curve
more closely. The default is 100; accepted values are 1 through 10,000.

## Validation and limits

- Non-finite controls and `NaN` parameters throw `std::invalid_argument`.
- Parameters outside `[0, 1]` throw `std::out_of_range`; no clamping occurs.
- Invalid segment counts throw `std::invalid_argument`.
- Raster coordinates outside the `int` range throw `std::out_of_range`.
- The curve is a sampled polyline on the pixel canvas. It is not an analytic
  fill or a substitute for clipping, and coarse sampling can look angular.

The checks cover exact endpoints, hand-calculated intermediate values,
repeated controls, straight-line controls, invalid inputs, sample counts and
raster pixels at the endpoints and midpoint.

## Project demonstration

Run `bezier_demo outputs/bezier-curves.ppm`. The left panel shows the four
coral control points, their thin teal control polygon, the sampled gold curve
and its cream midpoint. The right panel repeats paired curves as arched fabric
motifs that can inform a neckline or dress decoration. The selected PNG is
[`../evidence/bezier-curves.png`](../evidence/bezier-curves.png).
