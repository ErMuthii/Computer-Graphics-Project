#include "kitenge/lines.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

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

bool near(double actual, double expected) {
    return std::abs(actual - expected) < 1e-9;
}

void checkPoint(Point2D actual, Point2D expected, const char* description) {
    check(near(actual.x, expected.x) && near(actual.y, expected.y), description);
}

void testEveryOctantAndReverse() {
    const std::vector<Point> ends{{16, 12}, {12, 16}, {8, 16}, {4, 12},
                                  {4, 8}, {8, 4}, {12, 4}, {16, 8},
                                  {16, 10}, {10, 16}, {4, 10}, {10, 4}};
    for (const Point end : ends) {
        Canvas forward(21, 21, black);
        Canvas reverse(21, 21, black);
        drawLine(forward, {10, 10}, end, white);
        drawLine(reverse, end, {10, 10}, white);
        bool samePixels = true;
        for (std::size_t i = 0; i < forward.pixels().size(); ++i) {
            if (forward.pixels()[i].red != reverse.pixels()[i].red) {
                samePixels = false;
                break;
            }
        }
        check(samePixels, "reversed segment has identical pixels");
        check(forward.pixelAt(10, 10).red == 255 &&
              forward.pixelAt(end.x, end.y).red == 255, "both endpoints are drawn");
        const int expected = std::max(std::abs(end.x - 10), std::abs(end.y - 10)) + 1;
        const int actual = static_cast<int>(std::count_if(
            forward.pixels().begin(), forward.pixels().end(),
            [](Color pixel) { return pixel.red == 255; }));
        check(actual == expected, "line has one pixel per major-axis step");
    }
}

void testAxesPointAndCanvasEdge() {
    Canvas canvas(8, 8, black);
    drawLine(canvas, {2, 3}, {2, 3}, white);
    check(canvas.pixelAt(2, 3).red == 255, "single-point line");
    drawLine(canvas, {-3, 0}, {10, 0}, white);
    drawLine(canvas, {7, -3}, {7, 10}, white);
    for (int position = 0; position < 8; ++position) {
        check(canvas.pixelAt(position, 0).red == 255, "horizontal line reaches canvas edge");
        check(canvas.pixelAt(7, position).red == 255, "vertical line reaches canvas edge");
    }
}

void testClipping() {
    const Rect window{0, 0, 10, 10};
    const auto inside = clipLineToRect({2, 3}, {8, 9}, window);
    check(inside.has_value(), "fully visible line is accepted");
    if (inside) {
        checkPoint(inside->first, {2, 3}, "fully visible start is unchanged");
        checkPoint(inside->second, {8, 9}, "fully visible end is unchanged");
    }
    check(!clipLineToRect({-5, 2}, {-1, 8}, window), "fully left line is rejected");
    check(!clipLineToRect({-5, -5}, {-1, -1}, window), "fully outside corner is rejected");

    const auto horizontal = clipLineToRect({-5, 5}, {15, 5}, window);
    check(horizontal.has_value(), "horizontal crossing is accepted");
    if (horizontal) {
        checkPoint(horizontal->first, {0, 5}, "horizontal start meets left boundary");
        checkPoint(horizontal->second, {10, 5}, "horizontal end meets right boundary");
    }
    const auto reversed = clipLineToRect({15, 5}, {-5, 5}, window);
    check(reversed.has_value(), "reversed crossing is accepted");
    if (reversed) {
        checkPoint(reversed->first, {10, 5}, "reversed start meets right boundary");
        checkPoint(reversed->second, {0, 5}, "reversed end meets left boundary");
    }
    const auto vertical = clipLineToRect({5, -5}, {5, 15}, window);
    check(vertical.has_value(), "vertical crossing is accepted");
    if (vertical) {
        checkPoint(vertical->first, {5, 0}, "vertical start meets top boundary");
        checkPoint(vertical->second, {5, 10}, "vertical end meets bottom boundary");
    }
    const auto diagonal = clipLineToRect({-5, 0}, {15, 10}, window);
    check(diagonal.has_value(), "diagonal crossing is accepted");
    if (diagonal) {
        checkPoint(diagonal->first, {0, 2.5}, "fractional left crossing is retained");
        checkPoint(diagonal->second, {10, 7.5}, "fractional right crossing is retained");
    }
    const auto boundary = clipLineToRect({0, 0}, {10, 0}, window);
    check(boundary.has_value(), "line on top boundary is retained");
    const auto corner = clipLineToRect({-2, -2}, {0, 0}, window);
    check(corner.has_value(), "single corner touch is retained");
    if (corner) {
        checkPoint(corner->first, {0, 0}, "corner touch starts at corner");
        checkPoint(corner->second, {0, 0}, "corner touch ends at corner");
    }
    check(!clipLineToRect({-5, 0}, {-1, 0}, window), "boundary extension outside is rejected");
}

void testInvalidInputs() {
    bool threw = false;
    try { (void)clipLineToRect({0, 0}, {1, 1}, {10, 0, 0, 10}); }
    catch (const std::invalid_argument&) { threw = true; }
    check(threw, "reversed rectangle bounds are rejected");
    threw = false;
    try {
        (void)clipLineToRect({std::numeric_limits<double>::infinity(), 0},
                             {1, 1}, {0, 0, 10, 10});
    } catch (const std::invalid_argument&) { threw = true; }
    check(threw, "non-finite endpoint is rejected");
}

} // namespace

int main() {
    testEveryOctantAndReverse();
    testAxesPointAndCanvasEdge();
    testClipping();
    testInvalidInputs();
    if (failures == 0) std::cout << "lines: all checks passed\n";
    return failures == 0 ? 0 : 1;
}
