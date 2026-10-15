#pragma once

#include "kitenge/canvas.hpp"

#include <optional>
#include <utility>

namespace kitenge {

// Rasterize both endpoints with integer Bresenham stepping in every octant.
void drawLine(Canvas& canvas, Point start, Point end, Color color);

// Clip a continuous segment to an inclusive axis-aligned rectangle.
// Returns no segment when the window and line do not intersect.
// Non-finite coordinates and unordered rectangle bounds are invalid.
std::optional<std::pair<Point2D, Point2D>>
clipLineToRect(Point2D start, Point2D end, Rect window);

} // namespace kitenge
