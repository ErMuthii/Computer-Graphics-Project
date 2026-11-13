// Before/after demonstration of the 2D transformation module.
// Owner: Waruhiu Jeremy Kang'ethe, 166263.
//
// Eight panels show one asymmetric motif (a filled "F" contour plus an open
// zigzag stroke) under each transform. Every panel draws, in this order:
//   * a faint grid in motif-local units (10 units per cell),
//   * a dim ghost of the untransformed motif and its pivot axes,
//   * the transformed motif (filled with the CP1 scanline filler, outlined and
//     stroked with CP1 Bresenham lines),
//   * the image of the local +x axis (red) and +y axis (green) at the pivot,
//   * the pivot (white diamond) and, if it moved, its image (yellow diamond),
//   * for the reflection panels, the dashed mirror line through the pivot.
// All placement, including the view that maps motif units to pixels, uses the
// Mat3 functions from kitenge/transforms.hpp.

#include "kitenge/lines.hpp"
#include "kitenge/polygon_fill.hpp"
#include "kitenge/ppm.hpp"
#include "kitenge/transforms.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <exception>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace {

using namespace kitenge;

constexpr double kPi = 3.14159265358979323846;

constexpr Color background{21, 31, 41};
constexpr Color panelFill{31, 48, 59};
constexpr Color border{233, 220, 174};
constexpr Color gridColor{40, 60, 72};
constexpr Color gridAxisColor{58, 84, 98};
constexpr Color ghostColor{110, 128, 138};
constexpr Color primary{229, 95, 76};   // coral fill
constexpr Color outline{120, 34, 26};
constexpr Color accent{68, 201, 176};   // teal stroke
constexpr Color secondFill{82, 139, 230}; // blue (order panel)
constexpr Color secondOutline{28, 52, 110};
constexpr Color axisX{235, 64, 52};
constexpr Color axisY{92, 214, 92};
constexpr Color pivotColor{250, 250, 250};
constexpr Color movedPivotColor{244, 185, 62};
constexpr Color textColor{233, 220, 174};
constexpr Color mirrorColor{196, 160, 220};

constexpr int kMargin = 16;
constexpr int kPanel = 264;
constexpr int kTitleHeight = 40;
constexpr double kViewScale = 2.3; // pixels per motif-local unit
constexpr double kAxisLength = 14.0;

// --- Motif (local coordinates, y down, nominally within [-25, 25]) -----------

// A filled "F": concave, with no mirror symmetry and no rotational symmetry,
// so every reflection and rotation is visibly distinguishable.
const Polygon kLetterF{{-15, -20}, {15, -20}, {15, -12}, {-7, -12}, {-7, -4},
                       {8, -4},    {8, 3},    {-7, 3},   {-7, 20},  {-15, 20}};
// An open zigzag stroke to the lower right: the stroked-path case.
const std::vector<Point2D> kZigzag{{0, 10}, {5, 16}, {10, 10}, {15, 16}, {20, 10}};
// Pivot inside the foot of the F, deliberately away from the motif centre.
constexpr Point2D kPivot{-11.0, 12.0};

// --- Raster helpers built on the CP1 modules ---------------------------------

Point toPixel(Point2D p) {
    return {static_cast<int>(std::lround(p.x)), static_cast<int>(std::lround(p.y))};
}

void fillRect(Canvas& canvas, double x0, double y0, double x1, double y1, Color color) {
    fillPolygon(canvas, {{x0, y0}, {x1, y0}, {x1, y1}, {x0, y1}}, color);
}

void line(Canvas& canvas, Point2D a, Point2D b, Color color, bool thick = false) {
    const Point pa = toPixel(a);
    const Point pb = toPixel(b);
    drawLine(canvas, pa, pb, color);
    if (thick) {
        drawLine(canvas, {pa.x + 1, pa.y}, {pb.x + 1, pb.y}, color);
        drawLine(canvas, {pa.x, pa.y + 1}, {pb.x, pb.y + 1}, color);
    }
}

void polyline(Canvas& canvas, const std::vector<Point2D>& points, bool closed, Color color,
              bool thick = false) {
    for (std::size_t i = 0; i + 1 < points.size(); ++i) line(canvas, points[i], points[i + 1], color, thick);
    if (closed && points.size() > 2) line(canvas, points.back(), points.front(), color, thick);
}

void diamond(Canvas& canvas, Point2D centre, double radius, Color color) {
    fillPolygon(canvas,
                {{centre.x, centre.y - radius}, {centre.x + radius, centre.y},
                 {centre.x, centre.y + radius}, {centre.x - radius, centre.y}},
                color);
}

void arrow(Canvas& canvas, Point2D from, Point2D to, Color color) {
    line(canvas, from, to, color, true);
    const double dx = to.x - from.x;
    const double dy = to.y - from.y;
    const double length = std::hypot(dx, dy);
    if (length < 1e-9) return;
    const double ux = dx / length;
    const double uy = dy / length;
    const Point2D base{to.x - 8.0 * ux, to.y - 8.0 * uy};
    fillPolygon(canvas, {to, {base.x - 4.5 * uy, base.y + 4.5 * ux}, {base.x + 4.5 * uy, base.y - 4.5 * ux}},
                color);
}

// --- 3x5 bitmap font for panel labels, drawn with the scanline filler --------

struct Glyph {
    char symbol;
    std::array<const char*, 5> rows;
};

const std::array<Glyph, 46> kGlyphs{{
    {'A', {{".#.", "#.#", "###", "#.#", "#.#"}}}, {'B', {{"##.", "#.#", "##.", "#.#", "##."}}},
    {'C', {{".##", "#..", "#..", "#..", ".##"}}}, {'D', {{"##.", "#.#", "#.#", "#.#", "##."}}},
    {'E', {{"###", "#..", "##.", "#..", "###"}}}, {'F', {{"###", "#..", "##.", "#..", "#.."}}},
    {'G', {{".##", "#..", "#.#", "#.#", ".##"}}}, {'H', {{"#.#", "#.#", "###", "#.#", "#.#"}}},
    {'I', {{"###", ".#.", ".#.", ".#.", "###"}}}, {'K', {{"#.#", "#.#", "##.", "#.#", "#.#"}}},
    {'L', {{"#..", "#..", "#..", "#..", "###"}}}, {'M', {{"#.#", "###", "###", "#.#", "#.#"}}},
    {'N', {{"##.", "#.#", "#.#", "#.#", "#.#"}}}, {'O', {{".#.", "#.#", "#.#", "#.#", ".#."}}},
    {'P', {{"##.", "#.#", "##.", "#..", "#.."}}}, {'R', {{"##.", "#.#", "##.", "#.#", "#.#"}}},
    {'S', {{".##", "#..", ".#.", "..#", "##."}}}, {'T', {{"###", ".#.", ".#.", ".#.", ".#."}}},
    {'U', {{"#.#", "#.#", "#.#", "#.#", "###"}}}, {'V', {{"#.#", "#.#", "#.#", "#.#", ".#."}}},
    {'W', {{"#.#", "#.#", "###", "###", "#.#"}}}, {'X', {{"#.#", "#.#", ".#.", "#.#", "#.#"}}},
    {'Y', {{"#.#", "#.#", ".#.", ".#.", ".#."}}}, {'Z', {{"###", "..#", ".#.", "#..", "###"}}},
    {'0', {{"###", "#.#", "#.#", "#.#", "###"}}}, {'1', {{".#.", "##.", ".#.", ".#.", "###"}}},
    {'2', {{"##.", "..#", ".#.", "#..", "###"}}}, {'3', {{"##.", "..#", ".#.", "..#", "##."}}},
    {'4', {{"#.#", "#.#", "###", "..#", "..#"}}}, {'5', {{"###", "#..", "##.", "..#", "##."}}},
    {'6', {{".##", "#..", "###", "#.#", "###"}}}, {'7', {{"###", "..#", ".#.", ".#.", ".#."}}},
    {'8', {{"###", "#.#", "###", "#.#", "###"}}}, {'9', {{"###", "#.#", "###", "..#", "##."}}},
    {'+', {{"...", ".#.", "###", ".#.", "..."}}}, {'-', {{"...", "...", "###", "...", "..."}}},
    {'.', {{"...", "...", "...", "...", ".#."}}}, {',', {{"...", "...", "...", ".#.", "#.."}}},
    {'(', {{".#.", "#..", "#..", "#..", ".#."}}}, {')', {{".#.", "..#", "..#", "..#", ".#."}}},
    {'*', {{"...", "#.#", ".#.", "#.#", "..."}}}, {'=', {{"...", "###", "...", "###", "..."}}},
    {'/', {{"..#", "..#", ".#.", "#..", "#.."}}}, {':', {{"...", ".#.", "...", ".#.", "..."}}},
    {'>', {{"#..", ".#.", "..#", ".#.", "#.."}}}, {' ', {{"...", "...", "...", "...", "..."}}},
}};

void drawText(Canvas& canvas, double x, double y, const std::string& text, Color color,
              double cell = 2.0) {
    for (const char symbol : text) {
        for (const Glyph& glyph : kGlyphs) {
            if (glyph.symbol != symbol) continue;
            for (std::size_t row = 0; row < 5; ++row) {
                for (std::size_t column = 0; column < 3; ++column) {
                    if (glyph.rows[row][column] != '#') continue;
                    const double gx = x + static_cast<double>(column) * cell;
                    const double gy = y + static_cast<double>(row) * cell;
                    fillRect(canvas, gx, gy, gx + cell, gy + cell, color);
                }
            }
            break;
        }
        x += 4.0 * cell;
    }
}

// --- Panels ------------------------------------------------------------------

struct Panel {
    double left;
    double top;
    Mat3 view; // motif-local units -> canvas pixels
};

Panel makePanel(Canvas& canvas, int column, int row, const std::string& title,
                const std::string& subtitle) {
    const double left = kMargin + column * (kPanel + kMargin);
    const double top = kMargin + row * (kPanel + kMargin);
    const double right = left + kPanel - 1;
    const double bottom = top + kPanel - 1;
    fillRect(canvas, left, top, right + 1, bottom + 1, panelFill);
    polyline(canvas, {{left, top}, {right, top}, {right, bottom}, {left, bottom}}, true, border);
    drawText(canvas, left + 10, top + 9, title, textColor);
    drawText(canvas, left + 10, top + 23, subtitle, ghostColor);

    // The view is itself a transform: scale motif units to pixels, then move
    // the local origin to the centre of the drawing area.
    const Point2D centre{left + kPanel / 2.0, top + kTitleHeight + (kPanel - kTitleHeight) / 2.0};
    const Mat3 view = translationMatrix(centre) * scalingMatrix(kViewScale, kViewScale);

    // Faint grid, 10 local units per cell, brighter through the local origin.
    for (int k = -40; k <= 40; k += 10) {
        const Color color = k == 0 ? gridAxisColor : gridColor;
        const double v = static_cast<double>(k);
        line(canvas, transformPoint(view, {v, -40.0}), transformPoint(view, {v, 40.0}), color);
        line(canvas, transformPoint(view, {-40.0, v}), transformPoint(view, {40.0, v}), color);
    }
    return {left, top, view};
}

void drawGhost(Canvas& canvas, const Panel& panel, Point2D pivot) {
    polyline(canvas, transformPolygon(panel.view, kLetterF), true, ghostColor);
    polyline(canvas, transformPath(panel.view, kZigzag), false, ghostColor);
    const Point2D p = transformPoint(panel.view, pivot);
    line(canvas, p, transformPoint(panel.view, {pivot.x + kAxisLength, pivot.y}), ghostColor);
    line(canvas, p, transformPoint(panel.view, {pivot.x, pivot.y + kAxisLength}), ghostColor);
}

// Draws the motif under `model` (local -> local), seen through the panel view.
void drawTransformed(Canvas& canvas, const Panel& panel, const Mat3& model, Point2D pivot,
                     Color fill = primary, Color edge = outline) {
    const Mat3 toCanvas = panel.view * model;
    // New containers each time: kLetterF and kZigzag are never modified.
    const Polygon shape = transformPolygon(toCanvas, kLetterF);
    fillPolygon(canvas, shape, fill);
    polyline(canvas, shape, true, edge);
    polyline(canvas, transformPath(toCanvas, kZigzag), false, accent, true);

    // Images of the local axes at the pivot show what happened to the frame:
    // their turn is the rotation, their lengths the scale, a swapped
    // handedness the reflection.
    const Point2D origin = transformPoint(toCanvas, pivot);
    arrow(canvas, origin, transformPoint(toCanvas, {pivot.x + kAxisLength, pivot.y}), axisX);
    arrow(canvas, origin, transformPoint(toCanvas, {pivot.x, pivot.y + kAxisLength}), axisY);
}

void drawPivots(Canvas& canvas, const Panel& panel, const Mat3& model, Point2D pivot) {
    const Point2D before = transformPoint(panel.view, pivot);
    const Point2D after = transformPoint(panel.view * model, pivot);
    if (std::hypot(after.x - before.x, after.y - before.y) > 0.5) diamond(canvas, after, 4.5, movedPivotColor);
    diamond(canvas, before, 4.5, pivotColor);
}

enum class Mirror { None, Vertical, Horizontal };

// Dashed mirror line through the pivot: vertical for reflectX (x = pivot.x),
// horizontal for reflectY (y = pivot.y).
void drawMirrorLine(Canvas& canvas, const Panel& panel, Mirror mirror, Point2D pivot) {
    if (mirror == Mirror::None) return;
    for (double s = -40.0; s < 40.0; s += 4.0) {
        const Point2D a = mirror == Mirror::Vertical ? Point2D{pivot.x, s} : Point2D{s, pivot.y};
        const Point2D b = mirror == Mirror::Vertical ? Point2D{pivot.x, s + 2.0} : Point2D{s + 2.0, pivot.y};
        line(canvas, transformPoint(panel.view, a), transformPoint(panel.view, b), mirrorColor, true);
    }
}

void showcase(Canvas& canvas, int column, int row, const std::string& title, const std::string& subtitle,
              const Mat3& model, Mirror mirror = Mirror::None) {
    const Panel panel = makePanel(canvas, column, row, title, subtitle);
    drawMirrorLine(canvas, panel, mirror, kPivot);
    drawGhost(canvas, panel, kPivot);
    drawTransformed(canvas, panel, model, kPivot);
    drawPivots(canvas, panel, model, kPivot);
}

TransformParams aboutPivot() {
    TransformParams params;
    params.pivot = kPivot;
    return params;
}

void drawLegend(Canvas& canvas, double top) {
    double x = kMargin;
    const auto key = [&](Color color, const std::string& label) {
        fillRect(canvas, x, top, x + 12, top + 12, color);
        drawText(canvas, x + 18, top + 1, label, textColor);
        x += 18 + 8.0 * static_cast<double>(label.size()) + 14;
    };
    key(ghostColor, "ORIGINAL");
    key(primary, "TRANSFORMED");
    key(accent, "STROKED PATH");
    key(axisX, "LOCAL +X");
    key(axisY, "LOCAL +Y");
    key(pivotColor, "PIVOT");
    key(movedPivotColor, "MOVED PIVOT");
    key(mirrorColor, "MIRROR LINE");
    drawText(canvas, x, top + 1, "Y DOWN: +ANGLE = CLOCKWISE", ghostColor);
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc > 2) {
        std::cerr << "Usage: transforms_demo [output-image.ppm]\n";
        return 1;
    }
    try {
        const std::filesystem::path destination = argc == 2
            ? std::filesystem::path(argv[1])
            : std::filesystem::path("outputs/transforms.ppm");

        const int width = kMargin + 4 * (kPanel + kMargin);
        const int height = kMargin + 2 * (kPanel + kMargin) + 26;
        Canvas canvas(width, height, background);

        // 1. Untransformed reference.
        showcase(canvas, 0, 0, "ORIGINAL", "IDENTITY", Mat3::identity());

        // 2. Translation only: every point, including the pivot, shifts.
        TransformParams translate = aboutPivot();
        translate.translation = {12.0, -8.0};
        showcase(canvas, 1, 0, "TRANSLATE", "T(12,-8)", transformMatrix(translate));

        // 3. Rotation about the pivot. Positive angle, clockwise on screen.
        TransformParams rotate = aboutPivot();
        rotate.angleRadians = 30.0 * kPi / 180.0;
        showcase(canvas, 2, 0, "ROTATE +30 DEG", "ABOUT PIVOT, CLOCKWISE", transformMatrix(rotate));

        // 4. Non-uniform scale about the pivot, built from the Mat3 builders
        //    (TransformParams carries the uniform scale of the shared contract).
        const Mat3 stretch = translationMatrix(kPivot) * scalingMatrix(1.6, 0.6) *
                             translationMatrix(-kPivot.x, -kPivot.y);
        showcase(canvas, 3, 0, "SCALE SX 1.6 SY 0.6", "NON-UNIFORM, ABOUT PIVOT", stretch);

        // 5. reflectX negates x: mirror across the vertical line through the pivot.
        TransformParams mirrorX = aboutPivot();
        mirrorX.reflectX = true;
        showcase(canvas, 0, 1, "REFLECTX: X -> -X", "MIRROR ACROSS Y-AXIS", transformMatrix(mirrorX),
                 Mirror::Vertical);

        // 6. reflectY negates y: mirror across the horizontal line through the pivot.
        TransformParams mirrorY = aboutPivot();
        mirrorY.reflectY = true;
        showcase(canvas, 1, 1, "REFLECTY: Y -> -Y", "MIRROR ACROSS X-AXIS", transformMatrix(mirrorY),
                 Mirror::Horizontal);

        // 7. Every parameter at once, in the shared order T * Tp * R * F * S * T-p.
        TransformParams combined = aboutPivot();
        combined.translation = {8.0, -6.0};
        combined.angleRadians = 30.0 * kPi / 180.0;
        combined.scale = 0.8;
        combined.reflectX = true;
        showcase(canvas, 2, 1, "COMBINED", "T(8,-6) R30 FX S0.8", transformMatrix(combined));

        // 8. Order matters: the same R(90) and T(18,0), multiplied both ways,
        //    about the local origin so the effect of order is isolated.
        {
            const Panel panel = makePanel(canvas, 3, 1, "ORDER: T*R VS R*T", "CORAL T*R   BLUE R*T");
            const Point2D origin{0.0, 0.0};
            const Mat3 r = rotationMatrix(kPi / 2.0);
            const Mat3 t = translationMatrix(18.0, 0.0);
            drawGhost(canvas, panel, origin);
            drawTransformed(canvas, panel, t * r, origin);                         // rotate, then translate
            drawTransformed(canvas, panel, r * t, origin, secondFill, secondOutline); // translate, then rotate
            drawPivots(canvas, panel, t * r, origin);
            drawPivots(canvas, panel, r * t, origin);
        }

        drawLegend(canvas, kMargin + 2 * (kPanel + kMargin) + 2);

        writePpm(canvas, destination);
        std::cout << "Saved transformation demonstration to " << destination << '\n'
                  << "Panels (left to right, top to bottom): original, translate, rotate +30 deg,\n"
                  << "non-uniform scale, reflectX, reflectY, combined, multiplication order.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}
