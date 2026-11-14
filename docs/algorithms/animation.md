# Pattern animation and frame generation

Owner: Sayialel Eric Lenina, 161492.

This module animates the Kitenge pattern over time. It has two showcase modes:
continuous motif **rotation** and bounded scale **pulsing**. The mannequin, the
dress panels and the tile grid stay fixed. Only the motif inside each tile moves.
The module reads the studio clock and turns it into transform settings for
Kelvin's pattern engine, which applies them through Jeremy's matrix order. A
headless demo writes a numbered PPM frame sequence without any window or OpenGL.

```cpp
// include/kitenge/animation.hpp
enum class AnimationMode { Off, Rotate, Pulse };
struct AnimationSettings {
    AnimationMode mode = AnimationMode::Off;
    double angularSpeed = pi / 4;   // rad/s (45 degrees per second)
    double pulseFrequency = 0.5;    // cycles per second
    double pulseMinScale = 0.75;    // factors applied to the base scale
    double pulseMaxScale = 1.25;
};

double animatedAngle(double baseAngle, double angularSpeed, double t);
double pulseFactor(const AnimationSettings&, double t);
TransformParams animateTransform(const TransformParams& base, const AnimationSettings&, double t);
PatternSettings animatePatternSettings(const PatternSettings& base, const AnimationSettings&, double t);

double frameTime(int frameIndex, double fps);               // frameIndex / fps
std::string frameFileName(const std::string& prefix, int index, int count);
StudioHooks makeAnimatedPatternHooks(const PatternSettings& base, const AnimationSettings&);
```

## Timing equations

Every frame is evaluated directly from the user's **base** settings and the
elapsed time `t`, in seconds:

```text
rotate:  angle(t) = baseAngle + angularSpeed * t
pulse:   scale(t) = baseScale * pulseFactor(t)
         pulseFactor(t) = mid + amplitude * sin(2 * pi * frequency * t)
         mid       = (min + max) / 2
         amplitude = (max - min) / 2
```

Rotation changes only the angle, and pulsing changes only the scale. Translation,
pivot and reflection are copied unchanged from the base. The resulting
`TransformParams` then go through the shared order:

```text
p_canvas = T(origin + tileOffset) * T(pivot) * R(angle(t)) * F * S(scale(t)) * T(-pivot) * p
```

The rotation happens about each motif's pivot inside its own tile, so every
copy turns in place rather than the whole fabric swinging around one point.
Because screen y points down, a positive angular speed appears clockwise.

### Worked example

With the default 45 degrees per second (pi/4 rad/s) and a base angle of 0:

| Time | Angle | What is drawn |
| --- | --- | --- |
| 0 s | 0 | the base pattern |
| 0.5 s | pi/8 = 22.5 degrees | diamonds tilted clockwise |
| 1 s | pi/4 = 45 degrees | each diamond has become an axis-aligned square |
| 2 s | pi/2 = 90 degrees | a quarter turn, back to diamonds |

With the default pulse (0.75x to 1.25x at 0.5 Hz), one cycle takes 2 s. The factor is
1.0 at 0 s, 1.25 at 0.5 s, 1.0 at 1 s, 0.75 at 1.5 s and 1.0 again at 2 s.

### Why the base is never overwritten

A tempting approach is to rotate the geometry a little each frame
(`angle += speed * dt`, or worse, transform the already-transformed vertices).
That accumulates floating-point error, makes the result depend on the frame rate,
and loses the user's original setting. Here the base settings are copied, never
modified, and each frame is a pure function of `(base, t)`. Evaluating the same
time twice gives the identical result, and reset only needs to set `t = 0`.

### Bounds and precision

- The pulse factor always lies in `[min, max]` and `min > 0`, so the scale stays
  positive. Reflection is never produced by a negative scale. The final value is
  clamped to the range, guarding against `sin` rounding a hair past a bound.
- `animatePatternSettings` also clamps a pulsed scale to
  `[kMinPatternScale / kGarmentPatternZoom, kMaxPatternScale]`. The garment view
  multiplies the scale by 0.5, so this keeps both views inside the range that the
  pattern engine accepts.
- Long sessions: the angle is `baseAngle + fmod(speed * t, 2 pi)`, and the pulse
  phase is reduced with `fmod(frequency * t, 1)` before `sin`. Only whole turns or
  cycles are removed, so the drawn result is identical while precision is kept.
- Limits: |speed| at most 4 pi rad/s (two turns per second), pulse frequency
  0 to 10 Hz, frame rate up to 240 fps, at most 100000 frames. Non-finite values,
  negative time, a non-positive minimum scale or `min > max` throw
  `std::invalid_argument`.

## Who owns the clock

`advanceStudio()` in `studio.cpp` is the only code that adds `dt` to
`StudioState::timeSeconds`, and it does so only while `playing` is true. This
module only reads `timeSeconds`. `makeAnimatedPatternHooks` therefore installs
the fabric and garment hooks, evaluated at `state.timeSeconds`, and deliberately
installs **no** `animate` hook. A hook that also added `dt` would make the
animation run at double speed.

Consequences, all covered by the checks:

- **Pause:** `advanceStudio` does nothing while paused, so time and the drawn
  pattern freeze. Resuming continues from the same time without a jump.
- **Reset:** `StudioAction::Reset` restores `StudioState{}`, so time is 0 and the
  pattern shows the base settings exactly.
- **Different time steps:** one 1 s step, sixty 1/60 s steps or an uneven split
  reach the same elapsed time and therefore draw the same pose.

## Frame sampling

The headless demo samples frame `i` at exactly `t = i / fps`, computed from the
index rather than by summing `1 / fps`, which would drift. The frame count only
sets how long the sequence is, and the speed comes only from the timing equations.
The same second looks identical at 30 fps and at 60 fps; the 60 fps run just has
more frames in between. That is why frame count must never determine speed: if
each frame advanced the angle by a fixed step, doubling the frame rate would
double the speed.

Files are named `frame_0000.ppm`, `frame_0001.ppm` and so on. The width is at least
four digits and grows for longer sequences, so names always sort in frame order.

### Overwrite handling

Before writing, the demo looks for existing `frame_*.ppm` files in the output
folder. If any exist, it refuses and exits with status 1, so another evidence
sequence is never silently overwritten. With `--overwrite`, it first deletes
every old `frame_*.ppm` in that folder. A shorter run therefore never leaves
stale frames from a longer one mixed into the new sequence. Each run also writes
`sequence.txt`, which records the mode, frame count, rate and settings.

## Running it

From the repository root, with the core build configured:

```sh
cmake -S . -B build-core -DKITENGE_BUILD_DESKTOP=OFF -DCMAKE_BUILD_TYPE=Debug
cmake --build build-core --parallel
ctest --test-dir build-core -R animation --output-on-failure

# Acceptance fixture: 60 frames at 30 fps (2 seconds).
./build-core/animation_demo --mode rotate
./build-core/animation_demo --mode pulse --motif 2 --palette 1
```

These write `outputs/animation/rotate/` and `outputs/animation/pulse/`.

| Option | Meaning |
| --- | --- |
| `--mode off/rotate/pulse` | animation mode (default rotate) |
| `--frames N` | number of frames, 1 to 100000 (default 60) |
| `--fps F` | frames per second, above 0 and up to 240 (default 30) |
| `--out DIR` | output folder (default `outputs/animation/<mode>`) |
| `--speed DEG_PER_S` | rotation speed in degrees per second (default 45) |
| `--pulse-frequency HZ` | pulse cycles per second (default 0.5) |
| `--pulse-range MIN MAX` | pulse scale factors (default 0.75 1.25) |
| `--overwrite` | replace an existing frame sequence in the folder |

The scene options shared with `studio_render` (`--motif`, `--palette`, `--angle`,
`--scale`, `--pitch`, `--translate`, `--reflect-x`, `--reflect-y`) set the
**base** settings that the animation starts from.

The 60-frame fixture checks correctness. It is not a real-time benchmark; on the
development machine each 2-second sequence took about 2.4 s to render and write.

## Checks (`tests/animation_tests.cpp`)

- Time zero equals the base settings in every mode, and the rendered scene at
  `t = 0` is pixel-identical to the unanimated scene.
- Rotation gives a quarter turn at 1 s and a half turn at 2 s (pi/2 rad/s). A full
  turn at 4 s draws the base rotation. Negative speed reverses direction, and the
  animated matrix sends local `(1, 0)` to `(0, 1)`.
- The pulse is at its midpoint at 0, maximum at a quarter cycle, minimum at
  three quarters, and repeats after one cycle. It stays within `[min, max]` and
  positive across 2000 samples. Zero frequency holds the midpoint.
- Base settings are unchanged after evaluating many frames, and repeated
  evaluation at the same time does not compound.
- Invalid inputs are rejected: negative, NaN or infinite time; NaN or excessive
  speed; negative frequency; zero, negative or reversed pulse bounds; invalid
  frame index, rate or count.
- Frame times are exact (`frame 59 at 30 fps = 59/30 s`). One second looks the
  same at 30 and 60 fps, and file names are padded.
- With the studio clock: no `animate` hook is installed, and four different dt
  partitions of 1 s give the same angle. Pause freezes time, resume continues
  without a jump, and reset returns to time zero and the base pose.
- Rendering: the hooks draw exactly the settings evaluated at `timeSeconds`, the
  pattern visibly changes after 1 s, and a clamped pulse renders without the
  engine rejecting the scale.

The checks were confirmed to fail when the rotation formula is deliberately
perturbed (7 failures). They do not pass vacuously.

## Limitations and CP3 follow-up

- Only rotation and pulsing are animated. Translation drift, colour cycling and
  combined modes are not implemented.
- Animation settings are not yet part of `StudioState`. Eric owns that shared
  struct, so the module takes `AnimationSettings` separately for now. CP3 work on
  `feat/cp3-animation-playback` will connect the same evaluator to the desktop
  clock, with Andrew's Space play/pause, mode and speed controls, and with Eric's
  state and hook registration.
- The desktop already caps each frame's `dt` at 0.1 s. After a long stall, for
  example while the window is minimized, the animation therefore resumes from
  roughly where it stopped rather than jumping ahead. Live behaviour on
  minimize/restore still needs to be tested in CP3.
