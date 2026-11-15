#pragma once
#include "kitenge/canvas.hpp"
#include "kitenge/motifs.hpp"
#include <functional>
#include <string>

namespace kitenge {
constexpr int studioWidth = 1200;
constexpr int studioHeight = 760;
// Selection sizes come from Deborah's motif/palette catalog (motifs.hpp).
constexpr int studioMotifCount = kMotifCount;
constexpr int studioPaletteCount = kPaletteCount;
// `secondary` holds the catalog's accent role.
struct StudioPalette { Color background, primary, secondary; };
// Throws std::out_of_range for an index outside [0, studioPaletteCount).
StudioPalette studioPalette(int index);
// Scene regions shared by the shell (outlines) and the fabric/garment hooks
// (fills), so both always use the same geometry. All three are convex.
const Polygon& studioFabricArea();
const Polygon& studioBodice();
const Polygon& studioSkirt();
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
// Draws upper-case text with the built-in 5x7 bitmap font at `scale` pixels per
// font pixel. Characters the font lacks are drawn blank; check them with
// studioFontSupports. Used by the scene and by the controls readout.
void drawStudioText(Canvas&, const std::string& text, int x, int y, int scale, Color);
// True for A-Z, 0-9, space and - : / + . , [ ] =
bool studioFontSupports(char c);
void applyAction(StudioState&, StudioAction);
void advanceStudio(StudioState&, double deltaSeconds, const StudioHooks& = {});
void renderStudio(Canvas&, const StudioState&, const StudioHooks& = {});
} // namespace kitenge
