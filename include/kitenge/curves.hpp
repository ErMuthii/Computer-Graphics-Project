#pragma once

#include <array>
#include <vector>

#include "kitenge/types.hpp"

namespace kitenge {

// Cubic Bezier curves for CP1 (owner: Andrew Karanja Gathirwa, 167144).
//
// Coordinates follow the shared contract: origin top left, x grows right,
// y grows down. Evaluation stays in doubles. Rendering (drawCubicBezier) is
// added once the shared line module (drawLine) is merged into main.
//
// Input rules (invalid input throws instead of being silently "fixed"):
//   - Control points must be finite           -> std::invalid_argument
//   - t must not be NaN                        -> std::invalid_argument
//   - t must lie in [0, 1] (no clamping)       -> std::out_of_range
//   - segments must be in [1, kMaxBezierSegments] -> std::invalid_argument

inline constexpr int kMaxBezierSegments = 10000;

// Point on the curve at parameter t, computed with de Casteljau's algorithm.
// t = 0 returns control[0] exactly and t = 1 returns control[3] exactly.
Point2D evaluateCubicBezier(const std::array<Point2D, 4>& control, double t);

// segments + 1 points at t = 0, 1/segments, ..., 1 (first and last are exact endpoints).
std::vector<Point2D> sampleCubicBezier(const std::array<Point2D, 4>& control,
                                       int segments);

}  // namespace kitenge
