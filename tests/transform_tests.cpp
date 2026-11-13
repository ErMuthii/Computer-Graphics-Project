#include "kitenge/transforms.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <exception>
#include <functional>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace {

using namespace kitenge;

constexpr double kPi = 3.14159265358979323846;
// Floating-point results are compared with a tolerance: std::cos(pi / 2) is
// about 6.1e-17, not 0, so exact equality would reject correct rotations.
constexpr double kTolerance = 1e-9;
const double kNaN = std::numeric_limits<double>::quiet_NaN();
const double kInf = std::numeric_limits<double>::infinity();

int failures = 0;

void check(bool condition, const char* description) {
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << description << '\n';
    }
}

bool near(double actual, double expected) { return std::abs(actual - expected) < kTolerance; }

bool nearPoint(Point2D actual, Point2D expected) {
    return near(actual.x, expected.x) && near(actual.y, expected.y);
}

void checkPoint(Point2D actual, Point2D expected, const char* description) {
    if (!nearPoint(actual, expected)) {
        std::cerr << "  got (" << actual.x << ", " << actual.y << "), expected (" << expected.x
                  << ", " << expected.y << ")\n";
    }
    check(nearPoint(actual, expected), description);
}

bool nearMatrix(const Mat3& actual, const Mat3& expected) {
    for (std::size_t row = 0; row < 3; ++row) {
        for (std::size_t column = 0; column < 3; ++column) {
            if (!near(actual.m[row][column], expected.m[row][column])) return false;
        }
    }
    return true;
}

Mat3 matrixFromRows(std::array<double, 3> r0, std::array<double, 3> r1, std::array<double, 3> r2) {
    Mat3 result;
    result.m = {{r0, r1, r2}};
    return result;
}

bool samePoints(const std::vector<Point2D>& a, const std::vector<Point2D>& b) {
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0; i < a.size(); ++i) {
        // Exact comparison on purpose: the source must be bit-for-bit untouched.
        if (a[i].x != b[i].x || a[i].y != b[i].y) return false;
    }
    return true;
}

// Passes only if `action` throws std::invalid_argument. Any other outcome,
// including a different exception type, is a failure: invalid values must be
// rejected consistently.
void checkRejects(const std::function<void()>& action, const char* description) {
    bool rejected = false;
    try {
        action();
    } catch (const std::invalid_argument&) {
        rejected = true;
    } catch (const std::exception&) {
        rejected = false;
    }
    check(rejected, description);
}

void testIdentity() {
    const Mat3 identity = Mat3::identity();
    check(nearMatrix(Mat3{}, identity), "identity: default Mat3 is the identity");
    for (std::size_t row = 0; row < 3; ++row) {
        for (std::size_t column = 0; column < 3; ++column) {
            check(identity.m[row][column] == (row == column ? 1.0 : 0.0),
                  "identity: ones on the diagonal, zeros elsewhere");
        }
    }
    checkPoint(transformPoint(identity, {3.5, -7.25}), {3.5, -7.25}, "identity: point unchanged");
    const Mat3 sample = matrixFromRows({{2, 3, 4}}, {{5, 6, 7}}, {{0, 0, 1}});
    check(nearMatrix(identity * sample, sample), "identity: I * M = M");
    check(nearMatrix(sample * identity, sample), "identity: M * I = M");
    check(nearMatrix(transformMatrix(TransformParams{}), identity),
          "identity: default TransformParams compose to the identity");
    check(nearMatrix(rotationMatrix(0.0), identity), "identity: zero rotation");
    check(nearMatrix(scalingMatrix(1.0, 1.0), identity), "identity: unit scale");
    check(nearMatrix(reflectionMatrix(false, false), identity), "identity: no reflection flags");
}

void testMultiplication() {
    const Mat3 a = matrixFromRows({{1, 2, 3}}, {{4, 5, 6}}, {{7, 8, 9}});
    const Mat3 b = matrixFromRows({{9, 8, 7}}, {{6, 5, 4}}, {{3, 2, 1}});
    // Row 0 of A*B: (1*9+2*6+3*3, 1*8+2*5+3*2, 1*7+2*4+3*1) = (30, 24, 18), etc.
    const Mat3 expected = matrixFromRows({{30, 24, 18}}, {{84, 69, 54}}, {{138, 114, 90}});
    check(nearMatrix(multiply(a, b), expected), "multiply: hand-computed 3x3 product");
    check(nearMatrix(a * b, multiply(a, b)), "multiply: operator* matches multiply");
    check(!nearMatrix(a * b, b * a), "multiply: matrix product is not commutative");
}

void testTranslation() {
    const Mat3 first = translationMatrix(2.0, 3.0);
    const Mat3 second = translationMatrix(Point2D{4.0, -1.0});
    // (2,3) then (4,-1): the second acts last, so it is on the left.
    const Mat3 total = second * first;
    checkPoint(transformPoint(total, {0.0, 0.0}), {6.0, 2.0},
               "translation: (2,3) then (4,-1) moves the origin by (6,2)");
    checkPoint(transformPoint(total, {1.0, 1.0}), {7.0, 3.0},
               "translation: (2,3) then (4,-1) moves (1,1) to (7,3)");
    check(nearMatrix(total, translationMatrix(6.0, 2.0)), "translation: composite is T(6,2)");
    check(nearMatrix(first * second, second * first), "translation: translations commute");
}

void testRotation() {
    // cos(pi/2) is not exactly zero in floating point; a tolerance is required.
    check(std::cos(kPi / 2.0) != 0.0, "rotation: cos(pi/2) is a tiny non-zero double");
    const Mat3 quarter = rotationMatrix(kPi / 2.0);
    checkPoint(transformPoint(quarter, {1.0, 0.0}), {0.0, 1.0}, "rotation: 90 degrees takes (1,0) to (0,1)");
    checkPoint(transformPoint(quarter, {0.0, 1.0}), {-1.0, 0.0},
               "rotation: 90 degrees takes (0,1) to (-1,0)");
    // On screen y points down, so (1,0) -> (0,1) is right -> down: clockwise.
    checkPoint(transformPoint(rotationMatrix(kPi), {2.0, 5.0}), {-2.0, -5.0},
               "rotation: 180 degrees negates both coordinates");
    const Point2D turned = transformPoint(rotationMatrix(0.3), {3.0, 4.0});
    check(near(std::hypot(turned.x, turned.y), 5.0), "rotation: preserves distance from origin");
}

void testNonUniformScaling() {
    checkPoint(transformPoint(scalingMatrix(2.0, 3.0), {4.0, 5.0}), {8.0, 15.0},
               "scale: sx=2 and sy=3 act independently");
    checkPoint(transformPoint(scalingMatrix(2.0, 1.0), {4.0, 5.0}), {8.0, 5.0},
               "scale: sx only stretches x");
    checkPoint(transformPoint(scalingMatrix(1.0, 0.5), {4.0, 5.0}), {4.0, 2.5},
               "scale: sy only shrinks y");
    // Non-uniform scale about a pivot c=(1,1): (2,2) -> c + (2*1, 3*1) = (3,4).
    const Mat3 aboutPivot =
        translationMatrix(1.0, 1.0) * scalingMatrix(2.0, 3.0) * translationMatrix(-1.0, -1.0);
    checkPoint(transformPoint(aboutPivot, {2.0, 2.0}), {3.0, 4.0}, "scale: non-uniform about a pivot");
    checkPoint(transformPoint(aboutPivot, {1.0, 1.0}), {1.0, 1.0}, "scale: pivot stays fixed");
}

void testReflections() {
    const Point2D p{3.0, 4.0};
    checkPoint(transformPoint(reflectionMatrix(true, false), p), {-3.0, 4.0},
               "reflection: reflectX negates x (mirror across the Y-axis)");
    checkPoint(transformPoint(reflectionMatrix(false, true), p), {3.0, -4.0},
               "reflection: reflectY negates y (mirror across the X-axis)");
    checkPoint(transformPoint(reflectionMatrix(true, true), p), {-3.0, -4.0},
               "reflection: both flags negate both coordinates");
    check(nearMatrix(reflectionMatrix(true, true), rotationMatrix(kPi)),
          "reflection: both flags equal a 180-degree rotation");

    for (const bool rx : {false, true}) {
        for (const bool ry : {false, true}) {
            const Mat3 f = reflectionMatrix(rx, ry);
            check(nearMatrix(f * f, Mat3::identity()), "reflection: reflecting twice is the identity");
        }
    }

    // Through the parameter struct the mirror line passes through the pivot.
    TransformParams params;
    params.pivot = {5.0, 0.0};
    params.reflectX = true;
    const Mat3 mirrorX = transformMatrix(params);
    checkPoint(transformPoint(mirrorX, {7.0, 2.0}), {3.0, 2.0},
               "reflection: reflectX mirrors across the vertical line x = pivot.x");
    checkPoint(transformPoint(mirrorX * mirrorX, {7.0, 2.0}), {7.0, 2.0},
               "reflection: reflectX about a pivot twice restores the point");
    params.reflectX = false;
    params.reflectY = true;
    params.pivot = {0.0, 10.0};
    const Mat3 mirrorY = transformMatrix(params);
    checkPoint(transformPoint(mirrorY, {4.0, 13.0}), {4.0, 7.0},
               "reflection: reflectY mirrors across the horizontal line y = pivot.y");
    checkPoint(transformPoint(mirrorY * mirrorY, {4.0, 13.0}), {4.0, 13.0},
               "reflection: reflectY about a pivot twice restores the point");
}

void testPivotInvariance() {
    const std::vector<Point2D> pivots{{0.0, 0.0}, {12.5, -3.0}, {-40.0, 25.0}};
    const std::vector<double> angles{0.0, 0.7, kPi / 2.0, -2.4, 3.0 * kPi};
    for (const Point2D pivot : pivots) {
        for (const double angle : angles) {
            for (const double scale : {0.25, 1.0, 3.0}) {
                for (const bool rx : {false, true}) {
                    for (const bool ry : {false, true}) {
                        TransformParams params;
                        params.pivot = pivot;
                        params.angleRadians = angle;
                        params.scale = scale;
                        params.reflectX = rx;
                        params.reflectY = ry;
                        checkPoint(transformPoint(pivotTransformMatrix(params), pivot), pivot,
                                   "pivot: rotation/reflection/scale keep the pivot fixed");
                        params.translation = {7.0, -9.0};
                        checkPoint(transformPoint(transformMatrix(params), pivot),
                                   {pivot.x + 7.0, pivot.y - 9.0},
                                   "pivot: only the final translation moves the pivot");
                    }
                }
            }
        }
    }
    // Without the sandwich, rotation fixes the origin instead and moves the pivot.
    const Point2D moved = transformPoint(rotationMatrix(kPi / 2.0), {10.0, 0.0});
    check(!nearPoint(moved, {10.0, 0.0}), "pivot: plain rotation moves a non-origin point");
}

void testInverseRotation() {
    for (const double angle : {0.0, 0.3, kPi / 2.0, 2.0, -1.1, 7.0}) {
        check(nearMatrix(rotationMatrix(angle) * rotationMatrix(-angle), Mat3::identity()),
              "inverse rotation: R(a) * R(-a) is the identity");
        check(nearMatrix(rotationMatrix(-angle) * rotationMatrix(angle), Mat3::identity()),
              "inverse rotation: R(-a) * R(a) is the identity");
        const Point2D p{3.0, -8.0};
        checkPoint(transformPoint(rotationMatrix(-angle), transformPoint(rotationMatrix(angle), p)), p,
                   "inverse rotation: round trip restores the point");
    }
    // The inverse of a rotation is its transpose (rotation matrices are orthogonal).
    const Mat3 r = rotationMatrix(0.8);
    const Mat3 inverse = rotationMatrix(-0.8);
    for (std::size_t row = 0; row < 2; ++row) {
        for (std::size_t column = 0; column < 2; ++column) {
            check(near(inverse.m[row][column], r.m[column][row]),
                  "inverse rotation: R(-a) is the transpose of R(a)");
        }
    }
}

void testOrderMatters() {
    const Mat3 t = translationMatrix(10.0, 0.0);
    const Mat3 r = rotationMatrix(kPi / 2.0);
    const Point2D p{1.0, 0.0};
    // Translate then rotate: R * T. (1,0) -> (11,0) -> (0,11).
    checkPoint(transformPoint(r * t, p), {0.0, 11.0}, "order: translate then rotate");
    // Rotate then translate: T * R. (1,0) -> (0,1) -> (10,1).
    checkPoint(transformPoint(t * r, p), {10.0, 1.0}, "order: rotate then translate");
    check(!nearPoint(transformPoint(r * t, p), transformPoint(t * r, p)),
          "order: translate-then-rotate differs from rotate-then-translate");

    // Inside the shared order the reflection acts before the rotation.
    TransformParams params;
    params.angleRadians = kPi / 2.0;
    params.reflectX = true;
    // F first: (1,0) -> (-1,0); then R(90): (-1,0) -> (0,-1).
    checkPoint(transformPoint(transformMatrix(params), p), {0.0, -1.0},
               "order: the shared order reflects before it rotates");
    checkPoint(transformPoint(reflectionMatrix(true, false) * rotationMatrix(kPi / 2.0), p),
               {0.0, 1.0}, "order: rotating before reflecting gives a different point");
}

void testSharedOrderStepByStep() {
    TransformParams params;
    params.translation = {-6.0, 14.0};
    params.angleRadians = 0.9;
    params.scale = 1.75;
    params.pivot = {3.0, -2.0};
    params.reflectY = true;
    const Mat3 composite = transformMatrix(params);
    for (const Point2D p : {Point2D{0.0, 0.0}, Point2D{5.0, 1.0}, Point2D{-20.0, 13.5}}) {
        // Apply each factor separately, rightmost first.
        Point2D q = transformPoint(translationMatrix(-params.pivot.x, -params.pivot.y), p);
        q = transformPoint(scalingMatrix(params.scale, params.scale), q);
        q = transformPoint(reflectionMatrix(params.reflectX, params.reflectY), q);
        q = transformPoint(rotationMatrix(params.angleRadians), q);
        q = transformPoint(translationMatrix(params.pivot), q);
        q = transformPoint(translationMatrix(params.translation), q);
        checkPoint(transformPoint(composite, p), q,
                   "shared order: composite equals applying each factor in turn");
    }
    check(nearMatrix(composite, translationMatrix(params.translation) * pivotTransformMatrix(params)),
          "shared order: transformMatrix = T(translation) * pivotTransformMatrix");
}

// Independent closed form of the shared order, used to cross-check the matrix
// product. With L = R * F * S (the 2x2 linear part) and pivot c:
//     p' = L * (p - c) + c + translation
// This is also the expansion of the pattern engine's private localTransform
// (src/pattern.cpp) followed by its translation of the tile origin.
Point2D closedForm(const TransformParams& t, Point2D p) {
    const double c = std::cos(t.angleRadians);
    const double s = std::sin(t.angleRadians);
    const double fx = t.reflectX ? -1.0 : 1.0;
    const double fy = t.reflectY ? -1.0 : 1.0;
    const double dx = t.scale * fx * (p.x - t.pivot.x);
    const double dy = t.scale * fy * (p.y - t.pivot.y);
    return {c * dx - s * dy + t.pivot.x + t.translation.x,
            s * dx + c * dy + t.pivot.y + t.translation.y};
}

void testClosedFormAgreement() {
    for (const double angle : {0.0, 0.4, -1.3, kPi, 5.5}) {
        for (const bool rx : {false, true}) {
            for (const bool ry : {false, true}) {
                TransformParams params;
                params.translation = {11.0, -4.5};
                params.angleRadians = angle;
                params.scale = 0.6;
                params.pivot = {-7.0, 2.5};
                params.reflectX = rx;
                params.reflectY = ry;
                const Mat3 matrix = transformMatrix(params);
                for (const Point2D p : {Point2D{0.0, 0.0}, Point2D{25.0, -25.0}, Point2D{-3.0, 18.0}}) {
                    checkPoint(transformPoint(matrix, p), closedForm(params, p),
                               "closed form: matrix product matches p' = L(p - c) + c + t");
                }
            }
        }
    }
}

// Hand-calculated example, written out step by step in
// docs/algorithms/transformations.md.
//   translation (10,5), angle 90 degrees, scale 2, pivot (1,1), reflectX.
//   p = (3,2)
//   T(-pivot):      (3-1, 2-1)            = (2, 1)
//   S(2):           (2*2, 2*1)            = (4, 2)
//   F(reflectX):    (-4, 2)
//   R(90):          (0*-4 - 1*2, 1*-4 + 0*2) = (-2, -4)
//   T(pivot):       (-2+1, -4+1)          = (-1, -3)
//   T(translation): (-1+10, -3+5)         = (9, 2)
// Composite: L = R*F*S = [0 -2; -2 0], t = translation + pivot - L*pivot
//          = (10,5) + (1,1) - (-2,-2) = (13, 8)
//   M = | 0 -2 13 |
//       |-2  0  8 |
//       | 0  0  1 |
void testWorkedExample() {
    TransformParams params;
    params.translation = {10.0, 5.0};
    params.angleRadians = kPi / 2.0;
    params.scale = 2.0;
    params.pivot = {1.0, 1.0};
    params.reflectX = true;

    const Mat3 expected = matrixFromRows({{0.0, -2.0, 13.0}}, {{-2.0, 0.0, 8.0}}, {{0.0, 0.0, 1.0}});
    const Mat3 m = transformMatrix(params);
    check(nearMatrix(m, expected), "worked example: composite matrix matches the hand calculation");
    checkPoint(transformPoint(m, {3.0, 2.0}), {9.0, 2.0}, "worked example: (3,2) maps to (9,2)");

    // Each intermediate step from the hand calculation.
    Point2D q = transformPoint(translationMatrix(-1.0, -1.0), {3.0, 2.0});
    checkPoint(q, {2.0, 1.0}, "worked example step 1: T(-pivot)");
    q = transformPoint(scalingMatrix(2.0, 2.0), q);
    checkPoint(q, {4.0, 2.0}, "worked example step 2: S(2)");
    q = transformPoint(reflectionMatrix(true, false), q);
    checkPoint(q, {-4.0, 2.0}, "worked example step 3: F(reflectX)");
    q = transformPoint(rotationMatrix(kPi / 2.0), q);
    checkPoint(q, {-2.0, -4.0}, "worked example step 4: R(90)");
    q = transformPoint(translationMatrix(1.0, 1.0), q);
    checkPoint(q, {-1.0, -3.0}, "worked example step 5: T(pivot)");
    q = transformPoint(translationMatrix(10.0, 5.0), q);
    checkPoint(q, {9.0, 2.0}, "worked example step 6: T(translation)");
    // The pivot itself only moves by the translation.
    checkPoint(transformPoint(m, {1.0, 1.0}), {11.0, 6.0}, "worked example: pivot moves to (11,6)");
}

void testSourceUnchanged() {
    const Polygon source{{-10.0, -5.0}, {10.0, -5.0}, {4.0, 8.0}, {-2.0, 3.0}};
    const std::vector<Point2D> path{{0.0, 0.0}, {5.0, 5.0}, {10.0, 0.0}};
    const Polygon sourceCopy = source;
    const std::vector<Point2D> pathCopy = path;

    TransformParams params;
    params.angleRadians = 0.5;
    params.scale = 2.0;
    params.reflectY = true;
    const Mat3 local = transformMatrix(params);

    const Polygon moved = transformPolygon(local, source);
    const std::vector<Point2D> movedPath = transformPath(local, path);
    check(samePoints(source, sourceCopy), "source: polygon vertices are unchanged");
    check(samePoints(path, pathCopy), "source: path points are unchanged");
    check(moved.size() == source.size() && movedPath.size() == path.size(),
          "source: outputs keep the vertex count");
    check(!samePoints(moved, source), "source: the returned polygon is the transformed copy");

    // The pattern engine transforms the same motif once per tile. Each tile's
    // result must depend only on its own offset, not on earlier tiles.
    for (int tile = 0; tile < 4; ++tile) {
        const Mat3 placed = translationMatrix(64.0 * tile, 0.0) * local;
        const Polygon copy = transformPolygon(placed, source);
        for (std::size_t i = 0; i < copy.size(); ++i) {
            checkPoint(copy[i], {moved[i].x + 64.0 * tile, moved[i].y},
                       "source: each tile is the local result plus its own offset");
        }
    }
    check(samePoints(source, sourceCopy), "source: unchanged after transforming every tile");

    // A failed transform must also leave the source alone.
    Polygon withBadVertex = source;
    withBadVertex.push_back({kNaN, 0.0});
    checkRejects([&] { (void)transformPolygon(local, withBadVertex); },
                 "source: a non-finite vertex is rejected");
    // NaN never compares equal, so check the finite prefix and the NaN separately.
    check(withBadVertex.size() == source.size() + 1 &&
              samePoints(Polygon(withBadVertex.begin(), withBadVertex.end() - 1), source) &&
              std::isnan(withBadVertex.back().x),
          "source: a rejected polygon is unchanged");
}

void testEmptyInput() {
    const Mat3 m = transformMatrix({{3.0, 4.0}, 1.0, 2.0, {0.0, 0.0}, true, false});
    check(transformPolygon(m, Polygon{}).empty(), "empty: an empty polygon gives an empty polygon");
    check(transformPath(m, std::vector<Point2D>{}).empty(), "empty: an empty path gives an empty path");
    check(transformPolygon(Mat3::identity(), Polygon{}).empty(), "empty: identity on empty input");
}

void testInvalidValues() {
    for (const double bad : {kNaN, kInf, -kInf}) {
        checkRejects([&] { (void)translationMatrix(bad, 0.0); }, "invalid: translation dx");
        checkRejects([&] { (void)translationMatrix(0.0, bad); }, "invalid: translation dy");
        checkRejects([&] { (void)rotationMatrix(bad); }, "invalid: rotation angle");
        checkRejects([&] { (void)scalingMatrix(bad, 1.0); }, "invalid: scale sx");
        checkRejects([&] { (void)scalingMatrix(1.0, bad); }, "invalid: scale sy");
        checkRejects([&] { (void)transformPoint(Mat3::identity(), {bad, 0.0}); }, "invalid: point x");
        checkRejects([&] { (void)transformPoint(Mat3::identity(), {0.0, bad}); }, "invalid: point y");

        Mat3 broken;
        broken.m[1][2] = bad;
        checkRejects([&] { (void)transformPoint(broken, {1.0, 1.0}); }, "invalid: matrix entry");
        checkRejects([&] { (void)transformPolygon(broken, {{0, 0}, {1, 0}, {0, 1}}); },
                     "invalid: polygon with a non-finite matrix");
        checkRejects([&] { (void)transformPath(broken, {}); },
                     "invalid: a non-finite matrix is rejected even for an empty path");
        check(!isFinite(broken), "invalid: isFinite detects a non-finite entry");

        const std::vector<std::function<void(TransformParams&)>> corruptions{
            [&](TransformParams& t) { t.translation.x = bad; },
            [&](TransformParams& t) { t.translation.y = bad; },
            [&](TransformParams& t) { t.angleRadians = bad; },
            [&](TransformParams& t) { t.scale = bad; },
            [&](TransformParams& t) { t.pivot.x = bad; },
            [&](TransformParams& t) { t.pivot.y = bad; },
        };
        for (const auto& corrupt : corruptions) {
            TransformParams params;
            corrupt(params);
            checkRejects([&] { validateTransformParams(params); }, "invalid: params field validated");
            checkRejects([&] { (void)transformMatrix(params); }, "invalid: transformMatrix rejects");
            checkRejects([&] { (void)pivotTransformMatrix(params); },
                         "invalid: pivotTransformMatrix rejects");
        }
    }

    // Zero and negative scale are rejected; mirroring uses the reflection flags.
    for (const double bad : {0.0, -0.0, -1.0, -2.5}) {
        checkRejects([&] { (void)scalingMatrix(bad, 1.0); }, "invalid: non-positive sx");
        checkRejects([&] { (void)scalingMatrix(1.0, bad); }, "invalid: non-positive sy");
        TransformParams params;
        params.scale = bad;
        checkRejects([&] { (void)transformMatrix(params); }, "invalid: non-positive params scale");
    }

    // Finite inputs whose result overflows are rejected rather than returning inf.
    checkRejects([] { (void)transformPoint(scalingMatrix(1e300, 1e300), {1e300, 1.0}); },
                 "invalid: overflowing point result");
    // Sandwich translation column = pivot - scale * pivot = 1e300 - 1e600: overflow.
    TransformParams huge;
    huge.scale = 1e300;
    huge.pivot = {1e300, 0.0};
    checkRejects([&] { (void)pivotTransformMatrix(huge); }, "invalid: overflowing pivot sandwich");
    checkRejects([&] { (void)transformMatrix(huge); }, "invalid: overflowing composite matrix");
    // Overflow in the final translation step alone is also caught.
    TransformParams farAway;
    farAway.scale = 0.5;
    farAway.pivot = {1.5e308, 0.0};       // sandwich column: c - 0.5c = 0.75e308
    farAway.translation = {1.5e308, 0.0}; // + 1.5e308 = 2.25e308 overflows
    bool sandwichFinite = true;
    try {
        (void)pivotTransformMatrix(farAway);
    } catch (const std::exception&) {
        sandwichFinite = false;
    }
    check(sandwichFinite, "valid: a large but finite pivot sandwich is accepted");
    checkRejects([&] { (void)transformMatrix(farAway); }, "invalid: overflowing final translation");

    // A homogeneous w of zero has no Cartesian point.
    Mat3 projective;
    projective.m[2] = {{1.0, 0.0, 0.0}};
    checkRejects([&] { (void)transformPoint(projective, {0.0, 3.0}); }, "invalid: w = 0");

    // Valid extremes are still accepted.
    bool accepted = true;
    try {
        (void)scalingMatrix(1e-6, 1e6);
        (void)rotationMatrix(-1000.0);
        TransformParams tiny;
        tiny.scale = std::numeric_limits<double>::min();
        (void)transformMatrix(tiny);
    } catch (const std::exception&) {
        accepted = false;
    }
    check(accepted, "valid: tiny positive scales and large finite angles are accepted");
}

// TransformParams must be a drop-in for the pattern engine's PatternTransform:
// the same field names, types, order and defaults.
void testDropInContract() {
    static_assert(std::is_same_v<decltype(TransformParams::translation), Point2D>);
    static_assert(std::is_same_v<decltype(TransformParams::angleRadians), double>);
    static_assert(std::is_same_v<decltype(TransformParams::scale), double>);
    static_assert(std::is_same_v<decltype(TransformParams::pivot), Point2D>);
    static_assert(std::is_same_v<decltype(TransformParams::reflectX), bool>);
    static_assert(std::is_same_v<decltype(TransformParams::reflectY), bool>);
    static_assert(std::is_aggregate_v<TransformParams>);

    const TransformParams defaults;
    check(defaults.translation.x == 0.0 && defaults.translation.y == 0.0, "contract: translation 0");
    check(defaults.angleRadians == 0.0, "contract: angle 0");
    check(defaults.scale == 1.0, "contract: scale 1");
    check(defaults.pivot.x == 0.0 && defaults.pivot.y == 0.0, "contract: pivot 0");
    check(!defaults.reflectX && !defaults.reflectY, "contract: reflections off");

    // Aggregate initialisation in declaration order, as PatternTransform allows.
    const TransformParams ordered{{1.0, 2.0}, 0.5, 3.0, {4.0, 5.0}, true, false};
    check(ordered.translation.y == 2.0 && ordered.angleRadians == 0.5 && ordered.scale == 3.0 &&
              ordered.pivot.x == 4.0 && ordered.reflectX && !ordered.reflectY,
          "contract: aggregate field order matches PatternTransform");
}

} // namespace

int main() {
    testIdentity();
    testMultiplication();
    testTranslation();
    testRotation();
    testNonUniformScaling();
    testReflections();
    testPivotInvariance();
    testInverseRotation();
    testOrderMatters();
    testSharedOrderStepByStep();
    testClosedFormAgreement();
    testWorkedExample();
    testSourceUnchanged();
    testEmptyInput();
    testInvalidValues();
    testDropInContract();
    if (failures == 0) std::cout << "transforms: all checks passed\n";
    return failures == 0 ? 0 : 1;
}
