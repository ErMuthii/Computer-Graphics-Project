#include "kitenge/curves.hpp"
#include "kitenge/lines.hpp"
#include "kitenge/ppm.hpp"

#include <array>
#include <cmath>
#include <exception>
#include <filesystem>
#include <iostream>

namespace {

using namespace kitenge;

constexpr Color background{17, 29, 38};
constexpr Color fabric{32, 54, 61};
constexpr Color inset{38, 65, 70};
constexpr Color guide{82, 130, 136};
constexpr Color gold{244, 190, 77};
constexpr Color coral{231, 104, 82};
constexpr Color teal{82, 204, 178};
constexpr Color cream{237, 224, 185};

void fillRect(Canvas& canvas, int x0, int y0, int x1, int y1, Color color) {
    for (int y = y0; y <= y1; ++y) {
        for (int x = x0; x <= x1; ++x) {
            canvas.setPixel(x, y, color);
        }
    }
}

Point rounded(Point2D point) {
    return {static_cast<int>(std::lround(point.x)),
            static_cast<int>(std::lround(point.y))};
}

void marker(Canvas& canvas, Point center, Color color) {
    fillRect(canvas, center.x - 5, center.y - 5,
             center.x + 5, center.y + 5, color);
    fillRect(canvas, center.x - 2, center.y - 2,
             center.x + 2, center.y + 2, background);
}

void broadCurve(Canvas& canvas, const std::array<Point2D, 4>& control,
                Color color) {
    for (int offset = -1; offset <= 1; ++offset) {
        auto shifted = control;
        for (Point2D& point : shifted) point.y += offset;
        drawCubicBezier(canvas, shifted, color, 120);
    }
}

void constructionPanel(Canvas& canvas) {
    const std::array<Point2D, 4> control{{
        {88, 360}, {145, 75}, {345, 75}, {412, 360}}};
    for (std::size_t i = 1; i < control.size(); ++i) {
        drawLine(canvas, rounded(control[i - 1]), rounded(control[i]), guide);
    }
    broadCurve(canvas, control, gold);
    for (const Point2D& point : control) marker(canvas, rounded(point), coral);

    const Point midpoint = rounded(evaluateCubicBezier(control, 0.5));
    marker(canvas, midpoint, cream);
    fillRect(canvas, 52, 449, 212, 455, guide);
    fillRect(canvas, 52, 463, 212, 469, gold);
    marker(canvas, {234, 459}, coral);
}

void motifPanel(Canvas& canvas) {
    for (int row = 0; row < 2; ++row) {
        const double top = row == 0 ? 58.0 : 268.0;
        for (int column = 0; column < 3; ++column) {
            const double left = 546.0 + 140.0 * column;
            const std::array<Point2D, 4> arch{{
                {left, top + 137}, {left + 26, top},
                {left + 94, top}, {left + 120, top + 137}}};
            const std::array<Point2D, 4> inner{{
                {left + 17, top + 130}, {left + 42, top + 50},
                {left + 78, top + 50}, {left + 103, top + 130}}};
            broadCurve(canvas, arch, row == 0 ? gold : teal);
            broadCurve(canvas, inner, row == 0 ? coral : gold);
            marker(canvas, {static_cast<int>(left + 60),
                            static_cast<int>(top + 115)}, cream);
        }
    }
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc > 2) {
        std::cerr << "Usage: bezier_demo [output-image.ppm]\n";
        return 1;
    }
    try {
        const std::filesystem::path destination = argc == 2
            ? std::filesystem::path(argv[1])
            : std::filesystem::path("outputs/bezier-curves.ppm");
        Canvas canvas(1000, 520, background);
        fillRect(canvas, 20, 20, 979, 499, fabric);
        fillRect(canvas, 35, 35, 485, 484, inset);
        fillRect(canvas, 515, 35, 964, 484, inset);
        fillRect(canvas, 496, 35, 503, 484, gold);
        constructionPanel(canvas);
        motifPanel(canvas);
        writePpm(canvas, destination);
        std::cout << "Saved cubic Bezier demonstration to " << destination << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}
