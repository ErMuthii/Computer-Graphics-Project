// Evidence generator for the pattern engine (Murega Kelvin Mutwiri, 166914).
//
//   pattern_demo [output-directory]
//
// Writes, to outputs/ by default:
//   pattern-clipping.ppm   the same tiles before (left) and after (right) clipping
//                          to a slanted convex panel, with a concave motif and a
//                          stroked Bezier arch
//   pattern-swatch.ppm     an untransformed swatch (left) and a rotated, scaled,
//                          reflected and translated swatch (right)
//   pattern-mannequin.ppm  the studio scene: fabric preview and dress from the
//                          same renderer and settings
// and prints render timings for the densest settings the engine accepts.

#include "kitenge/curves.hpp"
#include "kitenge/lines.hpp"
#include "kitenge/pattern.hpp"
#include "kitenge/polygon_fill.hpp"
#include "kitenge/ppm.hpp"
#include "kitenge/studio.hpp"

#include <chrono>
#include <cmath>
#include <cstddef>
#include <exception>
#include <filesystem>
#include <iostream>
#include <vector>

namespace {

using namespace kitenge;

constexpr double kPi = 3.14159265358979323846;
constexpr Color kPaper{246, 240, 227};
constexpr Color kOutline{65, 42, 35};
const std::vector<Color> kPalette{{242, 221, 175}, {174, 65, 32}, {48, 81, 57}};

Point toPixel(Point2D p) {
    return {static_cast<int>(std::lround(p.x)), static_cast<int>(std::lround(p.y))};
}

void outline(Canvas& canvas, const Polygon& polygon, Color colour) {
    for (std::size_t i = 0; i < polygon.size(); ++i)
        drawLine(canvas, toPixel(polygon[i]), toPixel(polygon[(i + 1) % polygon.size()]), colour);
}

Polygon shifted(const Polygon& polygon, double dx) {
    Polygon out;
    for (const Point2D& p : polygon) out.push_back({p.x + dx, p.y});
    return out;
}

MotifShape decoratedMotif() {
    MotifShape motif = starterMotifShape(2); // concave cross
    motif.strokes.push_back(
        {sampleCubicBezier({Point2D{-24, 18}, Point2D{-12, -30}, Point2D{12, -30}, Point2D{24, 18}}, 24),
         false, 2});
    return motif;
}

// Draws every tile in the range whole, without clipping: the "before" view.
void drawUnclipped(Canvas& canvas, const MotifShape& motif, const PatternSettings& s,
                   const Polygon& destination) {
    fillPolygon(canvas, destination, kPalette[0]);
    const TileRange range = patternTileRange(motif, s, destination);
    for (long long row = range.firstRow; row <= range.lastRow; ++row)
        for (long long column = range.firstColumn; column <= range.lastColumn; ++column) {
            const bool odd = s.alternateTiles && (column + row) % 2 != 0;
            for (const MotifFill& fill : motif.fills) {
                Polygon placed;
                for (const Point2D& p : fill.contour) placed.push_back(patternPoint(s, column, row, p));
                const int role = odd && fill.role == 1 ? 2 : odd && fill.role == 2 ? 1 : fill.role;
                fillPolygon(canvas, placed, kPalette[static_cast<std::size_t>(role)]);
            }
            for (const MotifStroke& stroke : motif.strokes)
                for (std::size_t i = 0; i + 1 < stroke.points.size(); ++i)
                    drawLine(canvas, toPixel(patternPoint(s, column, row, stroke.points[i])),
                             toPixel(patternPoint(s, column, row, stroke.points[i + 1])), {20, 20, 20});
        }
}

void clippingEvidence(const std::filesystem::path& path) {
    Canvas canvas(900, 360, kPaper);
    const Polygon panel{{150, 40}, {300, 40}, {380, 320}, {70, 320}};
    PatternSettings s;
    s.tilePitch = {66, 72};
    s.origin = {225, 180};
    s.transform.angleRadians = 0.25;
    s.transform.scale = 1.2;
    s.alternateTiles = true;
    const MotifShape motif = decoratedMotif();
    std::vector<Color> palette = kPalette;
    palette.push_back({20, 20, 20});
    MotifShape coloured = motif;
    coloured.strokes[0].role = 3;

    drawUnclipped(canvas, motif, s, panel); // left: before clipping
    outline(canvas, panel, kOutline);

    PatternSettings right = s;
    right.origin.x += 450;
    const Polygon rightPanel = shifted(panel, 450);
    renderPattern(canvas, coloured, palette, right, rightPanel); // right: after clipping
    outline(canvas, rightPanel, kOutline);
    drawLine(canvas, {450, 20}, {450, 340}, {209, 199, 178});
    writePpm(canvas, path);
}

void swatchEvidence(const std::filesystem::path& path) {
    Canvas canvas(860, 340, kPaper);
    const Polygon left{{20, 20}, {420, 20}, {420, 320}, {20, 320}};
    const Polygon right = shifted(left, 420);
    const MotifShape motif = decoratedMotif();
    std::vector<Color> palette = kPalette;
    palette.push_back({20, 20, 20});
    MotifShape coloured = motif;
    coloured.strokes[0].role = 3;

    PatternSettings plain;
    plain.tilePitch = {64, 70};
    plain.origin = {220, 170};
    plain.alternateTiles = true;
    renderPattern(canvas, coloured, palette, plain, left);

    PatternSettings transformed = plain;
    transformed.origin.x += 420;
    transformed.transform.angleRadians = kPi / 6.0; // 30 degrees, clockwise on screen
    transformed.transform.scale = 0.8;
    transformed.transform.reflectY = true;
    transformed.transform.pivot = {0, 6};
    transformed.transform.translation = {17, -9};
    transformed.tilePitch = {52, 56};
    renderPattern(canvas, coloured, palette, transformed, right);
    outline(canvas, left, kOutline);
    outline(canvas, right, kOutline);
    writePpm(canvas, path);
}

void mannequinEvidence(const std::filesystem::path& path) {
    Canvas canvas(studioWidth, studioHeight, {0, 0, 0});
    StudioState state;
    state.motif = 2;
    state.palette = 1;
    const StudioHooks hooks = makePatternHooks([](const StudioState&) {
        PatternSettings s = defaultStudioPatternSettings();
        s.transform.angleRadians = kPi / 9.0; // 20 degrees
        s.transform.scale = 0.9;
        return s;
    });
    renderStudio(canvas, state, hooks);
    writePpm(canvas, path);
}

void profile() {
    // Densest accepted settings over the full fabric area, and the dress.
    const auto time = [](const char* label, const PatternSettings& s, const Polygon& dest) {
        Canvas canvas(studioWidth, studioHeight, kPaper);
        const MotifShape motif = decoratedMotif();
        std::vector<Color> palette = kPalette;
        const PatternGeometry g = buildPatternGeometry(motif, s, dest);
        const auto start = std::chrono::steady_clock::now();
        renderPattern(canvas, motif, palette, s, dest); // builds geometry, then rasterizes
        const auto ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
        std::cout << label << ": " << g.tiles.count() << " tiles in range, " << g.tilesVisited
                  << " visited, " << g.tilesClipped << " clipped, " << g.pieces.size()
                  << " pieces, renderPattern " << ms << " ms\n";
    };
    PatternSettings normal = defaultStudioPatternSettings();
    normal.origin = kFabricPatternOrigin;
    time("Fabric, default settings", normal, studioFabricArea());
    PatternSettings dense = normal;
    dense.tilePitch = {8, 8};
    dense.transform.scale = 0.15;
    dense.transform.angleRadians = 0.6;
    time("Fabric, pitch 8 px", dense, studioFabricArea());
    time("Skirt, pitch 8 px", garmentPatternSettings(dense), studioSkirt());
}

} // namespace

int main(int argc, char** argv) {
    if (argc > 2) {
        std::cerr << "Usage: pattern_demo [output-directory]\n";
        return 1;
    }
    try {
        const std::filesystem::path dir = argc == 2 ? argv[1] : "outputs";
        std::filesystem::create_directories(dir);
        clippingEvidence(dir / "pattern-clipping.ppm");
        swatchEvidence(dir / "pattern-swatch.ppm");
        mannequinEvidence(dir / "pattern-mannequin.ppm");
        std::cout << "Saved pattern-clipping.ppm, pattern-swatch.ppm and pattern-mannequin.ppm in "
                  << dir.string() << '\n';
        profile();
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
