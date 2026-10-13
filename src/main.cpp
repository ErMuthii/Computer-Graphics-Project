#include "kitenge/canvas.hpp"
#include "kitenge/ppm.hpp"

#include <array>
#include <exception>
#include <filesystem>
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc > 2) {
        std::cerr << "Usage: kitenge_cp1 [output-image.ppm]\n";
        return 1;
    }
    try {
        const std::filesystem::path destination = argc == 2
            ? std::filesystem::path(argv[1])
            : std::filesystem::path("outputs/foundation.ppm");
        kitenge::Canvas canvas(640, 480, {25, 29, 32});
        const std::array<kitenge::Color, 5> palette{{
            {210, 72, 46}, {235, 177, 55}, {42, 122, 108},
            {79, 57, 102}, {241, 227, 194}
        }};

        // Pixel diagnostic only: the assigned graphics modules will add motifs.
        for (int y = 64; y < 416; ++y) {
            for (int x = 64; x < 576; ++x) {
                const int column = (x - 64) / 64;
                const int row = (y - 64) / 64;
                const auto index = static_cast<std::size_t>((column + row) % 5);
                canvas.setPixel(x, y, palette[index]);
            }
        }
        kitenge::writePpm(canvas, destination);
        std::cout << "Saved foundation pixel diagnostic to " << destination.string() << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}
