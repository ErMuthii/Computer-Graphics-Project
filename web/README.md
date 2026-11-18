# Browser Kitenge Pattern Studio

## Run

Node 22.12 or newer is required (tested with 22.19). From `web/`:

```sh
npm ci
npm run dev
```

Open the printed localhost URL. `npm run build` produces `dist/`; `npm run preview`
serves it locally. Deploy the contents of `dist/` to an HTTPS static host. No
backend, login or camera permission is required. For a subdirectory deployment,
build with `npm run build -- --base=/your-path/`.

## Complete workflow

Open **Library · 10 sets** to start with a finished fabric and matching dress.
The sets are Rust diamonds, Indigo lattice, Sunburst gold, Coastal waves, Plum
rosette, Forest leaves, Coral cowries, Midnight stars, Earth chevrons and Teal
fans. **Use design** loads editable shapes, colours, spacing and a silhouette;
Undo restores your previous design. The five silhouettes are A-line, Maxi,
Midi, Shift and Fit & flare. The silhouette is saved in project JSON; older
project files open with A-line. Photo alignment and correction masks are kept
when switching designs, while the fabric updates.

1. Choose one of 18 editable motifs or clear the motif. The library includes
   Diamond, Triangle, Cross, Steps, Sunburst and Bézier arch from the native
   catalogue, plus Flower, Leaf, Star, Spiral, Chevron, Rosette, Rings, Fan,
   Wave, Cowrie, Seed cluster and Hexagon. Draw strokes or drag out rectangles,
   ellipses, triangles and diamonds. Select a shape to move it, change its size,
   rotation, fill, stroke and palette role. Layers provide keyboard selection.
   After creating a geometric shape, Select / move activates automatically.
   Drag its corner handles to resize both dimensions or its edge handles to
   change one dimension. Hold Shift while dragging a corner to keep proportions.
   Drag inside to move. The same handles work on rotated shapes and selected
   freehand strokes. A complete drag is one undo/redo action.
2. Choose among 24 themes and 84 swatches, or enter custom hex colours. Each
   design has a background, a primary and up to 30 accents (32 slots total).
   Select a drawing colour, then draw or recolour a selected layer. Add accents;
   removing one remaps its shapes to Primary and is undoable. Applying a theme
   retains extra accents. Adjust rotation, scale, spacing and reflection, or
   play/pause animation. Export the cloth as PNG.
3. Save/open project JSON to keep editable geometry and settings. Project files
   deliberately exclude photos. Nothing is automatically uploaded or persisted.
4. Orbit the generated 3D mannequin wearing the same design; export its view.
   Choose **Dress silhouette** to change its shape, or **Export dress PNG** for
   a transparent, garment-only illustration at 800 × 1200 pixels.
5. Choose Photo fitting and a front-facing full-body image. MediaPipe's local
   model estimates shoulders/hips, then positions a new dress. Drag six handles
   or use labelled percentage inputs for manual correction. Enlarge the fitting
   view, choose Round/V/Boat neckline, tune neck depth and print size, and toggle
   soft shading. Export without handles.
6. Use **Protect photo** to brush original hands, hair or accessories in front
   of the dress. **Restore dress** removes protected areas; **Undo brush** undoes
   the last stroke and **Clear corrections** resets the mask. Original forearm
   preservation is optional and off by default because it also retains sleeves.

The photo stays in memory until reload/replacement. Supported formats are JPG,
PNG and WebP up to 20 MB; long edges are downsampled to 1600px. Local bundled
WASM/model files avoid runtime calls to a remote model host. Serving the app still
requires a local server or website; opening index.html via file:// is unsupported.

## Export and save

**Export fabric PNG**, **Export dress PNG**, **Export 3D view** and **Export fitted
photograph** produce PNG files. Exports use `canvas.toBlob()` and live Blob URLs
instead of data-URL downloads, which can be ignored by embedded browsers.
The 3D scene is rendered immediately before capture. Photo export uses a separate
canvas without alignment handles. Project save downloads editable JSON.

Every prepared file also appears in a receipt at the top of the studio. Check
your browser's Downloads; if no file appears, choose **Save file again**. For
images, **Open preview** offers an additional save route. The receipt keeps its
URL valid until it is closed or replaced. Encoding failures show an error rather
than reporting that a file was saved. Downloads depend on browser settings;
the app reports that a download was requested, not that the operating system
successfully saved a file.

## Graphics and architecture

`src/catalog.js`: 18 editable vector motif factories, 24 themes and 84 swatches.
Curved presets sample cubic Bézier segments or parametric curves locally.

`src/library.js`: ten editable project factories, paired thumbnail rendering and
transparent garment illustration export. `src/garment-styles.js` holds the five
profile definitions shared by illustrations and the 3D mesh. Library previews
are generated from the actual editable geometry, not stock photographs.

`src/exports.js`: PNG Blob encoding, persistent object-URL download links, an
image preview receipt and resource cleanup.

`src/core.js`: editable vector motif model, affine Canvas transforms, tiled cloth,
hit testing, project validation and palette metrics. Harmony compares hue-pair
distances with analogous/complementary/triadic templates. Readability normalizes
all active colour pairs: 4.5 for foreground/background and 3 for foreground pairs.
The lowest-contrast pairs are shown alongside a pair count. The overall score
weights harmony and readability equally. These are disclosed
design heuristics, not claims of objective beauty or WCAG compliance for fabric.

`src/selection.js`: eight resize handles, hit testing, directional cursors and
anchor-preserving resize mathematics. Pointer positions are transformed into
the selected shape's local axes; the opposite edge/corner remains fixed as
dimensions change. Shift constrains the ratio, and freehand points scale from
the original gesture snapshot. Numeric width/height fields stay synchronized.

`src/photo.js`: MediaPipe Pose Landmarker in IMAGE mode with a local model. It
requires sufficiently visible shoulders and hips. On failure it reports that
manual correction is needed. Pose estimates the natural waist above the hips
and the hem from visible knees/ankles or a torso-based fallback. Six normalized
anchors define a dress silhouette with curved armholes and an adjustable
neckline. A clipped CanvasPattern samples one exact fabric grid period with a
uniform transform over the entire dress, avoiding the stretched motifs and
waist discontinuities of the previous four-triangle mapping. Optional edge
shading adds simple depth. A local alpha mask reveals original pixels using
normalized correction strokes, with restore, undo and clear operations.
Pose-guided forearm recompositing is optional. It is not pixel-perfect body
segmentation, realistic cloth draping, body measurement or physical fitting.
An uploaded image is a static photo, not a live-camera stream.

`src/model.js`: Three.js procedural mannequin with a fixed-resolution 512px
CanvasTexture containing one exact repeat tile, lighting, orbit controls and PNG
export. Print repetition follows tile spacing. Only the plinth receives cast
shadows; the dress uses smooth material lighting to avoid shadow-map acne.
It is a generated model wearing the design, not an AI reconstruction of the
photographed individual.

`src/dress-geometry.js`: shape-preserving cubic interpolation of the dress
profile, sampled at 97 height levels with 96 circumference segments. UVs use
physical height and local arc length rather than profile-row index. The print
is centered at the front; its construction seam is at the back, with matching
vertex normals. Subtle skirt folds are geometric, not a cloth simulation. This
replaces the coarse profile and whole-preview texture that produced visible
bands, abrupt changes in print scale and dense self-shadow artifacts.

`src/main.js`: state, bounded undo history, editor interactions, file handling
and shared updates to fabric, photo and 3D views. No private user data is sent to
a server. Reloading closes unsaved work; download a project first.

The original C++/OpenGL application remains in the repository. This browser
frontend implements its own Canvas vector transforms and raster rendering; it
does not execute the original C++ algorithms through WebAssembly. CP1/CP2 code
and tests remain the evidence for hand-written C++ rasterization. The boundary
is explicit so presentations do not misattribute browser rendering to C++.

## Verification

```sh
npm test
npx playwright install chromium
npx playwright test
npm run build
```

If Google Chrome is already installed, `PLAYWRIGHT_CHROME=1 npm run test:browser`
uses it instead of downloading a separate Chromium build (PowerShell:
`$env:PLAYWRIGHT_CHROME='1'; npm run test:browser`).

The browser tests exercise shape editing, freehand input, undo/redo, JSON round
trip, PNG downloads, real pose inference on the official sample photograph,
manual fit correction, WebGL initialization, invalid input and mobile overflow.
They also exercise animation, 3D export, a photograph with no detected person,
model download failure and a successful detection retry.
New scenarios cover the motif/theme/swatch libraries, multiple-accent project
round trips and deletion, fitting controls, brush undo, neckline cutouts,
alpha-mask restore and consistent print spacing at the bust and skirt.
Resize tests cover corners, edges, rotation, opposite-anchor stability, size
limits, freehand scaling, aspect locking, mobile coordinates and undo/redo.
3D tests check finite closed geometry, seam normals, physical UV coordinates,
texture repeat size, spacing updates and floor-only shadow reception. Front,
close-up and side renders are inspected. See [updated 3D preview](docs/3d-preview.png).
Generated screenshots and test reports are ignored working files.

## Technical demonstration

1. Keep the original C++ demonstration available for the hand-written raster
   algorithms. Launch this browser studio for the final editing workflow.
2. Draw a rectangle, ellipse and freehand stroke. Select a layer, resize/rotate
   it, recolour it, and show undo/redo. Explain local shape coordinates and
   Canvas translation/rotation during rendering (`paintShape` in `core.js`).
3. Change spacing, scale and reflection. Play/pause animation. Explain the tile
   loop in `renderPattern` and its time-based rotation or sinusoidal scale.
4. Change palette colours and show the score breakdown. Explain the hue-angle
   template comparison and the linearized sRGB luminance contrast calculation.
5. Orbit the mannequin. Explain the lathed dress mesh, UV coordinates, shared
   CanvasTexture and lighting in `model.js`. A preset changes all views.
6. Upload a clear, front-facing photograph, wait for detection, then correct the
   six handles. Explain pose landmarks, normalized anchors, the curved dress
   clipping path and uniform repeating texture in `photo.js`. Show neckline and
   print-size controls, then the protect/restore brushes. Try another image or manual reset if detection
   cannot locate shoulders and hips.
7. Export cloth, photo and 3D view. Save a project, clear the motif, then reopen
   the file to demonstrate that it preserves editable shapes and settings.

No live-camera alignment or photorealistic virtual try-on is claimed. The dress
is an explicit new overlay on the person, not a recolouring of their old dress.

## Sources and assets

- [MediaPipe Pose Landmarker web guide](https://ai.google.dev/edge/mediapipe/solutions/vision/pose_landmarker/web_js)
- [Official pose model](https://storage.googleapis.com/mediapipe-models/pose_landmarker/pose_landmarker_lite/float16/1/pose_landmarker_lite.task)
- `tests/person.jpg`: official [MediaPipe sample](https://storage.googleapis.com/mediapipe-assets/pose.jpg), for automated validation only.
- Three.js: https://threejs.org/ (MIT). MediaPipe Tasks Vision: https://github.com/google-ai-edge/mediapipe (Apache-2.0). Preserve distributed license files.
- The application creates all mannequin/garment geometry in code. It uses no
  downloaded human model or paid assets.

Limitations: side-facing/occluded/multiple people may not align correctly; use
manual correction or another image. Crossing limbs, loose clothing, lighting and
perspective can make the flat overlay visibly approximate. Old garments are not
erased. This is a visual dress overlay, not generative photo editing.
