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

1. Choose a preset or clear the motif. Draw strokes or drag out rectangles,
   ellipses, triangles and diamonds. Select a shape to move it, change its size,
   rotation, fill, stroke and palette role. Layers provide keyboard selection.
2. Undo/redo, edit three colours, adjust pattern rotation, scale, spacing and
   reflection. Play rotation or pulse animation. Export the cloth as PNG.
3. Save/open project JSON to keep editable geometry and settings. Project files
   deliberately exclude photos. Nothing is automatically uploaded or persisted.
4. Orbit the generated 3D mannequin wearing the same design; export its view.
5. Choose Photo fitting and a front-facing full-body image. MediaPipe's local
   model estimates shoulders/hips, then positions a new dress. Drag six handles
   or use labelled percentage inputs for manual correction. Export without handles.

The photo stays in memory until reload/replacement. Supported formats are JPG,
PNG and WebP up to 20 MB; long edges are downsampled to 1600px. Local bundled
WASM/model files avoid runtime calls to a remote model host. Serving the app still
requires a local server or website; opening index.html via file:// is unsupported.

## Graphics and architecture

`src/core.js`: editable vector motif model, affine Canvas transforms, tiled cloth,
hit testing, project validation and palette metrics. Harmony compares hue-pair
distances with analogous/complementary/triadic templates. Readability normalizes
three luminance contrast ratios (4.5 primary/background, 4.5 accent/background,
3 primary/accent). The overall score weights both equally. These are disclosed
design heuristics, not claims of objective beauty or WCAG compliance for fabric.

`src/photo.js`: MediaPipe Pose Landmarker in IMAGE mode with a local model. It
requires sufficiently visible shoulders and hips. On failure it reports that
manual correction is needed. Six normalized anchors define two garment panels;
four textured triangles map the cloth to the photo with affine transforms.
Pose-guided forearm recompositing is optional. It is not pixel-perfect body
segmentation, realistic cloth draping, body measurement or physical fitting.
An uploaded image is a static photo, not a live-camera stream.

`src/model.js`: Three.js procedural mannequin and lathed dress with UV-mapped
CanvasTexture, lighting, orbit controls and PNG export. It is a generated model
wearing the design, not an AI reconstruction of the photographed individual.

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
   six handles. Explain pose landmarks, normalized anchors and the four affine
   triangle maps in `photo.js`. Try another image or manual reset if detection
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
