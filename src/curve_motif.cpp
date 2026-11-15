// Cubic Bezier arch motif, sampled with the CP1 evaluator.
// Owner: Andrew Karanja Gathirwa, 167144.
#include "kitenge/curve_motif.hpp"

#include "kitenge/curves.hpp"

#include <cmath>

namespace kitenge {

namespace {

bool samePoint(Point2D a, Point2D b) { return std::fabs(a.x - b.x) < 1e-12 && std::fabs(a.y - b.y) < 1e-12; }

} // namespace

const std::vector<CubicControls>& bezierArchOutlineControls() {
    // Local coordinates, y down. The arch rises from the feet at y = 12 to a
    // peak at y = -15; the scallops dip to y = 18, so the bounding box is
    // centred on the origin. All control points lie in [-20, 20] x [-24, 20],
    // so by the convex hull property every curve point stays inside the
    // catalog extent [-25, 25].
    static const std::vector<CubicControls> outline{
        {{{-20, 12}, {-20, -24}, {20, -24}, {20, 12}}},  // arch top, left to right
        {{{20, 12}, {16, 20}, {4, 20}, {0, 12}}},                // right scallop
        {{{0, 12}, {-4, 20}, {-16, 20}, {-20, 12}}},             // left scallop, closes the path
    };
    return outline;
}

const std::vector<CubicControls>& bezierArchLeafControls() {
    static const std::vector<CubicControls> leaf{
        {{{0, 7}, {-10, 2}, {-10, -8}, {0, -14}}},  // left half: bottom tip -> top tip
        {{{0, -14}, {10, -8}, {10, 2}, {0, 7}}},    // right half: top tip -> bottom tip
    };
    return leaf;
}

std::vector<CubicControls> bezierArchControlPoints() {
    std::vector<CubicControls> all = bezierArchOutlineControls();
    const auto& leaf = bezierArchLeafControls();
    all.insert(all.end(), leaf.begin(), leaf.end());
    return all;
}

std::vector<Point2D> sampleJoinedCubics(const std::vector<CubicControls>& segments,
                                        int segmentsPerCurve) {
    std::vector<Point2D> path;
    for (const CubicControls& c : segments) {
        const std::vector<Point2D> samples = sampleCubicBezier(c, segmentsPerCurve);
        auto first = samples.begin();
        if (!path.empty() && samePoint(path.back(), *first)) ++first;
        path.insert(path.end(), first, samples.end());
    }
    return path;
}

MotifShape bezierArchMotif(int segmentsPerCurve) {
    std::vector<Point2D> outline = sampleJoinedCubics(bezierArchOutlineControls(), segmentsPerCurve);
    std::vector<Point2D> leaf = sampleJoinedCubics(bezierArchLeafControls(), segmentsPerCurve);
    // Both paths end where they started. A polygon and a closed stroke list
    // each vertex once, so drop the repeated closing point.
    if (outline.size() > 1 && samePoint(outline.front(), outline.back())) outline.pop_back();
    if (leaf.size() > 1 && samePoint(leaf.front(), leaf.back())) leaf.pop_back();

    MotifShape m;
    m.fills.push_back({outline, kRolePrimary});
    m.fills.push_back({leaf, kRoleAccent});
    m.strokes.push_back({outline, true, kRoleAccent});
    m.strokes.push_back({{{0, -14}, {0, 7}}, false, kRolePrimary});
    return m;
}

} // namespace kitenge
