#pragma once

// 2D transformation mathematics for the Kitenge pattern studio.
// Owner: Waruhiu Jeremy Kang'ethe, 166263.
//
// Every transform is a 3x3 matrix in homogeneous coordinates. A point (x, y) is
// carried as the column vector (x, y, 1). Translation is not a linear map in 2D
// (it moves the origin, and no 2x2 matrix can do that), but in homogeneous form
// it becomes a matrix product like rotation, scaling and reflection. That is
// what lets all four transforms compose by matrix multiplication alone.
//
// Conventions shared with the rest of the project (docs/team/README.md):
//   * Matrices are row-major, m[row][column], and act on column vectors:
//     p' = M * p. In a product A * B, the right-hand matrix B acts first.
//   * Screen coordinates: origin top left, x right, y DOWN. The rotation matrix
//     is the standard mathematical one, so a positive angle turns +x towards +y.
//     Because +y points down on this canvas, a positive angle appears CLOCKWISE.
//   * Angles are radians. Convert from the degrees shown in a UI explicitly.
//   * reflectX negates x: a mirror across the Y-axis (left-right flip).
//     reflectY negates y: a mirror across the X-axis (top-bottom flip).
//   * All geometry stays in doubles; round only when rasterizing.
//
// This header depends on kitenge/types.hpp only. Nothing here touches Canvas,
// PPM output, GLFW or OpenGL.

#include "kitenge/types.hpp"

#include <array>
#include <vector>

namespace kitenge {

// A 3x3 homogeneous matrix, row-major, acting on column vectors (x, y, 1).
// A default-constructed Mat3 is the identity. For the affine maps built below
// the bottom row is always (0, 0, 1):
//     | a  b  tx |   | x |   | a*x + b*y + tx |
//     | c  d  ty | * | y | = | c*x + d*y + ty |
//     | 0  0  1  |   | 1 |   |       1        |
struct Mat3 {
    std::array<std::array<double, 3>, 3> m{{{{1.0, 0.0, 0.0}},
                                             {{0.0, 1.0, 0.0}},
                                             {{0.0, 0.0, 1.0}}}};

    static Mat3 identity() noexcept { return Mat3{}; }
};

// left * right. The result applies `right` first, then `left`.
Mat3 multiply(const Mat3& left, const Mat3& right) noexcept;
Mat3 operator*(const Mat3& left, const Mat3& right) noexcept;

// True when all nine entries are finite.
bool isFinite(const Mat3& matrix) noexcept;

// Applies the matrix to the homogeneous point (x, y, 1) and returns the
// Cartesian result (x'/w, y'/w). For the affine matrices built here w is 1.
// Throws std::invalid_argument if the point or matrix is non-finite, if w is
// zero, or if the result overflows to a non-finite value.
Point2D transformPoint(const Mat3& matrix, Point2D point);

// --- Builders ----------------------------------------------------------------
// Each builder throws std::invalid_argument for a non-finite argument.

// T(offset): (x, y) -> (x + offset.x, y + offset.y).
Mat3 translationMatrix(Point2D offset);
Mat3 translationMatrix(double dx, double dy);

// R(angle) about the origin, angle in radians:
//     | cos  -sin  0 |
//     | sin   cos  0 |
//     |  0     0   1 |
// (1, 0) -> (cos, sin). With y down, a positive angle appears clockwise.
Mat3 rotationMatrix(double angleRadians);

// S(sx, sy) about the origin: (x, y) -> (sx * x, sy * y). The two factors are
// independent (non-uniform scaling). Both must be finite and strictly
// positive; mirroring uses reflectionMatrix, never a negative factor.
Mat3 scalingMatrix(double sx, double sy);

// F(reflectX, reflectY) about the origin. reflectX negates x (mirror across
// the Y-axis); reflectY negates y (mirror across the X-axis). Both flags
// together equal a 180-degree rotation. Neither flag gives the identity.
Mat3 reflectionMatrix(bool reflectX, bool reflectY) noexcept;

// --- Parameterised composition -----------------------------------------------

// Transform settings from the shared contract. This matches PatternTransform
// in include/kitenge/pattern.hpp field for field (names, types and defaults),
// so the pattern engine can alias or replace its interim struct with this one.
//   translation   moves the pattern origin, applied last
//   angleRadians  rotation about `pivot`; positive appears clockwise on screen
//   scale         uniform scale about `pivot`; finite and strictly positive
//   pivot         fixed point of rotation, reflection and scale (local coords)
//   reflectX      negate x about the pivot (mirror across the vertical axis)
//   reflectY      negate y about the pivot (mirror across the horizontal axis)
struct TransformParams {
    Point2D translation{0.0, 0.0};
    double angleRadians = 0.0;
    double scale = 1.0;
    Point2D pivot{0.0, 0.0};
    bool reflectX = false;
    bool reflectY = false;
};

// Throws std::invalid_argument unless every numeric field is finite and
// `scale` is strictly positive. The two composition functions below call this,
// and also throw if finite inputs overflow to a non-finite matrix entry.
void validateTransformParams(const TransformParams& params);

// The pivot sandwich, without the final translation:
//     T(pivot) * R(angle) * F(reflect) * S(scale) * T(-pivot)
// Rotation, reflection and scaling only keep the origin fixed, so the geometry
// is first shifted to put the pivot at the origin, transformed, then shifted
// back. The pivot is therefore a fixed point of this matrix.
// This equals the pattern engine's private localTransform.
Mat3 pivotTransformMatrix(const TransformParams& params);

// The full shared order (rightmost acts on the point first):
//     T(translation) * T(pivot) * R(angle) * F(reflect) * S(scale) * T(-pivot)
// The pattern engine places tile (column, row) with
//     translationMatrix(origin + tileOffset) * transformMatrix(params)
// which equals its T(origin + translation + tileOffset) * localTransform,
// because translations simply add.
Mat3 transformMatrix(const TransformParams& params);

// --- Geometry ----------------------------------------------------------------
// These return NEW containers and never modify their input. The pattern engine
// transforms the same catalog motif once per tile; mutating the source would
// make every tile compound the previous tile's transform.
// An empty input gives an empty output. Throws std::invalid_argument for a
// non-finite matrix or vertex, or a non-finite result (see transformPoint);
// on a throw no partial output is returned.

// Transforms every vertex of a filled contour. The vertex order is kept; a
// reflection reverses the contour's winding, which the even-odd scanline
// filler and the convex clipper both accept.
Polygon transformPolygon(const Mat3& matrix, const Polygon& polygon);

// Transforms every point of a stroked path (an open or closed polyline). An
// affine map sends straight segments to straight segments, so transforming the
// points transforms the whole stroke, and a closed path stays closed.
std::vector<Point2D> transformPath(const Mat3& matrix, const std::vector<Point2D>& path);

} // namespace kitenge
