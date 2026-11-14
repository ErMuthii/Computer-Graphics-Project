#include "kitenge/animation.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>

namespace kitenge {

namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kTwoPi = 2.0 * kPi;

void requireTime(double timeSeconds) {
    if (!std::isfinite(timeSeconds) || timeSeconds < 0.0) {
        throw std::invalid_argument("Animation time must be finite and non-negative");
    }
}

void requireFramesPerSecond(double framesPerSecond) {
    if (!std::isfinite(framesPerSecond) || framesPerSecond <= 0.0 ||
        framesPerSecond > kMaxFramesPerSecond) {
        throw std::invalid_argument("Frame rate must be greater than 0 and at most " +
                                    std::to_string(static_cast<int>(kMaxFramesPerSecond)));
    }
}

// Shared by both transform types: they have the same angle and scale fields.
template <typename Transform>
Transform evaluate(Transform transform, const AnimationSettings& settings, double timeSeconds) {
    switch (settings.mode) {
        case AnimationMode::Off: break;
        case AnimationMode::Rotate:
            transform.angleRadians =
                animatedAngle(transform.angleRadians, settings.angularSpeed, timeSeconds);
            break;
        case AnimationMode::Pulse:
            transform.scale *= pulseFactor(settings, timeSeconds);
            break;
    }
    return transform;
}

} // namespace

const char* animationModeLabel(AnimationMode mode) noexcept {
    switch (mode) {
        case AnimationMode::Off: return "off";
        case AnimationMode::Rotate: return "rotate";
        case AnimationMode::Pulse: return "pulse";
    }
    return "off";
}

AnimationMode nextAnimationMode(AnimationMode mode) noexcept {
    switch (mode) {
        case AnimationMode::Off: return AnimationMode::Rotate;
        case AnimationMode::Rotate: return AnimationMode::Pulse;
        case AnimationMode::Pulse: return AnimationMode::Off;
    }
    return AnimationMode::Off;
}

AnimationMode parseAnimationMode(const std::string& label) {
    for (AnimationMode mode : {AnimationMode::Off, AnimationMode::Rotate, AnimationMode::Pulse}) {
        if (label == animationModeLabel(mode)) return mode;
    }
    throw std::invalid_argument("Unknown animation mode '" + label +
                                "' (expected off, rotate or pulse)");
}

void validateAnimationSettings(const AnimationSettings& s) {
    if (s.mode != AnimationMode::Off && s.mode != AnimationMode::Rotate &&
        s.mode != AnimationMode::Pulse) {
        throw std::invalid_argument("Unknown animation mode");
    }
    if (!std::isfinite(s.angularSpeed) || std::abs(s.angularSpeed) > kMaxAngularSpeed) {
        throw std::invalid_argument("Angular speed must be finite and at most 4 pi rad/s");
    }
    if (!std::isfinite(s.pulseFrequency) || s.pulseFrequency < 0.0 ||
        s.pulseFrequency > kMaxPulseFrequency) {
        throw std::invalid_argument("Pulse frequency must be from 0 to 10 cycles per second");
    }
    if (!std::isfinite(s.pulseMinScale) || !std::isfinite(s.pulseMaxScale) ||
        s.pulseMinScale <= 0.0 || s.pulseMaxScale < s.pulseMinScale) {
        throw std::invalid_argument(
            "Pulse range must satisfy 0 < minimum scale <= maximum scale");
    }
}

double animatedAngle(double baseAngle, double angularSpeed, double timeSeconds) {
    if (!std::isfinite(baseAngle) || !std::isfinite(angularSpeed)) {
        throw std::invalid_argument("Animation angle and speed must be finite");
    }
    requireTime(timeSeconds);
    // Only whole turns are removed, so the drawn rotation is unchanged.
    return baseAngle + std::fmod(angularSpeed * timeSeconds, kTwoPi);
}

double pulseFactor(const AnimationSettings& settings, double timeSeconds) {
    validateAnimationSettings(settings);
    requireTime(timeSeconds);
    const double mid = 0.5 * (settings.pulseMinScale + settings.pulseMaxScale);
    const double amplitude = 0.5 * (settings.pulseMaxScale - settings.pulseMinScale);
    // Reduce the phase to one cycle first so long times keep their precision.
    const double cycles = std::fmod(settings.pulseFrequency * timeSeconds, 1.0);
    const double factor = mid + amplitude * std::sin(kTwoPi * cycles);
    // Rounding in sin can step a hair past a bound; the contract promises the range.
    return std::clamp(factor, settings.pulseMinScale, settings.pulseMaxScale);
}

TransformParams animateTransform(const TransformParams& base, const AnimationSettings& settings,
                                 double timeSeconds) {
    validateAnimationSettings(settings);
    validateTransformParams(base);
    requireTime(timeSeconds);
    TransformParams result = evaluate(base, settings, timeSeconds);
    validateTransformParams(result);
    return result;
}

PatternSettings animatePatternSettings(const PatternSettings& base,
                                       const AnimationSettings& settings, double timeSeconds) {
    validateAnimationSettings(settings);
    requireTime(timeSeconds);
    if (!std::isfinite(base.transform.angleRadians) || !std::isfinite(base.transform.scale) ||
        base.transform.scale <= 0.0) {
        throw std::invalid_argument("Base pattern angle must be finite and scale positive");
    }
    PatternSettings result = base;
    result.transform = evaluate(base.transform, settings, timeSeconds);
    if (settings.mode == AnimationMode::Pulse) {
        result.transform.scale = std::clamp(result.transform.scale,
                                            kMinPatternScale / kGarmentPatternZoom,
                                            kMaxPatternScale);
    }
    return result;
}

double frameTime(int frameIndex, double framesPerSecond) {
    if (frameIndex < 0) throw std::invalid_argument("Frame index must not be negative");
    requireFramesPerSecond(framesPerSecond);
    return static_cast<double>(frameIndex) / framesPerSecond;
}

void validateFrameSequence(int frameCount, double framesPerSecond) {
    if (frameCount < 1 || frameCount > kMaxAnimationFrames) {
        throw std::invalid_argument("Frame count must be from 1 to " +
                                    std::to_string(kMaxAnimationFrames));
    }
    requireFramesPerSecond(framesPerSecond);
}

std::string frameFileName(const std::string& prefix, int frameIndex, int frameCount) {
    if (frameCount < 1 || frameIndex < 0 || frameIndex >= frameCount) {
        throw std::invalid_argument("Frame index must be within the sequence");
    }
    const std::size_t width = std::max<std::size_t>(4, std::to_string(frameCount - 1).size());
    std::string digits = std::to_string(frameIndex);
    digits.insert(0, width - digits.size(), '0');
    return prefix + digits + ".ppm";
}

StudioHooks makeAnimatedPatternHooks(const PatternSettings& base,
                                     const AnimationSettings& animation) {
    validateAnimationSettings(animation);
    // Reads the clock only; advanceStudio remains the single owner of time.
    return makePatternHooks([base, animation](const StudioState& state) {
        return animatePatternSettings(base, animation, state.timeSeconds);
    });
}

} // namespace kitenge
