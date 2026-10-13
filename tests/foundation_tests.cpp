#include "kitenge/canvas.hpp"
#include "kitenge/ppm.hpp"

#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

bool equal(kitenge::Color first, kitenge::Color second) {
    return first.red == second.red && first.green == second.green
        && first.blue == second.blue;
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: foundation_tests output-image.ppm\n";
        return 1;
    }
    try {
        bool rejectedDimensions = false;
        try {
            kitenge::Canvas invalid(0, 4, {0, 0, 0});
        } catch (const std::invalid_argument&) {
            rejectedDimensions = true;
        }
        require(rejectedDimensions, "Nonpositive canvas dimensions were accepted.");

        kitenge::Canvas canvas(2, 2, {10, 20, 30});
        require(canvas.width() == 2 && canvas.height() == 2, "Dimensions changed.");
        canvas.setPixel(-1, 0, {255, 255, 255});
        canvas.setPixel(2, 0, {255, 255, 255});
        canvas.setPixel(0, -1, {255, 255, 255});
        canvas.setPixel(0, 2, {255, 255, 255});
        require(equal(canvas.pixelAt(0, 0), {10, 20, 30}),
                "An out-of-bounds write changed a valid pixel.");
        canvas.setPixel(1, 0, {255, 128, 0});
        require(equal(canvas.pixelAt(1, 0), {255, 128, 0}), "Pixel write failed.");

        bool rejectedRead = false;
        try {
            (void)canvas.pixelAt(2, 0);
        } catch (const std::out_of_range&) {
            rejectedRead = true;
        }
        require(rejectedRead, "Out-of-bounds reads must be rejected.");

        const std::filesystem::path imagePath(argv[1]);
        kitenge::writePpm(canvas, imagePath);
        std::ifstream image(imagePath, std::ios::binary);
        std::string magic;
        int width = 0;
        int height = 0;
        int maximum = 0;
        image >> magic >> width >> height >> maximum;
        require(magic == "P6" && width == 2 && height == 2 && maximum == 255,
                "PPM header cannot be read correctly.");
        require(image.get() == '\n', "PPM header must end before raster bytes.");
        const std::vector<unsigned char> raster{
            std::istreambuf_iterator<char>(image), std::istreambuf_iterator<char>()
        };
        const std::vector<unsigned char> expected{
            10, 20, 30, 255, 128, 0, 10, 20, 30, 10, 20, 30
        };
        require(raster == expected, "PPM RGB bytes or row order are incorrect.");

        canvas.clear({4, 5, 6});
        for (const auto& pixel : canvas.pixels()) {
            require(equal(pixel, {4, 5, 6}), "Canvas clear missed a pixel.");
        }
        std::cout << "Foundation checks passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Foundation check failed: " << error.what() << '\n';
        return 1;
    }
}
