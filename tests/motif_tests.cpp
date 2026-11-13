// Checks for the motif and palette catalog (Deborah Rehana, 168656).

#include "kitenge/motifs.hpp"
#include "kitenge/pattern.hpp"
#include "kitenge/polygon_fill.hpp"
#include "kitenge/studio.hpp"
#include "kitenge/transforms.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <iostream>
#include <limits>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace kitenge;

namespace {

int failures = 0;

void check(bool ok, const std::string& name) {
    if (!ok) {
        ++failures;
        std::cerr << "FAIL: " << name << "\n";
    }
}

bool throwsOutOfRange(const std::function<void()>& f) {
    try {
        f();
    } catch (const std::out_of_range&) {
        return true;
    } catch (...) {
        return false;
    }
    return false;
}

bool same(Color a, Color b) { return a.red == b.red && a.green == b.green && a.blue == b.blue; }

double signedArea(const Polygon& p) {
    double twice = 0.0;
    for (std::size_t i = 0; i < p.size(); ++i) {
        const Point2D a = p[i];
        const Point2D b = p[(i + 1) % p.size()];
        twice += a.x * b.y - b.x * a.y;
    }
    return twice / 2.0;
}

std::set<std::pair<double, double>> vertexSet(const Polygon& p) {
    std::set<std::pair<double, double>> s;
    for (const Point2D& v : p) s.insert({v.x, v.y});
    return s;
}

bool printableLabel(const std::string& label) {
    if (label.empty()) return false;
    for (char c : label)
        if (!((c >= 'A' && c <= 'Z') || c == ' ')) return false;
    return true;
}

// --- Catalog sizes and labels ------------------------------------------------

void testCountsAndLabels() {
    check(studioMotifCount == kMotifCount, "studio motif count comes from the catalog");
    check(studioPaletteCount == kPaletteCount, "studio palette count comes from the catalog");
    check(kMotifCount >= 3, "at least three motifs");
    check(kPaletteCount >= 3, "at least three palettes");

    std::set<std::string> motifLabels;
    for (int i = 0; i < kMotifCount; ++i) {
        const std::string label = catalogMotifLabel(i);
        check(printableLabel(label), "motif label is drawable by the studio font: " + label);
        motifLabels.insert(label);
    }
    check(static_cast<int>(motifLabels.size()) == kMotifCount, "motif labels are unique");

    std::set<std::string> paletteLabels;
    for (int i = 0; i < kPaletteCount; ++i) {
        const std::string label = catalogPaletteLabel(i);
        check(printableLabel(label), "palette label is drawable by the studio font: " + label);
        check(label == catalogPalette(i).label, "palette label lookup matches the palette");
        paletteLabels.insert(label);
    }
    check(static_cast<int>(paletteLabels.size()) == kPaletteCount, "palette labels are unique");
}

// --- Motif geometry ------------------------------------------------------------

void testMotifGeometry() {
    for (int i = 0; i < kMotifCount; ++i) {
        const std::string name = catalogMotifLabel(i);
        const MotifShape& m = catalogMotif(i);
        check(isValidMotif(m), name + ": meets the motif contract");
        check(!m.fills.empty(), name + ": has at least one filled contour");

        bool usesPrimary = false, usesAccent = false;
        double minX = 1e9, minY = 1e9, maxX = -1e9, maxY = -1e9;
        const auto extend = [&](Point2D p) {
            minX = std::min(minX, p.x);
            minY = std::min(minY, p.y);
            maxX = std::max(maxX, p.x);
            maxY = std::max(maxY, p.y);
        };
        for (const MotifFill& f : m.fills) {
            check(isValidPolygon(f.contour), name + ": contour is simple, finite and non-degenerate");
            check(std::abs(signedArea(f.contour)) > 1.0, name + ": contour has non-zero area");
            check(f.role >= 0 && f.role < kPaletteRoleCount, name + ": fill role in range");
            usesPrimary |= f.role == kRolePrimary;
            usesAccent |= f.role == kRoleAccent;
            for (const Point2D& p : f.contour) {
                check(std::isfinite(p.x) && std::isfinite(p.y), name + ": finite vertex");
                check(std::abs(p.x) <= kMotifHalfExtent && std::abs(p.y) <= kMotifHalfExtent,
                      name + ": vertex inside the local extent");
                extend(p);
            }
        }
        for (const MotifStroke& s : m.strokes) {
            check(s.points.size() >= (s.closed ? 3u : 2u), name + ": stroke has enough points");
            check(s.role >= 0 && s.role < kPaletteRoleCount, name + ": stroke role in range");
            usesAccent |= s.role == kRoleAccent;
            for (const Point2D& p : s.points) {
                check(std::abs(p.x) <= kMotifHalfExtent && std::abs(p.y) <= kMotifHalfExtent,
                      name + ": stroke point inside the local extent");
                extend(p);
            }
        }
        check(usesPrimary && usesAccent, name + ": uses both primary and accent roles");
        // Centred at the origin: the bounding box centre is close to (0,0).
        check(std::abs((minX + maxX) / 2.0) <= 3.0 && std::abs((minY + maxY) / 2.0) <= 3.0,
              name + ": centred on the local origin");
    }
}

// Rasterizes a motif's fills (ignoring colour) at the centre of a small canvas.
std::vector<bool> coverage(const MotifShape& m) {
    constexpr int size = 60;
    Canvas c(size, size, {0, 0, 0});
    for (const MotifFill& f : m.fills) {
        Polygon placed;
        for (const Point2D& p : f.contour) placed.push_back({p.x + size / 2.0, p.y + size / 2.0});
        fillPolygon(c, placed, {255, 255, 255});
    }
    std::vector<bool> mask;
    for (int y = 0; y < size; ++y)
        for (int x = 0; x < size; ++x) mask.push_back(c.pixelAt(x, y).red == 255);
    return mask;
}

void testMotifsAreDistinct() {
    std::vector<std::vector<bool>> masks;
    for (int i = 0; i < kMotifCount; ++i) masks.push_back(coverage(catalogMotif(i)));
    for (int a = 0; a < kMotifCount; ++a)
        for (int b = a + 1; b < kMotifCount; ++b) {
            int different = 0;
            for (std::size_t k = 0; k < masks[a].size(); ++k) different += masks[a][k] != masks[b][k];
            check(different > 200, std::string("motifs are visually distinct: ") +
                                       catalogMotifLabel(a) + " vs " + catalogMotifLabel(b));
        }
}

// --- Palettes --------------------------------------------------------------------

void testPalettes() {
    for (int i = 0; i < kPaletteCount; ++i) {
        const Palette& p = catalogPalette(i);
        const std::string name = p.label;
        check(!same(p.background, p.primary) && !same(p.background, p.accent) &&
                  !same(p.primary, p.accent),
              name + ": the three roles are distinguishable");
        const std::vector<Color> roles = paletteRoleColors(i);
        check(static_cast<int>(roles.size()) == kPaletteRoleCount, name + ": one colour per role");
        check(same(roles[kRoleBackground], p.background) && same(roles[kRolePrimary], p.primary) &&
                  same(roles[kRoleAccent], p.accent),
              name + ": role-indexed list follows the role numbers");
        for (int r = 0; r < kPaletteRoleCount; ++r)
            check(same(paletteRoleColor(p, r), roles[static_cast<std::size_t>(r)]),
                  name + ": role lookup agrees with the list");
        const StudioPalette sp = studioPalette(i);
        check(same(sp.background, p.background) && same(sp.primary, p.primary) &&
                  same(sp.secondary, p.accent),
              name + ": studio palette comes from the catalog");
        const std::vector<Color> engine = patternPaletteFor(i);
        check(engine.size() == roles.size() && same(engine[1], roles[1]) && same(engine[2], roles[2]),
              name + ": pattern engine receives the catalog roles");
    }
}

// --- Invalid lookups ---------------------------------------------------------------

void testInvalidLookups() {
    for (int bad : {-1, kMotifCount, std::numeric_limits<int>::max()}) {
        check(throwsOutOfRange([&] { catalogMotif(bad); }), "invalid motif index throws");
        check(throwsOutOfRange([&] { catalogMotifLabel(bad); }), "invalid motif label throws");
    }
    for (int bad : {-1, kPaletteCount, std::numeric_limits<int>::min()}) {
        check(throwsOutOfRange([&] { catalogPalette(bad); }), "invalid palette index throws");
        check(throwsOutOfRange([&] { catalogPaletteLabel(bad); }), "invalid palette label throws");
        check(throwsOutOfRange([&] { paletteRoleColors(bad); }), "invalid palette colours throw");
        check(throwsOutOfRange([&] { studioPalette(bad); }), "invalid studio palette throws");
    }
    for (int badRole : {-1, kPaletteRoleCount})
        check(throwsOutOfRange([&] { paletteRoleColor(catalogPalette(0), badRole); }),
              "invalid palette role throws");
}

// --- The validator itself ------------------------------------------------------------

void testValidator() {
    const Polygon triangle{{0, -10}, {10, 10}, {-10, 10}};
    check(isValidMotif({{{triangle, kRolePrimary}}, {}}), "a plain triangle is valid");
    check(isValidMotif({{}, {{{{-25, 0}, {25, 0}}, false, kRoleAccent}}}),
          "a stroke reaching the extent exactly is valid");

    check(!isValidMotif({}), "an empty motif is invalid");
    check(!isValidMotif({{{{{-10, -10}, {10, 10}, {10, -10}, {-10, 10}}, kRolePrimary}}, {}}),
          "a self-intersecting (bow-tie) contour is invalid");
    check(!isValidMotif({{{{{0, 0}, {5, 5}, {10, 10}}, kRolePrimary}}, {}}),
          "a zero-area contour is invalid");
    const double nan = std::numeric_limits<double>::quiet_NaN();
    check(!isValidMotif({{{{{0, nan}, {10, 10}, {-10, 10}}, kRolePrimary}}, {}}),
          "a non-finite vertex is invalid");
    check(!isValidMotif({{{{{0, -26}, {10, 10}, {-10, 10}}, kRolePrimary}}, {}}),
          "a vertex outside the local extent is invalid");
    check(!isValidMotif({{{triangle, kPaletteRoleCount}}, {}}), "an out-of-range role is invalid");
    check(!isValidMotif({{{triangle, -1}}, {}}), "a negative role is invalid");
    check(!isValidMotif({{}, {{{{0, 0}}, false, kRoleAccent}}}), "a one-point stroke is invalid");
    check(!isValidMotif({{}, {{{{0, 0}, {5, 5}}, true, kRoleAccent}}}),
          "a closed stroke needs three points");
}

// --- Reflections with Jeremy's transform module ------------------------------------------

void testReflections() {
    // STEPS is asymmetric in both directions, so each reflection must change it.
    int stepsIndex = -1;
    for (int i = 0; i < kMotifCount; ++i)
        if (std::string(catalogMotifLabel(i)) == "STEPS") stepsIndex = i;
    check(stepsIndex >= 0, "the asymmetric STEPS motif is in the catalog");
    if (stepsIndex < 0) return;

    const Polygon original = catalogMotif(stepsIndex).fills[0].contour;
    const Polygon mirrorX = transformPolygon(reflectionMatrix(true, false), original);
    const Polygon mirrorY = transformPolygon(reflectionMatrix(false, true), original);

    check(vertexSet(mirrorX) != vertexSet(original), "reflectX visibly changes STEPS");
    check(vertexSet(mirrorY) != vertexSet(original), "reflectY visibly changes STEPS");
    check(vertexSet(mirrorX) != vertexSet(mirrorY), "the two reflections differ from each other");
    check(original[0].x == -22 && mirrorX[0].x == 22 && mirrorX[0].y == original[0].y,
          "reflectX negates x and keeps y");
    check(mirrorY[0].x == original[0].x && mirrorY[0].y == -original[0].y,
          "reflectY negates y and keeps x");
    check(std::abs(signedArea(mirrorX) + signedArea(original)) < 1e-9,
          "a reflection keeps the area and reverses the winding");
    const Polygon back = transformPolygon(reflectionMatrix(true, false), mirrorX);
    check(vertexSet(back) == vertexSet(original), "reflecting twice restores the motif");
    check(vertexSet(catalogMotif(stepsIndex).fills[0].contour) == vertexSet(original),
          "transforming never modifies the catalog motif");

    // A symmetric outline is unchanged by reflectX, which is why STEPS exists.
    const Polygon diamond = catalogMotif(0).fills[0].contour;
    check(vertexSet(transformPolygon(reflectionMatrix(true, false), diamond)) == vertexSet(diamond),
          "the symmetric diamond outline is its own x-mirror");

    // Every reflected motif still meets the contract (the extent is symmetric).
    for (int i = 0; i < kMotifCount; ++i)
        for (const auto& flags : {std::pair<bool, bool>{true, false}, {false, true}, {true, true}}) {
            const Mat3 f = reflectionMatrix(flags.first, flags.second);
            MotifShape m = catalogMotif(i);
            for (MotifFill& fill : m.fills) fill.contour = transformPolygon(f, fill.contour);
            for (MotifStroke& s : m.strokes) s.points = transformPath(f, s.points);
            check(isValidMotif(m), std::string(catalogMotifLabel(i)) + ": reflected copy stays valid");
        }
}

// --- Fabric and garment use the roles consistently -------------------------------------------

bool contains(const Canvas& c, Color colour, int x0, int y0, int x1, int y1) {
    for (int y = y0; y < y1; ++y)
        for (int x = x0; x < x1; ++x)
            if (same(c.pixelAt(x, y), colour)) return true;
    return false;
}

void testRolesOnFabricAndGarment() {
    const StudioHooks hooks = makePatternHooks();
    Canvas canvas(studioWidth, studioHeight, {0, 0, 0});
    for (int m = 0; m < kMotifCount; ++m)
        for (int p = 0; p < kPaletteCount; ++p) {
            StudioState s;
            s.motif = m;
            s.palette = p;
            renderStudio(canvas, s, hooks);
            const Palette& pal = catalogPalette(p);
            const std::string name =
                std::string(catalogMotifLabel(m)) + " in " + catalogPaletteLabel(p);
            for (int r = 0; r < kPaletteRoleCount; ++r) {
                const Color colour = paletteRoleColor(pal, r);
                check(contains(canvas, colour, 60, 210, 690, 640),
                      name + ": role " + std::to_string(r) + " appears on the fabric");
                check(contains(canvas, colour, 905, 285, 995, 370) ||
                          contains(canvas, colour, 880, 390, 1030, 570),
                      name + ": role " + std::to_string(r) + " appears on the dress");
            }
        }
}

} // namespace

int main() {
    try {
        testCountsAndLabels();
        testMotifGeometry();
        testMotifsAreDistinct();
        testPalettes();
        testInvalidLookups();
        testValidator();
        testReflections();
        testRolesOnFabricAndGarment();
    } catch (const std::exception& e) {
        std::cerr << "Unexpected exception: " << e.what() << "\n";
        return 1;
    }
    if (failures == 0) {
        std::cout << "Motif catalog, palettes, reflections and role mapping passed.\n";
        return 0;
    }
    std::cerr << failures << " motif check(s) failed\n";
    return 1;
}
