#include "kitenge/polygon_clip.hpp"

#include <cmath>
#include <cstddef>
#include <stdexcept>

namespace kitenge {
namespace {

// Distances below this are treated as lying on a clip edge, so values that are
// exactly on the boundary are not lost to floating-point rounding.
constexpr double kEpsilon = 1e-9;

// Cross product of (b - a) and (p - a). Its sign says on which side of the
// directed line a -> b the point p lies; zero means p is on the line.
double cross(Point2D a, Point2D b, Point2D p) {
    return (b.x - a.x) * (p.y - a.y) - (b.y - a.y) * (p.x - a.x);
}

double signedArea(const Polygon& polygon) {
    double twiceArea = 0.0;
    for (std::size_t i = 0; i < polygon.size(); ++i) {
        const Point2D& current = polygon[i];
        const Point2D& next = polygon[(i + 1) % polygon.size()];
        twiceArea += current.x * next.y - next.x * current.y;
    }
    return twiceArea / 2.0;
}

// Returns +1 or -1 for the boundary's winding so that `orientation * cross(...)`
// is non-negative for every point inside the convex boundary.
double validateBoundary(const Polygon& boundary) {
    if (boundary.size() < 3) {
        throw std::invalid_argument("Clip boundary needs at least three vertices.");
    }
    const double area = signedArea(boundary);
    if (std::abs(area) <= kEpsilon) {
        throw std::invalid_argument("Clip boundary has no area.");
    }
    const double orientation = area > 0.0 ? 1.0 : -1.0;
    const std::size_t count = boundary.size();
    for (std::size_t i = 0; i < count; ++i) {
        const Point2D& a = boundary[i];
        const Point2D& b = boundary[(i + 1) % count];
        const Point2D& c = boundary[(i + 2) % count];
        if (orientation * cross(a, b, c) < -kEpsilon) {
            throw std::invalid_argument("Clip boundary must be convex.");
        }
    }
    return orientation;
}

bool samePoint(Point2D first, Point2D second) {
    return std::abs(first.x - second.x) <= kEpsilon
        && std::abs(first.y - second.y) <= kEpsilon;
}

// Intersection of segment start -> end with the clip line, given the signed
// distances of both endpoints from that line. The distances have opposite signs.
Point2D intersect(Point2D start, Point2D end, double startDistance, double endDistance) {
    const double t = startDistance / (startDistance - endDistance);
    return {start.x + t * (end.x - start.x), start.y + t * (end.y - start.y)};
}

Polygon clipAgainstEdge(const Polygon& input, Point2D edgeStart, Point2D edgeEnd,
                        double orientation) {
    Polygon output;
    const std::size_t count = input.size();
    for (std::size_t i = 0; i < count; ++i) {
        const Point2D& previous = input[(i + count - 1) % count];
        const Point2D& current = input[i];
        const double previousDistance = orientation * cross(edgeStart, edgeEnd, previous);
        const double currentDistance = orientation * cross(edgeStart, edgeEnd, current);
        const bool previousInside = previousDistance >= -kEpsilon;
        const bool currentInside = currentDistance >= -kEpsilon;

        if (currentInside) {
            if (!previousInside) {
                output.push_back(intersect(previous, current, previousDistance, currentDistance));
            }
            output.push_back(current);
        } else if (previousInside) {
            output.push_back(intersect(previous, current, previousDistance, currentDistance));
        }
    }
    return output;
}

// Removes repeated neighbours, which appear when a vertex lies on a clip edge.
Polygon removeDuplicatePoints(const Polygon& polygon) {
    Polygon result;
    for (const Point2D& point : polygon) {
        if (result.empty() || !samePoint(result.back(), point)) {
            result.push_back(point);
        }
    }
    while (result.size() > 1 && samePoint(result.front(), result.back())) {
        result.pop_back();
    }
    return result;
}

} // namespace

Polygon clipPolygonToConvex(const Polygon& subject, const Polygon& boundary) {
    const double orientation = validateBoundary(boundary);
    if (subject.size() < 3) {
        return {};
    }

    Polygon output = subject;
    for (std::size_t i = 0; i < boundary.size() && !output.empty(); ++i) {
        const Point2D& edgeStart = boundary[i];
        const Point2D& edgeEnd = boundary[(i + 1) % boundary.size()];
        output = clipAgainstEdge(output, edgeStart, edgeEnd, orientation);
    }

    output = removeDuplicatePoints(output);
    // A result that is a point, a segment or a sliver along the boundary has no
    // visible interior, so it is reported as an empty polygon.
    if (output.size() < 3 || std::abs(signedArea(output)) <= kEpsilon) {
        return {};
    }
    return output;
}

} // namespace kitenge
