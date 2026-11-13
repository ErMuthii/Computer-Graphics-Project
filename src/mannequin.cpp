#include "kitenge/mannequin.hpp"

#include "kitenge/circles.hpp"
#include "kitenge/lines.hpp"
#include "kitenge/polygon_fill.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <stdexcept>

namespace kitenge {

namespace {

MannequinGeometry buildMannequin() {
    MannequinGeometry m;
    m.headCentre = {950, 214};
    m.headRadiusX = 30;
    m.headRadiusY = 35;
    m.neck = {{937, 241}, {963, 241}, {963, 278}, {937, 278}};
    m.arms = {Polygon{{887, 288}, {900, 302}, {864, 420}, {847, 412}},
              Polygon{{1000, 302}, {1013, 288}, {1053, 412}, {1036, 420}}};
    m.legs = {Polygon{{917, 561}, {943, 561}, {939, 640}, {920, 640}},
              Polygon{{957, 561}, {983, 561}, {980, 640}, {961, 640}}};
    m.shoes = {PixelBox{910, 640, 31, 9}, PixelBox{960, 640, 31, 9}};
    m.waistband = {914, 374, 72, 8};
    // The bodice's lower edge and the skirt's upper edge are the same segment
    // (915,377)-(985,377), so the two panels meet at the waist without a gap.
    m.bodice = {{897, 278}, {1003, 278}, {985, 377}, {915, 377}};
    m.skirt = {{915, 377}, {985, 377}, {1041, 574}, {859, 574}};
    return m;
}

void fillBox(Canvas& canvas, const PixelBox& box, Color colour) {
    for (int y = box.y; y < box.y + box.height; ++y)
        for (int x = box.x; x < box.x + box.width; ++x) canvas.setPixel(x, y, colour);
}

Point rounded(Point2D p) {
    return {static_cast<int>(std::lround(p.x)), static_cast<int>(std::lround(p.y))};
}

void outlinePolygon(Canvas& canvas, const Polygon& polygon, Color colour) {
    for (std::size_t i = 0; i < polygon.size(); ++i)
        drawLine(canvas, rounded(polygon[i]), rounded(polygon[(i + 1) % polygon.size()]), colour);
}

double turn(Point2D a, Point2D b, Point2D c) {
    return (b.x - a.x) * (c.y - b.y) - (b.y - a.y) * (c.x - b.x);
}

} // namespace

const MannequinGeometry& mannequinGeometry() {
    static const MannequinGeometry geometry = buildMannequin();
    return geometry;
}

const Polygon& mannequinBodice() { return mannequinGeometry().bodice; }
const Polygon& mannequinSkirt() { return mannequinGeometry().skirt; }

bool isConvexPolygon(const Polygon& polygon) {
    if (!isValidPolygon(polygon)) return false;
    const std::size_t n = polygon.size();
    int sign = 0;
    for (std::size_t i = 0; i < n; ++i) {
        const double t = turn(polygon[i], polygon[(i + 1) % n], polygon[(i + 2) % n]);
        if (t == 0.0) continue; // collinear vertex
        const int s = t > 0.0 ? 1 : -1;
        if (sign == 0) sign = s;
        else if (s != sign) return false;
    }
    return sign != 0;
}

Rect polygonBounds(const Polygon& polygon) {
    if (polygon.empty()) throw std::invalid_argument("Bounds need at least one vertex.");
    Rect r{polygon[0].x, polygon[0].y, polygon[0].x, polygon[0].y};
    for (const Point2D& p : polygon) {
        r.minX = std::min(r.minX, p.x);
        r.minY = std::min(r.minY, p.y);
        r.maxX = std::max(r.maxX, p.x);
        r.maxY = std::max(r.maxY, p.y);
    }
    return r;
}

void drawMannequinFigure(Canvas& canvas, Color skin, Color outline) {
    const MannequinGeometry& m = mannequinGeometry();
    fillPolygon(canvas, m.neck, skin);
    for (const Polygon& arm : m.arms) fillPolygon(canvas, arm, skin);
    for (const Polygon& leg : m.legs) fillPolygon(canvas, leg, skin);

    // Face: one horizontal span per row, one pixel inside the outline's x
    // radius so the midpoint ellipse drawn next stays visible all round.
    const int ry = m.headRadiusY;
    const int fillRx = m.headRadiusX - 1;
    for (int y = -(ry - 1); y <= ry - 1; ++y) {
        const int half = static_cast<int>(fillRx * std::sqrt(1.0 - double(y * y) / (ry * ry)));
        drawLine(canvas, {m.headCentre.x - half, m.headCentre.y + y},
                 {m.headCentre.x + half, m.headCentre.y + y}, skin);
    }
    drawEllipseMidpoint(canvas, m.headCentre, m.headRadiusX, m.headRadiusY, outline);

    for (const PixelBox& shoe : m.shoes) fillBox(canvas, shoe, outline);
}

void drawPlainGarment(Canvas& canvas, Color cloth) {
    fillPolygon(canvas, mannequinBodice(), cloth);
    fillPolygon(canvas, mannequinSkirt(), cloth);
}

void drawGarmentTrim(Canvas& canvas, Color colour) {
    outlinePolygon(canvas, mannequinBodice(), colour);
    outlinePolygon(canvas, mannequinSkirt(), colour);
    fillBox(canvas, mannequinGeometry().waistband, colour);
}

void drawUnpatternedMannequin(Canvas& canvas, Color skin, Color outline, Color cloth) {
    drawMannequinFigure(canvas, skin, outline);
    drawPlainGarment(canvas, cloth);
    drawGarmentTrim(canvas, outline);
}

} // namespace kitenge
