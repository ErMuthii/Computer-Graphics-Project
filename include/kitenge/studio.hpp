#pragma once
#include "kitenge/canvas.hpp"
#include <functional>

namespace kitenge {
constexpr int studioWidth = 1200;
constexpr int studioHeight = 760;
enum class StudioAction { NextMotif, NextPalette, TogglePlayback, Reset };
struct StudioState {
    int motif = 0;
    int palette = 0;
    bool playing = false;
    double timeSeconds = 0.0;
};
// Coordinates passed to hooks are canvas coordinates. Hooks must stay independent
// of GLFW/OpenGL. An empty hook uses the deliberately simple starter scene.
struct StudioHooks {
    std::function<void(Canvas&, const StudioState&)> fabric;
    std::function<void(Canvas&, const StudioState&)> garment;
    std::function<void(StudioState&, double)> animate;
};
void applyAction(StudioState&, StudioAction);
void advanceStudio(StudioState&, double deltaSeconds, const StudioHooks& = {});
void renderStudio(Canvas&, const StudioState&, const StudioHooks& = {});
} // namespace kitenge
