#include "kitenge/canvas.hpp"
#include "kitenge/polygon_clip.hpp"
#include "kitenge/ppm.hpp"

#include <cmath>
#include <cstddef>
#include <exception>
#include <filesystem>
#include <iostream>
#include <vector>

namespace {

using kitenge::Canvas;
using kitenge::Color;
using kitenge::Point2D;
using kitenge::Polygon;

constexpr int kPanelWidth = 320;
constexpr Color kBackground{25, 29, 32};
constexpr Color kBoundary{241, 227, 194};
constexpr Color kMotif{235, 177, 55};
constexpr Color kClipped{210, 72, 46};
constexpr Color kVertex{42, 190, 160};

// Preview outline only. It samples each edge densely so the demo can be viewed
// before the lines module is merged; integration replaces it with drawLine and
// fillPolygon. It is not part of the clipping algorithm.
void previewOutline(Canvas& canvas, const Polygon& polygon, Color color) {
    for (std::size_t i = 0; i < polygon.size(); ++i) {
        const Point2D start = polygon[i];
        const Point2D end = polygon[(i + 1) % polygon.size()];
        const double length = std::hypot(end.x - start.x, end.y - start.y);
        const int samples = static_cast<int>(std::ceil(length * 2.0)) + 1;
        for (int step = 0; step <= samples; ++step) {
            const double t = static_cast<double>(step) / samples;
            canvas.setPixel(static_cast<int>(std::lround(start.x + t * (end.x - start.x))),
                            static_cast<int>(std::lround(start.y + t * (end.y - start.y))),
                            color);
        }
    }
}

void markVertices(Canvas& canvas, const Polygon& polygon) {
    for (const Point2D& vertex : polygon) {
        const int x = static_cast<int>(std::lround(vertex.x));
        const int y = static_cast<int>(std::lround(vertex.y));
        for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {
                canvas.setPixel(x + dx, y + dy, kVertex);
            }
        }
    }
}

Polygon shifted(const Polygon& polygon, double offsetX) {
    Polygon result;
    for (const Point2D& vertex : polygon) {
        result.push_back({vertex.x + offsetX, vertex.y});
    }
    return result;
}

Polygon diamond(double centerX, double centerY, double halfWidth, double halfHeight) {
    return {{centerX, centerY - halfHeight}, {centerX + halfWidth, centerY},
            {centerX, centerY + halfHeight}, {centerX - halfWidth, centerY}};
}

// A Kitenge-style motif set: a band of diamonds, a corner triangle and a
// concave chevron, placed so that several shapes cross the panel edges.
std::vector<Polygon> motifs() {
    std::vector<Polygon> shapes;
    for (int column = 0; column < 5; ++column) {
        shapes.push_back(diamond(40.0 + column * 60.0, 120.0, 34.0, 44.0));
    }
    shapes.push_back({{10, 230}, {120, 210}, {60, 330}});
    shapes.push_back({{150, 220}, {250, 220}, {310, 265}, {250, 310},
                      {150, 310}, {200, 265}});
    return shapes;
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc > 2) {
        std::cerr << "Usage: polygon_clip_demo [output-image.ppm]\n";
        return 1;
    }
    try {
        const std::filesystem::path destination = argc == 2
            ? std::filesystem::path(argv[1])
            : std::filesystem::path("outputs/polygon-clipping.ppm");
        Canvas canvas(kPanelWidth * 3, 360, kBackground);

        // Left: motifs before clipping. Middle: the same motifs clipped to the
        // rectangular fabric window. Right: motifs clipped to a trapezoid skirt panel.
        const Polygon window{{40, 60}, {280, 60}, {280, 300}, {40, 300}};
        const Polygon skirt{{110, 40}, {210, 40}, {290, 320}, {30, 320}};
        const std::vector<Polygon> shapes = motifs();

        previewOutline(canvas, window, kBoundary);
        for (const Polygon& shape : shapes) {
            previewOutline(canvas, shape, kMotif);
        }

        const Polygon middleWindow = shifted(window, kPanelWidth);
        previewOutline(canvas, middleWindow, kBoundary);
        for (const Polygon& shape : shapes) {
            const Polygon clipped =
                kitenge::clipPolygonToConvex(shifted(shape, kPanelWidth), middleWindow);
            previewOutline(canvas, clipped, kClipped);
            markVertices(canvas, clipped);
        }

        const Polygon rightSkirt = shifted(skirt, kPanelWidth * 2.0);
        previewOutline(canvas, rightSkirt, kBoundary);
        for (const Polygon& shape : shapes) {
            const Polygon clipped =
                kitenge::clipPolygonToConvex(shifted(shape, kPanelWidth * 2.0), rightSkirt);
            previewOutline(canvas, clipped, kClipped);
            markVertices(canvas, clipped);
        }

        kitenge::writePpm(canvas, destination);
        std::cout << "Saved polygon clipping demonstration to " << destination.string() << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}
