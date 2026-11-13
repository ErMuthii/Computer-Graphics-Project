#include "kitenge/polygon_clip.hpp"

#include <cmath>
#include <cstddef>
#include <exception>
#include <iostream>
#include <stdexcept>

namespace {

using kitenge::Point2D;
using kitenge::Polygon;

void require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

bool near(double first, double second) {
    return std::abs(first - second) <= 1e-9;
}

double area(const Polygon& polygon) {
    double twiceArea = 0.0;
    for (std::size_t i = 0; i < polygon.size(); ++i) {
        const Point2D& current = polygon[i];
        const Point2D& next = polygon[(i + 1) % polygon.size()];
        twiceArea += current.x * next.y - next.x * current.y;
    }
    return std::abs(twiceArea) / 2.0;
}

bool contains(const Polygon& polygon, Point2D point) {
    for (const Point2D& vertex : polygon) {
        if (near(vertex.x, point.x) && near(vertex.y, point.y)) {
            return true;
        }
    }
    return false;
}

bool samePolygon(const Polygon& first, const Polygon& second) {
    if (first.size() != second.size()) {
        return false;
    }
    for (std::size_t i = 0; i < first.size(); ++i) {
        if (!near(first[i].x, second[i].x) || !near(first[i].y, second[i].y)) {
            return false;
        }
    }
    return true;
}

bool rejectsBoundary(const Polygon& boundary) {
    const Polygon subject{{0, 0}, {1, 0}, {1, 1}};
    try {
        (void)kitenge::clipPolygonToConvex(subject, boundary);
    } catch (const std::invalid_argument&) {
        return true;
    }
    return false;
}

Polygon rectangle(double minX, double minY, double maxX, double maxY) {
    return {{minX, minY}, {maxX, minY}, {maxX, maxY}, {minX, maxY}};
}

} // namespace

int main() {
    try {
        using kitenge::clipPolygonToConvex;
        const Polygon window = rectangle(0, 0, 10, 10);

        require(clipPolygonToConvex({}, window).empty(),
                "An empty subject must produce an empty polygon.");
        require(clipPolygonToConvex({{1, 1}, {5, 5}}, window).empty(),
                "A subject with fewer than three vertices has no area to keep.");

        const Polygon inside{{2, 2}, {8, 3}, {5, 7}};
        require(samePolygon(clipPolygonToConvex(inside, window), inside),
                "A fully inside polygon must be returned unchanged.");

        require(clipPolygonToConvex(rectangle(20, 20, 30, 30), window).empty(),
                "A fully outside polygon must produce an empty polygon.");
        require(clipPolygonToConvex(rectangle(-8, 2, -2, 8), window).empty(),
                "A polygon beside the window must produce an empty polygon.");

        const Polygon crossing = clipPolygonToConvex(rectangle(5, 2, 15, 8), window);
        require(crossing.size() == 4, "Edge-crossing rectangle should keep four vertices.");
        require(near(area(crossing), 30.0), "Edge-crossing rectangle has the wrong area.");
        require(contains(crossing, {10, 2}) && contains(crossing, {10, 8}),
                "Edge-crossing result is missing its intersection points.");

        const Polygon corner = clipPolygonToConvex(rectangle(-5, -5, 5, 5), window);
        require(near(area(corner), 25.0) && contains(corner, {0, 0}),
                "A polygon over a corner should keep only the inside quarter.");

        const Polygon fractional = clipPolygonToConvex({{5, 5}, {13, 5}, {5, 8}}, window);
        require(contains(fractional, {10, 6.125}) && contains(fractional, {10, 5}),
                "Intersections must keep their fractional coordinates.");

        require(samePolygon(clipPolygonToConvex(window, window), window),
                "A polygon equal to the boundary must be returned unchanged.");
        const Polygon touchingInside{{0, 5}, {5, 0}, {10, 5}, {5, 10}};
        require(samePolygon(clipPolygonToConvex(touchingInside, window), touchingInside),
                "Vertices on the boundary must count as inside.");
        require(clipPolygonToConvex(rectangle(10, 0, 20, 10), window).empty(),
                "A polygon sharing only an edge from outside has no visible area.");
        require(clipPolygonToConvex(rectangle(10, 10, 20, 20), window).empty(),
                "A polygon touching only a corner has no visible area.");

        const Polygon enclosing = clipPolygonToConvex(rectangle(-5, -5, 15, 15), window);
        require(near(area(enclosing), 100.0), "An enclosing polygon should become the window.");

        const Polygon clockwiseWindow{{0, 0}, {0, 10}, {10, 10}, {10, 0}};
        require(near(area(clipPolygonToConvex(rectangle(5, 2, 15, 8), clockwiseWindow)), 30.0),
                "Boundary winding must not change the clipped result.");

        const Polygon trapezoid{{2, 0}, {8, 0}, {10, 10}, {0, 10}};
        const Polygon band = clipPolygonToConvex(rectangle(-5, 0, 15, 5), trapezoid);
        require(near(area(band), 35.0), "Clipping against a trapezoid panel has the wrong area.");
        require(contains(band, {1, 5}) && contains(band, {9, 5}),
                "Trapezoid clipping is missing its slanted-edge intersections.");

        const Polygon chevron{{-4, 2}, {6, 2}, {14, 6}, {6, 10}, {-4, 10}, {4, 6}};
        const Polygon clippedChevron = clipPolygonToConvex(chevron, window);
        require(near(area(clippedChevron), 64.0),
                "A concave subject should be clipped to the correct area.");

        require(rejectsBoundary({{0, 0}, {10, 0}}), "A two-vertex boundary was accepted.");
        require(rejectsBoundary({{0, 0}, {5, 5}, {10, 10}}),
                "A boundary with no area was accepted.");
        require(rejectsBoundary({{0, 0}, {10, 0}, {5, 3}, {10, 10}, {0, 10}}),
                "A concave boundary was accepted.");

        std::cout << "Polygon clipping checks passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Polygon clipping check failed: " << error.what() << '\n';
        return 1;
    }
}
