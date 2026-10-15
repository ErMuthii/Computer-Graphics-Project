#include "kitenge/circles.hpp"
#include "kitenge/ppm.hpp"

#include <exception>
#include <filesystem>
#include <iostream>

namespace {

using namespace kitenge;

constexpr Color background{17, 29, 38};
constexpr Color fabric{32, 54, 61};
constexpr Color inset{38, 65, 70};
constexpr Color gold{244, 190, 77};
constexpr Color coral{231, 104, 82};
constexpr Color teal{82, 204, 178};
constexpr Color cream{237, 224, 185};

void fillRect(Canvas& canvas, int x0, int y0, int x1, int y1, Color color) {
    for (int y = y0; y <= y1; ++y)
        for (int x = x0; x <= x1; ++x)
            canvas.setPixel(x, y, color);
}

void circleMedallion(Canvas& canvas, Point center, bool alternate) {
    const Color main = alternate ? coral : gold;
    const Color accent = alternate ? gold : coral;
    drawCircleMidpoint(canvas, center, 72, main);
    drawCircleMidpoint(canvas, center, 68, main);
    drawCircleMidpoint(canvas, center, 47, teal);
    drawCircleMidpoint(canvas, center, 28, accent);
    drawCircleMidpoint(canvas, center, 11, cream);
    for (const Point offset : {Point{0, -57}, Point{57, 0},
                               Point{0, 57}, Point{-57, 0}}) {
        drawCircleMidpoint(canvas, {center.x + offset.x, center.y + offset.y}, 6, cream);
    }
    for (const Point offset : {Point{40, -40}, Point{40, 40},
                               Point{-40, 40}, Point{-40, -40}}) {
        drawCircleMidpoint(canvas, {center.x + offset.x, center.y + offset.y}, 5, accent);
    }
}

void ovalMedallion(Canvas& canvas, Point center, bool alternate) {
    const Color main = alternate ? teal : gold;
    const Color accent = alternate ? coral : teal;
    drawEllipseMidpoint(canvas, center, 87, 67, main);
    drawEllipseMidpoint(canvas, center, 82, 62, main);
    drawEllipseMidpoint(canvas, center, 67, 45, accent);
    drawEllipseMidpoint(canvas, center, 47, 27, coral);
    drawCircleMidpoint(canvas, center, 12, cream);
    for (const Point offset : {Point{-60, 0}, Point{60, 0},
                               Point{0, -39}, Point{0, 39}}) {
        drawCircleMidpoint(canvas, {center.x + offset.x, center.y + offset.y}, 6, cream);
    }
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc > 2) {
        std::cerr << "Usage: circles_demo [output-image.ppm]\n";
        return 1;
    }
    try {
        const std::filesystem::path destination = argc == 2
            ? std::filesystem::path(argv[1])
            : std::filesystem::path("outputs/circles-ellipses.ppm");
        Canvas canvas(920, 470, background);
        fillRect(canvas, 22, 22, 897, 447, fabric);
        fillRect(canvas, 35, 35, 445, 434, inset);
        fillRect(canvas, 475, 35, 885, 434, inset);
        fillRect(canvas, 454, 35, 465, 434, gold);

        circleMedallion(canvas, {138, 136}, false);
        circleMedallion(canvas, {340, 136}, true);
        circleMedallion(canvas, {138, 334}, true);
        circleMedallion(canvas, {340, 334}, false);

        ovalMedallion(canvas, {580, 136}, false);
        ovalMedallion(canvas, {780, 136}, true);
        ovalMedallion(canvas, {580, 334}, true);
        ovalMedallion(canvas, {780, 334}, false);

        // Circular buttons and oval trim suggest how the motifs can be used
        // on the later simplified dress as well as on a fabric swatch.
        for (int y : {95, 165, 235, 305, 375}) {
            drawCircleMidpoint(canvas, {460, y}, 8, cream);
            drawEllipseMidpoint(canvas, {460, y}, 5, 3, coral);
        }

        writePpm(canvas, destination);
        std::cout << "Saved circle and ellipse demonstration to " << destination << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}
