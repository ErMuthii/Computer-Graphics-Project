// Motif catalog and unpatterned mannequin demonstration (Deborah Rehana, 168656).
//
// Left: every catalog motif (columns) in every palette (rows). Each cell shows
// the motif's local frame: the grey square is the [-25, 25] extent and the
// grey cross is the local origin (0,0). Motifs are drawn at 2x so their
// details are visible; this is placement for display, not a matrix transform.
// Right: the static mannequin with plain dress panels and the panel vertices
// marked, showing the two convex boundaries the pattern is clipped against.
//
// Everything is rasterized with the CP1 scanline filler and Bresenham lines.

#include "kitenge/canvas.hpp"
#include "kitenge/lines.hpp"
#include "kitenge/mannequin.hpp"
#include "kitenge/motifs.hpp"
#include "kitenge/polygon_fill.hpp"
#include "kitenge/ppm.hpp"

#include <cmath>
#include <cstddef>
#include <exception>
#include <filesystem>
#include <iostream>
#include <string>

using namespace kitenge;

namespace {

constexpr Color paper{246, 240, 227};
constexpr Color guide{190, 182, 165};
constexpr Color skin{187, 156, 121};
constexpr Color brown{65, 42, 35};
constexpr Color plainCloth{226, 213, 192};
constexpr Color marker{200, 40, 40};

constexpr int kCell = 120;
constexpr int kGap = 8;
constexpr int kGridX = 30;
constexpr int kGridY = 70;
constexpr double kDisplayScale = 2.0;

Point toPixel(Point2D p) {
    return {static_cast<int>(std::lround(p.x)), static_cast<int>(std::lround(p.y))};
}

Point2D place(Point2D local, Point2D centre) {
    return {centre.x + local.x * kDisplayScale, centre.y + local.y * kDisplayScale};
}

void fillRect(Canvas& c, int x, int y, int w, int h, Color colour) {
    fillPolygon(c, {{double(x), double(y)}, {double(x + w), double(y)},
                    {double(x + w), double(y + h)}, {double(x), double(y + h)}},
                colour);
}

void drawCell(Canvas& c, const MotifShape& motif, const Palette& palette, int left, int top) {
    fillRect(c, left, top, kCell, kCell, palette.background);
    const Point2D centre{left + kCell / 2.0, top + kCell / 2.0};

    // Local frame: extent square and origin cross.
    const double e = kMotifHalfExtent;
    const Polygon frame{place({-e, -e}, centre), place({e, -e}, centre), place({e, e}, centre),
                        place({-e, e}, centre)};
    for (std::size_t i = 0; i < frame.size(); ++i)
        drawLine(c, toPixel(frame[i]), toPixel(frame[(i + 1) % frame.size()]), guide);

    for (const MotifFill& fill : motif.fills) {
        Polygon placed;
        for (const Point2D& p : fill.contour) placed.push_back(place(p, centre));
        fillPolygon(c, placed, paletteRoleColor(palette, fill.role));
    }
    for (const MotifStroke& stroke : motif.strokes) {
        const std::size_t n = stroke.points.size();
        const std::size_t segments = stroke.closed ? n : n - 1;
        for (std::size_t i = 0; i < segments; ++i)
            drawLine(c, toPixel(place(stroke.points[i], centre)),
                     toPixel(place(stroke.points[(i + 1) % n], centre)),
                     paletteRoleColor(palette, stroke.role));
    }

    // Origin cross drawn last so it stays visible over the motif.
    drawLine(c, toPixel(place({-4, 0}, centre)), toPixel(place({4, 0}, centre)), guide);
    drawLine(c, toPixel(place({0, -4}, centre)), toPixel(place({0, 4}, centre)), guide);
}

void markVertices(Canvas& c, const Polygon& panel) {
    for (const Point2D& v : panel) {
        const Point p = toPixel(v);
        fillRect(c, p.x - 3, p.y - 3, 7, 7, marker);
    }
}

} // namespace

int main(int argc, char** argv) {
    try {
        const std::string path = argc > 1 ? argv[1] : "outputs/motif-catalog.ppm";
        const std::filesystem::path parent = std::filesystem::path(path).parent_path();
        if (!parent.empty()) std::filesystem::create_directories(parent);

        Canvas canvas(1200, 760, paper);
        for (int p = 0; p < kPaletteCount; ++p)
            for (int m = 0; m < kMotifCount; ++m)
                drawCell(canvas, catalogMotif(m), catalogPalette(p), kGridX + m * (kCell + kGap),
                         kGridY + p * (kCell + kGap));

        // Divider, then the unpatterned mannequin at its studio coordinates.
        drawLine(canvas, {735, 60}, {735, 700}, guide);
        drawUnpatternedMannequin(canvas, skin, brown, plainCloth);
        markVertices(canvas, mannequinBodice());
        markVertices(canvas, mannequinSkirt());

        writePpm(canvas, path);
        std::cout << "Saved " << path << " (" << kMotifCount << " motifs x " << kPaletteCount
                  << " palettes, unpatterned mannequin)\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
    return 0;
}
