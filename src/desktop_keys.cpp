// GLFW keyboard adapter for the desktop studio.
// Owner: Andrew Karanja Gathirwa, 167144.
#define GLFW_INCLUDE_NONE
#include "kitenge/desktop_keys.hpp"

#include <GLFW/glfw3.h>

namespace kitenge {

std::optional<ControlKey> controlKeyFromGlfw(int key) noexcept {
    switch (key) {
        case GLFW_KEY_M: return ControlKey::M;
        case GLFW_KEY_P: return ControlKey::P;
        case GLFW_KEY_R: return ControlKey::R;
        case GLFW_KEY_S: return ControlKey::S;
        case GLFW_KEY_ESCAPE: return ControlKey::Escape;
        case GLFW_KEY_LEFT: return ControlKey::Left;
        case GLFW_KEY_RIGHT: return ControlKey::Right;
        case GLFW_KEY_UP: return ControlKey::Up;
        case GLFW_KEY_DOWN: return ControlKey::Down;
        case GLFW_KEY_Q: return ControlKey::Q;
        case GLFW_KEY_E: return ControlKey::E;
        case GLFW_KEY_MINUS:
        case GLFW_KEY_KP_SUBTRACT: return ControlKey::Minus;
        case GLFW_KEY_EQUAL:
        case GLFW_KEY_KP_ADD: return ControlKey::Equal;
        case GLFW_KEY_X: return ControlKey::X;
        case GLFW_KEY_Y: return ControlKey::Y;
        case GLFW_KEY_LEFT_BRACKET: return ControlKey::LeftBracket;
        case GLFW_KEY_RIGHT_BRACKET: return ControlKey::RightBracket;
        case GLFW_KEY_SPACE: return ControlKey::Space;
        case GLFW_KEY_A: return ControlKey::A;
        case GLFW_KEY_COMMA: return ControlKey::Comma;
        case GLFW_KEY_PERIOD: return ControlKey::Period;
        default: return std::nullopt;
    }
}

KeyEventResult handleGlfwKey(ControlState& state, int glfwKey, int glfwAction) {
    KeyEventResult out;
    if (glfwAction != GLFW_PRESS && glfwAction != GLFW_REPEAT) return out;
    const std::optional<ControlKey> key = controlKeyFromGlfw(glfwKey);
    if (!key) return out;
    const std::optional<ControlAction> action = actionForKey(*key);
    if (!action || !shouldDispatch(*action, glfwAction == GLFW_REPEAT)) return out;

    ControlResult result = reduceControl(state, *action);
    out.handled = true;
    out.effect = result.effect;
    out.sceneChanged = result.sceneChanged();
    if (result.accepted) state = result.state;
    return out;
}

} // namespace kitenge
