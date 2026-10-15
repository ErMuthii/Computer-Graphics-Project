# Polygon validation and scanline fill

Author: Deborah Rehana (168656). Files: `include/kitenge/polygon_fill.hpp`,
`src/polygon_fill.cpp`, `tests/polygon_fill_tests.cpp`.

## Inputs
`fillPolygon(Canvas&, const Polygon&, Color)`. A polygon is an ordered list of
`Point2D` (doubles), first vertex not repeated. Origin is top left, y points down.

## Validation (`isValidPolygon`)
A polygon is accepted only if it has at least 3 vertices, all finite, non-zero
area (shoelace formula), and no two non-adjacent edges touch (simple polygon).
Invalid input draws nothing.

## Algorithm: even-odd scanline fill
1. Find the polygon's minimum and maximum y, clamped to the canvas.
2. For each pixel row, sample at the row centre, y + 0.5.
3. Find where each edge crosses that line and compute the crossing x.
4. Sort the crossings. They come in pairs: fill from the first to the second,
   skip to the third, fill to the fourth, and so on (even-odd rule).
5. Write pixels with `Canvas::setPixel`. Pixel (x, y) is filled when its centre
   lies inside the polygon.

## Edge cases
- Horizontal edges are skipped. The neighbouring edges already bound the span.
- Shared vertices: an edge counts for a row only if low <= y_centre < high.
  At a pass-through vertex exactly one edge counts. At a peak or valley both or
  neither count, so crossings stay in pairs and nothing is counted twice.
- Touching panels: a shared edge belongs to only one polygon (top and left
  included, bottom and right excluded), so there is no gap or overlap.
- Off-canvas parts are clamped to the canvas. Fully off-canvas polygons draw nothing.

## Limitations
Simple polygons only (no self-intersection, no holes). Concave shapes are
supported. No anti-aliasing.

## Project use
Fills the diamond motif and the bodice and skirt panels of the dress
(`polygon_fill_demo`, evidence in `docs/evidence/`).

## Verify
`ctest --test-dir build -C Debug --output-on-failure` runs `polygon_fill`
(rectangle, triangle, concave, vertex on scanline, off-canvas, invalid, shared edge).