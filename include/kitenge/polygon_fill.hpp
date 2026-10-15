#pragma once

#include "kitenge/canvas.hpp"
#include "kitenge/types.hpp"

namespace kitenge {

// True if the polygon has >= 3 finite vertices, non-zero area and no
// self-intersections (simple polygon).
bool isValidPolygon(const Polygon& polygon);

// Even-odd scanline fill. Invalid polygons draw nothing.
// Pixel (x, y) is filled if its center (x+0.5, y+0.5) is inside.
void fillPolygon(Canvas& canvas, const Polygon& polygon, Color color);

} // namespace kitenge