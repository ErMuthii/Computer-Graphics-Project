// Checks for the shared mannequin geometry (Deborah Rehana, 168656).

#include "kitenge/mannequin.hpp"
#include "kitenge/polygon_clip.hpp"
#include "kitenge/polygon_fill.hpp"
#include "kitenge/studio.hpp"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

using namespace kitenge;

namespace {

int failures = 0;

void check(bool ok, const std::string& name) {
    if (!ok) {
        ++failures;
        std::cerr << "FAIL: " << name << "\n";
    }
}

bool same(Color a, Color b) { return a.red == b.red && a.green == b.green && a.blue == b.blue; }

bool hasVertex(const Polygon& p, Point2D v) {
    for (const Point2D& q : p)
        if (q.x == v.x && q.y == v.y) return true;
    return false;
}

constexpr Color paper{246, 240, 227};
constexpr Color skin{187, 156, 121};
constexpr Color outline{65, 42, 35};
constexpr Color cloth{30, 120, 160};

void testPanelsAreConvexAndShared() {
    const Polygon& bodice = mannequinBodice();
    const Polygon& skirt = mannequinSkirt();
    check(isValidPolygon(bodice) && isValidPolygon(skirt), "panels are valid polygons");
    check(isConvexPolygon(bodice), "bodice is convex");
    check(isConvexPolygon(skirt), "skirt is convex");

    // One geometry source: the studio and the pattern engine see these objects.
    check(&studioBodice() == &mannequinBodice(), "studioBodice is the mannequin's bodice");
    check(&studioSkirt() == &mannequinSkirt(), "studioSkirt is the mannequin's skirt");
    check(&mannequinGeometry().bodice == &mannequinBodice(), "geometry struct holds the bodice");

    // The two panels meet along the same waist segment and do not overlap.
    for (Point2D waist : {Point2D{915, 377}, Point2D{985, 377}})
        check(hasVertex(bodice, waist) && hasVertex(skirt, waist), "panels share the waist edge");
    check(clipPolygonToConvex(bodice, skirt).empty(), "bodice and skirt do not overlap");
}

void testConvexityHelper() {
    check(isConvexPolygon({{0, 0}, {4, 0}, {4, 4}, {0, 4}}), "square (one winding) is convex");
    check(isConvexPolygon({{0, 0}, {0, 4}, {4, 4}, {4, 0}}), "square (other winding) is convex");
    check(isConvexPolygon({{0, 0}, {2, 0}, {4, 0}, {4, 4}, {0, 4}}),
          "a collinear vertex does not break convexity");
    check(!isConvexPolygon({{0, 0}, {6, 0}, {6, 6}, {4, 6}, {4, 2}, {2, 2}, {2, 6}, {0, 6}}),
          "a U shape is concave");
    check(!isConvexPolygon({{0, 0}, {4, 4}, {4, 0}, {0, 4}}), "a bow-tie is not convex");
    check(!isConvexPolygon({{0, 0}, {4, 0}}), "two points are not a polygon");
    const double nan = std::numeric_limits<double>::quiet_NaN();
    check(!isConvexPolygon({{0, 0}, {4, 0}, {nan, 4}}), "a non-finite vertex is rejected");
}

void testBounds() {
    const Rect b = polygonBounds(mannequinBodice());
    check(b.minX == 897 && b.minY == 278 && b.maxX == 1003 && b.maxY == 377, "bodice bounds");
    const Rect s = polygonBounds(mannequinSkirt());
    check(s.minX == 859 && s.minY == 377 && s.maxX == 1041 && s.maxY == 574, "skirt bounds");
    bool threw = false;
    try {
        polygonBounds({});
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    check(threw, "bounds of an empty polygon throw");
}

void testFigureGeometry() {
    const MannequinGeometry& m = mannequinGeometry();
    check(m.headRadiusX > 0 && m.headRadiusY > 0, "head has positive radii");
    check(isValidPolygon(m.neck), "neck is valid");
    for (const Polygon& arm : m.arms) check(isValidPolygon(arm), "arm is valid");
    for (const Polygon& leg : m.legs) check(isValidPolygon(leg), "leg is valid");
    for (const PixelBox& shoe : m.shoes) check(shoe.width > 0 && shoe.height > 0, "shoe has size");
    // Arms stay outside the bodice, so the pattern never covers them.
    for (const Polygon& arm : m.arms)
        check(clipPolygonToConvex(arm, mannequinBodice()).empty(), "arm does not overlap the bodice");
}

void testUnpatternedRender() {
    Canvas c(studioWidth, studioHeight, paper);
    drawUnpatternedMannequin(c, skin, outline, cloth);
    check(same(c.pixelAt(950, 320), cloth), "bodice interior is the dress colour");
    check(same(c.pixelAt(950, 480), cloth), "skirt interior is the dress colour");
    check(same(c.pixelAt(950, 378), outline), "waistband covers the waist seam");
    check(same(c.pixelAt(950, 214), skin), "face is skin");
    check(same(c.pixelAt(874, 355), skin), "left arm is skin");
    check(same(c.pixelAt(1026, 355), skin), "right arm is skin");
    check(same(c.pixelAt(930, 600), skin), "leg below the skirt is skin");
    check(same(c.pixelAt(920, 644), outline), "shoe is drawn");
    check(same(c.pixelAt(897, 278), outline), "bodice corner is outlined");
    check(same(c.pixelAt(100, 100), paper), "background away from the figure is untouched");

    // The figure alone never paints inside the bodice: the garment owns it.
    Canvas figure(studioWidth, studioHeight, paper);
    drawMannequinFigure(figure, skin, outline);
    Canvas mask(studioWidth, studioHeight, {0, 0, 0});
    fillPolygon(mask, mannequinBodice(), {255, 255, 255});
    bool clean = true;
    for (int y = 0; y < studioHeight; ++y)
        for (int x = 0; x < studioWidth; ++x)
            if (mask.pixelAt(x, y).red == 255 && !same(figure.pixelAt(x, y), paper)) clean = false;
    check(clean, "the figure leaves every bodice pixel to the garment");
}

} // namespace

int main() {
    try {
        testPanelsAreConvexAndShared();
        testConvexityHelper();
        testBounds();
        testFigureGeometry();
        testUnpatternedRender();
    } catch (const std::exception& e) {
        std::cerr << "Unexpected exception: " << e.what() << "\n";
        return 1;
    }
    if (failures == 0) {
        std::cout << "Mannequin panels, convexity and unpatterned figure passed.\n";
        return 0;
    }
    std::cerr << failures << " mannequin check(s) failed\n";
    return 1;
}
