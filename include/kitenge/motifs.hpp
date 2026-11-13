#pragma once

// Motif and palette catalog for the Kitenge Pattern Studio.
// Owner: Deborah Rehana, 168656.
//
// Motifs are data, not drawing code. Each motif is described once in its own
// local coordinates, centred on (0,0) and kept inside
// [-kMotifHalfExtent, kMotifHalfExtent] on both axes. The motif knows nothing
// about tiles, canvases or transforms: the pattern engine (Kelvin) decides
// where each copy goes, and the transform module (Jeremy) moves a fresh copy
// for every tile, so the catalog geometry is never modified.
//
// Colours are assigned by palette role rather than RGB, so one motif works
// with every palette and the fabric preview and the dress always agree:
//   role 0 = background, role 1 = primary, role 2 = accent.
//
// Filled contours are simple polygons without holes (concave is allowed);
// fills are drawn in order, so a later fill sits on top of an earlier one.
// Strokes are polylines; curves and circles are sampled into polylines or
// polygons here, before any transform, which is an approximation documented
// in docs/algorithms/motifs.md.
//
// Coordinates follow the shared contract: x right, y down.

#include "kitenge/types.hpp"

#include <vector>

namespace kitenge {

// --- Palette roles ---------------------------------------------------------

inline constexpr int kRoleBackground = 0;
inline constexpr int kRolePrimary = 1;
inline constexpr int kRoleAccent = 2;
inline constexpr int kPaletteRoleCount = 3;

// --- Motif data --------------------------------------------------------------

// These field names are the contract the pattern engine was written against.
struct MotifFill {
    Polygon contour;
    int role = kRolePrimary;
};

struct MotifStroke {
    std::vector<Point2D> points;
    bool closed = false;
    int role = kRoleAccent;
};

struct MotifShape {
    std::vector<MotifFill> fills;     // drawn in order
    std::vector<MotifStroke> strokes; // drawn after the fills of the same tile
};

inline constexpr double kMotifHalfExtent = 25.0;

// True when the motif meets the catalog contract: at least one fill or
// stroke; every fill passes isValidPolygon (finite, non-zero area, simple);
// every stroke has at least two finite points (three when closed); every
// point lies inside [-kMotifHalfExtent, kMotifHalfExtent]; and every role is
// in [0, kPaletteRoleCount).
bool isValidMotif(const MotifShape& motif);

// --- Catalog -------------------------------------------------------------------

// Counts are compile-time constants so the studio can size its selection
// controls from them instead of hardcoding "3".
inline constexpr int kMotifCount = 5;
inline constexpr int kPaletteCount = 4;

struct Palette {
    const char* label;
    Color background;
    Color primary;
    Color accent;
};

// Every lookup throws std::out_of_range for an index outside the catalog,
// so an invalid selection fails the same way everywhere.
const MotifShape& catalogMotif(int index);
// Upper-case letters and spaces only, so the studio's bitmap font can draw it.
const char* catalogMotifLabel(int index);
const Palette& catalogPalette(int index);
const char* catalogPaletteLabel(int index);

// Colour for a role. Throws std::out_of_range for a role outside
// [0, kPaletteRoleCount).
Color paletteRoleColor(const Palette& palette, int role);

// The palette as a role-indexed list {background, primary, accent}, the form
// the pattern renderer takes.
std::vector<Color> paletteRoleColors(int paletteIndex);

} // namespace kitenge
