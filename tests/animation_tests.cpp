// Checks for the animation module. Owner: Sayialel Eric Lenina, 161492.

#include "kitenge/animation.hpp"
#include "kitenge/pattern.hpp"
#include "kitenge/studio.hpp"
#include "kitenge/transforms.hpp"

#include <cmath>
#include <cstddef>
#include <exception>
#include <functional>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using namespace kitenge;

constexpr double kPi = 3.14159265358979323846;
constexpr double kTolerance = 1e-9;
const double kNaN = std::numeric_limits<double>::quiet_NaN();
const double kInf = std::numeric_limits<double>::infinity();

int failures = 0;

void check(bool condition, const std::string& description) {
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << description << '\n';
    }
}

bool near(double actual, double expected, double tolerance = kTolerance) {
    return std::abs(actual - expected) < tolerance;
}

bool throwsInvalid(const std::function<void()>& action) {
    try {
        action();
    } catch (const std::invalid_argument&) {
        return true;
    } catch (...) {
        return false;
    }
    return false;
}

// Two angles draw the same rotation when they differ by whole turns.
bool sameRotation(double a, double b) {
    return near(std::cos(a), std::cos(b)) && near(std::sin(a), std::sin(b));
}

bool sameCanvas(const Canvas& a, const Canvas& b) {
    if (a.width() != b.width() || a.height() != b.height()) return false;
    const auto& p = a.pixels();
    const auto& q = b.pixels();
    for (std::size_t i = 0; i < p.size(); ++i) {
        if (p[i].red != q[i].red || p[i].green != q[i].green || p[i].blue != q[i].blue) return false;
    }
    return true;
}

AnimationSettings rotating(double speed) {
    AnimationSettings s;
    s.mode = AnimationMode::Rotate;
    s.angularSpeed = speed;
    return s;
}

AnimationSettings pulsing(double frequency, double minimum, double maximum) {
    AnimationSettings s;
    s.mode = AnimationMode::Pulse;
    s.pulseFrequency = frequency;
    s.pulseMinScale = minimum;
    s.pulseMaxScale = maximum;
    return s;
}

TransformParams sampleBase() {
    TransformParams base;
    base.translation = {12.0, -5.0};
    base.angleRadians = 0.3;
    base.scale = 1.4;
    base.pivot = {2.0, 6.0};
    base.reflectX = true;
    return base;
}

void testModes() {
    check(std::string(animationModeLabel(AnimationMode::Off)) == "off", "off label");
    check(std::string(animationModeLabel(AnimationMode::Rotate)) == "rotate", "rotate label");
    check(std::string(animationModeLabel(AnimationMode::Pulse)) == "pulse", "pulse label");
    AnimationMode mode = AnimationMode::Off;
    for (int i = 0; i < kAnimationModeCount; ++i) mode = nextAnimationMode(mode);
    check(mode == AnimationMode::Off, "cycling through every mode returns to off");
    check(nextAnimationMode(AnimationMode::Off) == AnimationMode::Rotate, "off -> rotate");
    check(nextAnimationMode(AnimationMode::Pulse) == AnimationMode::Off, "pulse wraps to off");
    check(parseAnimationMode("pulse") == AnimationMode::Pulse, "parse pulse");
    check(throwsInvalid([] { parseAnimationMode("spin"); }), "unknown mode label rejected");
    check(AnimationSettings{}.mode == AnimationMode::Off, "animation is off by default");
}

void testTimeZero() {
    const TransformParams base = sampleBase();
    for (const AnimationSettings& s : {rotating(1.7), pulsing(0.5, 0.6, 1.4), AnimationSettings{}}) {
        const TransformParams r = animateTransform(base, s, 0.0);
        check(near(r.angleRadians, base.angleRadians) && near(r.scale, base.scale),
              std::string("time zero equals the base settings in mode ") +
                  animationModeLabel(s.mode));
    }
}

void testRotation() {
    const TransformParams base = sampleBase();
    // pi/2 rad/s: a quarter turn after 1 s, a full turn after 4 s.
    const AnimationSettings s = rotating(kPi / 2.0);
    check(near(animateTransform(base, s, 1.0).angleRadians, base.angleRadians + kPi / 2.0),
          "quarter turn after one second");
    check(near(animateTransform(base, s, 2.0).angleRadians, base.angleRadians + kPi),
          "half turn after two seconds");
    check(sameRotation(animateTransform(base, s, 4.0).angleRadians, base.angleRadians),
          "full turn after four seconds draws the base rotation");
    check(near(animateTransform(base, rotating(-kPi / 2.0), 1.0).angleRadians,
               base.angleRadians - kPi / 2.0),
          "negative speed turns the other way");

    // A quarter turn of local (1, 0) about the origin lands on (0, 1): clockwise on screen.
    TransformParams plain;
    const TransformParams quarter = animateTransform(plain, s, 1.0);
    const Point2D p = transformPoint(transformMatrix(quarter), {1.0, 0.0});
    check(near(p.x, 0.0) && near(p.y, 1.0), "animated matrix turns (1,0) to (0,1)");

    // Rotation only changes the angle.
    const TransformParams r = animateTransform(base, s, 3.3);
    check(r.translation.x == base.translation.x && r.translation.y == base.translation.y &&
              r.scale == base.scale && r.pivot.x == base.pivot.x && r.pivot.y == base.pivot.y &&
              r.reflectX == base.reflectX && r.reflectY == base.reflectY,
          "rotate mode leaves translation, scale, pivot and reflection unchanged");

    // Long sessions: at pi/2 rad/s, 100000.25 s is 25000.0625 turns, so the
    // drawn rotation is 0.0625 of a turn = pi/8.
    const double t = 100000.25;
    check(sameRotation(animatedAngle(0.0, kPi / 2.0, t), kPi / 8.0),
          "very long time keeps the correct rotation");
    check(std::abs(animatedAngle(0.0, kPi / 2.0, t)) <= 2.0 * kPi,
          "accumulated angle stays within one turn of the base");
}

void testPulse() {
    const AnimationSettings s = pulsing(0.5, 0.75, 1.25); // one cycle every 2 s
    check(near(pulseFactor(s, 0.0), 1.0), "pulse starts at the midpoint");
    check(near(pulseFactor(s, 0.5), 1.25), "pulse peaks a quarter cycle later");
    check(near(pulseFactor(s, 1.0), 1.0), "pulse returns to the midpoint at half a cycle");
    check(near(pulseFactor(s, 1.5), 0.75), "pulse reaches its minimum at three quarters");
    check(near(pulseFactor(s, 2.0), 1.0), "pulse repeats after a full cycle");

    bool inRange = true;
    for (int i = 0; i <= 2000; ++i) {
        const double f = pulseFactor(s, i * 0.0137);
        if (!(f >= 0.75 && f <= 1.25 && f > 0.0)) inRange = false;
    }
    check(inRange, "pulse factor stays within [min, max] and positive at every sample");

    const TransformParams base = sampleBase();
    const TransformParams peak = animateTransform(base, s, 0.5);
    check(near(peak.scale, base.scale * 1.25), "pulse multiplies the base scale");
    check(peak.angleRadians == base.angleRadians, "pulse mode leaves the angle unchanged");

    check(near(pulseFactor(pulsing(0.0, 0.5, 1.5), 7.0), 1.0),
          "zero frequency holds the midpoint");
    check(near(pulseFactor(pulsing(2.0, 0.9, 0.9), 0.3), 0.9), "equal bounds hold that scale");
}

void testBaseNotModified() {
    const TransformParams base = sampleBase();
    const TransformParams copy = base;
    for (int i = 0; i < 100; ++i) animateTransform(base, rotating(2.0), i * 0.1);
    check(base.angleRadians == copy.angleRadians && base.scale == copy.scale,
          "evaluating many frames never changes the base settings");
    // Evaluating twice at the same time gives identical results: nothing compounds.
    const TransformParams a = animateTransform(base, rotating(2.0), 1.25);
    const TransformParams b = animateTransform(base, rotating(2.0), 1.25);
    check(a.angleRadians == b.angleRadians, "repeated evaluation does not compound");
}

void testInvalidInputs() {
    const TransformParams base = sampleBase();
    check(throwsInvalid([&] { animateTransform(base, rotating(1.0), -0.1); }), "negative time");
    check(throwsInvalid([&] { animateTransform(base, rotating(1.0), kNaN); }), "NaN time");
    check(throwsInvalid([&] { animateTransform(base, rotating(1.0), kInf); }), "infinite time");
    check(throwsInvalid([&] { animateTransform(base, rotating(kNaN), 1.0); }), "NaN speed");
    check(throwsInvalid([&] { animateTransform(base, rotating(kMaxAngularSpeed * 2), 1.0); }),
          "speed above the limit");
    check(throwsInvalid([&] { pulseFactor(pulsing(-1.0, 0.5, 1.5), 1.0); }),
          "negative pulse frequency");
    check(throwsInvalid([&] { pulseFactor(pulsing(0.5, 0.0, 1.5), 1.0); }),
          "zero minimum pulse scale");
    check(throwsInvalid([&] { pulseFactor(pulsing(0.5, -0.5, 1.5), 1.0); }),
          "negative minimum pulse scale");
    check(throwsInvalid([&] { pulseFactor(pulsing(0.5, 1.5, 0.5), 1.0); }),
          "minimum above maximum");
    check(throwsInvalid([&] { pulseFactor(pulsing(0.5, 0.5, kInf), 1.0); }),
          "infinite maximum pulse scale");
    TransformParams badBase = base;
    badBase.scale = 0.0;
    check(throwsInvalid([&] { animateTransform(badBase, rotating(1.0), 1.0); }),
          "invalid base transform rejected");

    check(throwsInvalid([] { frameTime(-1, 30.0); }), "negative frame index");
    check(throwsInvalid([] { frameTime(0, 0.0); }), "zero frame rate");
    check(throwsInvalid([] { frameTime(0, -30.0); }), "negative frame rate");
    check(throwsInvalid([] { frameTime(0, kNaN); }), "NaN frame rate");
    check(throwsInvalid([] { frameTime(0, 1000.0); }), "frame rate above the limit");
    check(throwsInvalid([] { validateFrameSequence(0, 30.0); }), "zero frames");
    check(throwsInvalid([] { validateFrameSequence(kMaxAnimationFrames + 1, 30.0); }),
          "too many frames");
    check(!throwsInvalid([] { validateFrameSequence(60, 30.0); }), "60 frames at 30 fps is valid");
    check(throwsInvalid([] { frameFileName("frame_", 60, 60); }), "index past the sequence");
    check(throwsInvalid([] { makeAnimatedPatternHooks(defaultStudioPatternSettings(),
                                                      pulsing(0.5, 2.0, 1.0)); }),
          "hooks validate settings up front");
}

void testFrameSampling() {
    check(frameTime(0, 30.0) == 0.0, "frame 0 is time zero");
    check(frameTime(59, 30.0) == 59.0 / 30.0, "frame 59 at 30 fps is exactly 59/30 s");
    check(frameTime(30, 30.0) == 1.0, "frame 30 at 30 fps is exactly one second");
    // Summing 1/fps drifts; sampling from the index does not.
    double summed = 0.0;
    for (int i = 0; i < 59; ++i) summed += 1.0 / 30.0;
    check(near(summed, frameTime(59, 30.0), 1e-12), "index sampling matches the ideal sum");

    // Speed is independent of frame count and rate: the same time gives the same pose.
    const AnimationSettings s = rotating(kPi / 4.0);
    const double at30 = animatedAngle(0.0, s.angularSpeed, frameTime(30, 30.0));
    const double at60 = animatedAngle(0.0, s.angularSpeed, frameTime(60, 60.0));
    check(near(at30, at60), "one second looks the same at 30 fps and at 60 fps");

    check(frameFileName("frame_", 0, 60) == "frame_0000.ppm", "padded first frame name");
    check(frameFileName("frame_", 59, 60) == "frame_0059.ppm", "padded last frame name");
    check(frameFileName("f", 7, 12000) == "f00007.ppm", "width grows for long sequences");
}

void testStudioClock() {
    const AnimationSettings s = rotating(kPi / 3.0);
    const PatternSettings base = defaultStudioPatternSettings();
    const StudioHooks hooks = makeAnimatedPatternHooks(base, s);
    check(!hooks.animate, "no animate hook: advanceStudio stays the only owner of time");
    check(static_cast<bool>(hooks.fabric) && static_cast<bool>(hooks.garment),
          "fabric and garment hooks are installed");

    // Different dt partitions of the same elapsed time give the same pose.
    const std::vector<std::vector<double>> partitions = {
        std::vector<double>(60, 1.0 / 60.0),
        {1.0 / 3.0, 1.0 / 3.0, 1.0 / 3.0},
        {0.1, 0.05, 0.35, 0.2, 0.3},
        {1.0}};
    std::vector<double> angles;
    for (const auto& steps : partitions) {
        StudioState state;
        state.playing = true;
        for (double dt : steps) advanceStudio(state, dt, hooks);
        angles.push_back(animatePatternSettings(base, s, state.timeSeconds).transform.angleRadians);
    }
    bool equal = true;
    for (double a : angles) equal = equal && near(a, angles.front(), 1e-12);
    check(equal, "equal elapsed time gives the same angle for different dt partitions");
    check(near(angles.front(), kPi / 3.0, 1e-12), "one second at pi/3 rad/s");

    // Pause freezes time and so the pose; resume continues without a jump.
    StudioState state;
    state.playing = true;
    advanceStudio(state, 0.4, hooks);
    applyAction(state, StudioAction::TogglePlayback);
    const double frozen = state.timeSeconds;
    for (int i = 0; i < 10; ++i) advanceStudio(state, 0.1, hooks);
    check(state.timeSeconds == frozen, "paused playback does not advance time");
    check(animatePatternSettings(base, s, state.timeSeconds).transform.angleRadians ==
              animatePatternSettings(base, s, frozen).transform.angleRadians,
          "paused playback freezes the animated pose");
    applyAction(state, StudioAction::TogglePlayback);
    advanceStudio(state, 0.1, hooks);
    check(near(state.timeSeconds, frozen + 0.1), "resume continues from the frozen time");

    // Reset returns to time zero, which is the base pose.
    applyAction(state, StudioAction::Reset);
    check(state.timeSeconds == 0.0 && !state.playing, "reset restores time zero, paused");
    check(animatePatternSettings(base, s, state.timeSeconds).transform.angleRadians ==
              base.transform.angleRadians,
          "after reset the pattern shows the base settings");
}

void testRendering() {
    const PatternSettings base = defaultStudioPatternSettings();
    const AnimationSettings s = rotating(kPi / 4.0);
    StudioState state;
    state.motif = 1;

    const Color black{0, 0, 0};
    Canvas still(studioWidth, studioHeight, black), animated(studioWidth, studioHeight, black),
        expected(studioWidth, studioHeight, black);
    renderStudio(still, state, makePatternHooks());
    renderStudio(animated, state, makeAnimatedPatternHooks(base, s));
    check(sameCanvas(still, animated), "time zero renders exactly like the unanimated scene");

    state.timeSeconds = 1.0;
    renderStudio(animated, state, makeAnimatedPatternHooks(base, s));
    const PatternSettings at1 = animatePatternSettings(base, s, 1.0);
    renderStudio(expected, state, makePatternHooks([at1](const StudioState&) { return at1; }));
    check(sameCanvas(animated, expected), "hooks render the settings evaluated at timeSeconds");
    check(!sameCanvas(animated, still), "the pattern visibly changes after one second");

    // Pulsing a small base scale is clamped to what the garment view accepts.
    PatternSettings small = base;
    small.transform.scale = kMinPatternScale / kGarmentPatternZoom;
    const PatternSettings low =
        animatePatternSettings(small, pulsing(0.5, 0.5, 1.0), 1.5); // factor 0.5
    check(low.transform.scale == kMinPatternScale / kGarmentPatternZoom,
          "pulsed scale is clamped to the garment view's minimum");
    state.timeSeconds = 1.5;
    bool rendered = true;
    try {
        renderStudio(animated, state, makeAnimatedPatternHooks(small, pulsing(0.5, 0.5, 1.0)));
    } catch (const std::exception&) {
        rendered = false;
    }
    check(rendered, "clamped pulse renders without the engine rejecting the scale");
}

} // namespace

int main() {
    testModes();
    testTimeZero();
    testRotation();
    testPulse();
    testBaseNotModified();
    testInvalidInputs();
    testFrameSampling();
    testStudioClock();
    testRendering();
    if (failures != 0) {
        std::cerr << failures << " animation check(s) failed\n";
        return 1;
    }
    std::cout << "All animation checks passed\n";
    return 0;
}
