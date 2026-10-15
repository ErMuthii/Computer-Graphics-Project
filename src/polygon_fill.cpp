#include "kitenge/polygon_fill.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace kitenge {

namespace {

double cross(Point2D o, Point2D a, Point2D b) {
    return (a.x - o.x) * (b.y - o.y) - (a.y - o.y) * (b.x - o.x);
}

bool onSegment(Point2D a, Point2D b, Point2D p) {
    return std::min(a.x, b.x) <= p.x && p.x <= std::max(a.x, b.x) &&
           std::min(a.y, b.y) <= p.y && p.y <= std::max(a.y, b.y);
}

// Segments touch or cross (includes collinear overlap).
bool segmentsIntersect(Point2D a, Point2D b, Point2D c, Point2D d) {
    double d1 = cross(a, b, c), d2 = cross(a, b, d);
    double d3 = cross(c, d, a), d4 = cross(c, d, b);
    if (((d1 > 0 && d2 < 0) || (d1 < 0 && d2 > 0)) &&
        ((d3 > 0 && d4 < 0) || (d3 < 0 && d4 > 0))) return true;
    if (d1 == 0 && onSegment(a, b, c)) return true;
    if (d2 == 0 && onSegment(a, b, d)) return true;
    if (d3 == 0 && onSegment(c, d, a)) return true;
    if (d4 == 0 && onSegment(c, d, b)) return true;
    return false;
}

} // namespace

bool isValidPolygon(const Polygon& p) {
    const std::size_t n = p.size();
    if (n < 3) return false;

    for (const Point2D& v : p)
        if (!std::isfinite(v.x) || !std::isfinite(v.y)) return false;

    // Shoelace area must be non-zero.
    double area2 = 0.0;
    for (std::size_t i = 0; i < n; ++i) {
        const Point2D& a = p[i];
        const Point2D& b = p[(i + 1) % n];
        area2 += a.x * b.y - b.x * a.y;
    }
    if (area2 == 0.0) return false;

    // Non-adjacent edges must not touch.
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = i + 1; j < n; ++j) {
            bool adjacent = (j == i + 1) || (i == 0 && j == n - 1);
            if (adjacent) continue;
            if (segmentsIntersect(p[i], p[(i + 1) % n], p[j], p[(j + 1) % n]))
                return false;
        }
    }
    return true;
}

void fillPolygon(Canvas& canvas, const Polygon& polygon, Color color) {
    if (!isValidPolygon(polygon)) return;

    const std::size_t n = polygon.size();
    double minY = polygon[0].y, maxY = polygon[0].y;
    for (const Point2D& v : polygon) {
        minY = std::min(minY, v.y);
        maxY = std::max(maxY, v.y);
    }

    // Rows whose center (y + 0.5) lies in [minY, maxY), clamped to the canvas.
    long long yStart = std::max(0LL, static_cast<long long>(std::ceil(minY - 0.5)));
    long long yEnd = std::min(static_cast<long long>(canvas.height()) - 1,
                              static_cast<long long>(std::ceil(maxY - 0.5)) - 1);

    std::vector<double> xs;
    for (long long y = yStart; y <= yEnd; ++y) {
        const double yc = static_cast<double>(y) + 0.5;
        xs.clear();

        for (std::size_t i = 0; i < n; ++i) {
            const Point2D& a = polygon[i];
            const Point2D& b = polygon[(i + 1) % n];
            if (a.y == b.y) continue; // horizontal edge: skip

            // Half-open rule [low, high): a vertex exactly on the scanline
            // counts for the edge going down from it, never both edges.
            bool crosses = (a.y <= yc && b.y > yc) || (b.y <= yc && a.y > yc);
            if (!crosses) continue;

            xs.push_back(a.x + (yc - a.y) * (b.x - a.x) / (b.y - a.y));
        }

        std::sort(xs.begin(), xs.end());

        // Crossings come in pairs: fill between each pair.
        for (std::size_t k = 0; k + 1 < xs.size(); k += 2) {
            long long x0 = std::max(0LL, static_cast<long long>(std::ceil(xs[k] - 0.5)));
            long long x1 = std::min(static_cast<long long>(canvas.width()) - 1,
                                    static_cast<long long>(std::ceil(xs[k + 1] - 0.5)) - 1);
            for (long long x = x0; x <= x1; ++x)
                canvas.setPixel(static_cast<int>(x), static_cast<int>(y), color);
        }
    }
}

} // namespace kitenge