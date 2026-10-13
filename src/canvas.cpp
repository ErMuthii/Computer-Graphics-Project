#include "kitenge/canvas.hpp"

#include <algorithm>
#include <cstddef>
#include <stdexcept>

namespace kitenge {

Canvas::Canvas(int width, int height, Color background)
    : width_(width), height_(height) {
    if (width <= 0 || height <= 0) {
        throw std::invalid_argument("Canvas dimensions must be positive.");
    }
    const auto columns = static_cast<std::size_t>(width);
    const auto rows = static_cast<std::size_t>(height);
    if (columns > pixels_.max_size() / rows) {
        throw std::length_error("Canvas dimensions are too large.");
    }
    pixels_.assign(columns * rows, background);
}

int Canvas::width() const noexcept { return width_; }
int Canvas::height() const noexcept { return height_; }

void Canvas::clear(Color color) {
    std::fill(pixels_.begin(), pixels_.end(), color);
}

void Canvas::setPixel(int x, int y, Color color) noexcept {
    if (x < 0 || y < 0 || x >= width_ || y >= height_) {
        return;
    }
    const auto index = static_cast<std::size_t>(y) * static_cast<std::size_t>(width_)
                     + static_cast<std::size_t>(x);
    pixels_[index] = color;
}

Color Canvas::pixelAt(int x, int y) const {
    if (x < 0 || y < 0 || x >= width_ || y >= height_) {
        throw std::out_of_range("Requested pixel is outside the canvas.");
    }
    const auto index = static_cast<std::size_t>(y) * static_cast<std::size_t>(width_)
                     + static_cast<std::size_t>(x);
    return pixels_[index];
}

const std::vector<Color>& Canvas::pixels() const noexcept { return pixels_; }

} // namespace kitenge
