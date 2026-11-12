# CP2 and CP3 implementation guide

This is the team's implementation plan, not a claim that the planned features
already work. The desktop starter and the existing CP1 algorithms are the baseline.
The six members implement the modules below under their own Git identities.

## Navigation

- [Scope and current state](#scope-and-current-state)
- [Getting started and Git workflow](#getting-started-and-git-workflow)
- [Shared contracts and integration order](#shared-contracts-and-integration-order)
- [Jeremy: transformation mathematics](#jeremy-transformation-mathematics)
- [Eric: application and integration](#eric-application-and-integration)
- [Deborah: motifs, palettes and mannequin](#deborah-motifs-palettes-and-mannequin)
- [Kelvin: repetition and garment clipping](#kelvin-repetition-and-garment-clipping)
- [Lenina: animation and frame generation](#lenina-animation-and-frame-generation)
- [Andrew: interaction and curve integration](#andrew-interaction-and-curve-integration)
- [Checkpoint acceptance and presentation](#checkpoint-acceptance-and-presentation)

## Scope and current state

The CP3 target is a desktop Kitenge Pattern Studio with selectable predefined
motifs, palettes, translation, rotation, scaling, reflection, pattern animation
and a static 2D mannequin wearing a patterned dress. Fabric and dress respond to
the same selected motif and settings. The garment silhouette stays fixed.

CP2 delivers the mathematics and animation in Pure C++, with PPM images and
numbered frame sequences. CP3 connects that work to the existing OpenGL window.
OpenGL presents the software canvas, while the team's C++ code calculates and
rasterizes the graphics. Confirm any additional lecturer requirement for native
OpenGL geometry before extending the display architecture.

Excluded through CP3: freehand editor, arbitrary garment editing, 3D human model,
cloth simulation, camera/body detection, AI motifs, online collaboration and
automatic palette scoring. No extra calendar deadlines are assigned here.

### What exists now

| Existing component | Entry point |
| --- | --- |
| Canvas and geometry types | `include/kitenge/canvas.hpp`, `types.hpp` |
| CP1 algorithms | `lines`, `circles`, `curves`, `polygon_fill`, `polygon_clip` modules |
| Shared state/actions/hooks | `include/kitenge/studio.hpp` |
| Fixed motif, palette, mannequin and repetition scaffold | `src/studio.cpp` |
| Window, shader, RGB texture display, keyboard callbacks | `src/desktop_main.cpp` |
| Headless scene export | `src/studio_render.cpp` |
| Regression tests | `tests/`, seven CTest suites at starter publication |

The starter supports M/P/R/S/Esc. It has three fixed motifs and three palettes.
`TogglePlayback`, `playing`, `timeSeconds` and the animation hook exist, but no
visible animation or Space binding exists yet. Matrix transforms, reusable motif
data, general repetition controls and new interaction parameters are future work.

The fixed starter dress uses the CP1 fill and clipping code. This does not finish
Kelvin's reusable pattern engine or Deborah's motif/mannequin modules. Preserve
accurate authorship of the CP1 work and describe new contributions separately.

## Getting started and Git workflow

1. Read this guide and [desktop setup](../desktop-starter.md).
2. Pull the published baseline and build it before changing code.
3. Check `git config user.name` and `git config user.email`. Use your own name and
   a GitHub-verified email or your own GitHub no-reply email.
4. Create the branch listed in your section. Never develop on another member's branch.

```sh
git switch main
git pull --ff-only origin main
git switch -c feat/cp2-transforms
```

The last command is Jeremy's example; substitute your own branch. If a branch
already exists, switch to it rather than recreating it.

### Baseline commands

Run from the repository root. The core build has no OpenGL/GLFW dependency:

```sh
cmake -S . -B build-core -DKITENGE_BUILD_DESKTOP=OFF -DCMAKE_BUILD_TYPE=Debug
cmake --build build-core --parallel
ctest --test-dir build-core --output-on-failure
./build-core/studio_render outputs/studio.ppm
```

For Eric's macOS SDK workaround and Windows/MSYS2 setup, use the setup guide.
On Windows the executables have `.exe`; multi-configuration generators also use
a configuration subdirectory such as `Debug/` and `ctest -C Debug`.

After installing the platform dependencies in the setup guide:

```sh
cmake -S . -B build-desktop -DKITENGE_BUILD_DESKTOP=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build-desktop --parallel
ctest --test-dir build-desktop --output-on-failure
./build-desktop/kitenge_studio
```

Current controls: M motif, P palette, R reset, S PPM export, Esc close. A GUI
smoke test is separate from CTest and needs a graphical login:

```sh
./build-desktop/kitenge_studio --smoke-test --capture outputs/desktop-frame.ppm
```

That check scripts the callbacks; also test real key presses in the application.

### Submitting work

Implement small, complete steps and commit them as they happen. Explicitly add
your changed source, tests and documentation, inspect `git diff --cached`, then:

```sh
git commit -m "feat(transforms): add pivot rotation and reflection"
git push -u origin HEAD
```

Use a truthful scope/message for your actual changes. Open a PR into `main` with
the existing PR template. Include build/test commands, graphics evidence, inputs,
known limitations and your personal contribution. Request a teammate's review
through the group's normal process. Resolve review comments before Eric merges.
Do not push directly to main as a routine member workflow.

Keep `build*/` and generated `outputs/` out of commits. Curate selected evidence
under `docs/evidence/` with authorship, source commit and reproduction commands.
Do not fabricate commit history or attribute someone else's implementation to yourself.

## Shared contracts and integration order

### Existing API

`renderStudio(canvas, state, hooks)` draws the scene. `advanceStudio(state, dt,
hooks)` updates time and calls `hooks.animate` only while playing.
`hooks.fabric` and `hooks.garment` replace the corresponding starter interiors;
`hooks.animate` updates state. None of these hooks should include GLFW/OpenGL.
Eric wires completed modules into `App::hooks` and the headless renderer.

### Contract-first work required before dependent PRs merge

The following is a design specification to implement, not an existing header API.
Eric coordinates the shared header change with the module owners first:

- Jeremy publishes `Mat3` and a transform parameter type with translation, angle
  in radians, positive scale, pivot and two reflection flags. Use column vectors
  and document that screen y points down, so positive mathematical rotation
  appears clockwise on this canvas.
- Deborah publishes motif data in local coordinates centred at `(0,0)`, nominally
  within `[-25,25]` on both axes. A motif contains ordered filled polygon contours
  and open/closed stroked paths with palette-role indices. Publish motif/palette
  counts and labels so nobody duplicates hardcoded `%3` or name arrays.
- Filled contours must be simple and have no holes. Preserve concave contours
  where supported. Curve and round decorations can be sampled to paths before
  general affine transforms. Document the approximation rather than claiming
  the midpoint rasterizer supports arbitrary rotated ellipses. Keep CP1 midpoint
  and Bézier demos available as separate algorithm evidence.
- Deborah publishes the mannequin's convex bodice/skirt boundaries once. Kelvin
  consumes the same geometry that the scene outlines; remove duplicated panels.
- Kelvin publishes a Pure C++ pattern renderer that accepts motif data, palette,
  transform parameters, tile pitch, pattern origin and a convex destination boundary.
- Lenina publishes animation mode/speed and a way to derive animated parameters
  from base settings plus `timeSeconds`. Preserve base settings so repeated
  updates do not accumulate transformations onto already transformed geometry.
- Andrew consumes the shared settings through actions. Eric owns final changes
  to `StudioState`, scene composition, CMake wiring and hook registration.

Use radians in shared maths. Show degrees in the UI with explicit conversion.
For a local point `p`, the agreed transform order is:

```text
p_canvas = T(patternOrigin + tileOffset) * T(pivot)
           * R(angle) * F(reflection) * S(scale) * T(-pivot) * p
```

The rightmost operation acts first. Kelvin adds each tile translation after the
local transform. Pattern translation changes the pattern's origin, not the dress.
All intermediate geometry uses doubles. Round only for rasterization.

Do not invent separate versions of these shared structures in different branches.
If a module needs an interface change, agree it with Eric and the consumer before
merging. Owners may work against local fixtures until dependencies land.

### Integration sequence

| Order | Deliverable | Dependency |
| --- | --- | --- |
| 1 | Eric merges shared contracts with owners | Runnable starter |
| 2 | Jeremy's transforms and Deborah's motif/palette/panel data | Shared contracts; these can proceed independently |
| 3 | Kelvin's reusable fabric/garment renderer | Transform and motif modules |
| 4 | Lenina's deterministic animation and frame export | Settings contract, then renderer for visual output |
| 5 | Andrew's controls and curve fixture integration | Stable actions/settings and motif representation |
| 6 | Eric integrates the CP2 headless showcase | All core modules |
| 7 | Team connects and verifies CP3 desktop behaviour | CP2 showcase and existing OpenGL presenter |

Andrew can develop action tests early. Lenina can develop timing tests early.
Eric serializes shared CMake/header edits and keeps main buildable after each merge.

## Jeremy: transformation mathematics

**Waruhiu Jeremy Kang'ethe — 166263**

Own `include/kitenge/transforms.hpp`, `src/transforms.cpp`,
`tests/transform_tests.cpp`, and `docs/algorithms/transformations.md` (new files).
Use `feat/cp2-transforms`, then `feat/cp3-transforms-integration` for follow-up work.

### CP2 steps

1. Implement a 3 x 3 homogeneous matrix, identity, multiplication and point application.
2. Implement translation, rotation, nonuniform scaling, x/y reflection and pivot
   composition using the shared order. Keep geometry independent of Canvas/GLFW.
3. Transform polygons and stroked paths without mutating the source motif.
4. Reject nonfinite inputs. Interactive scale should stay positive; use explicit
   reflection flags for mirroring. Explain the difference between reflecting
   across the x-axis (negate y) and across the y-axis (negate x).
5. Produce a Pure C++ before/after demo using the existing rasterizers. Draw the
   pivot/axes so the effect is explainable, not just a different screenshot.
6. Register unit tests with Eric and document hand-calculated examples.

### CP3 steps

Work with Kelvin to apply matrices before tiling/clipping and with Andrew to
connect controls. Verify the same input settings produce the same geometry in
the headless renderer and desktop. Never substitute `glRotate`/shader maths for
the C++ transformation implementation being demonstrated.

### Tests and completion

Test identity, translation `(2,3)+(4,-1)=(6,2)`, a 90-degree rotation of `(1,0)`
to `(0,1)`, separate x/y scales, each reflection, pivot invariance, inverse rotation,
and the difference between translate-then-rotate and rotate-then-translate.
Use a floating tolerance. Check source vertices remain unchanged and invalid
values fail consistently. Completion means tests plus a reproducible visual demo
and working CP3 controls after integration.

For the showcase, explain homogeneous coordinates, multiplication order, pivot
handling and one verified numerical example. Record actual challenges as they occur.

## Eric: application and integration

**Muthii Eric Macharia — 166390**

Own `studio.hpp`, `src/studio.cpp`, `src/desktop_main.cpp`,
`src/studio_render.cpp`, `CMakeLists.txt`, setup docs and integration tests.
The published `codex/desktop-starter` work establishes the baseline. Use
`feat/cp2-scene-integration` and `feat/cp3-prototype-integration` for later work.

### CP2 steps

1. Merge the shared contracts before consumers depend on them. Retain the
   dependency-free default build and all CP1 executable/test names.
2. Add each new module as a CMake target with explicit links, compiler warnings
   and a CTest target. Keep OpenGL dependencies exclusive to the desktop target.
3. Replace fixed starter composition with Deborah's geometry, Kelvin's pattern
   renderer and Lenina's animation settings. Use one composition path for both
   `studio_render` and the desktop so their results do not diverge.
4. Extend the headless CLI to select motif/palette/transform inputs and demonstrate
   the team modules. Agree argument spelling with owners and document it only
   after implementation. Keep the current one-output-path invocation working.
5. Verify fabric and dress change together, clipping remains correct and default
   state/reset stays coherent. Remove stale hardcoded three-item validation only
   after switching it to actual catalog sizes.
6. Collect a clean CP2 build and evidence set without invoking OpenGL.

### CP3 steps

1. Register the completed hooks and keyboard actions. Preserve dirty-state redraw
   after settings changes, and redraw continuously only when animation needs it.
2. Retain shader/link error reporting, RGB upload alignment, top-left orientation,
   Retina framebuffer handling and aspect-preserving resize.
3. Confirm an export failure keeps the window running. Test closing, minimize/
   restore, reset during playback and changes while playback is paused.
4. Extend the smoke test for new actions; keep it separate from headless CTest.
5. Ask teammates to run the documented commands on their actual machines. Record
   platform/toolchain results; do not infer Windows/Linux success from macOS.

Completion means a clean clone builds, all regression tests pass, and the CP3
prototype performs the agreed demonstration. Explain the scene-to-pixel-buffer-
texture pipeline and exactly which work OpenGL performs. Your integration
ownership does not transfer authorship of teammates' algorithms.

## Deborah: motifs, palettes and mannequin

**Deborah Rehana — 168656**

Own new `motifs.hpp/.cpp`, `mannequin.hpp/.cpp` under `include/kitenge/` and `src/`,
`tests/motif_tests.cpp`, `tests/mannequin_tests.cpp` and `docs/algorithms/motifs.md`.
Use `feat/cp2-motif-model-data`, then `feat/cp3-motif-model-integration`.

### CP2 steps

1. Move fixed motif/palette data out of the private helpers in `studio.cpp` into
   the agreed reusable catalog. Publish labels and counts along with data.
2. Deliver at least three visually distinct motifs. Keep local geometry centred
   at the origin and assign palette roles rather than hardcoding RGB per vertex.
   Include a curve/round decorative variant in collaboration with Andrew, after
   the basic polygon motif contract works.
3. Define at least three palettes with background, primary and accent roles.
   Verify each role maps consistently on fabric and garment; automatic scoring
   is outside scope.
4. Extract the static mannequin and its dress panels into one shared geometry
   source. Keep head/limbs unpatterned and the bodice/skirt separately convex.
5. Keep motif geometry independent of tile coordinates. Kelvin determines where
   each copy goes. Keep mannequin movement out of the motif transform controls.
6. Render a motif catalog and an unpatterned mannequin using CP1 rasterizers,
   then supply the data to the integrated scene.

### CP3 steps

Remove stale motif/palette name arrays with Eric, verify all catalog choices
appear through Andrew's controls, and check every palette/motif on the model.
Use the same shared panel polygons for clipping and outlines. Refine the static
silhouette for readability without adding a 3D model or fit simulation.

### Tests and completion

Check every contour is finite, valid and nonzero-area; every palette-role index
is in range; each label/count matches the catalog; each garment panel is convex;
and catalog lookups handle invalid indices consistently. Check reflections of
asymmetric motifs with Jeremy. Completion includes catalog images, the mannequin
render and the integrated selection result. Explain polygon filling, local
coordinates, palette assignment and how fixed garment geometry defines clipping.

## Kelvin: repetition and garment clipping

**Murega Kelvin Mutwiri — 166914**

Own new `pattern.hpp`, `src/pattern.cpp`, `tests/pattern_tests.cpp` and
`docs/algorithms/pattern-generation.md`. Maintain the existing clipping module
where a verified integration issue requires it. Use `feat/cp2-pattern-engine`,
then `feat/cp3-pattern-model-integration`.

### CP2 steps

1. Implement one reusable renderer accepting the agreed data/settings and a
   convex destination boundary. Fill the destination background first.
2. Enumerate grid cells and transform a fresh local motif for each cell using
   Jeremy's matrix module. Validate positive finite tile pitches and constrain
   UI scale/spacing to keep the work bounded.
3. Compute a conservative tile range that includes copies crossing viewport
   edges after rotation/scale/translation. Test edge cases rather than assuming
   that untransformed motif dimensions are always sufficient.
4. Clip filled polygons using CP1 Sutherland-Hodgman and rasterize them using the
   scanline filler. For open strokes, clip each segment against the convex
   destination or use a panel mask. Rectangle-only line clipping is insufficient
   for slanted dress edges. Document whichever approach you implement.
5. Use the shared pattern origin/settings for both dress panels so the fabric
   does not restart with a different phase at the waist. The fabric preview and
   model show the same repeat design, even though their screen regions differ.
6. Feed fabric/garment hooks with that renderer. Keep head, limbs and background
   outside the garment unchanged. Document clipping of concave motif contours;
   ensure clipped results work with the fill validator. If clipping can create
   disconnected pieces, handle/decompose them rather than silently losing them.

### CP3 steps

Verify motif, palette and transformation changes update both views immediately.
Keep the renderer independent of OpenGL. Compare the desktop result to the
headless result at equal time/settings. Profile large settings before adding
unbounded density controls.

### Tests and completion

Test fully inside/outside/crossing motifs, rotated and reflected edge tiles,
invalid pitches, continuous placement across the waist, and no pattern pixels
outside the destination. Include a concave motif and a stroked curve near a
slanted edge. Completion requires before/after clipping evidence, a transformed
swatch and a patterned mannequin from the same renderer. Explain tile offsets,
matrix order, clipping and the difference between bounds-safe writes and geometric clipping.

## Lenina: animation and frame generation

**Sayialel Eric Lenina — 161492**

Own new `animation.hpp`, `src/animation.cpp`, `src/animation_demo.cpp`,
`tests/animation_tests.cpp` and `docs/algorithms/animation.md`.
Use `feat/cp2-animation`, then `feat/cp3-animation-playback`.

### CP2 steps

1. Implement two showcase modes: continuous motif rotation and bounded scale
   pulsing. Keep the mannequin static. Publish mode, speed and pulse range in
   the shared settings contract.
2. Derive the displayed transform from base parameters plus elapsed seconds.
   Use `angle = baseAngle + angularSpeed * time`; use a sinusoidal scale factor
   with a positive lower bound for pulsing. Do not repeatedly transform already
   transformed vertices or overwrite the user's base settings each frame.
3. Use `StudioHooks::animate` to update state, or expose a pure evaluation function
   that Eric calls while composing the scene. Agree one time owner: the existing
   `advanceStudio` already increments `timeSeconds`, so the hook must not add dt again.
4. Provide a headless animation demo with configurable frame count/output folder
   and a documented frame rate. Sample exact times `frameIndex / fps`, output
   zero-padded PPM filenames and reject invalid frame counts/rates. Avoid silently
   overwriting another evidence sequence; document overwrite handling.
5. Use 60 frames at 30 fps as a short acceptance fixture, not a real-time benchmark.
   Save representative frames and provide reproduction commands.

### CP3 steps

Connect the same evaluator to the desktop clock. Coordinate Space play/pause,
mode/speed controls and reset with Andrew. Pause freezes the visual state;
resume continues without a time jump. R restores base settings and time zero.
Document the existing desktop dt cap (0.1 seconds) and test minimize/restore.

### Tests and completion

Check time-zero output, known quarter/full rotation times, pulse min/max and
positive scale, invalid inputs, paused time, reset, and approximately equal
results at equal elapsed time for different dt partitions. Compare headless and
desktop transform results, not just screenshots. Completion includes a reproducible
frame sequence plus live playback and pause/reset. Explain timing equations,
frame sampling and why frame count must not determine animation speed.

## Andrew: interaction and curve integration

**Andrew Karanja Gathirwa — 167144**

Own new `controls.hpp`, `src/controls.cpp`, `tests/control_tests.cpp` and
`docs/controls.md`. Coordinate changes to `StudioAction`, `StudioState` and the
desktop key callback with Eric. Contribute curve motif data with Deborah.
Use `feat/cp2-controls-curves`, then `feat/cp3-interaction`.

### CP2 steps

1. Implement a Pure C++ action reducer over the agreed state. No GLFW key codes
   belong in the maths/action layer. Preserve the existing M/P/R/S/Esc behaviour.
2. Add actions for translation, rotation, uniform scale, x/y reflection and
   tile spacing. Define limits and steps in one documented place, using radians
   internally and degrees in labels. Reject invalid values and use actual
   catalog counts for selection wraparound.
3. Add playback, mode and speed actions once Lenina's contract exists. Test all
   actions headlessly before mapping keys.
4. Supply one cubic Bézier-based motif to Deborah using the existing evaluator,
   local control points and sampled paths. Show control points and the rendered
   curve separately. Keep the curve representation transformable and clip its
   strokes through Kelvin's renderer rather than drawing over dress boundaries.

### CP3 steps

The following is the planned mapping to implement, not current starter behaviour:

| Key | Planned action |
| --- | --- |
| M / P | Next motif / palette |
| Arrow keys | Translate the pattern |
| Q / E | Rotate counterclockwise / clockwise on screen |
| - / = | Decrease / increase uniform scale |
| X / Y | Reflect across the x-axis / y-axis |
| [ / ] | Decrease / increase tile pitch |
| Space | Play / pause |
| A | Cycle animation mode |
| , / . | Decrease / increase animation speed |
| R / S / Esc | Reset / export / close |

Work with Eric to support held-key repeats only for incremental adjustments,
not toggles. Mark the canvas dirty after state changes. Display current parameters,
playback state and a readable control legend/help view without covering previews.
Extend the bitmap font if new labels need unsupported punctuation. Clicking is
not required through CP3. Ensure all actions update both fabric and dress.

### Tests and completion

Test selection wraparound, lower/upper limits, reflection twice restoring the
original state, reset of every new field, playing/paused changes, valid curve
endpoints, finite samples and nonconflicting key mappings. Test real key presses
as well as scripted callbacks. Completion requires a controls README, a curve
motif demo and an end-to-end desktop interaction recording or selected frames.
Explain how input becomes state and then transformed/rasterized output.

## Checkpoint acceptance and presentation

### CP2 gate: Pure C++

- A clean `KITENGE_BUILD_DESKTOP=OFF` build and all tests pass.
- Matrix transformations produce visible, numerically checked results.
- A reusable renderer creates a repeat swatch and clips it into a static dress.
- A deterministic animation sequence shows the team's transform code over time.
- Catalog, palettes, controls and render state remain usable without GLFW.
- Evidence includes commands, settings, source commits and actual authorship.

### CP3 gate: desktop prototype

- The OpenGL window displays the integrated C++ scene with correct orientation.
- Motif/palette/translation/rotation/scale/reflection/spacing controls work.
- Fabric and garment use the same design/settings; mannequin shape stays fixed.
- Animation plays, pauses and resets correctly. Export produces the current scene.
- Resize, minimize/restore and close work, and known platform limits are documented.
- Old CP1 tests still pass; real key interaction accompanies automated checks.

### Each member's explanation

Prepare a concise segment covering assigned responsibility, files personally
changed, algorithm inputs/calculations/output, the component that uses it, a live
result, an actual challenge and its resolution, tests and one remaining limitation.
Do not invent challenges. Show a short relevant code excerpt alongside its result.

Eric opens with proposed-versus-actual scope, including the browser-to-desktop
change and the 2D mannequin addition. The group demonstrates one connected flow:
select motif, change palette, transform it, inspect dress clipping, play/pause,
reset and export. End with lessons and remaining final-week refinements.

`workfile.md` remains the CP1 historical plan. This guide controls the new CP2/CP3
division of work; update it when the group agrees changes rather than silently
following incompatible historical assignments.
