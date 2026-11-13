#include "kitenge/pattern.hpp"

#include "kitenge/lines.hpp"
#include "kitenge/polygon_clip.hpp"
#include "kitenge/polygon_fill.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <string>

namespace kitenge {
namespace {

// Same tolerance as the CP1 clipper: values this close to an edge count as on it.
constexpr double kEpsilon = 1e-9;

// --- Affine maths ------------------------------------------------------------
//
// A 2D affine map stored as the top two rows of a 3x3 matrix acting on column
// vectors (x, y, 1):
//     | a  b  tx |
//     | c  d  ty |
//     | 0  0  1  |
// Kept private: it exists only to apply the agreed order until Jeremy's Mat3
// module is shared.
struct Affine {
    double a = 1.0, b = 0.0, tx = 0.0;
    double c = 0.0, d = 1.0, ty = 0.0;
};

Affine multiply(const Affine& left, const Affine& right) {
    // left * right: `right` acts first.
    Affine r;
    r.a = left.a * right.a + left.b * right.c;
    r.b = left.a * right.b + left.b * right.d;
    r.tx = left.a * right.tx + left.b * right.ty + left.tx;
    r.c = left.c * right.a + left.d * right.c;
    r.d = left.c * right.b + left.d * right.d;
    r.ty = left.c * right.tx + left.d * right.ty + left.ty;
    return r;
}

Affine translation(Point2D offset) {
    Affine m;
    m.tx = offset.x;
    m.ty = offset.y;
    return m;
}

Point2D apply(const Affine& m, Point2D p) {
    return {m.a * p.x + m.b * p.y + m.tx, m.c * p.x + m.d * p.y + m.ty};
}

// T(pivot) * R(angle) * F(reflection) * S(scale) * T(-pivot).
Affine localTransform(const PatternTransform& t) {
    Affine rotate;
    rotate.a = std::cos(t.angleRadians);
    rotate.b = -std::sin(t.angleRadians);
    rotate.c = std::sin(t.angleRadians);
    rotate.d = std::cos(t.angleRadians);

    Affine reflect;
    reflect.a = t.reflectX ? -1.0 : 1.0;
    reflect.d = t.reflectY ? -1.0 : 1.0;

    Affine scale;
    scale.a = t.scale;
    scale.d = t.scale;

    Affine m = translation(t.pivot);
    m = multiply(m, rotate);
    m = multiply(m, reflect);
    m = multiply(m, scale);
    return multiply(m, translation({-t.pivot.x, -t.pivot.y}));
}

// Origin of tile (column, row): origin + translation + tile offset.
Point2D tileOrigin(const PatternSettings& s, long long column, long long row) {
    return {s.origin.x + s.transform.translation.x + static_cast<double>(column) * s.tilePitch.x,
            s.origin.y + s.transform.translation.y + static_cast<double>(row) * s.tilePitch.y};
}

// --- Validation --------------------------------------------------------------

bool finite(Point2D p) { return std::isfinite(p.x) && std::isfinite(p.y); }

void validateSettings(const PatternSettings& s) {
    const PatternTransform& t = s.transform;
    if (!finite(s.tilePitch) || s.tilePitch.x < kMinTilePitch || s.tilePitch.y < kMinTilePitch) {
        throw std::invalid_argument("Tile pitch must be finite and at least " +
                                    std::to_string(kMinTilePitch) + " pixels on both axes.");
    }
    if (!std::isfinite(t.scale) || t.scale < kMinPatternScale || t.scale > kMaxPatternScale) {
        throw std::invalid_argument("Pattern scale must be finite and within the supported range.");
    }
    if (!std::isfinite(t.angleRadians) || !finite(t.translation) || !finite(t.pivot) ||
        !finite(s.origin)) {
        throw std::invalid_argument("Pattern angle, translation, pivot and origin must be finite.");
    }
}

double cross(Point2D a, Point2D b, Point2D p) {
    return (b.x - a.x) * (p.y - a.y) - (b.y - a.y) * (p.x - a.x);
}

double signedArea(const Polygon& polygon) {
    double twice = 0.0;
    for (std::size_t i = 0; i < polygon.size(); ++i) {
        const Point2D& p = polygon[i];
        const Point2D& q = polygon[(i + 1) % polygon.size()];
        twice += p.x * q.y - q.x * p.y;
    }
    return twice / 2.0;
}

// +1 or -1 so that orientation * cross(edge, p) >= 0 for points inside.
double convexOrientation(const Polygon& boundary) {
    if (boundary.size() < 3) {
        throw std::invalid_argument("Destination needs at least three vertices.");
    }
    for (const Point2D& p : boundary) {
        if (!finite(p)) throw std::invalid_argument("Destination vertices must be finite.");
    }
    const double area = signedArea(boundary);
    if (std::abs(area) <= kEpsilon) throw std::invalid_argument("Destination has no area.");
    const double orientation = area > 0.0 ? 1.0 : -1.0;
    const std::size_t n = boundary.size();
    for (std::size_t i = 0; i < n; ++i) {
        if (orientation * cross(boundary[i], boundary[(i + 1) % n], boundary[(i + 2) % n]) <
            -kEpsilon) {
            throw std::invalid_argument("Destination must be convex.");
        }
    }
    return orientation;
}

bool insideConvex(const Polygon& boundary, double orientation, Point2D p) {
    const std::size_t n = boundary.size();
    for (std::size_t i = 0; i < n; ++i) {
        if (orientation * cross(boundary[i], boundary[(i + 1) % n], p) < -kEpsilon) return false;
    }
    return true;
}

bool isConvexPolygon(const Polygon& polygon) {
    const double orientation = signedArea(polygon) > 0.0 ? 1.0 : -1.0;
    const std::size_t n = polygon.size();
    for (std::size_t i = 0; i < n; ++i) {
        if (orientation * cross(polygon[i], polygon[(i + 1) % n], polygon[(i + 2) % n]) <
            -kEpsilon) {
            return false;
        }
    }
    return true;
}

Rect bounds(const Polygon& points) {
    Rect r{std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity(),
           -std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity()};
    for (const Point2D& p : points) {
        r.minX = std::min(r.minX, p.x);
        r.minY = std::min(r.minY, p.y);
        r.maxX = std::max(r.maxX, p.x);
        r.maxY = std::max(r.maxY, p.y);
    }
    return r;
}

// A motif prepared once per build: each fill keeps its original contour (used
// for tiles fully inside the destination) and the convex parts used for
// clipping (the contour itself when convex, otherwise its triangles).
struct PreparedFill {
    Polygon contour;
    std::vector<Polygon> convexParts;
    int role;
};

struct PreparedMotif {
    std::vector<PreparedFill> fills;
    std::vector<MotifStroke> strokes;
    Rect localBounds{0, 0, -1, -1}; // bounds after the local transform
    bool empty = true;
};

PreparedMotif prepareMotif(const MotifShape& motif, const Affine& local) {
    PreparedMotif prepared;
    Polygon transformed;
    for (const MotifFill& fill : motif.fills) {
        if (fill.role < 0) throw std::invalid_argument("Motif palette roles must be non-negative.");
        if (!isValidPolygon(fill.contour)) {
            throw std::invalid_argument("Motif fill contours must be simple polygons with area.");
        }
        PreparedFill p{fill.contour, {}, fill.role};
        if (isConvexPolygon(fill.contour)) {
            p.convexParts.push_back(fill.contour);
        } else {
            p.convexParts = triangulateSimplePolygon(fill.contour);
        }
        for (const Point2D& v : fill.contour) transformed.push_back(apply(local, v));
        prepared.fills.push_back(std::move(p));
    }
    for (const MotifStroke& stroke : motif.strokes) {
        if (stroke.role < 0) throw std::invalid_argument("Motif palette roles must be non-negative.");
        if (stroke.points.size() < 2) {
            throw std::invalid_argument("Motif strokes need at least two points.");
        }
        for (const Point2D& v : stroke.points) {
            if (!finite(v)) throw std::invalid_argument("Motif stroke points must be finite.");
            transformed.push_back(apply(local, v));
        }
        prepared.strokes.push_back(stroke);
    }
    if (!transformed.empty()) {
        prepared.localBounds = bounds(transformed);
        prepared.empty = false;
    }
    return prepared;
}

TileRange rangeFor(const PreparedMotif& motif, const PatternSettings& s, const Rect& dest) {
    TileRange range;
    if (motif.empty) return range;
    // Tile (c, r) covers [O + c*pitch + localMin, O + c*pitch + localMax] on
    // each axis. Solve for the tiles whose extent meets the destination bounds,
    // widened by one pixel so rounding at the raster boundary is covered too.
    constexpr double margin = 1.0;
    const Point2D o = tileOrigin(s, 0, 0);
    const double c0 = std::ceil((dest.minX - margin - o.x - motif.localBounds.maxX) / s.tilePitch.x);
    const double c1 = std::floor((dest.maxX + margin - o.x - motif.localBounds.minX) / s.tilePitch.x);
    const double r0 = std::ceil((dest.minY - margin - o.y - motif.localBounds.maxY) / s.tilePitch.y);
    const double r1 = std::floor((dest.maxY + margin - o.y - motif.localBounds.minY) / s.tilePitch.y);
    if (c1 < c0 || r1 < r0) return range;
    // Check the size in doubles first so huge ranges cannot overflow.
    const double tiles = (c1 - c0 + 1.0) * (r1 - r0 + 1.0);
    if (!std::isfinite(tiles) || tiles > static_cast<double>(kMaxPatternTiles)) {
        throw std::invalid_argument("Pattern settings need too many tiles; increase the pitch "
                                    "or reduce the destination.");
    }
    range.firstColumn = static_cast<long long>(c0);
    range.lastColumn = static_cast<long long>(c1);
    range.firstRow = static_cast<long long>(r0);
    range.lastRow = static_cast<long long>(r1);
    return range;
}

int tileRole(int role, bool alternate, long long column, long long row) {
    if (alternate && (column + row) % 2 != 0) {
        if (role == 1) return 2;
        if (role == 2) return 1;
    }
    return role;
}

} // namespace

// --- Public geometry ------------------------------------------------------------

Point2D patternPoint(const PatternSettings& settings, long long column, long long row,
                     Point2D local) {
    const Affine m = multiply(translation(tileOrigin(settings, column, row)),
                              localTransform(settings.transform));
    return apply(m, local);
}

std::vector<Polygon> triangulateSimplePolygon(const Polygon& polygon) {
    if (!isValidPolygon(polygon)) {
        throw std::invalid_argument("Only simple polygons with area can be triangulated.");
    }
    const double orientation = signedArea(polygon) > 0.0 ? 1.0 : -1.0;
    std::vector<std::size_t> remaining(polygon.size());
    for (std::size_t i = 0; i < remaining.size(); ++i) remaining[i] = i;

    std::vector<Polygon> triangles;
    while (remaining.size() > 3) {
        const std::size_t n = remaining.size();
        bool removed = false;
        for (std::size_t i = 0; i < n && !removed; ++i) {
            const Point2D& prev = polygon[remaining[(i + n - 1) % n]];
            const Point2D& cur = polygon[remaining[i]];
            const Point2D& next = polygon[remaining[(i + 1) % n]];
            const double turn = orientation * cross(prev, cur, next);
            if (std::abs(turn) <= kEpsilon) {
                // Collinear vertex: it adds no area, so drop it.
                remaining.erase(remaining.begin() + static_cast<std::ptrdiff_t>(i));
                removed = true;
                break;
            }
            if (turn < 0.0) continue; // reflex vertex cannot be an ear tip
            bool ear = true;
            for (std::size_t k = 0; k < n && ear; ++k) {
                if (k == i || k == (i + 1) % n || k == (i + n - 1) % n) continue;
                const Point2D& p = polygon[remaining[k]];
                // Points on the candidate triangle also block it (conservative).
                if (orientation * cross(prev, cur, p) >= -kEpsilon &&
                    orientation * cross(cur, next, p) >= -kEpsilon &&
                    orientation * cross(next, prev, p) >= -kEpsilon) {
                    ear = false;
                }
            }
            if (!ear) continue;
            triangles.push_back({prev, cur, next});
            remaining.erase(remaining.begin() + static_cast<std::ptrdiff_t>(i));
            removed = true;
        }
        if (!removed) {
            // Every simple polygon has two ears, so this means invalid input
            // slipped past validation (for example, near-duplicate vertices).
            throw std::invalid_argument("Polygon could not be triangulated.");
        }
    }
    const Polygon last{polygon[remaining[0]], polygon[remaining[1]], polygon[remaining[2]]};
    if (std::abs(signedArea(last)) > kEpsilon) triangles.push_back(last);
    return triangles;
}

std::optional<std::pair<Point2D, Point2D>>
clipSegmentToConvex(Point2D start, Point2D end, const Polygon& boundary) {
    const double orientation = convexOrientation(boundary);
    if (!finite(start) || !finite(end)) {
        throw std::invalid_argument("Segment endpoints must be finite.");
    }
    // Inside distance along the segment is linear in t: f(t) = f0 + t (f1 - f0).
    // Each edge keeps the t values where f(t) >= 0.
    double tEnter = 0.0;
    double tLeave = 1.0;
    const std::size_t n = boundary.size();
    for (std::size_t i = 0; i < n; ++i) {
        const Point2D& a = boundary[i];
        const Point2D& b = boundary[(i + 1) % n];
        const double f0 = orientation * cross(a, b, start);
        const double f1 = orientation * cross(a, b, end);
        const double change = f1 - f0;
        if (std::abs(change) <= kEpsilon) {
            if (f0 < -kEpsilon) return std::nullopt; // parallel and outside
            continue;                                // parallel and inside
        }
        const double t = -f0 / change;
        if (change > 0.0) {
            tEnter = std::max(tEnter, t); // entering this half-plane
        } else {
            tLeave = std::min(tLeave, t); // leaving this half-plane
        }
        if (tEnter > tLeave) return std::nullopt;
    }
    const auto at = [&](double t) {
        if (t <= 0.0) return start;
        if (t >= 1.0) return end;
        return Point2D{start.x + t * (end.x - start.x), start.y + t * (end.y - start.y)};
    };
    return std::make_pair(at(tEnter), at(tLeave));
}

TileRange patternTileRange(const MotifShape& motif, const PatternSettings& settings,
                           const Polygon& destination) {
    validateSettings(settings);
    convexOrientation(destination);
    const PreparedMotif prepared = prepareMotif(motif, localTransform(settings.transform));
    return rangeFor(prepared, settings, bounds(destination));
}

PatternGeometry buildPatternGeometry(const MotifShape& motif, const PatternSettings& settings,
                                     const Polygon& destination) {
    validateSettings(settings);
    const double orientation = convexOrientation(destination);
    const Affine local = localTransform(settings.transform);
    const PreparedMotif prepared = prepareMotif(motif, local);
    const Rect dest = bounds(destination);

    PatternGeometry geometry;
    geometry.tiles = rangeFor(prepared, settings, dest);
    if (geometry.tiles.empty()) return geometry;

    for (long long row = geometry.tiles.firstRow; row <= geometry.tiles.lastRow; ++row) {
        for (long long column = geometry.tiles.firstColumn; column <= geometry.tiles.lastColumn;
             ++column) {
            const Point2D o = tileOrigin(settings, column, row);
            const Rect tile{o.x + prepared.localBounds.minX, o.y + prepared.localBounds.minY,
                            o.x + prepared.localBounds.maxX, o.y + prepared.localBounds.maxY};
            if (tile.maxX < dest.minX || tile.minX > dest.maxX || tile.maxY < dest.minY ||
                tile.minY > dest.maxY) {
                continue; // the range is a rectangle of tiles; skip corner misses
            }
            ++geometry.tilesVisited;
            // If the tile's bounding box is inside the convex destination, so is
            // everything in the tile: no clipping is needed.
            const bool inside = insideConvex(destination, orientation, {tile.minX, tile.minY}) &&
                                insideConvex(destination, orientation, {tile.maxX, tile.minY}) &&
                                insideConvex(destination, orientation, {tile.maxX, tile.maxY}) &&
                                insideConvex(destination, orientation, {tile.minX, tile.maxY});
            if (!inside) ++geometry.tilesClipped;
            const Affine m = multiply(translation(o), local);
            const auto map = [&](const Polygon& points) {
                Polygon out;
                out.reserve(points.size());
                for (const Point2D& p : points) out.push_back(apply(m, p));
                return out;
            };

            for (const PreparedFill& fill : prepared.fills) {
                const int role = tileRole(fill.role, settings.alternateTiles, column, row);
                if (inside) {
                    geometry.pieces.push_back(
                        {PatternPiece::Kind::Fill, map(fill.contour), role, column, row});
                    continue;
                }
                for (const Polygon& part : fill.convexParts) {
                    Polygon clipped = clipPolygonToConvex(map(part), destination);
                    if (clipped.empty()) continue;
                    if (!isValidPolygon(clipped)) {
                        ++geometry.discardedSlivers;
                        continue;
                    }
                    geometry.pieces.push_back(
                        {PatternPiece::Kind::Fill, std::move(clipped), role, column, row});
                }
            }

            for (const MotifStroke& stroke : prepared.strokes) {
                const int role = tileRole(stroke.role, settings.alternateTiles, column, row);
                const std::size_t count = stroke.points.size();
                const std::size_t segments = stroke.closed ? count : count - 1;
                for (std::size_t i = 0; i < segments; ++i) {
                    const Point2D a = apply(m, stroke.points[i]);
                    const Point2D b = apply(m, stroke.points[(i + 1) % count]);
                    if (inside) {
                        geometry.pieces.push_back({PatternPiece::Kind::Stroke, {a, b}, role, column, row});
                        continue;
                    }
                    if (auto part = clipSegmentToConvex(a, b, destination)) {
                        geometry.pieces.push_back({PatternPiece::Kind::Stroke,
                                                   {part->first, part->second}, role, column, row});
                    }
                }
            }
        }
    }
    return geometry;
}

void renderPattern(Canvas& canvas, const MotifShape& motif, const std::vector<Color>& palette,
                   const PatternSettings& settings, const Polygon& destination) {
    if (palette.empty()) throw std::invalid_argument("Pattern palette needs a background colour.");
    const int colours = static_cast<int>(palette.size());
    // Check each role as drawn on even and odd tiles before touching the canvas.
    const auto checkRole = [&](int role) {
        for (int r : {role, tileRole(role, settings.alternateTiles, 0, 1)}) {
            if (r < 0 || r >= colours) {
                throw std::invalid_argument("Motif palette role is outside the palette.");
            }
        }
    };
    for (const MotifFill& f : motif.fills) checkRole(f.role);
    for (const MotifStroke& s : motif.strokes) checkRole(s.role);

    const PatternGeometry geometry = buildPatternGeometry(motif, settings, destination);

    // Work inside the destination's pixel bounding box, clamped to the canvas.
    const Rect dest = bounds(destination);
    const long long x0 = std::max(0LL, static_cast<long long>(std::floor(dest.minX)));
    const long long y0 = std::max(0LL, static_cast<long long>(std::floor(dest.minY)));
    const long long x1 = std::min<long long>(canvas.width() - 1, static_cast<long long>(std::ceil(dest.maxX)));
    const long long y1 = std::min<long long>(canvas.height() - 1, static_cast<long long>(std::ceil(dest.maxY)));
    if (x1 < x0 || y1 < y0) return;
    const int width = static_cast<int>(x1 - x0 + 1);
    const int height = static_cast<int>(y1 - y0 + 1);
    const auto shift = [&](const Polygon& points) {
        Polygon out;
        out.reserve(points.size());
        for (const Point2D& p : points) {
            out.push_back({p.x - static_cast<double>(x0), p.y - static_cast<double>(y0)});
        }
        return out;
    };

    // The mask uses the same scanline filler as the shell, so "inside the
    // destination" means exactly the pixels the panel itself would fill.
    constexpr Color off{0, 0, 0};
    constexpr Color on{255, 255, 255};
    Canvas mask(width, height, off);
    fillPolygon(mask, shift(destination), on);

    // Draw the clipped pattern on a layer that starts as the background colour.
    Canvas layer(width, height, palette[0]);
    for (const PatternPiece& piece : geometry.pieces) {
        const Color colour = palette[static_cast<std::size_t>(piece.role)];
        const Polygon local = shift(piece.points);
        if (piece.kind == PatternPiece::Kind::Fill) {
            fillPolygon(layer, local, colour);
        } else {
            drawLine(layer,
                     {static_cast<int>(std::lround(local[0].x)), static_cast<int>(std::lround(local[0].y))},
                     {static_cast<int>(std::lround(local[1].x)), static_cast<int>(std::lround(local[1].y))},
                     colour);
        }
    }

    // Copy only destination pixels to the canvas.
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const Color m = mask.pixelAt(x, y);
            if (m.red == on.red) {
                canvas.setPixel(static_cast<int>(x0) + x, static_cast<int>(y0) + y, layer.pixelAt(x, y));
            }
        }
    }
}

// --- Studio integration -----------------------------------------------------------

MotifShape starterMotifShape(int index) {
    // Same outlines as the starter scene, now as local data centred on (0,0).
    switch (index) {
    case 0:
        return {{{{{0, -25}, {22, 0}, {0, 25}, {-22, 0}}, 1}}, {}};
    case 1:
        return {{{{{0, -22}, {21, 19}, {-21, 19}}, 1}}, {}};
    case 2:
        return {{{{{-22, -10}, {-8, -10}, {-8, -23}, {8, -23}, {8, -10}, {22, -10},
                   {22, 10}, {8, 10}, {8, 23}, {-8, 23}, {-8, 10}, {-22, 10}},
                  1}},
                {}};
    default:
        throw std::out_of_range("Unknown starter motif.");
    }
}

std::vector<Color> patternPaletteFor(int studioPaletteIndex) {
    return paletteRoleColors(studioPaletteIndex);
}

PatternSettings defaultStudioPatternSettings() {
    PatternSettings s;
    s.tilePitch = {64.0, 70.0};
    s.alternateTiles = true;
    return s;
}

PatternSettings garmentPatternSettings(const PatternSettings& base) {
    // Zoom the whole repeat about the garment origin: scale, pitch and the
    // user's translation all shrink together, so the dress shows the same
    // design as the fabric preview at a size that suits the mannequin.
    PatternSettings s = base;
    s.origin = kGarmentPatternOrigin;
    s.transform.scale *= kGarmentPatternZoom;
    s.transform.translation = {base.transform.translation.x * kGarmentPatternZoom,
                               base.transform.translation.y * kGarmentPatternZoom};
    s.tilePitch = {base.tilePitch.x * kGarmentPatternZoom, base.tilePitch.y * kGarmentPatternZoom};
    return s;
}

StudioHooks makePatternHooks(PatternSettingsProvider provider) {
    const auto settingsFor = [provider](const StudioState& state) {
        return provider ? provider(state) : defaultStudioPatternSettings();
    };
    StudioHooks hooks;
    hooks.fabric = [settingsFor](Canvas& canvas, const StudioState& state) {
        PatternSettings s = settingsFor(state);
        s.origin = kFabricPatternOrigin;
        renderPattern(canvas, catalogMotif(state.motif), patternPaletteFor(state.palette), s,
                      studioFabricArea());
    };
    hooks.garment = [settingsFor](Canvas& canvas, const StudioState& state) {
        // One settings value for both panels keeps the repeat continuous at the waist.
        const PatternSettings s = garmentPatternSettings(settingsFor(state));
        const MotifShape& motif = catalogMotif(state.motif);
        const std::vector<Color> palette = patternPaletteFor(state.palette);
        renderPattern(canvas, motif, palette, s, studioBodice());
        renderPattern(canvas, motif, palette, s, studioSkirt());
    };
    return hooks;
}

// --- Scene options ---------------------------------------------------------------

double degreesToRadians(double degrees) { return degrees * 3.14159265358979323846 / 180.0; }
double radiansToDegrees(double radians) { return radians * 180.0 / 3.14159265358979323846; }

namespace {

double parseNumber(const std::string& flag, const char* text) {
    const std::string value = text;
    std::size_t used = 0;
    double number = 0.0;
    try {
        number = std::stod(value, &used);
    } catch (const std::exception&) {
        used = 0;
    }
    if (used == 0 || used != value.size() || !std::isfinite(number)) {
        throw std::invalid_argument(flag + " needs a finite number, not '" + value + "'");
    }
    return number;
}

int parseIndex(const std::string& flag, const char* text, int count) {
    const double number = parseNumber(flag, text);
    if (number != std::floor(number) || number < 0 || number >= count) {
        throw std::invalid_argument(flag + " must be a whole number from 0 to " +
                                    std::to_string(count - 1));
    }
    return static_cast<int>(number);
}

} // namespace

bool parsePatternSceneOption(int argc, const char* const* argv, int& index,
                             PatternSceneOptions& options) {
    const std::string flag = argv[index];
    const auto value = [&](int offset) -> const char* {
        if (index + offset >= argc) throw std::invalid_argument(flag + " is missing a value");
        return argv[index + offset];
    };
    PatternSettings& s = options.settings;
    if (flag == "--motif") {
        options.state.motif = parseIndex(flag, value(1), studioMotifCount);
        index += 1;
    } else if (flag == "--palette") {
        options.state.palette = parseIndex(flag, value(1), studioPaletteCount);
        index += 1;
    } else if (flag == "--angle") {
        s.transform.angleRadians = degreesToRadians(parseNumber(flag, value(1)));
        index += 1;
    } else if (flag == "--scale") {
        const double scale = parseNumber(flag, value(1));
        // The garment view multiplies by kGarmentPatternZoom, so both views
        // must stay within the supported range.
        if (scale > kMaxPatternScale || scale * kGarmentPatternZoom < kMinPatternScale) {
            throw std::invalid_argument("--scale must be from " +
                                        std::to_string(kMinPatternScale / kGarmentPatternZoom) +
                                        " to " + std::to_string(kMaxPatternScale));
        }
        s.transform.scale = scale;
        index += 1;
    } else if (flag == "--pitch") {
        const Point2D pitch{parseNumber(flag, value(1)), parseNumber(flag, value(2))};
        if (pitch.x * kGarmentPatternZoom < kMinTilePitch || pitch.y * kGarmentPatternZoom < kMinTilePitch) {
            throw std::invalid_argument("--pitch values must be at least " +
                                        std::to_string(kMinTilePitch / kGarmentPatternZoom));
        }
        s.tilePitch = pitch;
        index += 2;
    } else if (flag == "--translate") {
        s.transform.translation = {parseNumber(flag, value(1)), parseNumber(flag, value(2))};
        index += 2;
    } else if (flag == "--reflect-x") {
        s.transform.reflectX = true;
    } else if (flag == "--reflect-y") {
        s.transform.reflectY = true;
    } else {
        return false;
    }
    ++index;
    return true;
}

const char* patternSceneOptionsUsage() {
    return "  --motif N          starter motif 0-2\n"
           "  --palette N        palette 0-2\n"
           "  --angle DEGREES    pattern rotation, clockwise on screen\n"
           "  --scale S          pattern scale (0.1 to 20)\n"
           "  --pitch X Y        tile pitch in pixels (at least 8)\n"
           "  --translate X Y    pattern translation in pixels\n"
           "  --reflect-x        mirror motifs left-right\n"
           "  --reflect-y        mirror motifs top-bottom\n";
}

} // namespace kitenge
