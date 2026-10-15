#pragma once

#include <array>
#include <vector>

#include "kitenge/canvas.hpp"

namespace kitenge {

// Cubic Bezier evaluation by Andrew Karanja Gathirwa (167144);
// canvas rendering added during module integration.
//
// Coordinates follow the shared contract: origin top left, x grows right,
// y grows down. Evaluation stays in doubles; drawing rounds at the raster boundary.
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

// Connect sampled points with Bresenham segments. All four controls must be
// finite and sampled raster coordinates must fit in an int.
void drawCubicBezier(Canvas& canvas, const std::array<Point2D, 4>& control,
                     Color color, int segments = 100);

}  // namespace kitenge
