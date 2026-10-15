#include "kitenge/polygon_fill.hpp"

#include <iostream>

using namespace kitenge;

namespace {

int failures = 0;

void check(bool ok, const char* name) {
    if (!ok) {
        ++failures;
        std::cerr << "FAIL: " << name << "\n";
    }
}

const Color black{0, 0, 0};
const Color white{255, 255, 255};

bool isFilled(const Canvas& c, int x, int y) { return c.pixelAt(x, y).red == 255; }
int countFilled(const Canvas& c) {
    int n = 0;
    for (int y = 0; y < c.height(); ++y)
        for (int x = 0; x < c.width(); ++x)
            if (isFilled(c, x, y)) ++n;
    return n;
}

void testRectangle() {
    Canvas c(20, 20, black);
    fillPolygon(c, {{2, 3}, {10, 3}, {10, 8}, {2, 8}}, white);
    check(countFilled(c) == 40, "rectangle: 8x5 = 40 pixels");
    check(isFilled(c, 2, 3) && isFilled(c, 9, 7), "rectangle: top-left/bottom-right inside");
    check(!isFilled(c, 10, 3) && !isFilled(c, 2, 8), "rectangle: right/bottom edge excluded");
}

void testTriangle() {
    Canvas c(20, 20, black);
    fillPolygon(c, {{0, 0}, {10, 0}, {0, 10}}, white);
    check(isFilled(c, 1, 1), "triangle: inside point filled");
    check(!isFilled(c, 9, 9), "triangle: outside point empty");
}

void testConcave() {
    Canvas c(20, 20, black);
    // U shape with a notch at x 6..8, y 6..12
    fillPolygon(c, {{2, 2}, {12, 2}, {12, 12}, {8, 12}, {8, 6}, {6, 6}, {6, 12}, {2, 12}}, white);
    check(isFilled(c, 4, 9), "concave: left arm filled");
    check(isFilled(c, 10, 9), "concave: right arm filled");
    check(isFilled(c, 7, 4), "concave: top bar filled");
    check(!isFilled(c, 7, 9), "concave: notch empty");
}

void testVertexOnScanline() {
    Canvas c(20, 20, black);
    // left and right vertices sit exactly on the center of row 5 (y = 5.5)
    fillPolygon(c, {{10, 1.5}, {14, 5.5}, {10, 9.5}, {6, 5.5}}, white);
    check(isFilled(c, 6, 5) && isFilled(c, 13, 5), "diamond: widest row filled");
    check(!isFilled(c, 5, 5) && !isFilled(c, 14, 5), "diamond: widest row not overfilled");
}

void testOffCanvas() {
    Canvas c(20, 20, black);
    fillPolygon(c, {{-5, -5}, {5, -5}, {5, 5}, {-5, 5}}, white);
    check(countFilled(c) == 25, "off-canvas: top-left clipped to 5x5");

    Canvas d(20, 20, black);
    fillPolygon(d, {{15, 15}, {30, 15}, {30, 30}, {15, 30}}, white);
    check(countFilled(d) == 25, "off-canvas: bottom-right clipped to 5x5");

    Canvas e(20, 20, black);
    fillPolygon(e, {{100, 100}, {120, 100}, {120, 120}, {100, 120}}, white);
    check(countFilled(e) == 0, "off-canvas: fully outside draws nothing");
}

void testInvalid() {
    Canvas c(20, 20, black);
    fillPolygon(c, {{2, 2}, {10, 10}}, white);
    check(countFilled(c) == 0, "invalid: two vertices");
    fillPolygon(c, {{2, 2}, {10, 10}, {10, 2}, {2, 10}}, white); // bowtie
    check(countFilled(c) == 0, "invalid: self-intersecting bowtie");
    fillPolygon(c, {{2, 2}, {5, 5}, {8, 8}}, white); // collinear
    check(countFilled(c) == 0, "invalid: zero area");
}

void testSharedEdge() {
    Canvas c(20, 20, black);
    fillPolygon(c, {{2, 2}, {6, 2}, {6, 6}, {2, 6}}, white);
    fillPolygon(c, {{6, 2}, {10, 2}, {10, 6}, {6, 6}}, white);
    check(countFilled(c) == 32, "shared edge: 8x4, no gap");
    check(isFilled(c, 5, 3) && isFilled(c, 6, 3), "shared edge: both sides filled");
}

} // namespace

int main() {
    testRectangle();
    testTriangle();
    testConcave();
    testVertexOnScanline();
    testOffCanvas();
    testInvalid();
    testSharedEdge();
    if (failures == 0) std::cout << "polygon_fill: all checks passed\n";
    return failures == 0 ? 0 : 1;
}