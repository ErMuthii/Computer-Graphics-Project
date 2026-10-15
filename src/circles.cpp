#include "kitenge/circles.hpp"

#include <algorithm>
#include <cstdint>
#include <stdexcept>

namespace kitenge {

namespace {

void plot(Canvas& canvas, std::int64_t x, std::int64_t y, Color color) {
    if (x >= 0 && y >= 0 && x < canvas.width() && y < canvas.height()) {
        canvas.setPixel(static_cast<int>(x), static_cast<int>(y), color);
    }
}

void plotCircleSymmetry(Canvas& canvas, Point center,
                        std::int64_t x, std::int64_t y, Color color) {
    plot(canvas, static_cast<std::int64_t>(center.x) + x, static_cast<std::int64_t>(center.y) + y, color);
    plot(canvas, static_cast<std::int64_t>(center.x) - x, static_cast<std::int64_t>(center.y) + y, color);
    plot(canvas, static_cast<std::int64_t>(center.x) + x, static_cast<std::int64_t>(center.y) - y, color);
    plot(canvas, static_cast<std::int64_t>(center.x) - x, static_cast<std::int64_t>(center.y) - y, color);
    plot(canvas, static_cast<std::int64_t>(center.x) + y, static_cast<std::int64_t>(center.y) + x, color);
    plot(canvas, static_cast<std::int64_t>(center.x) - y, static_cast<std::int64_t>(center.y) + x, color);
    plot(canvas, static_cast<std::int64_t>(center.x) + y, static_cast<std::int64_t>(center.y) - x, color);
    plot(canvas, static_cast<std::int64_t>(center.x) - y, static_cast<std::int64_t>(center.y) - x, color);
}

void plotEllipseSymmetry(Canvas& canvas, Point center,
                         std::int64_t x, std::int64_t y, Color color) {
    plot(canvas, static_cast<std::int64_t>(center.x) + x, static_cast<std::int64_t>(center.y) + y, color);
    plot(canvas, static_cast<std::int64_t>(center.x) - x, static_cast<std::int64_t>(center.y) + y, color);
    plot(canvas, static_cast<std::int64_t>(center.x) + x, static_cast<std::int64_t>(center.y) - y, color);
    plot(canvas, static_cast<std::int64_t>(center.x) - x, static_cast<std::int64_t>(center.y) - y, color);
}

} // namespace

void drawCircleMidpoint(Canvas& canvas, Point center, int radius, Color color) {
    if (radius < 0) throw std::invalid_argument("Circle radius must be non-negative.");

    std::int64_t x = 0;
    std::int64_t y = radius;
    std::int64_t decision = 1 - y;
    while (x <= y) {
        plotCircleSymmetry(canvas, center, x, y, color);
        if (decision < 0) {
            decision += 2 * x + 3;
        } else {
            decision += 2 * (x - y) + 5;
            --y;
        }
        ++x;
    }
}

void drawEllipseMidpoint(Canvas& canvas, Point center,
                         int radiusX, int radiusY, Color color) {
    if (radiusX < 0 || radiusY < 0) {
        throw std::invalid_argument("Ellipse radii must be non-negative.");
    }
    if (radiusX == 0) {
        const std::int64_t first = std::max<std::int64_t>(0,
            static_cast<std::int64_t>(center.y) - radiusY);
        const std::int64_t last = std::min<std::int64_t>(canvas.height() - 1,
            static_cast<std::int64_t>(center.y) + radiusY);
        for (std::int64_t y = first; y <= last; ++y) {
            plot(canvas, center.x, y, color);
        }
        return;
    }
    if (radiusY == 0) {
        const std::int64_t first = std::max<std::int64_t>(0,
            static_cast<std::int64_t>(center.x) - radiusX);
        const std::int64_t last = std::min<std::int64_t>(canvas.width() - 1,
            static_cast<std::int64_t>(center.x) + radiusX);
        for (std::int64_t x = first; x <= last; ++x) {
            plot(canvas, x, center.y, color);
        }
        return;
    }

    const long double rx2 = static_cast<long double>(radiusX) * radiusX;
    const long double ry2 = static_cast<long double>(radiusY) * radiusY;
    std::int64_t x = 0;
    std::int64_t y = radiusY;
    long double dx = 0;
    long double dy = 2 * rx2 * y;
    long double decision = ry2 - rx2 * y + rx2 / 4;

    // Region 1: the tangent is shallow, so advance x every step.
    while (dx < dy) {
        plotEllipseSymmetry(canvas, center, x, y, color);
        ++x;
        dx += 2 * ry2;
        if (decision < 0) {
            decision += dx + ry2;
        } else {
            --y;
            dy -= 2 * rx2;
            decision += dx - dy + ry2;
        }
    }

    decision = ry2 * (static_cast<long double>(x) + 0.5L) *
               (static_cast<long double>(x) + 0.5L) +
               rx2 * (static_cast<long double>(y) - 1) *
               (static_cast<long double>(y) - 1) - rx2 * ry2;

    // Region 2: the tangent is steep, so advance y every step.
    while (y >= 0) {
        plotEllipseSymmetry(canvas, center, x, y, color);
        --y;
        dy -= 2 * rx2;
        if (decision > 0) {
            decision += rx2 - dy;
        } else {
            ++x;
            dx += 2 * ry2;
            decision += dx - dy + rx2;
        }
    }
}

} // namespace kitenge
