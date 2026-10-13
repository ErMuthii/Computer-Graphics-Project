#pragma once

#include "kitenge/canvas.hpp"

#include <filesystem>

namespace kitenge {

void writePpm(const Canvas& canvas, const std::filesystem::path& destination);

} // namespace kitenge
