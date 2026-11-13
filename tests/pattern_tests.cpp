#include "kitenge/curves.hpp"
#include "kitenge/pattern.hpp"
#include "kitenge/polygon_clip.hpp"
#include "kitenge/polygon_fill.hpp"
#include "kitenge/studio.hpp"

#include <cmath>
#include <cstddef>
#include <exception>
#include <functional>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using namespace kitenge;

constexpr double kPi = 3.14159265358979323846;
constexpr Color kSentinel{1, 2, 3};

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

bool near(double a, double b, double tolerance = 1e-6) { return std::abs(a - b) <= tolerance; }

bool same(Color a, Color b) { return a.red == b.red && a.green == b.green && a.blue == b.blue; }

double area(const Polygon& polygon) {
    double twice = 0.0;
    for (std::size_t i = 0; i < polygon.size(); ++i) {
        const Point2D& p = polygon[i];
        const Point2D& q = polygon[(i + 1) % polygon.size()];
        twice += p.x * q.y - q.x * p.y;
    }
    return std::abs(twice) / 2.0;
}

double fillArea(const PatternGeometry& g) {
    double total = 0.0;
    for (const PatternPiece& piece : g.pieces) {
        if (piece.kind == PatternPiece::Kind::Fill) total += area(piece.points);
    }
    return total;
}

std::size_t countKind(const PatternGeometry& g, PatternPiece::Kind kind) {
    std::size_t n = 0;
    for (const PatternPiece& piece : g.pieces) n += piece.kind == kind ? 1 : 0;
    return n;
}

bool throwsInvalid(const std::function<void()>& action) {
    try {
        action();
    } catch (const std::invalid_argument&) {
        return true;
    }
    return false;
}

Polygon rectangle(double x0, double y0, double x1, double y1) {
    return {{x0, y0}, {x1, y0}, {x1, y1}, {x0, y1}};
}

// A point is inside the convex polygon (closed, with a small tolerance).
bool insideConvex(const Polygon& boundary, Point2D p, double tolerance = 1e-6) {
    double orientation = 0.0;
    for (std::size_t i = 0; i < boundary.size(); ++i) {
        const Point2D& a = boundary[i];
        const Point2D& b = boundary[(i + 1) % boundary.size()];
        orientation += a.x * b.y - b.x * a.y;
    }
    orientation = orientation > 0 ? 1.0 : -1.0;
    for (std::size_t i = 0; i < boundary.size(); ++i) {
        const Point2D& a = boundary[i];
        const Point2D& b = boundary[(i + 1) % boundary.size()];
        const double d = (b.x - a.x) * (p.y - a.y) - (b.y - a.y) * (p.x - a.x);
        if (orientation * d < -tolerance) return false;
    }
    return true;
}

// Pixels the scanline filler assigns to the destination: the definition of
// "inside" used by the renderer's mask and by the studio shell.
std::vector<bool> destinationPixels(int width, int height, const Polygon& destination) {
    Canvas c(width, height, {0, 0, 0});
    fillPolygon(c, destination, {255, 255, 255});
    std::vector<bool> inside(static_cast<std::size_t>(width * height));
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x)
            inside[static_cast<std::size_t>(y * width + x)] = c.pixelAt(x, y).red == 255;
    return inside;
}

// Fails if any pixel outside `destination` differs from the sentinel colour.
void requireNothingOutside(const Canvas& canvas, const Polygon& destination, const std::string& what) {
    const auto inside = destinationPixels(canvas.width(), canvas.height(), destination);
    for (int y = 0; y < canvas.height(); ++y)
        for (int x = 0; x < canvas.width(); ++x)
            if (!inside[static_cast<std::size_t>(y * canvas.width() + x)])
                require(same(canvas.pixelAt(x, y), kSentinel),
                        what + ": pattern pixel at (" + std::to_string(x) + "," + std::to_string(y) +
                            ") lies outside the destination");
}

// The convex skirt-like panel used for slanted-edge tests.
const Polygon& slantedPanel() {
    static const Polygon panel{{80, 40}, {220, 40}, {290, 260}, {10, 260}};
    return panel;
}

// Concave U shape: arms at the top, opening upwards.
Polygon uShape() {
    return {{-15, -15}, {15, -15}, {15, 15}, {5, 15}, {5, -5}, {-5, -5}, {-5, 15}, {-15, 15}};
}

// A concave cross with a sampled Bezier arch stroked across it.
MotifShape decoratedMotif() {
    MotifShape motif = starterMotifShape(2);
    const std::vector<Point2D> arch =
        sampleCubicBezier({Point2D{-24, 18}, Point2D{-12, -30}, Point2D{12, -30}, Point2D{24, 18}}, 24);
    motif.strokes.push_back({arch, false, 2});
    return motif;
}

PatternSettings singleTileAt(Point2D origin) {
    PatternSettings s;
    s.origin = origin;
    s.tilePitch = {1000.0, 1000.0}; // neighbours are far away: one tile only
    return s;
}

// --- Tests ------------------------------------------------------------------------

void testMatrixOrder() {
    PatternSettings s;
    s.origin = {100, 200};
    s.tilePitch = {64, 70};
    s.transform.translation = {10, 0};
    s.transform.pivot = {5, 0};
    s.transform.angleRadians = kPi / 2.0;
    s.transform.scale = 2.0;
    s.transform.reflectX = true;
    // (6,0) -T(-pivot)-> (1,0) -S-> (2,0) -F-> (-2,0) -R(90)-> (0,-2)
    //       -T(pivot)-> (5,-2) -tile (1,0) at origin+translation-> (179,198)
    const Point2D p = patternPoint(s, 1, 0, {6, 0});
    require(near(p.x, 179) && near(p.y, 198), "Matrix order must be T(o+tile) T(p) R F S T(-p)");
    const Point2D q = patternPoint(s, 0, 2, {5, 0}); // the pivot never moves locally
    require(near(q.x, 115) && near(q.y, 340), "Pivot must stay fixed under R, F and S");
    // Positive rotation turns +x towards +y, which is clockwise on a y-down canvas.
    PatternSettings r;
    r.transform.angleRadians = kPi / 2.0;
    const Point2D down = patternPoint(r, 0, 0, {10, 0});
    require(near(down.x, 0) && near(down.y, 10), "Positive angles must rotate +x towards +y");
}

void testValidation() {
    const MotifShape motif = starterMotifShape(0);
    const Polygon dest = rectangle(0, 0, 200, 200);
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    for (Point2D pitch : {Point2D{0, 70}, Point2D{64, -5}, Point2D{nan, 70}, Point2D{64, inf},
                          Point2D{kMinTilePitch - 0.5, 70}}) {
        PatternSettings s;
        s.tilePitch = pitch;
        require(throwsInvalid([&] { buildPatternGeometry(motif, s, dest); }),
                "Invalid tile pitches must be rejected");
    }
    for (double scale : {0.0, -1.0, nan, kMaxPatternScale * 2}) {
        PatternSettings s;
        s.transform.scale = scale;
        require(throwsInvalid([&] { buildPatternGeometry(motif, s, dest); }),
                "Invalid scales must be rejected");
    }
    PatternSettings angle;
    angle.transform.angleRadians = inf;
    require(throwsInvalid([&] { buildPatternGeometry(motif, angle, dest); }),
            "Non-finite angles must be rejected");

    PatternSettings dense;
    dense.tilePitch = {kMinTilePitch, kMinTilePitch};
    dense.transform.scale = kMinPatternScale;
    require(throwsInvalid([&] { buildPatternGeometry(motif, dense, rectangle(0, 0, 4000, 4000)); }),
            "Work must stay bounded by kMaxPatternTiles");

    const Polygon concaveDest{{0, 0}, {100, 0}, {50, 20}, {100, 100}, {0, 100}};
    require(throwsInvalid([&] { buildPatternGeometry(motif, PatternSettings{}, concaveDest); }),
            "Concave destinations must be rejected");
    MotifShape bowtie{{{{{-10, -10}, {10, 10}, {10, -10}, {-10, 10}}, 1}}, {}};
    require(throwsInvalid([&] { buildPatternGeometry(bowtie, PatternSettings{}, dest); }),
            "Self-intersecting motif contours must be rejected");

    Canvas canvas(50, 50, kSentinel);
    require(throwsInvalid([&] { renderPattern(canvas, motif, {}, PatternSettings{}, dest); }),
            "An empty palette must be rejected");
    require(throwsInvalid([&] { renderPattern(canvas, motif, {{0, 0, 0}}, PatternSettings{}, dest); }),
            "Roles outside the palette must be rejected");
    PatternSettings alternate;
    alternate.alternateTiles = true;
    require(throwsInvalid([&] {
                renderPattern(canvas, motif, {{0, 0, 0}, {9, 9, 9}}, alternate, dest);
            }),
            "Swapped roles on odd tiles must also be inside the palette");
    for (int y = 0; y < 50; ++y)
        for (int x = 0; x < 50; ++x)
            require(same(canvas.pixelAt(x, y), kSentinel), "Rejected renders must not draw");
}

void testTriangulation() {
    const Polygon cross = starterMotifShape(2).fills[0].contour;
    for (const Polygon& shape : {cross, uShape()}) {
        const auto triangles = triangulateSimplePolygon(shape);
        require(triangles.size() == shape.size() - 2, "Ear clipping must give n - 2 triangles");
        double total = 0.0;
        for (const Polygon& t : triangles) {
            require(isValidPolygon(t), "Every triangle must be a valid polygon");
            total += area(t);
        }
        require(near(total, area(shape)), "Triangles must cover exactly the polygon's area");
    }
    const Polygon withCollinear{{0, 0}, {10, 0}, {20, 0}, {20, 10}, {0, 10}};
    double collinearTotal = 0.0;
    for (const Polygon& t : triangulateSimplePolygon(withCollinear)) {
        require(isValidPolygon(t), "Collinear input must not produce degenerate triangles");
        collinearTotal += area(t);
    }
    require(near(collinearTotal, 200.0), "Collinear vertices must not lose area");
    require(throwsInvalid([] { triangulateSimplePolygon({{0, 0}, {10, 10}, {10, 0}, {0, 10}}); }),
            "Self-intersecting polygons cannot be triangulated");
}

void testSegmentClipping() {
    const Polygon& panel = slantedPanel();
    auto inside = clipSegmentToConvex({100, 100}, {200, 200}, panel);
    require(inside && near(inside->first.x, 100) && near(inside->second.y, 200),
            "Segments inside the panel must be kept unchanged");
    require(!clipSegmentToConvex({0, 0}, {60, 20}, panel), "Segments outside must be removed");
    // Horizontal line y = 150 crosses the slanted sides at x = 45 and x = 255.
    auto crossing = clipSegmentToConvex({0, 150}, {300, 150}, panel);
    require(crossing && near(crossing->first.x, 45) && near(crossing->second.x, 255),
            "Crossing segments must end exactly on the slanted edges");
    auto reversed = clipSegmentToConvex({300, 150}, {0, 150}, panel);
    require(reversed && near(reversed->first.x, 255) && near(reversed->second.x, 45),
            "Clipping must keep the segment's direction");
    require(!clipSegmentToConvex({0, 30}, {300, 30}, panel),
            "A segment parallel to an edge and outside it must be removed");
    auto alongEdge = clipSegmentToConvex({100, 40}, {200, 40}, panel);
    require(alongEdge.has_value(), "A segment lying on the boundary counts as inside");
    require(throwsInvalid([] { clipSegmentToConvex({0, 0}, {1, 1}, {{0, 0}, {5, 5}}); }),
            "Degenerate boundaries must be rejected");
}

void testInsideOutsideCrossing() {
    const Polygon dest = rectangle(0, 0, 200, 200);
    const MotifShape diamond = starterMotifShape(0); // area 2 * 22 * 25 = 1100

    const PatternGeometry inside = buildPatternGeometry(diamond, singleTileAt({100, 100}), dest);
    require(inside.pieces.size() == 1 && inside.tilesClipped == 0,
            "A motif fully inside must be kept whole without clipping");
    require(near(fillArea(inside), 1100), "An inside motif keeps its full area");

    const PatternGeometry outside = buildPatternGeometry(diamond, singleTileAt({500, 500}), dest);
    require(outside.pieces.empty(), "A motif fully outside must produce no pieces");

    const PatternGeometry crossing = buildPatternGeometry(diamond, singleTileAt({0, 100}), dest);
    require(crossing.tilesClipped == 1, "A crossing motif must be clipped");
    require(near(fillArea(crossing), 550), "A diamond centred on an edge keeps exactly half");
    for (const PatternPiece& piece : crossing.pieces)
        for (const Point2D& p : piece.points)
            require(insideConvex(dest, p), "Clipped vertices must lie inside the destination");
}

void testRotatedAndReflectedEdgeTiles() {
    const Polygon dest = rectangle(0, 0, 200, 200);
    MotifShape square{{{rectangle(-10, -10, 10, 10), 1}}, {}};
    PatternSettings rotated = singleTileAt({0, 100});
    rotated.transform.angleRadians = kPi / 4.0;
    const PatternGeometry r = buildPatternGeometry(square, rotated, dest);
    require(near(fillArea(r), 200), "A rotated square on the edge keeps half of its 400 area");
    bool reachesFurther = false;
    for (const PatternPiece& piece : r.pieces)
        for (const Point2D& p : piece.points) reachesFurther |= p.x > 10.0 + 1e-6;
    require(reachesFurther, "Rotation must happen before the tile offset and clipping");

    // The triangle lies at x >= 0; reflecting x moves it to x <= 0, outside.
    MotifShape corner{{{{{0, 0}, {20, 0}, {0, 20}}, 1}}, {}};
    const PatternGeometry plain = buildPatternGeometry(corner, singleTileAt({0, 100}), dest);
    require(near(fillArea(plain), 200), "The unreflected triangle is fully inside");
    PatternSettings mirrored = singleTileAt({0, 100});
    mirrored.transform.reflectX = true;
    require(buildPatternGeometry(corner, mirrored, dest).pieces.empty(),
            "The reflected triangle only touches the edge, so nothing remains");
    PatternSettings flipped = singleTileAt({100, 0});
    flipped.transform.reflectY = true;
    require(buildPatternGeometry(corner, flipped, dest).pieces.empty(),
            "Reflecting y must also be applied before clipping");
}

void testConservativeTileRange() {
    const MotifShape motif = decoratedMotif();
    const Polygon& panel = slantedPanel();
    for (double angle : {0.0, 0.4, 1.3, 2.9, -2.2}) {
        PatternSettings s;
        s.transform.angleRadians = angle;
        s.transform.scale = 1.7;
        s.transform.pivot = {9, -4};
        s.transform.translation = {13, -7};
        s.transform.reflectX = angle > 1.0;
        s.tilePitch = {40, 45};
        s.origin = {150, 150};
        const TileRange range = patternTileRange(motif, s, panel);
        require(!range.empty(), "The panel must need some tiles");
        // Every tile outside the computed range must miss the panel completely.
        for (long long row = range.firstRow - 4; row <= range.lastRow + 4; ++row) {
            for (long long column = range.firstColumn - 4; column <= range.lastColumn + 4; ++column) {
                if (row >= range.firstRow && row <= range.lastRow && column >= range.firstColumn &&
                    column <= range.lastColumn) {
                    continue;
                }
                for (const MotifFill& fill : motif.fills) {
                    Polygon placed;
                    for (const Point2D& p : fill.contour) placed.push_back(patternPoint(s, column, row, p));
                    require(clipPolygonToConvex(placed, panel).empty(),
                            "A tile outside the range still reaches the panel");
                }
                for (const MotifStroke& stroke : motif.strokes)
                    for (std::size_t i = 0; i + 1 < stroke.points.size(); ++i)
                        require(!clipSegmentToConvex(patternPoint(s, column, row, stroke.points[i]),
                                                     patternPoint(s, column, row, stroke.points[i + 1]),
                                                     panel),
                                "A stroke outside the range still reaches the panel");
            }
        }
        const PatternGeometry g = buildPatternGeometry(motif, s, panel);
        require(g.discardedSlivers == 0, "No visible pieces may be discarded");
        for (const PatternPiece& piece : g.pieces) {
            for (const Point2D& p : piece.points)
                require(insideConvex(panel, p), "Rotated edge tiles must be clipped to the panel");
            if (piece.kind == PatternPiece::Kind::Fill)
                require(isValidPolygon(piece.points), "Clipped fills must pass the fill validator");
        }
    }
}

void testConcaveMotifAtSlantedEdge() {
    const Polygon& panel = slantedPanel();
    MotifShape u{{{uShape(), 1}}, {}};

    // Why the engine triangulates: plain Sutherland-Hodgman on a concave shape
    // split in two by the boundary returns one polygon joined by a zero-width
    // bridge along the edge, which the fill validator rejects.
    Polygon placed;
    for (const Point2D& p : uShape()) placed.push_back({p.x + 150, p.y + 40});
    const Polygon naive = clipPolygonToConvex(placed, panel);
    require(!naive.empty() && !isValidPolygon(naive),
            "Expected the naive clip of a split U shape to be degenerate");
    const PatternGeometry split = buildPatternGeometry(u, singleTileAt({150, 40}), panel);
    double leftArm = 0.0, rightArm = 0.0;
    for (const PatternPiece& piece : split.pieces) {
        require(isValidPolygon(piece.points), "Every clipped piece must be valid");
        const double cx = (piece.points[0].x + piece.points[1].x + piece.points[2].x) / 3.0;
        (cx < 150 ? leftArm : rightArm) += area(piece.points);
    }
    // The panel's top edge (y = 40) removes the U's base (y 25..35) and keeps
    // each arm from y = 40 to 55: two separate 10 x 15 pieces.
    require(near(leftArm, 150) && near(rightArm, 150),
            "Both disconnected arms must survive clipping");

    // Pixel check on a slanted edge: rendering the clipped geometry must equal
    // rendering the whole motif and keeping only destination pixels.
    for (const Point2D origin : {Point2D{40, 150}, Point2D{262, 200}, Point2D{150, 45}}) {
        PatternSettings s = singleTileAt(origin);
        s.transform.angleRadians = 0.3;
        s.transform.scale = 1.6;
        const MotifShape cross = starterMotifShape(2);
        Canvas clipped(300, 300, kSentinel);
        renderPattern(clipped, cross, {{240, 240, 240}, {200, 30, 30}}, s, panel);

        Canvas whole(300, 300, {240, 240, 240});
        Polygon full;
        for (const Point2D& p : cross.fills[0].contour) full.push_back(patternPoint(s, 0, 0, p));
        fillPolygon(whole, full, {200, 30, 30});
        const auto inside = destinationPixels(300, 300, panel);
        int differences = 0, motifPixels = 0;
        for (int y = 0; y < 300; ++y)
            for (int x = 0; x < 300; ++x) {
                if (!inside[static_cast<std::size_t>(y * 300 + x)]) continue;
                motifPixels += same(clipped.pixelAt(x, y), {200, 30, 30}) ? 1 : 0;
                differences += same(clipped.pixelAt(x, y), whole.pixelAt(x, y)) ? 0 : 1;
            }
        require(motifPixels > 50, "The clipped concave motif must stay visible");
        require(differences <= 6, "Clipped concave motif differs from the masked reference by " +
                                      std::to_string(differences) + " pixels");
        requireNothingOutside(clipped, panel, "Concave motif");
    }
}

void testStrokeNearSlantedEdge() {
    const Polygon& panel = slantedPanel();
    const MotifShape motif = decoratedMotif();
    const std::vector<Color> palette{{240, 240, 240}, {30, 90, 160}, {20, 20, 20}};
    // Tiles along the left slanted edge, rotated so strokes cross it obliquely.
    PatternSettings s;
    s.origin = {45, 150};
    s.tilePitch = {500, 48};
    s.transform.angleRadians = 0.5;
    const PatternGeometry g = buildPatternGeometry(motif, s, panel);
    require(countKind(g, PatternPiece::Kind::Stroke) > 0, "Strokes must survive clipping");
    for (const PatternPiece& piece : g.pieces)
        if (piece.kind == PatternPiece::Kind::Stroke)
            for (const Point2D& p : piece.points)
                require(insideConvex(panel, p), "Clipped strokes must end inside the slanted panel");

    Canvas canvas(300, 300, kSentinel);
    renderPattern(canvas, motif, palette, s, panel);
    requireNothingOutside(canvas, panel, "Stroked curve");
    int strokePixels = 0;
    for (int y = 0; y < 300; ++y)
        for (int x = 0; x < 300; ++x) strokePixels += same(canvas.pixelAt(x, y), palette[2]) ? 1 : 0;
    require(strokePixels > 20, "The stroked curve must be visible inside the panel");
}

void testNoPixelsOutside() {
    const MotifShape motif = decoratedMotif();
    const std::vector<Color> palette{{240, 230, 200}, {170, 60, 30}, {40, 80, 60}};
    for (const Polygon& dest : {slantedPanel(), rectangle(-30, -30, 140, 90),
                                Polygon{{150, 10}, {290, 150}, {150, 290}, {10, 150}}}) {
        for (double angle : {0.0, 0.8, 2.4}) {
            PatternSettings s;
            s.transform.angleRadians = angle;
            s.transform.scale = 1.3;
            s.transform.reflectY = angle > 1.0;
            s.tilePitch = {37, 41};
            s.origin = {150, 150};
            s.alternateTiles = true;
            Canvas canvas(300, 300, kSentinel);
            renderPattern(canvas, motif, palette, s, dest);
            requireNothingOutside(canvas, dest, "Pattern");
            const auto inside = destinationPixels(300, 300, dest);
            for (int y = 0; y < 300; ++y)
                for (int x = 0; x < 300; ++x)
                    if (inside[static_cast<std::size_t>(y * 300 + x)])
                        require(!same(canvas.pixelAt(x, y), kSentinel),
                                "Every destination pixel must receive background or pattern");
        }
    }
}

void testContinuityAcrossWaist() {
    PatternSettings base = defaultStudioPatternSettings();
    base.transform.angleRadians = 0.35;
    base.transform.translation = {11, -6};
    const PatternSettings s = garmentPatternSettings(base);
    const std::vector<Color> palette = patternPaletteFor(0);
    const MotifShape motif = starterMotifShape(2);

    Canvas panels(studioWidth, studioHeight, kSentinel);
    renderPattern(panels, motif, palette, s, studioBodice());
    renderPattern(panels, motif, palette, s, studioSkirt());
    // Reference: the same repeat over one rectangle covering the whole dress.
    Canvas reference(studioWidth, studioHeight, kSentinel);
    renderPattern(reference, motif, palette, s, rectangle(850, 270, 1050, 580));
    int compared = 0;
    for (int y = 270; y <= 580; ++y)
        for (int x = 850; x <= 1050; ++x) {
            const Color p = panels.pixelAt(x, y);
            if (same(p, kSentinel)) continue;
            ++compared;
            require(same(p, reference.pixelAt(x, y)),
                    "Dress pixel (" + std::to_string(x) + "," + std::to_string(y) +
                        ") breaks the shared repeat");
        }
    require(compared > 20000, "The dress panels must be patterned");

    // Some tile must be cut by the waist and appear in both panels.
    const PatternGeometry bodice = buildPatternGeometry(motif, s, studioBodice());
    const PatternGeometry skirt = buildPatternGeometry(motif, s, studioSkirt());
    bool shared = false;
    for (const PatternPiece& a : bodice.pieces)
        for (const PatternPiece& b : skirt.pieces) shared |= a.column == b.column && a.row == b.row;
    require(shared, "A motif crossing the waist must continue into the skirt");
}

bool regionChanged(const Canvas& a, const Canvas& b, int x0, int y0, int x1, int y1) {
    for (int y = y0; y < y1; ++y)
        for (int x = x0; x < x1; ++x)
            if (!same(a.pixelAt(x, y), b.pixelAt(x, y))) return true;
    return false;
}

void testStudioHooks() {
    const StudioHooks hooks = makePatternHooks();
    require(hooks.fabric && hooks.garment && !hooks.animate,
            "Pattern hooks provide fabric and garment only");
    StudioState state;
    Canvas starter(studioWidth, studioHeight, {0, 0, 0}), engine(studioWidth, studioHeight, {0, 0, 0});
    renderStudio(starter, state);
    renderStudio(engine, state, hooks);

    // Everything outside the fabric area and the dress panels is the shell's.
    const auto fabric = destinationPixels(studioWidth, studioHeight, studioFabricArea());
    const auto bodice = destinationPixels(studioWidth, studioHeight, studioBodice());
    const auto skirt = destinationPixels(studioWidth, studioHeight, studioSkirt());
    for (int y = 0; y < studioHeight; ++y)
        for (int x = 0; x < studioWidth; ++x) {
            const auto i = static_cast<std::size_t>(y * studioWidth + x);
            if (fabric[i] || bodice[i] || skirt[i]) continue;
            require(same(starter.pixelAt(x, y), engine.pixelAt(x, y)),
                    "Hooks changed a pixel outside the fabric and dress at (" + std::to_string(x) +
                        "," + std::to_string(y) + ")");
        }

    Canvas updated(studioWidth, studioHeight, {0, 0, 0});
    for (StudioAction action : {StudioAction::NextMotif, StudioAction::NextPalette}) {
        StudioState changed;
        applyAction(changed, action);
        renderStudio(updated, changed, hooks);
        require(regionChanged(engine, updated, 60, 210, 690, 640), "Fabric must follow the control");
        require(regionChanged(engine, updated, 900, 285, 1000, 370), "Bodice must follow the control");
        require(regionChanged(engine, updated, 880, 390, 1030, 570), "Skirt must follow the control");
    }

    // A transformation setting reaches both views through the provider.
    const StudioHooks rotated = makePatternHooks([](const StudioState&) {
        PatternSettings s = defaultStudioPatternSettings();
        s.transform.angleRadians = kPi / 5.0;
        s.transform.scale = 1.4;
        return s;
    });
    renderStudio(updated, state, rotated);
    require(regionChanged(engine, updated, 60, 210, 690, 640), "Transforms must update the fabric");
    require(regionChanged(engine, updated, 900, 285, 1000, 570), "Transforms must update the dress");

    // Headless and desktop share renderStudio, so equal state gives equal images.
    Canvas again(studioWidth, studioHeight, {0, 0, 0});
    renderStudio(again, state, hooks);
    require(!regionChanged(engine, again, 0, 0, studioWidth, studioHeight),
            "Rendering must be deterministic");

    // Every motif and palette combination renders.
    for (int m = 0; m < studioMotifCount; ++m)
        for (int p = 0; p < studioPaletteCount; ++p) {
            StudioState s;
            s.motif = m;
            s.palette = p;
            renderStudio(updated, s, hooks);
        }
}

bool parses(std::vector<const char*> args, PatternSceneOptions& options) {
    args.insert(args.begin(), "program");
    const int argc = static_cast<int>(args.size());
    for (int i = 1; i < argc;) {
        if (!parsePatternSceneOption(argc, args.data(), i, options)) return false;
    }
    return true;
}

void testSceneOptions() {
    PatternSceneOptions o;
    require(parses({"--motif", "2", "--palette", "1", "--angle", "90", "--scale", "1.5", "--pitch",
                    "40", "50", "--translate", "-3", "4.5", "--reflect-x", "--reflect-y"},
                   o),
            "All scene options must parse");
    require(o.state.motif == 2 && o.state.palette == 1, "Motif and palette must be set");
    require(near(o.settings.transform.angleRadians, kPi / 2.0), "Degrees must convert to radians");
    require(near(radiansToDegrees(o.settings.transform.angleRadians), 90.0), "Radians convert back");
    require(near(o.settings.transform.scale, 1.5) && near(o.settings.tilePitch.x, 40) &&
                near(o.settings.tilePitch.y, 50),
            "Scale and pitch must be set");
    require(near(o.settings.transform.translation.x, -3) && near(o.settings.transform.translation.y, 4.5),
            "Translation must accept negative and fractional values");
    require(o.settings.transform.reflectX && o.settings.transform.reflectY, "Reflections must be set");
    require(o.settings.alternateTiles, "Options start from the studio defaults");

    PatternSceneOptions other;
    require(!parses({"output.ppm"}, other), "Non-option arguments must be left to the caller");
    for (const std::vector<const char*>& bad :
         std::vector<std::vector<const char*>>{{"--motif", "3"}, {"--motif", "1.5"}, {"--palette", "-1"},
                                               {"--angle", "abc"}, {"--angle", "inf"}, {"--scale", "0.05"},
                                               {"--scale", "25"}, {"--pitch", "6", "40"}, {"--pitch", "40"},
                                               {"--translate", "1"}, {"--motif"}, {"--scale", "2x"}}) {
        PatternSceneOptions rejected;
        require(throwsInvalid([&] { parses(bad, rejected); }),
                std::string("Invalid option must be rejected: ") + bad[0]);
    }

    // Every accepted option combination renders in both views without throwing.
    PatternSceneOptions edge;
    require(parses({"--scale", "0.1", "--pitch", "8", "8", "--angle", "-720"}, edge), "Bounds parse");
    Canvas canvas(studioWidth, studioHeight, {0, 0, 0});
    const PatternSettings settings = edge.settings;
    renderStudio(canvas, edge.state, makePatternHooks([settings](const StudioState&) { return settings; }));
}

} // namespace

int main() {
    const std::vector<std::pair<const char*, void (*)()>> tests{
        {"matrix order", testMatrixOrder},
        {"validation", testValidation},
        {"triangulation", testTriangulation},
        {"segment clipping", testSegmentClipping},
        {"inside/outside/crossing", testInsideOutsideCrossing},
        {"rotated and reflected edge tiles", testRotatedAndReflectedEdgeTiles},
        {"conservative tile range", testConservativeTileRange},
        {"concave motif at slanted edge", testConcaveMotifAtSlantedEdge},
        {"stroke near slanted edge", testStrokeNearSlantedEdge},
        {"no pixels outside destination", testNoPixelsOutside},
        {"continuity across the waist", testContinuityAcrossWaist},
        {"studio hooks", testStudioHooks},
        {"scene options", testSceneOptions},
    };
    int failures = 0;
    for (const auto& [name, test] : tests) {
        try {
            test();
            std::cout << "PASS " << name << '\n';
        } catch (const std::exception& e) {
            ++failures;
            std::cerr << "FAIL " << name << ": " << e.what() << '\n';
        }
    }
    if (failures == 0) std::cout << "All pattern engine checks passed.\n";
    return failures == 0 ? 0 : 1;
}
