#include "kitenge/lines.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <stdexcept>

namespace kitenge {

void drawLine(Canvas& canvas, Point start, Point end, Color color) {
    // A canonical endpoint order makes tie-breaking identical when callers
    // supply the same segment in reverse.
    if (start.x > end.x || (start.x == end.x && start.y > end.y)) {
        std::swap(start, end);
    }
    std::int64_t x = start.x;
    std::int64_t y = start.y;
    const std::int64_t dx = std::abs(static_cast<std::int64_t>(end.x) - start.x);
    const std::int64_t dy = std::abs(static_cast<std::int64_t>(end.y) - start.y);
    const std::int64_t stepX = start.x < end.x ? 1 : -1;
    const std::int64_t stepY = start.y < end.y ? 1 : -1;
    std::int64_t error = dx - dy;

    while (true) {
        canvas.setPixel(static_cast<int>(x), static_cast<int>(y), color);
        if (x == end.x && y == end.y) break;
        const std::int64_t twiceError = 2 * error;
        if (twiceError > -dy) {
            error -= dy;
            x += stepX;
        }
        if (twiceError < dx) {
            error += dx;
            y += stepY;
        }
    }
}

namespace {

constexpr unsigned left = 1;
constexpr unsigned right = 2;
constexpr unsigned top = 4;
constexpr unsigned bottom = 8;

unsigned outcode(Point2D point, Rect window) {
    unsigned code = 0;
    if (point.x < window.minX) code |= left;
    if (point.x > window.maxX) code |= right;
    if (point.y < window.minY) code |= top;
    if (point.y > window.maxY) code |= bottom;
    return code;
}

} // namespace

std::optional<std::pair<Point2D, Point2D>>
clipLineToRect(Point2D start, Point2D end, Rect window) {
    if (!std::isfinite(start.x) || !std::isfinite(start.y) ||
        !std::isfinite(end.x) || !std::isfinite(end.y) ||
        !std::isfinite(window.minX) || !std::isfinite(window.minY) ||
        !std::isfinite(window.maxX) || !std::isfinite(window.maxY) ||
        window.minX > window.maxX || window.minY > window.maxY) {
        throw std::invalid_argument("Line and clipping window coordinates must be finite, with ordered bounds.");
    }

    while (true) {
        const unsigned startCode = outcode(start, window);
        const unsigned endCode = outcode(end, window);
        if ((startCode | endCode) == 0) return std::make_pair(start, end);
        if ((startCode & endCode) != 0) return std::nullopt;

        const unsigned code = startCode != 0 ? startCode : endCode;
        Point2D crossing{};
        if (code & top) {
            crossing = {start.x + (end.x - start.x) *
                (window.minY - start.y) / (end.y - start.y), window.minY};
        } else if (code & bottom) {
            crossing = {start.x + (end.x - start.x) *
                (window.maxY - start.y) / (end.y - start.y), window.maxY};
        } else if (code & right) {
            crossing = {window.maxX, start.y + (end.y - start.y) *
                (window.maxX - start.x) / (end.x - start.x)};
        } else {
            crossing = {window.minX, start.y + (end.y - start.y) *
                (window.minX - start.x) / (end.x - start.x)};
        }

        if (code == startCode) start = crossing;
        else end = crossing;
    }
}

} // namespace kitenge
