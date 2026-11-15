#pragma once

// GLFW keyboard adapter for the desktop studio.
// Owner: Andrew Karanja Gathirwa, 167144 (callback wiring agreed with Eric).
//
// The only code that knows GLFW key codes. It maps them onto ControlKey and
// then runs the shared Pure C++ path: key map -> repeat filter -> reducer.
// The header itself does not include GLFW; pass GLFW_KEY_* / GLFW_PRESS values.

#include "kitenge/controls.hpp"

#include <optional>

namespace kitenge {

// GLFW_KEY_* -> ControlKey; std::nullopt for an unbound key. GLFW names keys by
// their US-layout position, so on other layouts the punctuation keys are the
// keys in those positions. Keypad - and + also scale.
std::optional<ControlKey> controlKeyFromGlfw(int glfwKey) noexcept;

// What one key event did.
struct KeyEventResult {
    bool handled = false;      // a bound key passed the repeat filter
    bool sceneChanged = false; // re-render needed
    ControlEffect effect = ControlEffect::None;
};

// Handles one GLFW key event. Releases are ignored; GLFW_REPEAT only reaches
// repeatable adjustments. `state` is updated only when the action is accepted.
KeyEventResult handleGlfwKey(ControlState& state, int glfwKey, int glfwAction);

} // namespace kitenge
