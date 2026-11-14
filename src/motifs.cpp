#include "kitenge/motifs.hpp"

#include "kitenge/curves.hpp"
#include "kitenge/polygon_fill.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <string>

namespace kitenge {

namespace {

constexpr double kPi = 3.14159265358979323846;

// Points on a circle of the given radius about the origin, starting at angle
// zero. A circle cannot survive rotation and non-uniform scaling as a
// midpoint-rasterized primitive, so the round motif stores it as a polygon.
std::vector<Point2D> sampledCircle(double radius, int samples) {
    std::vector<Point2D> points;
    points.reserve(static_cast<std::size_t>(samples));
    for (int i = 0; i < samples; ++i) {
        const double a = 2.0 * kPi * i / samples;
        points.push_back({radius * std::cos(a), radius * std::sin(a)});
    }
    return points;
}

// Quarter turn about the origin, exact for integer inputs.
Point2D quarterTurn(Point2D p, int turns) {
    for (int i = 0; i < turns; ++i) p = {-p.y, p.x};
    return p;
}

// One petal pointing up (towards negative y): two mirrored cubic Bezier
// curves from the base to the tip, sampled with Andrew's CP1 evaluator. The
// straight base edge closes the polygon and is later hidden under the disc.
Polygon petal(int quarterTurns) {
    constexpr int segments = 8;
    const std::array<Point2D, 4> left{{{-4, -9}, {-11, -17}, {-5, -24}, {0, -24}}};
    const std::array<Point2D, 4> right{{{0, -24}, {5, -24}, {11, -17}, {4, -9}}};
    Polygon outline = sampleCubicBezier(left, segments);
    const std::vector<Point2D> second = sampleCubicBezier(right, segments);
    outline.insert(outline.end(), second.begin() + 1, second.end()); // tip shared once
    for (Point2D& p : outline) p = quarterTurn(p, quarterTurns);
    return outline;
}

MotifShape diamond() {
    return {{{{{0, -25}, {22, 0}, {0, 25}, {-22, 0}}, kRolePrimary},
             {{{0, -11}, {10, 0}, {0, 11}, {-10, 0}}, kRoleAccent}},
            {}};
}

MotifShape triangle() {
    // An inverted accent triangle inside the primary one.
    return {{{{{0, -22}, {21, 19}, {-21, 19}}, kRolePrimary},
             {{{-9, 1}, {9, 1}, {0, 15}}, kRoleAccent}},
            {}};
}

MotifShape cross() {
    // Concave 12-vertex cross with an accent centre square.
    return {{{{{-22, -10}, {-8, -10}, {-8, -23}, {8, -23}, {8, -10}, {22, -10},
               {22, 10}, {8, 10}, {8, 23}, {-8, 23}, {-8, 10}, {-22, 10}},
              kRolePrimary},
             {{{-5, -5}, {5, -5}, {5, 5}, {-5, 5}}, kRoleAccent}},
            {}};
}

MotifShape steps() {
    // A staircase, deliberately asymmetric in x and in y, so x and y
    // reflections each give a visibly different motif.
    return {{{{{-22, 22}, {-22, 12}, {-11, 12}, {-11, 1}, {0, 1}, {0, -10},
               {11, -10}, {11, -21}, {22, -21}, {22, 22}},
              kRolePrimary},
             {{{8, 8}, {18, 8}, {18, 18}, {8, 18}}, kRoleAccent}},
            {}};
}

MotifShape sunburst() {
    // Round and curved variant: four Bezier petals, a sampled disc on top of
    // their bases and a sampled ring stroke inside the disc.
    MotifShape m;
    for (int k = 0; k < 4; ++k) m.fills.push_back({petal(k), kRoleAccent});
    m.fills.push_back({sampledCircle(12.0, 32), kRolePrimary});
    m.strokes.push_back({sampledCircle(6.0, 24), true, kRoleAccent});
    return m;
}

struct CatalogEntry {
    const char* label;
    MotifShape shape;
};

const std::array<CatalogEntry, kMotifCount>& motifCatalog() {
    static const std::array<CatalogEntry, kMotifCount> catalog{{
        {"DIAMOND", diamond()},
        {"TRIANGLE", triangle()},
        {"CROSS", cross()},
        {"STEPS", steps()},
        {"SUNBURST", sunburst()},
    }};
    return catalog;
}

// The first three palettes keep the starter scene's colours.
constexpr std::array<Palette, kPaletteCount> paletteCatalog{{
    {"EARTH", {242, 221, 175}, {174, 65, 32}, {48, 81, 57}},
    {"LAGOON", {228, 231, 216}, {40, 96, 110}, {190, 113, 43}},
    {"PLUM", {234, 214, 207}, {110, 56, 86}, {55, 76, 77}},
    {"INDIGO", {236, 229, 212}, {38, 52, 105}, {214, 160, 36}},
}};

std::size_t checkedIndex(int index, int count, const char* what) {
    if (index < 0 || index >= count) {
        throw std::out_of_range(std::string("Unknown ") + what + " index " + std::to_string(index));
    }
    return static_cast<std::size_t>(index);
}

bool inExtent(Point2D p) {
    return std::isfinite(p.x) && std::isfinite(p.y) && std::abs(p.x) <= kMotifHalfExtent &&
           std::abs(p.y) <= kMotifHalfExtent;
}

bool validRole(int role) { return role >= 0 && role < kPaletteRoleCount; }

} // namespace

bool isValidMotif(const MotifShape& motif) {
    if (motif.fills.empty() && motif.strokes.empty()) return false;
    for (const MotifFill& fill : motif.fills) {
        if (!validRole(fill.role) || !isValidPolygon(fill.contour)) return false;
        for (const Point2D& p : fill.contour)
            if (!inExtent(p)) return false;
    }
    for (const MotifStroke& stroke : motif.strokes) {
        const std::size_t needed = stroke.closed ? 3 : 2;
        if (!validRole(stroke.role) || stroke.points.size() < needed) return false;
        for (const Point2D& p : stroke.points)
            if (!inExtent(p)) return false;
    }
    return true;
}

const MotifShape& catalogMotif(int index) {
    return motifCatalog()[checkedIndex(index, kMotifCount, "motif")].shape;
}

const char* catalogMotifLabel(int index) {
    return motifCatalog()[checkedIndex(index, kMotifCount, "motif")].label;
}

const Palette& catalogPalette(int index) {
    return paletteCatalog[checkedIndex(index, kPaletteCount, "palette")];
}

const char* catalogPaletteLabel(int index) { return catalogPalette(index).label; }

Color paletteRoleColor(const Palette& palette, int role) {
    switch (role) {
    case kRoleBackground: return palette.background;
    case kRolePrimary: return palette.primary;
    case kRoleAccent: return palette.accent;
    default: throw std::out_of_range("Unknown palette role " + std::to_string(role));
    }
}

std::vector<Color> paletteRoleColors(int paletteIndex) {
    const Palette& p = catalogPalette(paletteIndex);
    return {p.background, p.primary, p.accent};
}

} // namespace kitenge
