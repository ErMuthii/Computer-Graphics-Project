#include "kitenge/transforms.hpp"

#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <string>

namespace kitenge {
namespace {

bool finite(Point2D p) noexcept { return std::isfinite(p.x) && std::isfinite(p.y); }

void requireFinite(double value, const char* name) {
    if (!std::isfinite(value)) {
        throw std::invalid_argument(std::string(name) + " must be finite.");
    }
}

void requireFinite(Point2D value, const char* name) {
    if (!finite(value)) {
        throw std::invalid_argument(std::string(name) + " must have finite coordinates.");
    }
}

void requirePositiveScale(double value, const char* name) {
    requireFinite(value, name);
    if (value <= 0.0) {
        throw std::invalid_argument(std::string(name) +
                                    " must be greater than zero; use the reflection flags to mirror.");
    }
}

void requireFiniteMatrix(const Mat3& matrix) {
    if (!isFinite(matrix)) {
        throw std::invalid_argument("Transform matrix entries must be finite.");
    }
}

// Applies a matrix already known to be finite. Validates the point and result.
Point2D applyChecked(const Mat3& matrix, Point2D point) {
    requireFinite(point, "Transformed point");
    const auto& m = matrix.m;
    const double x = m[0][0] * point.x + m[0][1] * point.y + m[0][2];
    const double y = m[1][0] * point.x + m[1][1] * point.y + m[1][2];
    const double w = m[2][0] * point.x + m[2][1] * point.y + m[2][2];
    if (w == 0.0) {
        throw std::invalid_argument("Homogeneous w is zero; the point maps to infinity.");
    }
    const Point2D result = w == 1.0 ? Point2D{x, y} : Point2D{x / w, y / w};
    if (!finite(result)) {
        throw std::invalid_argument("Transformed point overflowed to a non-finite value.");
    }
    return result;
}

std::vector<Point2D> transformPoints(const Mat3& matrix, const std::vector<Point2D>& points) {
    requireFiniteMatrix(matrix);
    std::vector<Point2D> result;
    result.reserve(points.size());
    for (const Point2D& point : points) result.push_back(applyChecked(matrix, point));
    return result;
}

} // namespace

Mat3 multiply(const Mat3& left, const Mat3& right) noexcept {
    Mat3 result;
    for (std::size_t row = 0; row < 3; ++row) {
        for (std::size_t column = 0; column < 3; ++column) {
            double sum = 0.0;
            for (std::size_t k = 0; k < 3; ++k) sum += left.m[row][k] * right.m[k][column];
            result.m[row][column] = sum;
        }
    }
    return result;
}

Mat3 operator*(const Mat3& left, const Mat3& right) noexcept { return multiply(left, right); }

bool isFinite(const Mat3& matrix) noexcept {
    for (const auto& row : matrix.m) {
        for (const double value : row) {
            if (!std::isfinite(value)) return false;
        }
    }
    return true;
}

Point2D transformPoint(const Mat3& matrix, Point2D point) {
    requireFiniteMatrix(matrix);
    return applyChecked(matrix, point);
}

Mat3 translationMatrix(Point2D offset) {
    requireFinite(offset, "Translation");
    Mat3 result;
    result.m[0][2] = offset.x;
    result.m[1][2] = offset.y;
    return result;
}

Mat3 translationMatrix(double dx, double dy) { return translationMatrix(Point2D{dx, dy}); }

Mat3 rotationMatrix(double angleRadians) {
    requireFinite(angleRadians, "Rotation angle");
    const double c = std::cos(angleRadians);
    const double s = std::sin(angleRadians);
    Mat3 result;
    result.m[0][0] = c;
    result.m[0][1] = -s;
    result.m[1][0] = s;
    result.m[1][1] = c;
    return result;
}

Mat3 scalingMatrix(double sx, double sy) {
    requirePositiveScale(sx, "Scale factor sx");
    requirePositiveScale(sy, "Scale factor sy");
    Mat3 result;
    result.m[0][0] = sx;
    result.m[1][1] = sy;
    return result;
}

Mat3 reflectionMatrix(bool reflectX, bool reflectY) noexcept {
    Mat3 result;
    result.m[0][0] = reflectX ? -1.0 : 1.0;
    result.m[1][1] = reflectY ? -1.0 : 1.0;
    return result;
}

void validateTransformParams(const TransformParams& params) {
    requireFinite(params.translation, "Translation");
    requireFinite(params.angleRadians, "Rotation angle");
    requirePositiveScale(params.scale, "Scale");
    requireFinite(params.pivot, "Pivot");
}

Mat3 pivotTransformMatrix(const TransformParams& params) {
    validateTransformParams(params);
    const Point2D pivot = params.pivot;
    const Mat3 result = translationMatrix(pivot) * rotationMatrix(params.angleRadians) *
                        reflectionMatrix(params.reflectX, params.reflectY) *
                        scalingMatrix(params.scale, params.scale) *
                        translationMatrix(-pivot.x, -pivot.y);
    // Finite inputs can still overflow when multiplied (for example a huge
    // scale times a huge pivot); report that instead of returning infinities.
    requireFiniteMatrix(result);
    return result;
}

Mat3 transformMatrix(const TransformParams& params) {
    const Mat3 result = translationMatrix(params.translation) * pivotTransformMatrix(params);
    requireFiniteMatrix(result);
    return result;
}

Polygon transformPolygon(const Mat3& matrix, const Polygon& polygon) {
    return transformPoints(matrix, polygon);
}

std::vector<Point2D> transformPath(const Mat3& matrix, const std::vector<Point2D>& path) {
    return transformPoints(matrix, path);
}

} // namespace kitenge
