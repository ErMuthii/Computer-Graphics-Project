// Interaction layer: action reducer, key map and parameter readout.
// Owner: Andrew Karanja Gathirwa, 167144.
#include "kitenge/controls.hpp"

#include <cctype>
#include <cmath>
#include <cstdio>
#include <set>

namespace kitenge {

namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kEpsilon = 1e-9;

ControlResult accepted(ControlState next) { return {std::move(next), true, ControlEffect::None, {}}; }

ControlResult rejected(const ControlState& unchanged, std::string why) {
    return {unchanged, false, ControlEffect::None, std::move(why)};
}

bool inRange(double value, double lo, double hi) {
    return std::isfinite(value) && value >= lo - kEpsilon && value <= hi + kEpsilon;
}

// A step is allowed when it lands inside [lo, hi], or when the value started
// outside the range and the step moves it back towards the range.
bool stepAllowed(double current, double delta, double lo, double hi) {
    const double proposed = current + delta;
    if (!std::isfinite(proposed)) return false;
    if (inRange(proposed, lo, hi)) return true;
    if (proposed < lo && delta > 0.0) return true;
    if (proposed > hi && delta < 0.0) return true;
    return false;
}

// `field` refers to a member of `next`, so `next` is taken by reference and
// only copied into the result once the step is known to be allowed.
ControlResult stepValue(const ControlState& state, ControlState& next, double& field, double delta,
                        double lo, double hi, const char* name) {
    if (!stepAllowed(field, delta, lo, hi)) return rejected(state, std::string(name) + " limit reached");
    field += delta;
    return accepted(next);
}

// Wraps into (-pi, pi]. Whole turns only, so the drawn result is identical.
double wrapAngle(double radians) {
    double r = std::fmod(radians, 2.0 * kPi);
    if (r <= -kPi) r += 2.0 * kPi;
    if (r > kPi) r -= 2.0 * kPi;
    if (std::fabs(r) < 1e-12) r = 0.0; // 24 x 15 degrees returns exactly to zero
    return r;
}

ControlResult rotate(const ControlState& state, double deltaRadians) {
    ControlState next = state;
    const double angle = next.pattern.transform.angleRadians;
    if (!std::isfinite(angle)) return rejected(state, "angle is not finite");
    next.pattern.transform.angleRadians = wrapAngle(angle + deltaRadians);
    return accepted(std::move(next));
}

ControlResult stepPitch(const ControlState& state, double delta) {
    ControlState next = state;
    Point2D& pitch = next.pattern.tilePitch;
    if (!stepAllowed(pitch.x, delta, ControlLimits::pitchMin, ControlLimits::pitchMax) ||
        !stepAllowed(pitch.y, delta, ControlLimits::pitchMin, ControlLimits::pitchMax))
        return rejected(state, "tile pitch limit reached");
    pitch.x += delta;
    pitch.y += delta;
    return accepted(std::move(next));
}

// Changing the speed multiplier from m to m' rescales the clock by m / m'.
// Both animated quantities depend only on (speed * time), so the frame on
// screen is identical before and after the change: no jump.
ControlResult stepSpeed(const ControlState& state, double delta) {
    if (!stepAllowed(state.animationSpeed, delta, ControlLimits::speedMin, ControlLimits::speedMax))
        return rejected(state, "animation speed limit reached");
    ControlState next = state;
    next.animationSpeed = state.animationSpeed + delta;
    next.studio.timeSeconds = state.studio.timeSeconds * state.animationSpeed / next.animationSpeed;
    return accepted(std::move(next));
}

ControlResult viaStudioAction(const ControlState& state, StudioAction action) {
    ControlState next = state;
    applyAction(next.studio, action);
    return accepted(std::move(next));
}

std::string upper(const char* text) {
    std::string out = text;
    for (char& c : out) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return out;
}

// Integer formatting that never prints "-0".
std::string whole(double value) {
    long long n = std::llround(value);
    return std::to_string(n);
}

std::string twoDecimals(double value) {
    char buffer[32];
    std::snprintf(buffer, sizeof buffer, "%.2f", value);
    return buffer;
}

} // namespace

// --- Labels and repeat policy ---------------------------------------------------

const char* controlActionLabel(ControlAction action) noexcept {
    switch (action) {
        case ControlAction::NextMotif: return "NEXT MOTIF";
        case ControlAction::NextPalette: return "NEXT PALETTE";
        case ControlAction::Reset: return "RESET";
        case ControlAction::Export: return "EXPORT";
        case ControlAction::Close: return "CLOSE";
        case ControlAction::TranslateLeft: return "MOVE LEFT";
        case ControlAction::TranslateRight: return "MOVE RIGHT";
        case ControlAction::TranslateUp: return "MOVE UP";
        case ControlAction::TranslateDown: return "MOVE DOWN";
        case ControlAction::RotateCounterclockwise: return "ROTATE COUNTERCLOCKWISE";
        case ControlAction::RotateClockwise: return "ROTATE CLOCKWISE";
        case ControlAction::ScaleDown: return "SCALE DOWN";
        case ControlAction::ScaleUp: return "SCALE UP";
        case ControlAction::ReflectAcrossXAxis: return "MIRROR X-AXIS";
        case ControlAction::ReflectAcrossYAxis: return "MIRROR Y-AXIS";
        case ControlAction::PitchDown: return "PITCH DOWN";
        case ControlAction::PitchUp: return "PITCH UP";
        case ControlAction::TogglePlayback: return "PLAY/PAUSE";
        case ControlAction::CycleAnimationMode: return "ANIMATION MODE";
        case ControlAction::SpeedDown: return "SPEED DOWN";
        case ControlAction::SpeedUp: return "SPEED UP";
    }
    return "UNKNOWN";
}

bool isRepeatable(ControlAction action) noexcept {
    switch (action) {
        case ControlAction::TranslateLeft:
        case ControlAction::TranslateRight:
        case ControlAction::TranslateUp:
        case ControlAction::TranslateDown:
        case ControlAction::RotateCounterclockwise:
        case ControlAction::RotateClockwise:
        case ControlAction::ScaleDown:
        case ControlAction::ScaleUp:
        case ControlAction::PitchDown:
        case ControlAction::PitchUp:
        case ControlAction::SpeedDown:
        case ControlAction::SpeedUp:
            return true;
        default:
            return false;
    }
}

// --- State ------------------------------------------------------------------------

ControlState initialControlState() { return ControlState{}; }

ControlState controlStateFrom(const PatternSceneOptions& options) {
    ControlState state;
    state.studio = options.state;
    state.pattern = options.settings;
    return state;
}

bool withinControlLimits(const ControlState& s) {
    const PatternTransform& t = s.pattern.transform;
    return s.studio.motif >= 0 && s.studio.motif < kMotifCount &&
           s.studio.palette >= 0 && s.studio.palette < kPaletteCount &&
           std::isfinite(s.studio.timeSeconds) && s.studio.timeSeconds >= 0.0 &&
           inRange(t.translation.x, -ControlLimits::translateMax, ControlLimits::translateMax) &&
           inRange(t.translation.y, -ControlLimits::translateMax, ControlLimits::translateMax) &&
           std::isfinite(t.angleRadians) && t.angleRadians > -kPi - kEpsilon &&
           t.angleRadians <= kPi + kEpsilon &&
           inRange(t.scale, ControlLimits::scaleMin, ControlLimits::scaleMax) &&
           inRange(s.pattern.tilePitch.x, ControlLimits::pitchMin, ControlLimits::pitchMax) &&
           inRange(s.pattern.tilePitch.y, ControlLimits::pitchMin, ControlLimits::pitchMax) &&
           inRange(s.animationSpeed, ControlLimits::speedMin, ControlLimits::speedMax);
}

// --- Reducer ----------------------------------------------------------------------

ControlResult reduceControl(const ControlState& state, ControlAction action) {
    ControlState next = state;
    PatternTransform& t = next.pattern.transform;
    constexpr double move = ControlLimits::translateStep;
    constexpr double maxMove = ControlLimits::translateMax;

    switch (action) {
        case ControlAction::NextMotif: return viaStudioAction(state, StudioAction::NextMotif);
        case ControlAction::NextPalette: return viaStudioAction(state, StudioAction::NextPalette);
        case ControlAction::TogglePlayback: return viaStudioAction(state, StudioAction::TogglePlayback);
        case ControlAction::Reset: {
            ControlState fresh = initialControlState();
            applyAction(fresh.studio, StudioAction::Reset);
            return accepted(std::move(fresh));
        }
        case ControlAction::Export: return {state, true, ControlEffect::Export, {}};
        case ControlAction::Close: return {state, true, ControlEffect::Close, {}};

        case ControlAction::TranslateLeft:
            return stepValue(state, next, t.translation.x, -move, -maxMove, maxMove, "translation");
        case ControlAction::TranslateRight:
            return stepValue(state, next, t.translation.x, move, -maxMove, maxMove, "translation");
        case ControlAction::TranslateUp: // y down: up the screen is negative y
            return stepValue(state, next, t.translation.y, -move, -maxMove, maxMove, "translation");
        case ControlAction::TranslateDown:
            return stepValue(state, next, t.translation.y, move, -maxMove, maxMove, "translation");

        // Positive angles appear clockwise because screen y points down.
        case ControlAction::RotateCounterclockwise:
            return rotate(state, -ControlLimits::rotateStepRadians());
        case ControlAction::RotateClockwise:
            return rotate(state, ControlLimits::rotateStepRadians());

        case ControlAction::ScaleDown:
            return stepValue(state, next, t.scale, -ControlLimits::scaleStep, ControlLimits::scaleMin,
                             ControlLimits::scaleMax, "scale");
        case ControlAction::ScaleUp:
            return stepValue(state, next, t.scale, ControlLimits::scaleStep, ControlLimits::scaleMin,
                             ControlLimits::scaleMax, "scale");

        case ControlAction::ReflectAcrossXAxis: // negate y
            t.reflectY = !t.reflectY;
            return accepted(std::move(next));
        case ControlAction::ReflectAcrossYAxis: // negate x
            t.reflectX = !t.reflectX;
            return accepted(std::move(next));

        case ControlAction::PitchDown: return stepPitch(state, -ControlLimits::pitchStep);
        case ControlAction::PitchUp: return stepPitch(state, ControlLimits::pitchStep);

        case ControlAction::CycleAnimationMode:
            next.animation.mode = nextAnimationMode(state.animation.mode);
            return accepted(std::move(next));
        case ControlAction::SpeedDown: return stepSpeed(state, -ControlLimits::speedStep);
        case ControlAction::SpeedUp: return stepSpeed(state, ControlLimits::speedStep);
    }
    return rejected(state, "unknown action");
}

ControlState reduceControls(ControlState state, const std::vector<ControlAction>& actions) {
    for (ControlAction a : actions) state = reduceControl(state, a).state;
    return state;
}

// --- Rendering bridge -------------------------------------------------------------

AnimationSettings effectiveAnimation(const ControlState& state) {
    AnimationSettings a = state.animation;
    a.angularSpeed *= state.animationSpeed;
    a.pulseFrequency *= state.animationSpeed;
    return a;
}

PatternSettings displayedPatternSettings(const ControlState& state, double timeSeconds) {
    return animatePatternSettings(state.pattern, effectiveAnimation(state), timeSeconds);
}

StudioHooks makeControlledHooks(const ControlState& live) {
    const ControlState* source = &live;
    return makePatternHooks([source](const StudioState& studio) {
        return displayedPatternSettings(*source, studio.timeSeconds);
    });
}

// --- Key map ----------------------------------------------------------------------

const std::vector<ControlKeyBinding>& controlKeyBindings() {
    static const std::vector<ControlKeyBinding> table{
        {ControlKey::M, ControlAction::NextMotif, "M"},
        {ControlKey::P, ControlAction::NextPalette, "P"},
        {ControlKey::Left, ControlAction::TranslateLeft, "LEFT"},
        {ControlKey::Right, ControlAction::TranslateRight, "RIGHT"},
        {ControlKey::Up, ControlAction::TranslateUp, "UP"},
        {ControlKey::Down, ControlAction::TranslateDown, "DOWN"},
        {ControlKey::Q, ControlAction::RotateCounterclockwise, "Q"},
        {ControlKey::E, ControlAction::RotateClockwise, "E"},
        {ControlKey::Minus, ControlAction::ScaleDown, "-"},
        {ControlKey::Equal, ControlAction::ScaleUp, "="},
        {ControlKey::X, ControlAction::ReflectAcrossXAxis, "X"},
        {ControlKey::Y, ControlAction::ReflectAcrossYAxis, "Y"},
        {ControlKey::LeftBracket, ControlAction::PitchDown, "["},
        {ControlKey::RightBracket, ControlAction::PitchUp, "]"},
        {ControlKey::Space, ControlAction::TogglePlayback, "SPACE"},
        {ControlKey::A, ControlAction::CycleAnimationMode, "A"},
        {ControlKey::Comma, ControlAction::SpeedDown, ","},
        {ControlKey::Period, ControlAction::SpeedUp, "."},
        {ControlKey::R, ControlAction::Reset, "R"},
        {ControlKey::S, ControlAction::Export, "S"},
        {ControlKey::Escape, ControlAction::Close, "ESC"},
    };
    return table;
}

std::optional<ControlAction> actionForKey(ControlKey key) {
    for (const ControlKeyBinding& b : controlKeyBindings())
        if (b.key == key) return b.action;
    return std::nullopt;
}

bool shouldDispatch(ControlAction action, bool isRepeat) noexcept {
    return !isRepeat || isRepeatable(action);
}

// --- Parameter readout ------------------------------------------------------------

std::vector<std::string> controlStatusLines(const ControlState& s) {
    const PatternTransform& t = s.pattern.transform;
    std::string mirror = "NONE";
    if (t.reflectY && t.reflectX) mirror = "X-AXIS + Y-AXIS";
    else if (t.reflectY) mirror = "X-AXIS";
    else if (t.reflectX) mirror = "Y-AXIS";

    return {
        "MOVE X " + whole(t.translation.x) + " Y " + whole(t.translation.y) +
            "   ROTATE " + whole(radiansToDegrees(t.angleRadians)) + " DEG",
        "SCALE " + twoDecimals(t.scale) + "   PITCH " + whole(s.pattern.tilePitch.x) + " X " +
            whole(s.pattern.tilePitch.y),
        "MIRROR " + mirror,
        std::string(s.studio.playing ? "PLAYING" : "PAUSED") + "   MODE " +
            upper(animationModeLabel(s.animation.mode)) + "   SPEED " +
            twoDecimals(s.animationSpeed) + "X",
    };
}

void drawControlStatus(Canvas& canvas, const ControlState& state) {
    constexpr Color gold{238, 174, 46};
    constexpr int scale = 2, lineHeight = 22;
    int y = kControlStatusArea.y + 6;
    for (const std::string& line : controlStatusLines(state)) {
        drawStudioText(canvas, line, kControlStatusArea.x, y, scale, gold);
        y += lineHeight;
    }
}

} // namespace kitenge
