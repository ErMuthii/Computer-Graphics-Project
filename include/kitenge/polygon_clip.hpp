#pragma once

#include "kitenge/types.hpp"

namespace kitenge {

// Clips `subject` against the convex polygon `boundary` with Sutherland-Hodgman.
//
// The boundary may be listed clockwise or counterclockwise; its orientation is
// detected from its signed area. The boundary region is closed, so subject
// points lying exactly on a boundary edge count as inside. Intersection points
// keep their double precision; round them only when rasterizing.
//
// Returns the visible polygon with its vertices in the subject's traversal order,
// without consecutive duplicate points. Returns an empty polygon when the subject
// has fewer than three vertices or no area remains visible, including a subject
// that only touches the boundary along an edge or at a vertex from outside.
//
// Throws std::invalid_argument when the boundary has fewer than three vertices,
// has no area, or is not convex.
Polygon clipPolygonToConvex(const Polygon& subject, const Polygon& boundary);

} // namespace kitenge
