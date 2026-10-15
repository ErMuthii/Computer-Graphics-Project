#include "kitenge/circles.hpp"

#include <iostream>
#include <stdexcept>

namespace {

using namespace kitenge;
constexpr Color black{0, 0, 0};
constexpr Color white{255, 255, 255};
int failures = 0;

void check(bool condition, const char* description) {
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << description << '\n';
    }
}

bool lit(const Canvas& canvas, int x, int y) {
    return canvas.pixelAt(x, y).red == 255;
}

void checkSymmetry(const Canvas& canvas, Point center, bool eightWay) {
    for (int y = 0; y < canvas.height(); ++y) {
        for (int x = 0; x < canvas.width(); ++x) {
            if (!lit(canvas, x, y)) continue;
            const int dx = x - center.x;
            const int dy = y - center.y;
            check(lit(canvas, center.x - dx, center.y + dy), "left-right symmetry");
            check(lit(canvas, center.x + dx, center.y - dy), "top-bottom symmetry");
            if (eightWay) {
                check(lit(canvas, center.x + dy, center.y + dx), "circle diagonal symmetry");
            }
        }
    }
}

void testCircles() {
    for (int radius : {1, 2, 5, 9}) {
        Canvas canvas(61, 61, black);
        drawCircleMidpoint(canvas, {30, 30}, radius, white);
        check(lit(canvas, 30 + radius, 30), "circle right cardinal pixel");
        check(lit(canvas, 30 - radius, 30), "circle left cardinal pixel");
        check(lit(canvas, 30, 30 + radius), "circle bottom cardinal pixel");
        check(lit(canvas, 30, 30 - radius), "circle top cardinal pixel");
        check(!lit(canvas, 30, 30), "positive-radius circle is an outline");
        checkSymmetry(canvas, {30, 30}, true);
    }
    Canvas point(11, 11, black);
    drawCircleMidpoint(point, {5, 5}, 0, white);
    check(lit(point, 5, 5), "zero-radius circle draws center");
    check(!lit(point, 4, 5), "zero-radius circle draws no neighbor");
}

void testEllipses() {
    for (const Point radii : {Point{12, 5}, Point{5, 12}, Point{2, 1}}) {
        Canvas canvas(61, 61, black);
        drawEllipseMidpoint(canvas, {30, 30}, radii.x, radii.y, white);
        check(lit(canvas, 30 + radii.x, 30), "ellipse right cardinal pixel");
        check(lit(canvas, 30 - radii.x, 30), "ellipse left cardinal pixel");
        check(lit(canvas, 30, 30 + radii.y), "ellipse bottom cardinal pixel");
        check(lit(canvas, 30, 30 - radii.y), "ellipse top cardinal pixel");
        check(!lit(canvas, 30, 30), "positive-axis ellipse is an outline");
        checkSymmetry(canvas, {30, 30}, false);
    }
}

void testZeroAxesAndCanvasEdges() {
    Canvas canvas(11, 11, black);
    drawEllipseMidpoint(canvas, {5, 5}, 0, 3, white);
    for (int y = 2; y <= 8; ++y) check(lit(canvas, 5, y), "zero x-axis draws vertical segment");
    check(!lit(canvas, 4, 5), "zero x-axis has no horizontal width");
    canvas.clear(black);
    drawEllipseMidpoint(canvas, {5, 5}, 3, 0, white);
    for (int x = 2; x <= 8; ++x) check(lit(canvas, x, 5), "zero y-axis draws horizontal segment");
    check(!lit(canvas, 5, 4), "zero y-axis has no vertical height");
    canvas.clear(black);
    drawEllipseMidpoint(canvas, {5, 5}, 0, 0, white);
    check(lit(canvas, 5, 5) && !lit(canvas, 6, 5), "two zero axes draw one pixel");

    canvas.clear(black);
    drawCircleMidpoint(canvas, {0, 0}, 5, white);
    check(lit(canvas, 5, 0) && lit(canvas, 0, 5), "circle safely meets canvas edges");
    canvas.clear(black);
    drawEllipseMidpoint(canvas, {10, 10}, 4, 2, white);
    check(lit(canvas, 6, 10) && lit(canvas, 10, 8), "ellipse safely meets canvas edges");
}

void testNegativeRadii() {
    Canvas canvas(11, 11, black);
    bool threw = false;
    try { drawCircleMidpoint(canvas, {5, 5}, -1, white); }
    catch (const std::invalid_argument&) { threw = true; }
    check(threw, "negative circle radius is rejected");
    threw = false;
    try { drawEllipseMidpoint(canvas, {5, 5}, -1, 2, white); }
    catch (const std::invalid_argument&) { threw = true; }
    check(threw, "negative x radius is rejected");
    threw = false;
    try { drawEllipseMidpoint(canvas, {5, 5}, 2, -1, white); }
    catch (const std::invalid_argument&) { threw = true; }
    check(threw, "negative y radius is rejected");
}

} // namespace

int main() {
    testCircles();
    testEllipses();
    testZeroAxesAndCanvasEdges();
    testNegativeRadii();
    if (failures == 0) std::cout << "circles: all checks passed\n";
    return failures == 0 ? 0 : 1;
}
