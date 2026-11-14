#pragma once

// Pattern animation and deterministic frame sampling for the Kitenge studio.
// Owner: Sayialel Eric Lenina, 161492.
//
// The animation never moves geometry step by step. Every frame is evaluated
// directly from the user's BASE settings and the elapsed time:
//
//     rotate:  angle(t) = baseAngle + angularSpeed * t
//     pulse:   scale(t) = baseScale * pulseFactor(t)
//              pulseFactor(t) = mid + amplitude * sin(2 * pi * frequency * t)
//              mid = (min + max) / 2, amplitude = (max - min) / 2
//
// Because the result depends only on (base settings, t), it never accumulates
// rounding error, never overwrites the base settings, and gives the same
// picture at the same elapsed time whatever the frame rate or the sizes of the
// time steps that reached that time. Frame count therefore sets the length of a
// sequence, never the speed of the animation.
//
// Time ownership: advanceStudio() in studio.cpp is the only code that adds dt
// to StudioState::timeSeconds (and only while playing). This module only READS
// timeSeconds, so it must never be registered in a way that adds dt again.
//
// Conventions follow the shared contract: radians internally, positive angles
// appear clockwise on screen because y points down. Nothing here depends on
// GLFW or OpenGL.

#include "kitenge/pattern.hpp"
#include "kitenge/studio.hpp"
#include "kitenge/transforms.hpp"

#include <string>

namespace kitenge {

// Off keeps the base settings unchanged. Rotate turns every motif about its
// pivot at a constant angular speed. Pulse scales every motif up and down
// between two positive bounds. The mannequin and the tile grid stay fixed in
// every mode; only the motif inside each tile moves.
enum class AnimationMode { Off, Rotate, Pulse };

inline constexpr int kAnimationModeCount = 3;

// Display label for a mode ("off", "rotate", "pulse").
const char* animationModeLabel(AnimationMode mode) noexcept;

// The mode after `mode`, wrapping Pulse -> Off, for a "cycle mode" control.
AnimationMode nextAnimationMode(AnimationMode mode) noexcept;

// Parses a label produced by animationModeLabel. Throws std::invalid_argument
// for any other text.
AnimationMode parseAnimationMode(const std::string& label);

// Animation settings for the shared settings contract.
//   angularSpeed    radians per second; negative turns the other way
//   pulseFrequency  complete pulse cycles per second; zero holds the midpoint
//   pulseMinScale   smallest factor applied to the base scale; must be > 0
//   pulseMaxScale   largest factor applied to the base scale; >= pulseMinScale
// The defaults turn motifs 45 degrees per second and pulse them between 0.75x
// and 1.25x once every two seconds, so time zero equals the base settings.
struct AnimationSettings {
    AnimationMode mode = AnimationMode::Off;
    double angularSpeed = 0.785398163397448309616; // pi / 4 rad/s
    double pulseFrequency = 0.5;
    double pulseMinScale = 0.75;
    double pulseMaxScale = 1.25;
};

// Bounds that keep controls and the per-frame work predictable.
inline constexpr double kMaxAngularSpeed = 12.566370614359172954; // 4 pi rad/s
inline constexpr double kMaxPulseFrequency = 10.0;                // cycles per second

// Throws std::invalid_argument unless every field is finite, |angularSpeed|
// <= kMaxAngularSpeed, 0 <= pulseFrequency <= kMaxPulseFrequency and
// 0 < pulseMinScale <= pulseMaxScale.
void validateAnimationSettings(const AnimationSettings& settings);

// angle(t) = baseAngle + angularSpeed * t, reduced into [baseAngle - 2 pi,
// baseAngle + 2 pi] so very long sessions keep full double precision. The
// reduction removes whole turns only, so the drawn result is identical.
// Throws std::invalid_argument for non-finite input or negative time.
double animatedAngle(double baseAngle, double angularSpeed, double timeSeconds);

// The pulse factor at time t (see the header comment). It equals the midpoint
// at t = 0, reaches pulseMaxScale a quarter cycle later and pulseMinScale at
// three quarters, and is always within [pulseMinScale, pulseMaxScale], so it is
// always positive. Validates settings; throws for non-finite or negative time.
double pulseFactor(const AnimationSettings& settings, double timeSeconds);

// The transform to display at time t, derived from `base`. Only the angle
// (Rotate) or the scale (Pulse) changes; translation, pivot and reflection are
// copied from `base`. Off returns `base` unchanged. `base` is never modified.
// Throws std::invalid_argument for invalid settings, an invalid base transform
// or invalid time.
TransformParams animateTransform(const TransformParams& base, const AnimationSettings& settings,
                                 double timeSeconds);

// The same evaluation for the pattern engine's settings. A pulsed scale is
// clamped to the range the studio views support, [kMinPatternScale /
// kGarmentPatternZoom, kMaxPatternScale], so the garment view (which zooms by
// kGarmentPatternZoom) never receives a scale the engine would reject.
PatternSettings animatePatternSettings(const PatternSettings& base,
                                       const AnimationSettings& settings, double timeSeconds);

// --- Frame sampling -----------------------------------------------------------

inline constexpr int kMaxAnimationFrames = 100000;
inline constexpr double kMaxFramesPerSecond = 240.0;

// Exact sample time of a frame: frameIndex / fps. Sampling every frame from its
// index (rather than summing 1 / fps) keeps frame 59 at exactly 59 / 30 s.
// Throws std::invalid_argument unless 0 <= frameIndex and 0 < fps <=
// kMaxFramesPerSecond.
double frameTime(int frameIndex, double framesPerSecond);

// Throws std::invalid_argument unless 1 <= frameCount <= kMaxAnimationFrames
// and 0 < fps <= kMaxFramesPerSecond (both finite).
void validateFrameSequence(int frameCount, double framesPerSecond);

// Zero-padded file name "<prefix><index>.ppm". The width is at least four
// digits and grows to fit frameCount - 1, so names always sort in frame order.
// Throws std::invalid_argument for an index outside [0, frameCount).
std::string frameFileName(const std::string& prefix, int frameIndex, int frameCount);

// --- Studio integration -------------------------------------------------------

// Pattern hooks whose settings are evaluated at state.timeSeconds from the
// fixed `base` and `animation` values. No animate hook is installed, because
// advanceStudio already owns the clock. The motif and palette still come from
// the state. Validates `animation` once, up front.
StudioHooks makeAnimatedPatternHooks(const PatternSettings& base,
                                     const AnimationSettings& animation);

} // namespace kitenge
