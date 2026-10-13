#include "kitenge/ppm.hpp"

#include <array>
#include <fstream>
#include <stdexcept>

namespace kitenge {

void writePpm(const Canvas& canvas, const std::filesystem::path& destination) {
    if (!destination.parent_path().empty()) {
        std::filesystem::create_directories(destination.parent_path());
    }
    std::ofstream output(destination, std::ios::binary);
    if (!output) {
        throw std::runtime_error("Cannot open output image: " + destination.string());
    }
    output << "P6\n" << canvas.width() << ' ' << canvas.height() << "\n255\n";
    for (const auto& pixel : canvas.pixels()) {
        const std::array<char, 3> channels{
            static_cast<char>(pixel.red),
            static_cast<char>(pixel.green),
            static_cast<char>(pixel.blue)
        };
        output.write(channels.data(), static_cast<std::streamsize>(channels.size()));
    }
    output.close();
    if (!output) {
        throw std::runtime_error("Failed to write output image: " + destination.string());
    }
}

} // namespace kitenge
