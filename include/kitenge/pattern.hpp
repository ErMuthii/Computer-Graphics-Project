#pragma once

// Reusable repeat-pattern renderer for the fabric swatch and the dress panels.
// Owner: Murega Kelvin Mutwiri, 166914.
//
// The renderer tiles one motif over a convex destination region. Every tile is
// built from a fresh copy of the motif in local coordinates, transformed with
// the agreed matrix order, clipped geometrically against the destination with
// the CP1 Sutherland-Hodgman clipper (fills) or Cyrus-Beck (strokes), and then
// rasterized with the CP1 scanline filler and Bresenham line drawer. A
// destination pixel mask is applied last, so no pattern pixel is ever written
// outside the destination, even where rounding a clipped stroke endpoint lands
// on a pixel whose centre lies just outside a slanted edge.
//
// Coordinates follow the shared contract: origin top left, x right, y down.
// All geometry stays in doubles; values are rounded only when rasterizing.
// Nothing here depends on GLFW or OpenGL.
//
// INTEGRATION NOTE. The shared contracts in docs/team/README.md give Jeremy the
// matrix/transform-parameter type and Deborah the motif and palette data. Those
// modules were not on main when this engine was written, so `PatternTransform`
// and `MotifShape` below are interim inputs that follow the agreed contract
// field for field. When the shared types merge, they replace these two
// structures (or become aliases of them) without changing the algorithm.

#include "kitenge/canvas.hpp"
#include "kitenge/studio.hpp"
#include "kitenge/types.hpp"

#include <cstddef>
#include <functional>
#include <optional>
#include <utility>
#include <vector>

namespace kitenge {

// --- Inputs ---------------------------------------------------------------

// Transform parameters from the shared contract. Angles are radians. Because
// screen y points down, a positive angle appears clockwise on the canvas.
// `pivot` is in motif-local coordinates. `reflectX` mirrors x (x -> -x, a
// left-right flip about the pivot); `reflectY` mirrors y (an up-down flip).
// `translation` moves the pattern's origin; it never moves the destination.
struct PatternTransform {
    Point2D translation{0.0, 0.0};
    double angleRadians = 0.0;
    double scale = 1.0;
    Point2D pivot{0.0, 0.0};
    bool reflectX = false;
    bool reflectY = false;
};

// Motif data in local coordinates centred on (0,0), nominally within
// [-25, 25] on both axes. Filled contours must be simple polygons without
// holes; they may be concave. Strokes are polylines (curves are sampled to
// polylines before they reach the engine). `role` indexes the palette.
struct MotifFill {
    Polygon contour;
    int role = 1;
};

struct MotifStroke {
    std::vector<Point2D> points;
    bool closed = false;
    int role = 2;
};

struct MotifShape {
    std::vector<MotifFill> fills;     // drawn in order
    std::vector<MotifStroke> strokes; // drawn after the fills of the same tile
};

struct PatternSettings {
    PatternTransform transform;
    // Distance between neighbouring tiles; both components must be finite and
    // at least kMinTilePitch.
    Point2D tilePitch{64.0, 70.0};
    // Canvas position of tile (0, 0) before `transform.translation` is added.
    // Panels that must line up (bodice and skirt) share one origin.
    Point2D origin{0.0, 0.0};
    // Swap palette roles 1 and 2 on tiles whose column + row is odd, giving the
    // checkerboard colouring of the starter fabric.
    bool alternateTiles = false;
};

// Bounds that keep the work per render predictable. The UI should clamp its
// scale and spacing controls to these values.
inline constexpr double kMinTilePitch = 4.0;
inline constexpr double kMinPatternScale = 0.05;
inline constexpr double kMaxPatternScale = 20.0;
inline constexpr long long kMaxPatternTiles = 20000;

// --- Geometry --------------------------------------------------------------

// Inclusive column/row range of tiles that can touch a destination.
struct TileRange {
    long long firstColumn = 0;
    long long lastColumn = -1;
    long long firstRow = 0;
    long long lastRow = -1;

    bool empty() const noexcept { return lastColumn < firstColumn || lastRow < firstRow; }
    long long count() const noexcept {
        return empty() ? 0 : (lastColumn - firstColumn + 1) * (lastRow - firstRow + 1);
    }
};

// Maps a motif-local point into canvas coordinates for tile (column, row):
//   p_canvas = T(origin + translation + tileOffset) * T(pivot) * R(angle)
//              * F(reflection) * S(scale) * T(-pivot) * p
// where tileOffset = (column * pitch.x, row * pitch.y). The rightmost
// operation acts first, so the tile offset is added after the local transform.
Point2D patternPoint(const PatternSettings& settings, long long column, long long row,
                     Point2D local);

// Splits a simple polygon (convex or concave, no holes) into triangles by ear
// clipping. Collinear vertices are dropped. Throws std::invalid_argument when
// the polygon is not simple (see isValidPolygon).
std::vector<Polygon> triangulateSimplePolygon(const Polygon& polygon);

// Clips a segment against a convex boundary (either winding) with the
// Cyrus-Beck parametric method. Points on the boundary count as inside.
// Returns nothing when no part of the segment is inside. Throws
// std::invalid_argument when the boundary is not a convex polygon with area.
std::optional<std::pair<Point2D, Point2D>>
clipSegmentToConvex(Point2D start, Point2D end, const Polygon& boundary);

// Conservative range of tiles whose transformed motif can overlap the
// destination's bounding box. It is computed from the transformed motif's
// actual extent, so rotation, reflection, scale and translation are covered.
// Throws std::invalid_argument for invalid settings, motifs or destinations,
// or when the range exceeds kMaxPatternTiles.
TileRange patternTileRange(const MotifShape& motif, const PatternSettings& settings,
                           const Polygon& destination);

// One piece of clipped geometry in canvas coordinates. A fill piece is a
// simple polygon that passes isValidPolygon; a stroke piece is a segment
// (exactly two points). `role` already includes the alternateTiles swap.
struct PatternPiece {
    enum class Kind { Fill, Stroke };
    Kind kind = Kind::Fill;
    Polygon points;
    int role = 0;
    long long column = 0;
    long long row = 0;
};

struct PatternGeometry {
    TileRange tiles;
    std::vector<PatternPiece> pieces; // in drawing order
    std::size_t tilesVisited = 0;     // tiles whose bounds met the destination
    std::size_t tilesClipped = 0;     // tiles that needed geometric clipping
    std::size_t discardedSlivers = 0; // clipped fills too thin to be valid
};

// Builds the clipped pattern geometry without drawing it. Concave fill
// contours that cross the destination edge are triangulated before clipping,
// so every clipped piece is convex, simple and never split into disconnected
// parts. Tiles whose transformed bounds lie fully inside the destination keep
// their original contours unclipped.
PatternGeometry buildPatternGeometry(const MotifShape& motif, const PatternSettings& settings,
                                     const Polygon& destination);

// Fills the destination with palette[0], then draws the clipped pattern.
// Every motif role must index `palette`. Pixels outside the destination (by
// the scanline filler's pixel-centre rule) are never written.
void renderPattern(Canvas& canvas, const MotifShape& motif, const std::vector<Color>& palette,
                   const PatternSettings& settings, const Polygon& destination);

// --- Studio integration ----------------------------------------------------

// The three starter motifs (diamond, triangle, cross) as local motif data, so
// the engine reproduces the starter look until Deborah's motif module lands.
// Throws std::out_of_range for an unknown index.
MotifShape starterMotifShape(int index);

// Palette roles for the engine: {background, primary, secondary}.
std::vector<Color> patternPaletteFor(int studioPaletteIndex);

// Tile pitch, transform and colouring used by both studio views. Each view
// overrides `origin`; the garment view also zooms by kGarmentPatternZoom.
PatternSettings defaultStudioPatternSettings();

inline constexpr double kGarmentPatternZoom = 0.5;
inline constexpr Point2D kFabricPatternOrigin{374.0, 425.0};
inline constexpr Point2D kGarmentPatternOrigin{950.0, 278.0};

// Supplies the user's pattern settings for a state. Jeremy's and Lenina's
// settings plug in here once StudioState carries them.
using PatternSettingsProvider = std::function<PatternSettings(const StudioState&)>;

// Settings actually used for the garment: the provider's settings with the
// pattern zoomed about the shared garment origin. Bodice and skirt use this
// one value, so the repeat continues across the waist.
PatternSettings garmentPatternSettings(const PatternSettings& base);

// Fabric and garment hooks backed by this renderer. Head, limbs, outlines and
// the background stay with the studio shell.
StudioHooks makePatternHooks(PatternSettingsProvider provider = {});

// --- Scene options shared by studio_render and kitenge_studio ----------------
//
// Both programs accept the same options, so the desktop window and the
// headless renderer can be run at identical state and settings and their
// images compared pixel for pixel:
//
//   --motif N          starter motif index, 0..studioMotifCount-1
//   --palette N        palette index, 0..studioPaletteCount-1
//   --angle DEGREES    pattern rotation (converted to radians; clockwise on screen)
//   --scale S          pattern scale, within [kMinPatternScale, kMaxPatternScale]
//   --pitch X Y        tile pitch in pixels, each at least kMinTilePitch
//   --translate X Y    pattern translation in pixels
//   --reflect-x        mirror the motif left-right
//   --reflect-y        mirror the motif top-bottom
//
// These are developer and verification options, not the final UI controls.
struct PatternSceneOptions {
    StudioState state;
    PatternSettings settings = defaultStudioPatternSettings();
};

// If argv[index] is a scene option, applies it (consuming its values and
// advancing `index` past them) and returns true. Returns false for any other
// argument. Throws std::invalid_argument for a missing or invalid value.
bool parsePatternSceneOption(int argc, const char* const* argv, int& index,
                             PatternSceneOptions& options);

// Usage text for the options above, one per line.
const char* patternSceneOptionsUsage();

// Degrees shown to the user <-> radians used by the maths.
double degreesToRadians(double degrees);
double radiansToDegrees(double radians);

} // namespace kitenge
