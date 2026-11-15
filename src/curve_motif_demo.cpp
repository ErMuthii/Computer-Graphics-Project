// Evidence image for the Bezier arch motif.
// Owner: Andrew Karanja Gathirwa, 167144.
//
// Three panels, left to right:
//   1. CONTROL POINTS  - every cubic's control polygon and its four points only
//   2. BEZIER CURVES   - the same cubics drawn by the CP1 rasterizer (drawCubicBezier)
//   3. TILED + CLIPPED - the catalog motif repeated by Kelvin's renderer with the
//                        settings a key script produces, clipped to a skirt panel
//
// Usage: curve_motif_demo [output.ppm] [key-script]
// The key script replays studio keys through the real key map and reducer, so
// panel 3 shows what the desktop shows after those presses. Keys:
//   M P Q E - = X Y [ ] R and < > ^ v for the arrow keys. Default: "Q==X]".
#include "kitenge/controls.hpp"
#include "kitenge/curve_motif.hpp"
#include "kitenge/curves.hpp"
#include "kitenge/lines.hpp"
#include "kitenge/pattern.hpp"
#include "kitenge/ppm.hpp"

#include <cmath>
#include <exception>
#include <iostream>
#include <optional>
#include <string>

using namespace kitenge;

namespace {

constexpr Color paper{246, 240, 227}, ink{49, 57, 37}, hull{170, 165, 150},
    point{200, 60, 40}, curve{30, 90, 160}, trim{65, 42, 35};

std::optional<ControlKey> keyFor(char c) {
    switch (c) {
        case 'M': return ControlKey::M;      case 'P': return ControlKey::P;
        case 'Q': return ControlKey::Q;      case 'E': return ControlKey::E;
        case '-': return ControlKey::Minus;  case '=': return ControlKey::Equal;
        case 'X': return ControlKey::X;      case 'Y': return ControlKey::Y;
        case '[': return ControlKey::LeftBracket; case ']': return ControlKey::RightBracket;
        case '<': return ControlKey::Left;   case '>': return ControlKey::Right;
        case '^': return ControlKey::Up;     case 'v': return ControlKey::Down;
        case 'R': return ControlKey::R;
        default: return std::nullopt;
    }
}

// Motif-local point -> panel pixel (y already points down in both).
Point toPanel(Point2D p, Point2D centre, double zoom) {
    return {static_cast<int>(std::lround(centre.x + p.x * zoom)),
            static_cast<int>(std::lround(centre.y + p.y * zoom))};
}

CubicControls scaled(const CubicControls& c, Point2D centre, double zoom) {
    CubicControls out;
    for (int k = 0; k < 4; ++k) out[k] = {centre.x + c[k].x * zoom, centre.y + c[k].y * zoom};
    return out;
}

void square(Canvas& canvas, Point p, int r, Color color) {
    for (int y = -r; y <= r; ++y)
        for (int x = -r; x <= r; ++x) canvas.setPixel(p.x + x, p.y + y, color);
}

} // namespace

int main(int argc, char** argv) {
    try {
        const std::string out = argc > 1 ? argv[1] : "outputs/curve-motif.ppm";
        const std::string script = argc > 2 ? argv[2] : "Q==X]";

        ControlState state = initialControlState();
        for (char c : script) {
            const auto key = keyFor(c);
            if (!key) throw std::invalid_argument(std::string("unknown key in script: ") + c);
            if (const auto action = actionForKey(*key)) state = reduceControl(state, *action).state;
        }

        Canvas canvas(1080, 420, paper);
        constexpr double zoom = 6.0;

        // Panel 1: control polygons and control points only.
        const Point2D c1{180, 230};
        for (const CubicControls& c : bezierArchControlPoints()) {
            for (int k = 0; k < 3; ++k)
                drawLine(canvas, toPanel(c[k], c1, zoom), toPanel(c[k + 1], c1, zoom), hull);
            for (const Point2D& p : c) square(canvas, toPanel(p, c1, zoom), 2, point);
        }

        // Panel 2: the same cubics drawn by the CP1 Bezier rasterizer.
        const Point2D c2{540, 230};
        for (const CubicControls& c : bezierArchControlPoints())
            drawCubicBezier(canvas, scaled(c, c2, zoom), curve, 100);

        // Panel 3: catalog motif, tiled and clipped by the pattern engine.
        const Polygon skirt{{840, 90}, {960, 90}, {1050, 390}, {750, 390}};
        PatternSettings settings = displayedPatternSettings(state, 0.0);
        settings.origin = {900, 240};
        renderPattern(canvas, bezierArchMotif(), paletteRoleColors(state.studio.palette), settings, skirt);
        for (std::size_t i = 0; i < skirt.size(); ++i) {
            const Point2D a = skirt[i], b = skirt[(i + 1) % skirt.size()];
            drawLine(canvas, {static_cast<int>(a.x), static_cast<int>(a.y)},
                     {static_cast<int>(b.x), static_cast<int>(b.y)}, trim);
        }

        drawStudioText(canvas, "1 CONTROL POINTS", 70, 24, 2, ink);
        drawStudioText(canvas, "2 CP1 BEZIER CURVES", 420, 24, 2, ink);
        drawStudioText(canvas, "3 TILED + CLIPPED", 800, 24, 2, ink);
        drawStudioText(canvas, "KEYS " + script, 800, 52, 2, ink);
        for (int y = 20; y < 400; ++y) {
            canvas.setPixel(360, y, hull);
            canvas.setPixel(720, y, hull);
        }

        writePpm(canvas, out);
        std::cout << "Saved " << out << "\n";
        for (const std::string& line : controlStatusLines(state)) std::cout << "  " << line << "\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }
    return 0;
}
