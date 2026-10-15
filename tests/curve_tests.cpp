#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

#include "kitenge/curves.hpp"

using kitenge::Point2D;

namespace {

int failures = 0;

void check(bool condition, const char* name) {
    if (condition) {
        std::cout << "pass: " << name << '\n';
    } else {
        std::cerr << "FAIL: " << name << '\n';
        ++failures;
    }
}

bool near(const Point2D& p, double x, double y, double eps = 1e-9) {
    return std::fabs(p.x - x) <= eps && std::fabs(p.y - y) <= eps;
}

bool same(const Point2D& a, const Point2D& b) { return a.x == b.x && a.y == b.y; }

template <typename Exception, typename Function>
bool throws(Function function) {
    try {
        function();
    } catch (const Exception&) {
        return true;
    } catch (...) {
        return false;
    }
    return false;
}

const std::array<Point2D, 4> kSCurve{
    Point2D{0.0, 0.0}, Point2D{0.0, 100.0}, Point2D{100.0, 100.0}, Point2D{100.0, 0.0}};

void testEndpointsAndKnownPoints() {
    check(same(kitenge::evaluateCubicBezier(kSCurve, 0.0), kSCurve[0]), "t = 0 returns P0 exactly");
    check(same(kitenge::evaluateCubicBezier(kSCurve, 1.0), kSCurve[3]), "t = 1 returns P3 exactly");
    // Bernstein form by hand: t = 0.5 -> (50, 75); t = 0.25 -> (15.625, 56.25).
    check(near(kitenge::evaluateCubicBezier(kSCurve, 0.5), 50.0, 75.0), "t = 0.5 matches hand calculation");
    check(near(kitenge::evaluateCubicBezier(kSCurve, 0.25), 15.625, 56.25), "t = 0.25 matches hand calculation");
}

void testRepeatedControlPoints() {
    const std::array<Point2D, 4> allSame{
        Point2D{7.0, 9.0}, Point2D{7.0, 9.0}, Point2D{7.0, 9.0}, Point2D{7.0, 9.0}};
    bool stays = true;
    for (double t : {0.0, 0.2, 0.5, 0.9, 1.0}) {
        stays = stays && near(kitenge::evaluateCubicBezier(allSame, t), 7.0, 9.0);
    }
    check(stays, "four identical control points give a single point");

    // Handles sitting on the endpoints: the curve is the straight segment y = x.
    const std::array<Point2D, 4> doubled{
        Point2D{0.0, 0.0}, Point2D{0.0, 0.0}, Point2D{30.0, 30.0}, Point2D{30.0, 30.0}};
    const Point2D p = kitenge::evaluateCubicBezier(doubled, 0.3);
    check(std::fabs(p.x - p.y) < 1e-9 && p.x > 0.0 && p.x < 30.0,
          "repeated endpoint handles stay on the straight segment");
}

void testStraightLineConfiguration() {
    // Evenly spaced collinear control points: B(t) = P0 + t (P3 - P0).
    const std::array<Point2D, 4> line{
        Point2D{0.0, 0.0}, Point2D{10.0, 20.0}, Point2D{20.0, 40.0}, Point2D{30.0, 60.0}};
    check(near(kitenge::evaluateCubicBezier(line, 0.3), 9.0, 18.0), "evenly spaced collinear points give B(t) = P0 + t(P3 - P0)");
}

void testInvalidT() {
    const double inf = std::numeric_limits<double>::infinity();
    const double nan = std::numeric_limits<double>::quiet_NaN();
    check(throws<std::out_of_range>([] { kitenge::evaluateCubicBezier(kSCurve, -0.01); }), "t < 0 throws out_of_range");
    check(throws<std::out_of_range>([] { kitenge::evaluateCubicBezier(kSCurve, 1.01); }), "t > 1 throws out_of_range");
    check(throws<std::out_of_range>([inf] { kitenge::evaluateCubicBezier(kSCurve, inf); }), "infinite t throws out_of_range");
    check(throws<std::invalid_argument>([nan] { kitenge::evaluateCubicBezier(kSCurve, nan); }), "NaN t throws invalid_argument");

    std::array<Point2D, 4> bad = kSCurve;
    bad[2].x = nan;
    check(throws<std::invalid_argument>([bad] { kitenge::evaluateCubicBezier(bad, 0.5); }), "non-finite control point throws invalid_argument");
}

void testSampling() {
    const std::vector<Point2D> samples = kitenge::sampleCubicBezier(kSCurve, 4);
    check(samples.size() == 5, "4 segments give 5 samples");
    check(same(samples.front(), kSCurve[0]) && same(samples.back(), kSCurve[3]), "samples start at P0 and end at P3 exactly");
    check(near(samples[2], 50.0, 75.0), "middle sample is the t = 0.5 point");

    check(throws<std::invalid_argument>([] { kitenge::sampleCubicBezier(kSCurve, 0); }), "0 segments throws");
    check(throws<std::invalid_argument>([] { kitenge::sampleCubicBezier(kSCurve, -3); }), "negative segments throws");
    check(throws<std::invalid_argument>([] { kitenge::sampleCubicBezier(kSCurve, kitenge::kMaxBezierSegments + 1); }), "too many segments throws");
    check(kitenge::sampleCubicBezier(kSCurve, 1).size() == 2, "1 segment gives just the two endpoints");
}

}  // namespace

int main() {
    testEndpointsAndKnownPoints();
    testRepeatedControlPoints();
    testStraightLineConfiguration();
    testInvalidT();
    testSampling();

    if (failures != 0) {
        std::cerr << failures << " curve check(s) failed\n";
        return 1;
    }
    std::cout << "All curve checks passed\n";
    return 0;
}
