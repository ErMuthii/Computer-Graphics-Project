# Checkpoint 1 evidence

Commit selected screenshots here when the algorithms have been implemented.
Generated images in `outputs/` remain reproducible working files.

For each screenshot, document the source commit, command used to generate the
image, author, algorithms shown, and the project component represented.

The planned evidence set is:

1. A geometric fabric swatch showing hand-implemented primitives and filling.
2. A motif before and after clipping to a fabric window or convex dress panel.
3. A simplified dress containing the generated Kitenge-inspired pattern and curves.

The current foundation diagnostic only verifies the pixel canvas and image export;
it is not sufficient CP1 evidence by itself. A PDF can combine the final screenshots
if requested by the lecturer.

## Recorded evidence

### Cubic Bézier construction and fabric motifs (`bezier-curves.png`)

- Original evaluation and sampling: Andrew Karanja Gathirwa, 167144;
  source commit `b7fc479` on `feat/cp1-bezier`.
- Canvas rendering and demo source: Muthii Eric Macharia, 166390;
  source commit `780b631` on `fix/cp1-bezier-evidence`. Eric generated the
  evidence PNG from that demo.
- Algorithms: de Casteljau evaluation, uniform sampling and Bresenham
  connections between rounded samples.
- Commands: `bezier_demo outputs/bezier-curves.ppm`, then
  `sips -s format png --out docs/evidence/bezier-curves.png outputs/bezier-curves.ppm`.
- Project component: visible control-point construction and repeated arched
  fabric motifs for future neckline or dress decoration.

### Lines before and after clipping (`lines-clipping.png`)

- Author: Muthii Eric Macharia, 166390, covering the lines assignment.
- Algorithms: integer Bresenham drawing and Cohen-Sutherland rectangular clipping.
- Source commit: `b9c6749` on `feat/cp1-lines`.
- Commands: `lines_demo outputs/lines-clipping.ppm`, then
  `sips -s format png --out docs/evidence/lines-clipping.png outputs/lines-clipping.ppm`.
- Project component: diamond-and-cross fabric motif drawn before and after
  clipping to the cream rectangular fabric window.

### Circle and ellipse motifs (`circles-ellipses.png`)

- Author: Muthii Eric Macharia, 166390, covering the circles/ellipses assignment.
- Algorithms: midpoint circle rasterization and midpoint ellipse rasterization.
- Source commit: `807a7e0` on `feat/cp1-circles-ellipses`.
- Commands: `circles_demo outputs/circles-ellipses.ppm`, then
  `sips -s format png --out docs/evidence/circles-ellipses.png outputs/circles-ellipses.ppm`.
- Project component: round and oval decorative fabric medallions with a central
  strip suggesting dress buttons.

### Polygon clipping before and after (`polygon-clipping.png`)

- Author: Murega Kelvin Mutwiri, 166914.
- Algorithm: Sutherland-Hodgman polygon clipping against convex boundaries.
- Source commit: `ecc73fc` on `feat/cp1-polygon-clipping`.
- Command: `polygon_clip_demo outputs/polygon-clipping.ppm`, converted to PNG
  for viewing.
- Project component: Kitenge motifs cropped to a rectangular fabric window and
  to a trapezoid skirt panel. Outlines use a temporary preview helper until the
  lines and polygon fill modules are integrated.
