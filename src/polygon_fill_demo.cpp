#include "kitenge/polygon_fill.hpp"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

using namespace kitenge;

int main(int argc, char** argv) {
    const std::string path = argc > 1 ? argv[1] : "outputs/polygon_fill.ppm";
    std::filesystem::create_directories(std::filesystem::path(path).parent_path());

    Canvas canvas(400, 300, Color{245, 240, 230});

    // Diamond motif
    fillPolygon(canvas, {{90, 60}, {150, 130}, {90, 200}, {30, 130}}, Color{200, 60, 40});

    // Bodice and skirt: two separate convex panels sharing the waist edge
    fillPolygon(canvas, {{260, 50}, {340, 50}, {330, 150}, {270, 150}}, Color{30, 120, 160});
    fillPolygon(canvas, {{270, 150}, {330, 150}, {375, 270}, {225, 270}}, Color{230, 170, 30});

    // Binary PPM (P6), rows top to bottom
    std::ofstream out(path, std::ios::binary);
    out << "P6\n" << canvas.width() << " " << canvas.height() << "\n255\n";
    for (int y = 0; y < canvas.height(); ++y) {
        for (int x = 0; x < canvas.width(); ++x) {
            Color c = canvas.pixelAt(x, y);
            out.put(static_cast<char>(c.red));
            out.put(static_cast<char>(c.green));
            out.put(static_cast<char>(c.blue));
        }
    }
    std::printf("wrote %s\n", path.c_str());
    return 0;
}