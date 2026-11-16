# Browser extension verification

Verified locally on macOS with Node 22.19.0 and installed Google Chrome.

- `npm ci`: reproducible installation using the lockfile.
- `npm test`: 5 passing tests covering contrast mathematics, scoring bounds,
  project validation, pose-to-garment anchors and rotated picking.
- `PLAYWRIGHT_CHROME=1 npm run test:browser`: 4 passing browser scenarios.
  These cover editable geometry/freehand, undo/redo, animation, palette display,
  save/open JSON, fabric/3D/photo PNG downloads, actual pose inference using the
  bundled model, manual correction, no-person fallback, missing-model recovery
  and retry, malformed project handling and mobile overflow.
- `npm run build`: production build passes.
- `npm audit`: no reported vulnerabilities at verification time.
- Existing `ctest --test-dir build-presentation --output-on-failure`: 19 passing
  C++/OpenGL tests, including desktop/headless rendering comparisons.
- Desktop, photo-fitting and 390px mobile screenshots inspected.

Tests verify function and outputs, not photorealism or physical fitting.
The official pose test photograph is a person in a wide yoga stance, so it
tests landmark inference and manual editing rather than ideal garment fit.
Use a clear, front-facing standing photo for the presentation.
