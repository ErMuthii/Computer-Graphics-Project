// Real GLFW key codes and press/repeat/release events through the desktop
// adapter. Needs the GLFW header but no window or display.
// Owner: Andrew Karanja Gathirwa, 167144.
#define GLFW_INCLUDE_NONE
#include "kitenge/desktop_keys.hpp"

#include <GLFW/glfw3.h>

#include <cmath>
#include <iostream>
#include <string>

using namespace kitenge;

namespace {
int failures = 0;
void check(bool ok, const std::string& name) {
    if (!ok) { ++failures; std::cerr << "FAILED: " << name << "\n"; }
}
bool near(double a, double b) { return std::fabs(a - b) < 1e-9; }
} // namespace

int main() {
    ControlState s = initialControlState();

    // Holding the right arrow: one press, four OS repeats, one release.
    check(handleGlfwKey(s, GLFW_KEY_RIGHT, GLFW_PRESS).sceneChanged, "arrow press redraws");
    for (int i = 0; i < 4; ++i) handleGlfwKey(s, GLFW_KEY_RIGHT, GLFW_REPEAT);
    check(!handleGlfwKey(s, GLFW_KEY_RIGHT, GLFW_RELEASE).handled, "release is ignored");
    check(near(s.pattern.transform.translation.x, 5 * ControlLimits::translateStep),
          "held arrow repeats: five steps");

    // Holding X toggles once.
    handleGlfwKey(s, GLFW_KEY_X, GLFW_PRESS);
    check(!handleGlfwKey(s, GLFW_KEY_X, GLFW_REPEAT).handled, "repeat of a toggle is filtered");
    check(s.pattern.transform.reflectY, "X toggled exactly once");

    // Every planned key is bound, keypad scale keys included.
    for (int key : {GLFW_KEY_M, GLFW_KEY_P, GLFW_KEY_R, GLFW_KEY_S, GLFW_KEY_ESCAPE,
                    GLFW_KEY_LEFT, GLFW_KEY_RIGHT, GLFW_KEY_UP, GLFW_KEY_DOWN, GLFW_KEY_Q,
                    GLFW_KEY_E, GLFW_KEY_MINUS, GLFW_KEY_EQUAL, GLFW_KEY_KP_SUBTRACT,
                    GLFW_KEY_KP_ADD, GLFW_KEY_X, GLFW_KEY_Y, GLFW_KEY_LEFT_BRACKET,
                    GLFW_KEY_RIGHT_BRACKET, GLFW_KEY_SPACE, GLFW_KEY_A, GLFW_KEY_COMMA,
                    GLFW_KEY_PERIOD})
        check(controlKeyFromGlfw(key).has_value(), "GLFW key is bound: " + std::to_string(key));
    check(!controlKeyFromGlfw(GLFW_KEY_F5).has_value(), "unbound key is ignored");

    // Keypad + behaves like =.
    const double scale = s.pattern.transform.scale;
    handleGlfwKey(s, GLFW_KEY_KP_ADD, GLFW_PRESS);
    check(near(s.pattern.transform.scale, scale + ControlLimits::scaleStep), "keypad + scales up");

    // Existing starter keys keep their behaviour.
    check(handleGlfwKey(s, GLFW_KEY_M, GLFW_PRESS).sceneChanged && s.studio.motif == 1, "M next motif");
    check(handleGlfwKey(s, GLFW_KEY_S, GLFW_PRESS).effect == ControlEffect::Export, "S exports");
    check(handleGlfwKey(s, GLFW_KEY_ESCAPE, GLFW_PRESS).effect == ControlEffect::Close, "Esc closes");
    check(handleGlfwKey(s, GLFW_KEY_R, GLFW_PRESS).sceneChanged && s.studio.motif == 0 &&
              !s.pattern.transform.reflectY && near(s.pattern.transform.translation.x, 0.0),
          "R resets");

    // A rejected step at a limit is handled but changes nothing.
    for (int i = 0; i < 100; ++i) handleGlfwKey(s, GLFW_KEY_EQUAL, GLFW_REPEAT);
    const KeyEventResult atLimit = handleGlfwKey(s, GLFW_KEY_EQUAL, GLFW_PRESS);
    check(atLimit.handled && !atLimit.sceneChanged, "step past the limit does not redraw");

    if (failures) { std::cerr << failures << " desktop key check(s) failed\n"; return 1; }
    std::cout << "Desktop GLFW key mapping, repeat filtering and starter keys passed.\n";
    return 0;
}
