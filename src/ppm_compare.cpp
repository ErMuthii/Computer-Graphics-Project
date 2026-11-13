// Compares two binary PPM (P6) images pixel for pixel.
//
//   ppm_compare expected.ppm actual.ppm [difference.ppm]
//
// Exit code 0 when the images have the same size and identical pixels, 1 when
// they differ, 2 on a usage or file error. The optional third path receives an
// image that shows differing pixels in red over a dimmed copy of `expected`.
// Used to check that the OpenGL window shows exactly the canvas that the
// headless renderer produces.

#include "kitenge/canvas.hpp"
#include "kitenge/ppm.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

using kitenge::Canvas;
using kitenge::Color;

// Reads the next header token, skipping whitespace and '#' comments.
std::string token(std::istream& in) {
    std::string out;
    char c = 0;
    while (in.get(c)) {
        if (c == '#') {
            std::string ignored;
            std::getline(in, ignored);
        } else if (!std::isspace(static_cast<unsigned char>(c))) {
            out += c;
            break;
        }
    }
    while (in.get(c) && !std::isspace(static_cast<unsigned char>(c))) out += c;
    return out;
}

Canvas readPpm(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("Cannot open " + path);
    if (token(in) != "P6") throw std::runtime_error(path + " is not a binary PPM (P6)");
    const int width = std::stoi(token(in));
    const int height = std::stoi(token(in));
    if (std::stoi(token(in)) != 255) throw std::runtime_error(path + " must use 8-bit channels");
    Canvas canvas(width, height, {0, 0, 0});
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x) {
            unsigned char rgb[3];
            if (!in.read(reinterpret_cast<char*>(rgb), 3)) throw std::runtime_error(path + " is truncated");
            canvas.setPixel(x, y, {rgb[0], rgb[1], rgb[2]});
        }
    return canvas;
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 3 || argc > 4) {
        std::cerr << "Usage: ppm_compare expected.ppm actual.ppm [difference.ppm]\n";
        return 2;
    }
    try {
        const Canvas expected = readPpm(argv[1]);
        const Canvas actual = readPpm(argv[2]);
        if (expected.width() != actual.width() || expected.height() != actual.height()) {
            std::cout << "DIFFERENT: sizes " << expected.width() << "x" << expected.height() << " and "
                      << actual.width() << "x" << actual.height() << '\n';
            return 1;
        }
        Canvas difference(expected.width(), expected.height(), {0, 0, 0});
        long long differing = 0;
        int largest = 0;
        for (int y = 0; y < expected.height(); ++y)
            for (int x = 0; x < expected.width(); ++x) {
                const Color e = expected.pixelAt(x, y);
                const Color a = actual.pixelAt(x, y);
                const int delta = std::max({std::abs(e.red - a.red), std::abs(e.green - a.green),
                                            std::abs(e.blue - a.blue)});
                if (delta > 0) {
                    ++differing;
                    largest = std::max(largest, delta);
                    difference.setPixel(x, y, {255, 0, 0});
                } else {
                    difference.setPixel(x, y, {static_cast<unsigned char>(e.red / 3),
                                               static_cast<unsigned char>(e.green / 3),
                                               static_cast<unsigned char>(e.blue / 3)});
                }
            }
        if (argc == 4) kitenge::writePpm(difference, argv[3]);
        const long long total = static_cast<long long>(expected.width()) * expected.height();
        if (differing == 0) {
            std::cout << "IDENTICAL: " << expected.width() << "x" << expected.height() << ", " << total
                      << " pixels\n";
            return 0;
        }
        std::cout << "DIFFERENT: " << differing << " of " << total
                  << " pixels differ, largest channel difference " << largest << '\n';
        return 1;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 2;
    }
}
