#pragma once

// Static 2D mannequin and its two dress panels.
// Owner: Deborah Rehana, 168656.
//
// This is the single source of the mannequin's geometry. The studio shell
// draws the figure and outlines from it, and the pattern engine clips the
// garment pattern against the same `mannequinBodice()` and `mannequinSkirt()`
// polygons, so the outline a viewer sees is exactly the clipping boundary.
//
// Coordinates are studio canvas pixels (1200 x 760): origin top left, x right,
// y down. The mannequin is fixed: no control moves or reshapes it. Pattern
// transforms move the fabric design inside the panels, never the panels.
//
// Only the bodice and skirt are patterned. The head, neck, arms, legs and
// shoes are drawn in plain colours. Bodice and skirt are separate convex
// polygons that share the waist edge, because Sutherland-Hodgman clipping
// needs a convex boundary; a single concave dress outline would not work.

#include "kitenge/canvas.hpp"
#include "kitenge/types.hpp"

#include <array>

namespace kitenge {

// An axis-aligned block of whole pixels: columns [x, x + width) and rows
// [y, y + height).
struct PixelBox {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
};

struct MannequinGeometry {
    // The head is an ellipse: a filled face and a midpoint ellipse outline.
    Point headCentre{0, 0};
    int headRadiusX = 0;
    int headRadiusY = 0;
    Polygon neck;
    std::array<Polygon, 2> arms; // left, right
    std::array<Polygon, 2> legs; // left, right
    std::array<PixelBox, 2> shoes;
    PixelBox waistband;          // drawn over the seam between bodice and skirt
    Polygon bodice;              // convex, patterned
    Polygon skirt;               // convex, patterned
};

// The one shared mannequin. References stay valid for the whole program.
const MannequinGeometry& mannequinGeometry();
const Polygon& mannequinBodice();
const Polygon& mannequinSkirt();

// True when the polygon is valid (see isValidPolygon) and convex: every turn
// has the same sign, ignoring collinear vertices. Either winding is accepted.
bool isConvexPolygon(const Polygon& polygon);

// Smallest axis-aligned rectangle containing every vertex. Throws
// std::invalid_argument for an empty polygon.
Rect polygonBounds(const Polygon& polygon);

// Draws the unpatterned figure: neck, arms, legs, face, head outline and
// shoes. It does not touch the garment, so call it before the garment.
void drawMannequinFigure(Canvas& canvas, Color skin, Color outline);

// Fills both dress panels with one plain colour (the unpatterned dress).
void drawPlainGarment(Canvas& canvas, Color cloth);

// Draws the panel outlines and the waistband. Call it after the garment, so
// the trim sits on top of the pattern.
void drawGarmentTrim(Canvas& canvas, Color colour);

// Figure, plain dress and trim in the correct order.
void drawUnpatternedMannequin(Canvas& canvas, Color skin, Color outline, Color cloth);

} // namespace kitenge
