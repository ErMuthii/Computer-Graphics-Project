// Headless checks for the controls reducer, key map, readout and the Bezier
// arch motif. No GLFW or OpenGL.
// Owner: Andrew Karanja Gathirwa, 167144.
#include "kitenge/controls.hpp"
#include "kitenge/curve_motif.hpp"
#include "kitenge/curves.hpp"
#include "kitenge/pattern.hpp"
#include "kitenge/transforms.hpp"

#include <algorithm>
#include <cmath>
#include <exception>
#include <functional>
#include <iostream>
#include <set>
#include <string>
#include <vector>

using namespace kitenge;

namespace {

int failures = 0;

void check(bool ok, const std::string& name) {
    if (!ok) {
        ++failures;
        std::cerr << "FAILED: " << name << "\n";
    }
}

bool near(double a, double b, double eps = 1e-9) { return std::fabs(a - b) <= eps; }

constexpr double kPi = 3.14159265358979323846;

// Every field the controls can change.
bool sameControls(const ControlState& a, const ControlState& b) {
    const PatternTransform &x = a.pattern.transform, &y = b.pattern.transform;
    return a.studio.motif == b.studio.motif && a.studio.palette == b.studio.palette &&
           a.studio.playing == b.studio.playing && near(a.studio.timeSeconds, b.studio.timeSeconds) &&
           near(x.translation.x, y.translation.x) && near(x.translation.y, y.translation.y) &&
           near(x.angleRadians, y.angleRadians) && near(x.scale, y.scale) &&
           x.reflectX == y.reflectX && x.reflectY == y.reflectY &&
           near(a.pattern.tilePitch.x, b.pattern.tilePitch.x) &&
           near(a.pattern.tilePitch.y, b.pattern.tilePitch.y) &&
           a.animation.mode == b.animation.mode && near(a.animationSpeed, b.animationSpeed);
}

ControlState press(ControlState s, ControlAction a, int times = 1) {
    for (int i = 0; i < times; ++i) s = reduceControl(s, a).state;
    return s;
}

// --- Selection and starter behaviour ------------------------------------------------

void testInitialState() {
    const ControlState s = initialControlState();
    check(withinControlLimits(s), "initial state is inside every limit");
    check(s.studio.motif == 0 && s.studio.palette == 0 && !s.studio.playing &&
              s.studio.timeSeconds == 0.0,
          "initial studio state matches the starter");
    const PatternSettings d = defaultStudioPatternSettings();
    check(near(s.pattern.tilePitch.x, d.tilePitch.x) && near(s.pattern.tilePitch.y, d.tilePitch.y) &&
              near(s.pattern.transform.scale, d.transform.scale),
          "initial pattern settings are the studio defaults");
    check(s.animation.mode == AnimationMode::Off && near(s.animationSpeed, 1.0),
          "animation starts off at normal speed");
    check(sameControls(s, controlStateFrom(PatternSceneOptions{})),
          "default scene options give the initial state");
}

void testSelectionWraparound() {
    ControlState s = initialControlState();
    s = press(s, ControlAction::NextMotif, kMotifCount - 1);
    check(s.studio.motif == kMotifCount - 1, "motif reaches the last catalog entry");
    s = press(s, ControlAction::NextMotif);
    check(s.studio.motif == 0, "motif wraps after kMotifCount presses");
    s = press(s, ControlAction::NextPalette, kPaletteCount);
    check(s.studio.palette == 0, "palette wraps after kPaletteCount presses");
    s = press(s, ControlAction::CycleAnimationMode, kAnimationModeCount - 1);
    check(s.animation.mode == AnimationMode::Pulse, "mode cycles off -> rotate -> pulse");
    s = press(s, ControlAction::CycleAnimationMode);
    check(s.animation.mode == AnimationMode::Off, "mode wraps after kAnimationModeCount presses");
}

void testStarterActionsUnchanged() {
    // The reducer must agree with the existing applyAction for the starter keys.
    const std::vector<std::pair<ControlAction, StudioAction>> pairs{
        {ControlAction::NextMotif, StudioAction::NextMotif},
        {ControlAction::NextPalette, StudioAction::NextPalette},
        {ControlAction::TogglePlayback, StudioAction::TogglePlayback},
    };
    StudioState reference;
    ControlState s = initialControlState();
    for (int round = 0; round < 7; ++round)
        for (const auto& [control, studio] : pairs) {
            applyAction(reference, studio);
            s = press(s, control);
            check(s.studio.motif == reference.motif && s.studio.palette == reference.palette &&
                      s.studio.playing == reference.playing,
                  std::string("starter action matches applyAction: ") + controlActionLabel(control));
        }
}

// --- Limits ---------------------------------------------------------------------------

void checkLimits(ControlAction down, ControlAction up, double lo, double hi,
                 const std::function<double(const ControlState&)>& get, const std::string& name) {
    ControlState s = initialControlState();
    for (int i = 0; i < 400; ++i) {
        s = press(s, up);
        check(withinControlLimits(s), name + " never leaves its range going up");
    }
    check(get(s) <= hi + 1e-9 && get(s) > hi - 9.0, name + " stops at its upper limit");
    ControlResult r = reduceControl(s, up);
    check(!r.accepted && !r.reason.empty(), name + " step past the upper limit is rejected");
    check(sameControls(r.state, s), name + " rejected step leaves the state unchanged");
    check(!r.sceneChanged(), name + " rejected step does not request a redraw");

    for (int i = 0; i < 400; ++i) s = press(s, down);
    check(get(s) >= lo - 1e-9 && get(s) < lo + 9.0, name + " stops at its lower limit");
    r = reduceControl(s, down);
    check(!r.accepted, name + " step past the lower limit is rejected");
    check(sameControls(r.state, s), name + " rejected lower step leaves the state unchanged");
}

void testLimits() {
    using L = ControlLimits;
    checkLimits(ControlAction::TranslateLeft, ControlAction::TranslateRight, -L::translateMax,
                L::translateMax, [](const ControlState& s) { return s.pattern.transform.translation.x; },
                "translation x");
    checkLimits(ControlAction::TranslateUp, ControlAction::TranslateDown, -L::translateMax,
                L::translateMax, [](const ControlState& s) { return s.pattern.transform.translation.y; },
                "translation y");
    checkLimits(ControlAction::ScaleDown, ControlAction::ScaleUp, L::scaleMin, L::scaleMax,
                [](const ControlState& s) { return s.pattern.transform.scale; }, "scale");
    checkLimits(ControlAction::PitchDown, ControlAction::PitchUp, L::pitchMin, L::pitchMax,
                [](const ControlState& s) { return s.pattern.tilePitch.x; }, "tile pitch x");
    checkLimits(ControlAction::PitchDown, ControlAction::PitchUp, L::pitchMin, L::pitchMax,
                [](const ControlState& s) { return s.pattern.tilePitch.y; }, "tile pitch y");
    checkLimits(ControlAction::SpeedDown, ControlAction::SpeedUp, L::speedMin, L::speedMax,
                [](const ControlState& s) { return s.animationSpeed; }, "animation speed");

    // Exact single steps from the defaults.
    const ControlState s0 = initialControlState();
    check(near(press(s0, ControlAction::ScaleUp).pattern.transform.scale, 1.0 + L::scaleStep),
          "scale step is exact");
    check(near(press(s0, ControlAction::PitchUp).pattern.tilePitch.x, 64.0 + L::pitchStep) &&
              near(press(s0, ControlAction::PitchUp).pattern.tilePitch.y, 70.0 + L::pitchStep),
          "pitch step moves both components");
}

void testOutOfRangeStartCanRecover() {
    // A command-line start such as --scale 5 is outside the interactive range.
    ControlState s = initialControlState();
    s.pattern.transform.scale = 5.0;
    check(!withinControlLimits(s), "scale 5 is outside the interactive range");
    check(!reduceControl(s, ControlAction::ScaleUp).accepted, "cannot step further out of range");
    ControlResult r = reduceControl(s, ControlAction::ScaleDown);
    check(r.accepted && near(r.state.pattern.transform.scale, 4.75), "can step back towards the range");
    s = press(s, ControlAction::ScaleDown, 20);
    check(withinControlLimits(s), "repeated steps return inside the range");
}

// --- Rotation and reflection ------------------------------------------------------------

void testRotation() {
    const double step = ControlLimits::rotateStepRadians();
    ControlState s = press(initialControlState(), ControlAction::RotateCounterclockwise);
    check(near(s.pattern.transform.angleRadians, -step),
          "counterclockwise on screen decreases the angle (y points down)");
    check(near(radiansToDegrees(s.pattern.transform.angleRadians), -15.0), "step is 15 degrees");

    // Confirm the screen direction with Jeremy's matrix: a negative angle moves
    // the point to the right of the centre (+x) upwards (towards -y).
    TransformParams p;
    p.angleRadians = s.pattern.transform.angleRadians;
    const Point2D moved = transformPoint(transformMatrix(p), {10.0, 0.0});
    check(moved.y < 0.0, "Q turns +x towards screen up, which is counterclockwise on screen");

    s = press(s, ControlAction::RotateClockwise, 2);
    check(near(s.pattern.transform.angleRadians, step), "clockwise increases the angle");

    ControlState full = initialControlState();
    for (int i = 0; i < 24; ++i) {
        full = press(full, ControlAction::RotateClockwise);
        const double a = full.pattern.transform.angleRadians;
        check(a > -kPi - 1e-12 && a <= kPi + 1e-12, "angle stays in (-pi, pi]");
    }
    check(near(full.pattern.transform.angleRadians, 0.0), "24 steps make a full turn back to zero");
    full = press(full, ControlAction::RotateCounterclockwise, 12);
    check(near(std::fabs(full.pattern.transform.angleRadians), kPi), "half turn reaches 180 degrees");
}

void testReflection() {
    const ControlState base = reduceControls(initialControlState(),
        {ControlAction::RotateClockwise, ControlAction::ScaleUp, ControlAction::TranslateLeft,
         ControlAction::PitchUp, ControlAction::NextMotif});
    ControlState s = press(base, ControlAction::ReflectAcrossXAxis);
    check(s.pattern.transform.reflectY && !s.pattern.transform.reflectX,
          "X key reflects across the x-axis, which negates y (reflectY)");
    check(sameControls(press(s, ControlAction::ReflectAcrossXAxis), base),
          "reflecting across the x-axis twice restores the state");
    s = press(base, ControlAction::ReflectAcrossYAxis);
    check(s.pattern.transform.reflectX && !s.pattern.transform.reflectY,
          "Y key reflects across the y-axis, which negates x (reflectX)");
    check(sameControls(press(s, ControlAction::ReflectAcrossYAxis), base),
          "reflecting across the y-axis twice restores the state");
}

// --- Reset, playback and effects ------------------------------------------------------

void testResetClearsEverything() {
    ControlState s = reduceControls(initialControlState(),
        {ControlAction::NextMotif, ControlAction::NextPalette, ControlAction::TranslateLeft,
         ControlAction::TranslateUp, ControlAction::RotateClockwise, ControlAction::ScaleUp,
         ControlAction::ReflectAcrossXAxis, ControlAction::ReflectAcrossYAxis,
         ControlAction::PitchUp, ControlAction::TogglePlayback,
         ControlAction::CycleAnimationMode, ControlAction::SpeedUp});
    s.studio.timeSeconds = 3.5;
    check(!sameControls(s, initialControlState()), "fixture changed every field");
    const ControlResult r = reduceControl(s, ControlAction::Reset);
    check(r.accepted && r.sceneChanged(), "reset is accepted and redraws");
    check(sameControls(r.state, initialControlState()), "reset restores every field, time included");

    // Reset also restores a command-line start to the defaults, as before.
    PatternSceneOptions options;
    options.state.motif = 2;
    options.settings.transform.angleRadians = degreesToRadians(20);
    check(sameControls(press(controlStateFrom(options), ControlAction::Reset), initialControlState()),
          "reset from launch options returns to the defaults");
}

void testPlayback() {
    ControlState s = initialControlState();
    s = press(s, ControlAction::TogglePlayback);
    check(s.studio.playing, "space starts playback");
    advanceStudio(s.studio, 0.25);
    check(near(s.studio.timeSeconds, 0.25), "playing clock advances");
    s = press(s, ControlAction::TogglePlayback);
    check(!s.studio.playing, "space pauses playback");
    advanceStudio(s.studio, 1.0);
    check(near(s.studio.timeSeconds, 0.25), "paused clock stays still");
    s = press(s, ControlAction::TogglePlayback);
    advanceStudio(s.studio, 0.1);
    check(near(s.studio.timeSeconds, 0.35), "resume continues without a time jump");
}

void testSpeedChangeKeepsTheFrame() {
    ControlState s = initialControlState();
    s.animation.mode = AnimationMode::Rotate;
    s.studio.playing = true;
    s.studio.timeSeconds = 1.3;
    const double before = displayedPatternSettings(s, s.studio.timeSeconds).transform.angleRadians;
    ControlState faster = press(s, ControlAction::SpeedUp);
    const double after =
        displayedPatternSettings(faster, faster.studio.timeSeconds).transform.angleRadians;
    check(near(before, after), "speeding up keeps the rotation frame on screen (no jump)");
    check(near(effectiveAnimation(faster).angularSpeed, s.animation.angularSpeed * 1.25),
          "speed multiplier scales the angular speed");

    s.animation.mode = AnimationMode::Pulse;
    const double scaleBefore = displayedPatternSettings(s, s.studio.timeSeconds).transform.scale;
    ControlState slower = press(s, ControlAction::SpeedDown);
    check(near(scaleBefore,
               displayedPatternSettings(slower, slower.studio.timeSeconds).transform.scale),
          "slowing down keeps the pulse frame on screen (no jump)");
    check(near(s.pattern.transform.scale, 1.0) && near(slower.pattern.transform.scale, 1.0),
          "evaluating the animation never overwrites the base scale");
}

void testEffects() {
    const ControlState s = press(initialControlState(), ControlAction::ScaleUp);
    const ControlResult e = reduceControl(s, ControlAction::Export);
    check(e.accepted && e.effect == ControlEffect::Export && !e.sceneChanged() &&
              sameControls(e.state, s),
          "export requests a file without changing the scene");
    const ControlResult c = reduceControl(s, ControlAction::Close);
    check(c.accepted && c.effect == ControlEffect::Close && sameControls(c.state, s),
          "close requests shutdown without changing the scene");
    for (ControlAction a : kAllControlActions) {
        const ControlResult r = reduceControl(s, a);
        if (a != ControlAction::Export && a != ControlAction::Close)
            check(r.effect == ControlEffect::None, std::string("no side effect: ") + controlActionLabel(a));
        if (r.accepted && r.effect == ControlEffect::None)
            check(r.sceneChanged(), std::string("accepted change redraws: ") + controlActionLabel(a));
    }
}

// --- Key map and repeat policy ----------------------------------------------------------

void testKeyMap() {
    std::set<int> keys, actions;
    for (const ControlKeyBinding& b : controlKeyBindings()) {
        check(keys.insert(static_cast<int>(b.key)).second, std::string("key bound once: ") + b.label);
        check(actions.insert(static_cast<int>(b.action)).second,
              std::string("action bound once: ") + controlActionLabel(b.action));
    }
    check(actions.size() == kAllControlActions.size(), "every action has a key");

    const std::vector<std::pair<ControlKey, ControlAction>> expected{
        {ControlKey::M, ControlAction::NextMotif}, {ControlKey::P, ControlAction::NextPalette},
        {ControlKey::R, ControlAction::Reset}, {ControlKey::S, ControlAction::Export},
        {ControlKey::Escape, ControlAction::Close},
        {ControlKey::Left, ControlAction::TranslateLeft}, {ControlKey::Right, ControlAction::TranslateRight},
        {ControlKey::Up, ControlAction::TranslateUp}, {ControlKey::Down, ControlAction::TranslateDown},
        {ControlKey::Q, ControlAction::RotateCounterclockwise}, {ControlKey::E, ControlAction::RotateClockwise},
        {ControlKey::Minus, ControlAction::ScaleDown}, {ControlKey::Equal, ControlAction::ScaleUp},
        {ControlKey::X, ControlAction::ReflectAcrossXAxis}, {ControlKey::Y, ControlAction::ReflectAcrossYAxis},
        {ControlKey::LeftBracket, ControlAction::PitchDown}, {ControlKey::RightBracket, ControlAction::PitchUp},
        {ControlKey::Space, ControlAction::TogglePlayback}, {ControlKey::A, ControlAction::CycleAnimationMode},
        {ControlKey::Comma, ControlAction::SpeedDown}, {ControlKey::Period, ControlAction::SpeedUp},
    };
    for (const auto& [key, action] : expected) {
        const auto found = actionForKey(key);
        check(found && *found == action, std::string("planned key mapping: ") + controlActionLabel(action));
    }
}

void testRepeatPolicy() {
    for (ControlAction a : kAllControlActions)
        check(shouldDispatch(a, false), std::string("first press always dispatches: ") + controlActionLabel(a));
    for (ControlAction a : {ControlAction::TranslateLeft, ControlAction::RotateClockwise,
                            ControlAction::ScaleUp, ControlAction::PitchDown, ControlAction::SpeedUp})
        check(shouldDispatch(a, true), std::string("held adjustment repeats: ") + controlActionLabel(a));
    for (ControlAction a : {ControlAction::NextMotif, ControlAction::NextPalette,
                            ControlAction::ReflectAcrossXAxis, ControlAction::ReflectAcrossYAxis,
                            ControlAction::TogglePlayback, ControlAction::CycleAnimationMode,
                            ControlAction::Reset, ControlAction::Export, ControlAction::Close})
        check(!shouldDispatch(a, true), std::string("held toggle fires once: ") + controlActionLabel(a));
}

void testScriptedCallback() {
    // Same path as the desktop callback: key -> action -> repeat filter -> reducer.
    struct Event { ControlKey key; bool repeat; };
    const std::vector<Event> events{
        {ControlKey::Right, false}, {ControlKey::Right, true}, {ControlKey::Right, true},
        {ControlKey::Q, false}, {ControlKey::X, false}, {ControlKey::X, true}, {ControlKey::X, true},
        {ControlKey::Space, false}, {ControlKey::Space, true}, {ControlKey::RightBracket, false},
    };
    ControlState s = initialControlState();
    for (const Event& e : events) {
        const auto action = actionForKey(e.key);
        if (action && shouldDispatch(*action, e.repeat)) s = reduceControl(s, *action).state;
    }
    check(near(s.pattern.transform.translation.x, 3 * ControlLimits::translateStep),
          "held right arrow moves three steps");
    check(near(s.pattern.transform.angleRadians, -ControlLimits::rotateStepRadians()), "Q rotates once");
    check(s.pattern.transform.reflectY, "held X toggles exactly once");
    check(s.studio.playing, "held space toggles exactly once");
    check(near(s.pattern.tilePitch.x, 72.0), "] widens the tile pitch");
}

// --- Readout ------------------------------------------------------------------------

void testStatusLines() {
    ControlState s = reduceControls(initialControlState(),
        {ControlAction::RotateCounterclockwise, ControlAction::TranslateLeft,
         ControlAction::TranslateUp, ControlAction::ReflectAcrossXAxis,
         ControlAction::ReflectAcrossYAxis, ControlAction::TogglePlayback,
         ControlAction::CycleAnimationMode, ControlAction::SpeedUp, ControlAction::ScaleUp});
    const std::vector<std::string> lines = controlStatusLines(s);
    check(lines.size() == 4, "four readout lines");
    for (const std::string& line : lines) {
        check(line.size() <= static_cast<std::size_t>(kControlStatusMaxChars),
              "readout line fits the header: " + line);
        check(std::all_of(line.begin(), line.end(), studioFontSupports),
              "readout uses only font glyphs: " + line);
    }
    check(lines[0] == "MOVE X -8 Y -8   ROTATE -15 DEG", "translation and degrees: " + lines[0]);
    check(lines[1] == "SCALE 1.25   PITCH 64 X 70", "scale and pitch: " + lines[1]);
    check(lines[2] == "MIRROR X-AXIS + Y-AXIS", "mirror state: " + lines[2]);
    check(lines[3] == "PLAYING   MODE ROTATE   SPEED 1.25X", "playback state: " + lines[3]);

    // Widest values still fit and never print "-0".
    ControlState wide = initialControlState();
    wide = press(wide, ControlAction::TranslateLeft, 100);
    wide = press(wide, ControlAction::TranslateUp, 100);
    wide = press(wide, ControlAction::RotateCounterclockwise, 11);
    wide = press(wide, ControlAction::PitchUp, 50);
    for (const std::string& line : controlStatusLines(wide))
        check(line.size() <= static_cast<std::size_t>(kControlStatusMaxChars), "widest line fits: " + line);
    ControlState back = press(press(initialControlState(), ControlAction::RotateClockwise),
                              ControlAction::RotateCounterclockwise);
    check(controlStatusLines(back)[0].find("-0") == std::string::npos, "no negative zero");

    for (const ControlKeyBinding& b : controlKeyBindings()) {
        const std::string label = b.label;
        check(std::all_of(label.begin(), label.end(), studioFontSupports), "key label drawable: " + label);
    }
}

// --- Rendering: every control reaches both the fabric and the dress -----------------------

struct Box { int x0, y0, x1, y1; };

Box boundsOf(const Polygon& p) {
    double x0 = p[0].x, y0 = p[0].y, x1 = x0, y1 = y0;
    for (const Point2D& q : p) {
        x0 = std::min(x0, q.x); y0 = std::min(y0, q.y);
        x1 = std::max(x1, q.x); y1 = std::max(y1, q.y);
    }
    return {static_cast<int>(x0), static_cast<int>(y0), static_cast<int>(x1), static_cast<int>(y1)};
}

bool changed(const Canvas& a, const Canvas& b, Box r) {
    for (int y = r.y0; y <= r.y1; ++y)
        for (int x = r.x0; x <= r.x1; ++x) {
            const Color p = a.pixelAt(x, y), q = b.pixelAt(x, y);
            if (p.red != q.red || p.green != q.green || p.blue != q.blue) return true;
        }
    return false;
}

Canvas render(const ControlState& s) {
    Canvas c(studioWidth, studioHeight, {0, 0, 0});
    renderStudio(c, s.studio, makeControlledHooks(s));
    drawControlStatus(c, s);
    return c;
}

void testEveryControlUpdatesFabricAndDress() {
    ControlState base = initialControlState();
    base.studio.motif = 3; // STEPS: asymmetric in x and y, so both mirrors are visible
    const Canvas before = render(base);
    const Box fabric = boundsOf(studioFabricArea());
    const Box bodice = boundsOf(studioBodice());
    const Box skirt = boundsOf(studioSkirt());
    const Box header{kControlStatusArea.x, kControlStatusArea.y,
                     kControlStatusArea.x + kControlStatusArea.width - 1,
                     kControlStatusArea.y + kControlStatusArea.height - 1};

    for (ControlAction a : {ControlAction::NextMotif, ControlAction::NextPalette,
                            ControlAction::TranslateLeft, ControlAction::TranslateRight,
                            ControlAction::TranslateUp, ControlAction::TranslateDown,
                            ControlAction::RotateCounterclockwise, ControlAction::RotateClockwise,
                            ControlAction::ScaleDown, ControlAction::ScaleUp,
                            ControlAction::ReflectAcrossXAxis, ControlAction::ReflectAcrossYAxis,
                            ControlAction::PitchDown, ControlAction::PitchUp}) {
        const ControlState next = press(base, a);
        const Canvas after = render(next);
        const std::string name = controlActionLabel(a);
        check(changed(before, after, fabric), "updates the fabric preview: " + name);
        check(changed(before, after, bodice), "updates the bodice: " + name);
        check(changed(before, after, skirt), "updates the skirt: " + name);
    }

    // Playback-only changes update the readout but not the base picture.
    const Canvas playing = render(press(base, ControlAction::TogglePlayback));
    check(changed(before, playing, header), "play state appears in the readout");
    check(!changed(before, playing, fabric), "starting playback at t = 0 keeps the frame");

    // A running animation moves both views over time.
    ControlState anim = press(press(base, ControlAction::CycleAnimationMode), ControlAction::TogglePlayback);
    const Canvas t0 = render(anim);
    advanceStudio(anim.studio, 0.5);
    const Canvas t1 = render(anim);
    check(changed(t0, t1, fabric) && changed(t0, t1, skirt), "rotation animation updates fabric and dress");

    // Reset gives back exactly the initial image.
    ControlState messy = reduceControls(base, {ControlAction::ScaleUp, ControlAction::ReflectAcrossXAxis,
                                               ControlAction::TogglePlayback});
    messy.studio.timeSeconds = 2.0;
    const Canvas reset = render(press(messy, ControlAction::Reset));
    const Canvas initial = render(initialControlState());
    check(!changed(reset, initial, {0, 0, studioWidth - 1, studioHeight - 1}),
          "reset restores the initial image pixel for pixel");
}

void testReadoutStaysInHeader() {
    // The readout must not cover the previews: compare renders with and without it.
    const ControlState s = reduceControls(initialControlState(),
        {ControlAction::TranslateLeft, ControlAction::RotateClockwise, ControlAction::TogglePlayback});
    Canvas with(studioWidth, studioHeight, {0, 0, 0}), without(studioWidth, studioHeight, {0, 0, 0});
    renderStudio(without, s.studio, makeControlledHooks(s));
    with = without;
    drawControlStatus(with, s);
    bool insideOnly = true, drewSomething = false;
    for (int y = 0; y < studioHeight; ++y)
        for (int x = 0; x < studioWidth; ++x) {
            const Color p = with.pixelAt(x, y), q = without.pixelAt(x, y);
            if (p.red == q.red && p.green == q.green && p.blue == q.blue) continue;
            drewSomething = true;
            const bool inside = x >= kControlStatusArea.x && x < kControlStatusArea.x + kControlStatusArea.width &&
                                y >= kControlStatusArea.y && y < kControlStatusArea.y + kControlStatusArea.height;
            if (!inside || y >= 110) insideOnly = false;
        }
    check(drewSomething, "readout draws text");
    check(insideOnly, "readout only draws inside the header band, never over the previews");
}

// --- Bezier arch motif ------------------------------------------------------------------

void testCurveMotif() {
    const MotifShape m = bezierArchMotif();
    check(isValidMotif(m), "Bezier arch passes the catalog validator");
    check(m.fills.size() == 2 && m.strokes.size() == 2, "two fills and two strokes");
    check(isValidMotif(bezierArchMotif(2)), "valid even at 2 segments per curve");

    // Control points inside the extent imply the whole curve is inside (convex hull).
    for (const CubicControls& c : bezierArchControlPoints())
        for (const Point2D& p : c)
            check(std::fabs(p.x) <= kMotifHalfExtent && std::fabs(p.y) <= kMotifHalfExtent,
                  "control point inside the motif extent");

    // Valid endpoints: each sampled segment starts at P0 and ends at P3, and the
    // outline and leaf are closed chains.
    for (const CubicControls& c : bezierArchControlPoints()) {
        const std::vector<Point2D> samples = sampleCubicBezier(c, 16);
        check(samples.size() == 17, "16 segments give 17 samples");
        check(near(samples.front().x, c[0].x) && near(samples.front().y, c[0].y), "curve starts at P0");
        check(near(samples.back().x, c[3].x) && near(samples.back().y, c[3].y), "curve ends at P3");
        for (const Point2D& p : samples)
            check(std::isfinite(p.x) && std::isfinite(p.y), "sample is finite");
    }
    for (const auto* chain : {&bezierArchOutlineControls(), &bezierArchLeafControls()}) {
        for (std::size_t i = 0; i < chain->size(); ++i) {
            const Point2D end = (*chain)[i][3], next = (*chain)[(i + 1) % chain->size()][0];
            check(near(end.x, next.x) && near(end.y, next.y), "segments join end to start");
        }
    }

    // Leaf halves are mirror images across x = 0.
    const auto& leaf = bezierArchLeafControls();
    for (int k = 0; k < 4; ++k)
        check(near(leaf[0][k].x, -leaf[1][3 - k].x) && near(leaf[0][k].y, leaf[1][3 - k].y),
              "leaf halves mirror each other");

    // Joined sampling drops shared joints exactly once.
    const std::vector<Point2D> joined = sampleJoinedCubics(bezierArchOutlineControls(), 12);
    check(joined.size() == 3 * 12 + 1, "three joined segments share their joints");

    // In the catalog.
    bool found = false;
    for (int i = 0; i < kMotifCount; ++i)
        if (std::string(catalogMotifLabel(i)) == kBezierArchLabel) found = true;
    check(found, "Bezier arch is registered in the catalog");
}

void testCurveMotifIsTransformable() {
    // Affine invariance: sampling transformed control points equals
    // transforming the samples, so the sampled stroke stays on the true curve.
    TransformParams p;
    p.translation = {13.0, -7.0};
    p.angleRadians = 0.7;
    p.scale = 1.8;
    p.reflectY = true;
    const Mat3 m = transformMatrix(p);
    for (const CubicControls& c : bezierArchControlPoints()) {
        CubicControls moved;
        for (int k = 0; k < 4; ++k) moved[k] = transformPoint(m, c[k]);
        const std::vector<Point2D> a = sampleCubicBezier(moved, 20);
        const std::vector<Point2D> b = transformPath(m, sampleCubicBezier(c, 20));
        bool same = a.size() == b.size();
        for (std::size_t i = 0; same && i < a.size(); ++i) same = near(a[i].x, b[i].x, 1e-9) && near(a[i].y, b[i].y, 1e-9);
        check(same, "transforming control points equals transforming samples");
    }
}

void testCurveStrokesAreClipped() {
    // Rendered through Kelvin's engine into a small convex window: no pixel
    // may change outside it, even with the strokes crossing every edge.
    const Color paper{1, 2, 3};
    Canvas canvas(220, 220, paper);
    const Polygon window{{60, 50}, {170, 70}, {150, 170}, {50, 150}};
    PatternSettings s = defaultStudioPatternSettings();
    s.origin = {110, 110};
    s.transform.scale = 1.6;
    s.transform.angleRadians = 0.4;
    renderPattern(canvas, bezierArchMotif(), {{240, 240, 240}, {200, 40, 40}, {20, 20, 120}}, s, window);
    int outside = 0, inside = 0;
    for (int y = 0; y < 220; ++y)
        for (int x = 0; x < 220; ++x) {
            const Color c = canvas.pixelAt(x, y);
            const bool painted = !(c.red == paper.red && c.green == paper.green && c.blue == paper.blue);
            const bool inBounds = x >= 50 && x <= 170 && y >= 50 && y <= 170;
            if (painted && !inBounds) ++outside;
            if (painted && inBounds) ++inside;
        }
    check(inside > 0, "curve motif draws inside the window");
    check(outside == 0, "curve strokes never spill outside the clipping window's bounds");
}

} // namespace

int main() {
    try {
        testInitialState();
        testSelectionWraparound();
        testStarterActionsUnchanged();
        testLimits();
        testOutOfRangeStartCanRecover();
        testRotation();
        testReflection();
        testResetClearsEverything();
        testPlayback();
        testSpeedChangeKeepsTheFrame();
        testEffects();
        testKeyMap();
        testRepeatPolicy();
        testScriptedCallback();
        testStatusLines();
        testEveryControlUpdatesFabricAndDress();
        testReadoutStaysInHeader();
        testCurveMotif();
        testCurveMotifIsTransformable();
        testCurveStrokesAreClipped();
    } catch (const std::exception& e) {
        std::cerr << "Unexpected exception: " << e.what() << "\n";
        return 1;
    }
    if (failures == 0) {
        std::cout << "Controls, key map, readout and Bezier arch motif passed.\n";
        return 0;
    }
    std::cerr << failures << " control check(s) failed\n";
    return 1;
}
