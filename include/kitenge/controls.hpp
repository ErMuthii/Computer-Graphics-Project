#pragma once

// Interaction layer for the Kitenge studio: actions, limits, key map and the
// on-screen parameter readout.
// Owner: Andrew Karanja Gathirwa, 167144.
//
// Input becomes pixels in four steps:
//
//   GLFW key -> ControlKey -> ControlAction -> reduceControl() -> ControlState
//                                                     |
//      renderStudio(canvas, state.studio, makeControlledHooks(state))
//          -> animatePatternSettings(state.pattern, ..., timeSeconds)   (Lenina)
//          -> transformMatrix / tiling / clipping / rasterizing         (Jeremy, Kelvin)
//
// Only src/desktop_keys.cpp knows GLFW key codes. Everything declared here is
// Pure C++, so every control is tested headlessly with KITENGE_BUILD_DESKTOP=OFF.
//
// ControlState holds the team's existing shared types; it adds no second copy
// of any field. The only new value is the animation speed multiplier.
//
// Conventions follow the shared contract: radians internally and degrees in
// labels; screen y points down, so a positive angle appears clockwise.
//   * Q (rotate counterclockwise on screen) therefore DECREASES the angle.
//   * Up arrow moves the pattern up the screen, i.e. DECREASES translation.y.
//   * X reflects across the x-axis, which negates y: PatternTransform::reflectY.
//   * Y reflects across the y-axis, which negates x: PatternTransform::reflectX.

#include "kitenge/animation.hpp"
#include "kitenge/canvas.hpp"
#include "kitenge/pattern.hpp"
#include "kitenge/studio.hpp"

#include <array>
#include <optional>
#include <string>
#include <vector>

namespace kitenge {

// --- Limits and steps ----------------------------------------------------------
// The single documented place for every interactive limit (docs/controls.md).
// A step that would leave a range is rejected and the state is left unchanged.
// A state that starts outside a range (for example from --scale 5 on the
// command line) may still step back towards it, so the user is never stuck.
struct ControlLimits {
    static constexpr double translateStep = 8.0;   // canvas pixels per press
    static constexpr double translateMax = 256.0;  // |translation.x|, |translation.y|

    static constexpr double rotateStepDegrees = 15.0; // wraps; never clamps

    static constexpr double scaleStep = 0.25;
    static constexpr double scaleMin = 0.25;
    static constexpr double scaleMax = 3.0;

    static constexpr double pitchStep = 8.0;      // added to both pitch components
    static constexpr double pitchMin = 24.0;      // per component
    static constexpr double pitchMax = 176.0;     // per component

    static constexpr double speedStep = 0.25;     // animation speed multiplier
    static constexpr double speedMin = 0.25;
    static constexpr double speedMax = 4.0;
    static constexpr double speedDefault = 1.0;

    static constexpr double rotateStepRadians() {
        return rotateStepDegrees * 3.14159265358979323846 / 180.0;
    }
};

// --- Actions -------------------------------------------------------------------
enum class ControlAction {
    // Existing starter behaviour, unchanged.
    NextMotif,
    NextPalette,
    Reset,
    Export,
    Close,
    // Transform controls.
    TranslateLeft,
    TranslateRight,
    TranslateUp,
    TranslateDown,
    RotateCounterclockwise, // on screen
    RotateClockwise,        // on screen
    ScaleDown,
    ScaleUp,
    ReflectAcrossXAxis,     // negates y
    ReflectAcrossYAxis,     // negates x
    PitchDown,
    PitchUp,
    // Playback (Lenina's animation contract).
    TogglePlayback,
    CycleAnimationMode,
    SpeedDown,
    SpeedUp,
};

inline constexpr std::array<ControlAction, 21> kAllControlActions{{
    ControlAction::NextMotif, ControlAction::NextPalette, ControlAction::Reset,
    ControlAction::Export, ControlAction::Close, ControlAction::TranslateLeft,
    ControlAction::TranslateRight, ControlAction::TranslateUp, ControlAction::TranslateDown,
    ControlAction::RotateCounterclockwise, ControlAction::RotateClockwise,
    ControlAction::ScaleDown, ControlAction::ScaleUp, ControlAction::ReflectAcrossXAxis,
    ControlAction::ReflectAcrossYAxis, ControlAction::PitchDown, ControlAction::PitchUp,
    ControlAction::TogglePlayback, ControlAction::CycleAnimationMode,
    ControlAction::SpeedDown, ControlAction::SpeedUp,
}};

// Upper-case label for logs and documentation.
const char* controlActionLabel(ControlAction action) noexcept;

// True for incremental adjustments that may auto-repeat while a key is held.
// Toggles and one-shot commands fire once per physical press.
bool isRepeatable(ControlAction action) noexcept;

// --- State ---------------------------------------------------------------------
struct ControlState {
    StudioState studio;                                    // Eric: motif, palette, playing, time
    PatternSettings pattern = defaultStudioPatternSettings(); // Kelvin/Jeremy: BASE settings
    AnimationSettings animation;                           // Lenina: mode and base speeds
    double animationSpeed = ControlLimits::speedDefault;   // multiplier on both speeds
};

// Defaults, identical to the starter scene (motif 0, palette 0, paused, t = 0).
ControlState initialControlState();

// Start-up state from the shared --motif/--angle/... options, so the desktop
// and studio_render begin from exactly the same settings.
ControlState controlStateFrom(const PatternSceneOptions& options);

// True when every field is inside the ControlLimits ranges and every index is
// valid for the real catalog sizes. The reducer never leaves this set once in it.
bool withinControlLimits(const ControlState& state);

// --- Reducer -------------------------------------------------------------------
// Export and Close do not change the picture; the application performs them.
enum class ControlEffect { None, Export, Close };

struct ControlResult {
    ControlState state;
    bool accepted = false;          // false: limit reached, state unchanged
    ControlEffect effect = ControlEffect::None;
    std::string reason;             // why it was rejected; empty when accepted

    // True when the canvas must be re-rendered.
    bool sceneChanged() const noexcept { return accepted && effect == ControlEffect::None; }
};

// Pure function: no globals, no I/O, no GLFW. Selection wraps with the real
// catalog counts (kMotifCount, kPaletteCount, kAnimationModeCount).
// NextMotif, NextPalette, TogglePlayback and Reset go through the existing
// applyAction(), so the starter behaviour is preserved by construction.
ControlResult reduceControl(const ControlState& state, ControlAction action);

// Applies a sequence and ignores rejections (scripted tests and demos).
ControlState reduceControls(ControlState state, const std::vector<ControlAction>& actions);

// --- Rendering bridge ----------------------------------------------------------
// Lenina's settings with both speeds multiplied by state.animationSpeed.
AnimationSettings effectiveAnimation(const ControlState& state);

// The settings to draw at `timeSeconds`: Lenina's evaluator applied to the
// user's base settings, which are never modified.
PatternSettings displayedPatternSettings(const ControlState& state, double timeSeconds);

// Fabric and garment hooks that read `live` on every render, so a change from
// the reducer reaches both views in the next frame. `live` must outlive the
// returned hooks. No animate hook: advanceStudio stays the only clock owner.
StudioHooks makeControlledHooks(const ControlState& live);

// --- Key map -------------------------------------------------------------------
// Platform-neutral key names. The desktop adapter maps GLFW codes onto these.
enum class ControlKey {
    M, P, R, S, Escape,
    Left, Right, Up, Down,
    Q, E,
    Minus, Equal,
    X, Y,
    LeftBracket, RightBracket,
    Space, A,
    Comma, Period,
};

struct ControlKeyBinding {
    ControlKey key;
    ControlAction action;
    const char* label; // key name shown in docs and help
};

// The one key table. Every action appears exactly once.
const std::vector<ControlKeyBinding>& controlKeyBindings();

std::optional<ControlAction> actionForKey(ControlKey key);

// Press-versus-repeat filter shared by the desktop callback and the tests.
// `isRepeat` is the operating system's auto-repeat event (GLFW_REPEAT).
bool shouldDispatch(ControlAction action, bool isRepeat) noexcept;

// --- Parameter readout ---------------------------------------------------------
// Four lines that fit the header band to the right of the title, using only
// glyphs the studio bitmap font supports (studioFontSupports).
std::vector<std::string> controlStatusLines(const ControlState& state);

// Draws controlStatusLines() into the header band. It never touches the
// fabric preview, the mannequin or the bottom legend.
void drawControlStatus(Canvas& canvas, const ControlState& state);

// Header region used by drawControlStatus (x, y, width, height), for tests.
struct ControlStatusArea { int x, y, width, height; };
inline constexpr ControlStatusArea kControlStatusArea{700, 8, 492, 96};
inline constexpr int kControlStatusMaxChars = 40;

} // namespace kitenge
