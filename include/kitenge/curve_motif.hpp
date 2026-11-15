#pragma once

// Cubic Bezier "arch" motif for the catalog.
// Owner: Andrew Karanja Gathirwa, 167144 (registered in Deborah's catalog).
//
// The motif is defined once as cubic control points in the catalog's local
// coordinates (centred on (0,0), inside [-25, 25], x right, y down). It is
// sampled with the CP1 evaluator (sampleCubicBezier in curves.hpp) into the
// polygon fills and polyline strokes that MotifShape requires.
//
// Why sampling keeps it transformable: every studio transform is affine, and an
// affine map applied to a curve's control points gives exactly the same curve as
// applying it to every point on the curve. The pattern engine transforms the
// sampled points, which therefore lie on the transformed Bezier curve. Strokes
// are clipped by Kelvin's renderer (Cyrus-Beck), never drawn over the dress edge.
//
// Shape, in drawing order:
//   arch body   fill,   primary role   arch top plus two scalloped feet
//   leaf        fill,   accent role    two mirrored curves meeting at both tips
//   outline     stroke, accent role    closed path round the arch body
//   vein        stroke, primary role   straight line between the leaf tips

#include "kitenge/motifs.hpp"
#include "kitenge/types.hpp"

#include <array>
#include <vector>

namespace kitenge {

using CubicControls = std::array<Point2D, 4>;

inline constexpr const char* kBezierArchLabel = "BEZIER ARCH";
inline constexpr int kBezierArchSegmentsPerCurve = 12;

// Closed outline: arch top, right scallop, left scallop. Each segment starts
// where the previous one ends, and the last ends where the first starts.
const std::vector<CubicControls>& bezierArchOutlineControls();

// Leaf: left half from the bottom tip to the top tip, then the right half from
// the top tip back to the bottom tip (mirror images across x = 0).
const std::vector<CubicControls>& bezierArchLeafControls();

// Every segment above, outline first: for drawing control points separately.
std::vector<CubicControls> bezierArchControlPoints();

// Joins sampled cubic segments into one path. A segment's first point is
// dropped when it repeats the previous segment's last point, so shared joints
// appear once. Throws std::invalid_argument for segments outside
// [1, kMaxBezierSegments] (from sampleCubicBezier).
std::vector<Point2D> sampleJoinedCubics(const std::vector<CubicControls>& segments,
                                        int segmentsPerCurve);

// The catalog motif. Passes isValidMotif for any segmentsPerCurve >= 2.
MotifShape bezierArchMotif(int segmentsPerCurve = kBezierArchSegmentsPerCurve);

} // namespace kitenge
