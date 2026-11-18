# Browser extension verification

Verified locally on macOS with Node 22.19.0 and installed Google Chrome.

- `npm ci`: reproducible installation using the lockfile.
- `npm test`: 15 passing tests covering the complete motif catalogue, themes and
  swatches, 32-colour project validation, contrast mathematics, scoring bounds,
  project validation, pose-to-garment anchors and rotated picking.
- `PLAYWRIGHT_CHROME=1 npm run test:browser`: 13 passing browser scenarios.
  These cover editable geometry/freehand, undo/redo, animation, palette display,
  save/open JSON, fabric/3D/photo PNG downloads, actual pose inference using the
  bundled model, manual correction, no-person fallback, missing-model recovery
  and retry, malformed project handling and mobile overflow, multiple-accent
  round trips and removal/undo, enlarged fitting controls, brush undo,
  neckline exposure, mask protection/restoration and uniform print spacing.
  Resize coverage includes corner/edge drags, scaled mobile coordinates,
  Shift aspect locking, undo/redo, rotated geometry, opposite-anchor stability,
  limits and freehand point scaling.
  Inspector updates are deferred until the end of a drawing gesture, avoiding
  mobile canvas displacement when a layer is added.
  3D coverage checks bounded smooth profile sampling, closed geometry, matching
  back-seam normals, physical UV scaling, fixed repeat textures, spacing updates,
  floor-only shadows and actual WebGL rendering. Front, close-up and side
  screenshots inspected after the dress rendering corrections.
  Library coverage loads all ten designs, checks five distinct silhouettes,
  validates editable projects, checks independent copies, exercises undo and
  JSON persistence, and verifies older project compatibility. Export coverage
  reads downloaded files, checks PNG signatures and dimensions for fabric,
  garment, 3D and photo images, tests retry links and failed encoding feedback.
- `npm run build`: production build passes.
- `npm audit`: no reported vulnerabilities at verification time.
- `cmake --build build-presentation -j 4` and
  `ctest --test-dir build-presentation --output-on-failure`: 19 passing C++/OpenGL
  tests, including desktop/headless rendering comparisons, rechecked before the
  library/export update was published to main.
- Desktop, photo-fitting and 390px mobile screenshots inspected.
- In-app browser verification on 2026-11-18: exported `kitenge-fabric.png` to
  Downloads, then checked its PNG signature and 720 × 480 dimensions. The
  ten-set library and prepared-file save/preview receipt were also inspected.

Tests verify function and outputs, not photorealism or physical fitting.
The official pose test photograph is a person in a wide yoga stance, so it
tests landmark inference and manual editing rather than ideal garment fit.
Use a clear, front-facing standing photo for the presentation.
