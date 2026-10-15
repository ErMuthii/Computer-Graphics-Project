#pragma once

#include "kitenge/canvas.hpp"

namespace kitenge {

// Plot an outline using eight-way midpoint symmetry. Radius zero draws center.
void drawCircleMidpoint(Canvas& canvas, Point center, int radius, Color color);

// Plot an outline using four-way midpoint symmetry. A zero axis draws a
// straight segment on the other axis; two zero axes draw the center pixel.
void drawEllipseMidpoint(Canvas& canvas, Point center,
                         int radiusX, int radiusY, Color color);

} // namespace kitenge
