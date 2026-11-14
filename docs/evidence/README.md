# Checkpoint 1 evidence

The [five-page evidence PDF](cp1-evidence.pdf) collects the selected images for
submission or offline review. The original PNGs and generation details are below.

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

## Checkpoint 2 evidence: pattern engine

Author: Murega Kelvin Mutwiri, 166914. Source commit: `3a38393` on
`feat/cp2-pattern-engine` (engine in `1c58454`). All three images come
from the same `renderPattern` function described in
[pattern generation](../algorithms/pattern-generation.md).

Commands, from a configured `build-core`:

```sh
./build-core/pattern_demo outputs
python -c "import sys;from PIL import Image;Image.open(sys.argv[1]).save(sys.argv[2])" outputs/pattern-clipping.ppm docs/evidence/pattern-clipping.png
```

(repeat the conversion for `pattern-swatch` and `pattern-mannequin`).

### Repeat clipping before and after (`pattern-clipping.png`)

- Left: every tile in the computed range drawn whole over a slanted convex
  panel, showing how far the repeat spills past the edges.
- Right: the same tiles after clipping. Fills use Sutherland-Hodgman on
  ear-clipped triangles of the concave cross; the sampled Bezier arches use
  Cyrus-Beck segment clipping; a scanline-filled panel mask guards the pixels.
- Settings: pitch 66 x 72, rotation 0.25 rad, scale 1.2, alternating colours.

### Transformed swatch (`pattern-swatch.png`)

- Left: the untransformed repeat (pitch 64 x 70).
- Right: the same motif rotated 30 degrees (clockwise on screen), scaled 0.8,
  reflected in y about pivot (0, 6), translated by (17, -9), pitch 52 x 56.
  Each copy turns in place because the tile offset is applied after the local
  transform.

### Patterned mannequin (`pattern-mannequin.png`)

- The studio scene rendered through `makePatternHooks`: the fabric preview and
  both dress panels show the cross motif, palette 2, rotated 20 degrees and
  scaled 0.9. The bodice and skirt share one pattern origin, so the repeat
  continues across the waist; head, limbs and outlines are unchanged.

## Checkpoint 3 evidence: pattern in the desktop studio

Author: Murega Kelvin Mutwiri, 166914. Source commit: `eadfcb4` on
`feat/cp3-pattern-model-integration`.
Captured on Windows 11, Intel Iris Xe Graphics, OpenGL 3.3.

### Desktop capture (`pattern-desktop-capture.png`)

- The OpenGL back buffer of `kitenge_studio`, read with `glReadPixels` after
  presentation, for `--motif 2 --palette 1 --angle 20 --scale 0.9`.
- Command: configure `build-desktop` with `-DKITENGE_BUILD_DESKTOP=ON
  -DKITENGE_GUI_TESTS=ON`, run
  `ctest --test-dir build-desktop -R desktop_matches_headless`, then convert
  `build-desktop/test-output/desktop/transformed-desktop.ppm` to PNG.

### Desktop against headless (`pattern-desktop-difference.png`)

- `ppm_compare` output for the same scene: a dimmed copy of the headless image
  with any differing pixel in red. It contains no red: all 912 000 pixels are
  identical. The default and reflected scenes are identical too.
- Generated by the same CTest run (`transformed-difference.ppm`).
### 2D transformations before and after (`transforms.png`)

- Author: Waruhiu Jeremy Kang'ethe, 166263.
- Algorithms: 3 x 3 homogeneous matrices for translation, rotation about a
  pivot, non-uniform scaling and x/y reflection, composed in the shared order
  `T(translation) * T(pivot) * R * F * S * T(-pivot)`. Drawn with the CP1
  scanline filler and Bresenham lines.
- Source commit: `77b3738` on `feat/cp2-transforms`.
- Command: `transforms_demo outputs/transforms.ppm`, converted to PNG for viewing.
- Project component: an asymmetric "F" motif with a zigzag stroke, shown
  untransformed and then translated, rotated +30 degrees (clockwise on screen),
  scaled 1.6 x 0.6, reflected in x and in y, with all transforms combined, and
  with the same two factors multiplied in both orders. Each panel shows the
  original as a ghost, the pivot and the images of the local axes. See
  [transformations.md](../algorithms/transformations.md).

## Checkpoint 2 evidence: pattern animation

### Rotation and pulse frames (`animation-frames.png`)

- Author: Sayialel Eric Lenina, 161492.
- Algorithms: time-based evaluation from base settings,
  `angle(t) = baseAngle + speed * t` and
  `scale(t) = baseScale * (mid + amplitude * sin(2 pi f t))`, sampled at exact
  frame times `t = frameIndex / fps`. The pattern is drawn by the pattern engine
  with the shared matrix order. See [animation.md](../algorithms/animation.md).
- Source: `feat/cp2-animation` (`src/animation.cpp`, `src/animation_demo.cpp`).
- Commands, from a configured `build-core`:

  ```sh
  ./build-core/animation_demo --mode rotate
  ./build-core/animation_demo --mode pulse --motif 2 --palette 1
  ```

  Frames 0, 15, 30 and 45 of each 60-frame, 30 fps sequence were scaled to a
  third and placed side by side with Pillow.
- Top row: the diamond motif rotating at 45 degrees per second (0, 22.5, 45 and
  67.5 degrees). At 1 s each diamond has turned into an axis-aligned square.
- Bottom row: the cross motif pulsing between 0.75x and 1.25x once every 2 s
  (factor 1.0, 1.25, 1.0, 0.75).
- In every frame the mannequin, dress outline and tile grid stay fixed, and the
  fabric and dress show the same animated pattern. Motif and palette numbers are
  the 0-based command-line indices. The on-screen footer counts palettes from 1.
