# Presentation rehearsal and integration review

Review date: 15 November 2026. Platform: Apple M1, Apple Clang, OpenGL 4.1.

## Work integrated

The fetched main ended at `6aae7c4`. Andrew's existing controls/curve branch ended
at `f7a0713` and was ahead of main by four commits. Its original history is
preserved; the review reused that contribution instead of recreating it.

Integration fixes by Eric's review session:

- Replace the pattern renderer's interim private matrix implementation with
  Jeremy's shared Mat3 operations and alias PatternTransform to TransformParams.
- Correct Retina GUI comparison tests: explicitly permit integer display scaling
  and compare every physical pixel with its logical source. Strict image
  comparison remains strict by default. A corrupt subpixel still fails.
- Extend desktop smoke coverage to visible animation in both dress panels,
  playback pause, and export. Keep the existing selection, transformation,
  held-toggle, reset, resize and GL-error checks.

## Build for the presentation

From the repository root on Eric's Mac:

```sh
cmake -S . -B build-presentation -DKITENGE_BUILD_DESKTOP=ON -DKITENGE_GUI_TESTS=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_SYSROOT=/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk
cmake --build build-presentation --parallel 6
ctest --test-dir build-presentation --output-on-failure
./build-presentation/kitenge_studio
```

Release is recommended for animation, especially the sampled curved motifs.
The GUI tests open real windows. Close any older prototype instance first so
you rehearse the freshly built executable. See desktop-starter.md for other
platform dependencies; this review does not certify Windows/Linux execution.

The 19-check desktop suite includes 15 checks without real windows (including
the GLFW key adapter), three desktop/headless pixel comparisons, and the live
OpenGL smoke test. Scripted callback checks do not replace a rehearsal with real
keyboard input. Native UI automation could not select the unbundled executable
on this Mac, so physical key input remains a manual rehearsal item.

## Rehearsal sequence

1. Press R. Explain fabric preview, static mannequin and shared pattern settings.
2. Press M through the catalog. Show the asymmetric motifs and the final Bezier
   arch motif. Press P to show palette roles on both fabric and dress.
3. Use arrows to translate; Q/E to rotate; -/= to scale; X/Y to reflect.
   Use an asymmetric motif when demonstrating reflection so it is visible.
4. Change pitch with [ and ]. Point to shapes clipped at the fabric boundary
   and slanted skirt edge. The mannequin and garment boundaries remain fixed.
5. Press R, then A once (OFF to ROTATE), then Space. Space alone with mode OFF
   advances the clock without visual motion. The header shows the selected mode.
6. Press Space to pause. Press . or , to change speed, then Space to resume.
7. Press A to show PULSE. Press Space to pause before discussing the frame.
8. Press S. Confirm the terminal reports the saved PPM file. Export includes the
   current control readout and the visible scene. R restores the defaults.
9. Resize the window and close with Esc.

Before presenting, run steps 1–9 on the actual presentation machine. Check that
the projected legend and header are readable and the terminal is available to
show exports and errors.

## Pure C++ and offline fallback

```sh
cmake -S . -B build-core-review -DKITENGE_BUILD_DESKTOP=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build-core-review --parallel 6
ctest --test-dir build-core-review --output-on-failure
./build-core-review/studio_render outputs/presentation-static.ppm --motif 5 --palette 1 --angle 30
./build-core-review/animation_demo --mode rotate --frames 60 --fps 30 --out outputs/presentation-rotate
./build-core-review/animation_demo --mode pulse --frames 60 --fps 30 --out outputs/presentation-pulse
```

Add the SDK flag from the desktop command on Eric's Mac. Sequence export refuses
to replace an existing sequence unless --overwrite is supplied. Use a new output
folder to retain an earlier rehearsal. PPM sequences need a viewer/conversion for
playback; exporting frames does not create a video automatically.

## Presentation claims

Demonstrate the real implementations: Jeremy's homogeneous transforms, Deborah's
motif/palette/mannequin data, Kelvin's repeat/clipping/raster pipeline, Lenina's
time-based animation, Andrew's action reducer/curve motif and Eric's application
integration/OpenGL presenter. Show the original authors' modules and accurately
identify the integration fixes separately.

The model is a fixed 2D mannequin, not a fitted 3D garment. Motifs are predefined,
not a freehand editor. OpenGL presents the software pixel buffer. Camera input,
body tracking and physical cloth simulation are outside this prototype.
