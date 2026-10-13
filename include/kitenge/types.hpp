#pragma once

#include <cstdint>
#include <vector>

namespace kitenge {

struct Color {
    std::uint8_t red;
    std::uint8_t green;
    std::uint8_t blue;
};

struct Point {
    int x;
    int y;
};

struct Point2D {
    double x;
    double y;
};

using Polygon = std::vector<Point2D>;

struct Rect {
    double minX;
    double minY;
    double maxX;
    double maxY;
};

} // namespace kitenge
