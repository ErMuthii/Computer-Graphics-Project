#pragma once

#include "kitenge/types.hpp"

#include <vector>

namespace kitenge {

class Canvas {
public:
    Canvas(int width, int height, Color background);

    int width() const noexcept;
    int height() const noexcept;
    void clear(Color color);
    // Writes outside the canvas are ignored so rasterizers can safely touch edges.
    void setPixel(int x, int y, Color color) noexcept;
    Color pixelAt(int x, int y) const;
    const std::vector<Color>& pixels() const noexcept;

private:
    int width_;
    int height_;
    std::vector<Color> pixels_;
};

} // namespace kitenge
