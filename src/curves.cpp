#include "kitenge/curves.hpp"

#include <cmath>
#include <cstddef>
#include <stdexcept>

namespace kitenge {
namespace {

Point2D lerp(const Point2D& a, const Point2D& b, double t) {
    return Point2D{a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t};
}

void requireFiniteControl(const std::array<Point2D, 4>& control) {
    for (const Point2D& p : control) {
        if (!std::isfinite(p.x) || !std::isfinite(p.y)) {
            throw std::invalid_argument("cubic Bezier control points must be finite");
        }
    }
}

void requireValidSegments(int segments) {
    if (segments < 1 || segments > kMaxBezierSegments) {
        throw std::invalid_argument("cubic Bezier segments must be between 1 and 10000");
    }
}

}  // namespace

Point2D evaluateCubicBezier(const std::array<Point2D, 4>& control, double t) {
    requireFiniteControl(control);
    if (std::isnan(t)) {
        throw std::invalid_argument("cubic Bezier parameter t must be a number");
    }
    if (t < 0.0 || t > 1.0) {
        throw std::out_of_range("cubic Bezier parameter t must be in [0, 1]");
    }

    // Exact endpoints: avoids floating-point drift such as a + (b - a) * 1 != b.
    if (t == 0.0) {
        return control[0];
    }
    if (t == 1.0) {
        return control[3];
    }

    // de Casteljau: repeatedly interpolate between neighbouring points.
    // Level 1: three points on the control polygon's edges.
    const Point2D p01 = lerp(control[0], control[1], t);
    const Point2D p12 = lerp(control[1], control[2], t);
    const Point2D p23 = lerp(control[2], control[3], t);
    // Level 2: two points.
    const Point2D p012 = lerp(p01, p12, t);
    const Point2D p123 = lerp(p12, p23, t);
    // Level 3: the point on the curve.
    return lerp(p012, p123, t);
}

std::vector<Point2D> sampleCubicBezier(const std::array<Point2D, 4>& control,
                                       int segments) {
    requireValidSegments(segments);
    requireFiniteControl(control);

    std::vector<Point2D> samples;
    samples.reserve(static_cast<std::size_t>(segments) + 1);
    for (int i = 0; i <= segments; ++i) {
        // i == segments gives exactly 1.0, so the last sample is control[3].
        const double t = static_cast<double>(i) / static_cast<double>(segments);
        samples.push_back(evaluateCubicBezier(control, t));
    }
    return samples;
}

}  // namespace kitenge
