#include "kitenge/lines.hpp"
#include "kitenge/ppm.hpp"

#include <cmath>
#include <exception>
#include <filesystem>
#include <iostream>
#include <utility>
#include <vector>

namespace {

using namespace kitenge;

constexpr Color background{21, 31, 41};
constexpr Color panel{31, 48, 59};
constexpr Color windowFill{42, 62, 67};
constexpr Color border{233, 220, 174};
constexpr Color gold{244, 185, 62};
constexpr Color coral{229, 95, 76};
constexpr Color teal{68, 201, 176};
constexpr int panelWidth = 420;

struct Stroke {
    Point start;
    Point end;
    Color color;
};

void fillRect(Canvas& canvas, int x0, int y0, int x1, int y1, Color color) {
    for (int y = y0; y <= y1; ++y)
        for (int x = x0; x <= x1; ++x)
            canvas.setPixel(x, y, color);
}

void outlineRect(Canvas& canvas, int offset, Rect rect, Color color) {
    const int x0 = offset + static_cast<int>(rect.minX);
    const int x1 = offset + static_cast<int>(rect.maxX);
    const int y0 = static_cast<int>(rect.minY);
    const int y1 = static_cast<int>(rect.maxY);
    drawLine(canvas, {x0, y0}, {x1, y0}, color);
    drawLine(canvas, {x1, y0}, {x1, y1}, color);
    drawLine(canvas, {x1, y1}, {x0, y1}, color);
    drawLine(canvas, {x0, y1}, {x0, y0}, color);
}

std::vector<Stroke> motif() {
    std::vector<Stroke> strokes;
    for (int centerX : {60, 150, 240, 330}) {
        const Point top{centerX, 108};
        const Point right{centerX + 62, 205};
        const Point bottom{centerX, 302};
        const Point left{centerX - 62, 205};
        strokes.push_back({top, right, gold});
        strokes.push_back({right, bottom, gold});
        strokes.push_back({bottom, left, gold});
        strokes.push_back({left, top, gold});
        strokes.push_back({top, bottom, teal});
        strokes.push_back({left, right, teal});
        strokes.push_back({{centerX - 42, 138}, {centerX + 42, 272}, coral});
        strokes.push_back({{centerX + 42, 138}, {centerX - 42, 272}, coral});
    }
    strokes.push_back({{18, 62}, {402, 348}, coral});
    strokes.push_back({{18, 348}, {402, 62}, coral});
    strokes.push_back({{20, 205}, {400, 205}, gold});
    return strokes;
}

void drawStroke(Canvas& canvas, Point start, Point end, Color color) {
    // Two adjacent Bresenham traces make the demonstration visible at display size.
    drawLine(canvas, start, end, color);
    drawLine(canvas, {start.x, start.y + 1}, {end.x, end.y + 1}, color);
}

void renderPanel(Canvas& canvas, int offset, bool clipped,
                 const std::vector<Stroke>& strokes) {
    fillRect(canvas, offset, 20, offset + panelWidth - 1, 390, panel);
    fillRect(canvas, offset + 70, 70, offset + 350, 350, windowFill);
    const Rect window{80, 80, 340, 340};

    for (const Stroke& stroke : strokes) {
        if (clipped) {
            const auto segment = clipLineToRect(
                {static_cast<double>(stroke.start.x), static_cast<double>(stroke.start.y)},
                {static_cast<double>(stroke.end.x), static_cast<double>(stroke.end.y)}, window);
            if (!segment) continue;
            drawStroke(canvas,
                {offset + static_cast<int>(std::lround(segment->first.x)),
                 static_cast<int>(std::lround(segment->first.y))},
                {offset + static_cast<int>(std::lround(segment->second.x)),
                 static_cast<int>(std::lround(segment->second.y))}, stroke.color);
        } else {
            drawStroke(canvas, {offset + stroke.start.x, stroke.start.y},
                       {offset + stroke.end.x, stroke.end.y}, stroke.color);
        }
    }
    outlineRect(canvas, offset, window, border);
    fillRect(canvas, offset + 14, 26, offset + (clipped ? 150 : 86), 33,
             clipped ? teal : coral);
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc > 2) {
        std::cerr << "Usage: lines_demo [output-image.ppm]\n";
        return 1;
    }
    try {
        const std::filesystem::path destination = argc == 2
            ? std::filesystem::path(argv[1])
            : std::filesystem::path("outputs/lines-clipping.ppm");
        Canvas canvas(880, 410, background);
        const auto strokes = motif();
        renderPanel(canvas, 10, false, strokes);
        renderPanel(canvas, 450, true, strokes);
        writePpm(canvas, destination);
        std::cout << "Saved line clipping demonstration to " << destination << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}
