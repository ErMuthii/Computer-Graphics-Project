// Headless animation demo: renders a numbered PPM frame sequence of the studio
// scene with the pattern animated by the animation module.
// Owner: Sayialel Eric Lenina, 161492.
//
// Every frame is sampled at the exact time frameIndex / fps and evaluated from
// the fixed base settings, so the sequence is deterministic and the frame count
// only sets its length. No window or OpenGL is involved.

#include "kitenge/animation.hpp"
#include "kitenge/pattern.hpp"
#include "kitenge/ppm.hpp"
#include "kitenge/studio.hpp"

#include <cmath>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

const char* const kPrefix = "frame_";

void usage() {
    std::cerr << "Usage: animation_demo [options]\n"
                 "  --mode off|rotate|pulse   animation mode (default rotate)\n"
                 "  --frames N                number of frames, 1-100000 (default 60)\n"
                 "  --fps F                   frames per second, up to 240 (default 30)\n"
                 "  --out DIR                 output folder (default outputs/animation/<mode>)\n"
                 "  --speed DEG_PER_S         rotation speed in degrees per second (default 45)\n"
                 "  --pulse-frequency HZ      pulse cycles per second (default 0.5)\n"
                 "  --pulse-range MIN MAX     pulse scale factors (default 0.75 1.25)\n"
                 "  --overwrite               replace an existing frame sequence in DIR\n"
                 "Scene options (base settings before animation):\n"
              << kitenge::patternSceneOptionsUsage();
}

double number(const std::string& flag, const char* text) {
    try {
        std::size_t used = 0;
        const double value = std::stod(text, &used);
        if (used == std::string(text).size() && std::isfinite(value)) return value;
    } catch (const std::exception&) {
    }
    throw std::invalid_argument(flag + " needs a number, got '" + text + "'");
}

int wholeNumber(const std::string& flag, const char* text) {
    const double value = number(flag, text);
    if (value != std::floor(value) || value < 1 || value > kitenge::kMaxAnimationFrames) {
        throw std::invalid_argument(flag + " must be a whole number from 1 to " +
                                    std::to_string(kitenge::kMaxAnimationFrames));
    }
    return static_cast<int>(value);
}

std::vector<fs::path> existingFrames(const fs::path& folder) {
    std::vector<fs::path> frames;
    if (!fs::is_directory(folder)) return frames;
    for (const auto& entry : fs::directory_iterator(folder)) {
        const std::string name = entry.path().filename().string();
        if (entry.is_regular_file() && name.rfind(kPrefix, 0) == 0 &&
            entry.path().extension() == ".ppm") {
            frames.push_back(entry.path());
        }
    }
    return frames;
}

} // namespace

int main(int argc, char** argv) {
    try {
        kitenge::PatternSceneOptions scene;
        kitenge::AnimationSettings animation;
        animation.mode = kitenge::AnimationMode::Rotate;
        int frameCount = 60;
        double fps = 30.0;
        fs::path folder;
        bool overwrite = false;

        for (int i = 1; i < argc;) {
            if (kitenge::parsePatternSceneOption(argc, argv, i, scene)) continue;
            const std::string flag = argv[i];
            const auto value = [&](int offset) -> const char* {
                if (i + offset >= argc) throw std::invalid_argument(flag + " is missing a value");
                return argv[i + offset];
            };
            if (flag == "--mode") {
                animation.mode = kitenge::parseAnimationMode(value(1));
                i += 2;
            } else if (flag == "--frames") {
                frameCount = wholeNumber(flag, value(1));
                i += 2;
            } else if (flag == "--fps") {
                fps = number(flag, value(1));
                i += 2;
            } else if (flag == "--out") {
                folder = value(1);
                i += 2;
            } else if (flag == "--speed") {
                animation.angularSpeed = kitenge::degreesToRadians(number(flag, value(1)));
                i += 2;
            } else if (flag == "--pulse-frequency") {
                animation.pulseFrequency = number(flag, value(1));
                i += 2;
            } else if (flag == "--pulse-range") {
                animation.pulseMinScale = number(flag, value(1));
                animation.pulseMaxScale = number(flag, value(2));
                i += 3;
            } else if (flag == "--overwrite") {
                overwrite = true;
                ++i;
            } else {
                usage();
                return 1;
            }
        }

        kitenge::validateFrameSequence(frameCount, fps);
        kitenge::validateAnimationSettings(animation);
        if (folder.empty()) {
            folder = fs::path("outputs") / "animation" / kitenge::animationModeLabel(animation.mode);
        }

        // Never mix two sequences in one folder: refuse unless asked, and then
        // remove every old frame so a shorter run leaves no stale frames behind.
        const std::vector<fs::path> previous = existingFrames(folder);
        if (!previous.empty()) {
            if (!overwrite) {
                std::cerr << folder.string() << " already holds " << previous.size()
                          << " frame(s). Use --overwrite to replace them or --out for a new folder.\n";
                return 1;
            }
            for (const fs::path& old : previous) fs::remove(old);
        }
        fs::create_directories(folder);

        const kitenge::PatternSettings base = scene.settings;
        const kitenge::StudioHooks hooks = kitenge::makeAnimatedPatternHooks(base, animation);
        kitenge::StudioState state = scene.state;
        kitenge::Canvas canvas(kitenge::studioWidth, kitenge::studioHeight, {0, 0, 0});

        for (int frame = 0; frame < frameCount; ++frame) {
            // Exact sample time from the index; nothing is accumulated.
            state.timeSeconds = kitenge::frameTime(frame, fps);
            kitenge::renderStudio(canvas, state, hooks);
            kitenge::writePpm(canvas, folder / kitenge::frameFileName(kPrefix, frame, frameCount));
        }

        // A short record of the inputs, so the sequence can be reproduced.
        std::ofstream info(folder / "sequence.txt");
        info << "mode " << kitenge::animationModeLabel(animation.mode) << '\n'
             << "frames " << frameCount << '\n'
             << "fps " << fps << '\n'
             << "duration_seconds " << frameCount / fps << '\n'
             << "speed_degrees_per_second " << kitenge::radiansToDegrees(animation.angularSpeed) << '\n'
             << "pulse_frequency " << animation.pulseFrequency << '\n'
             << "pulse_range " << animation.pulseMinScale << ' ' << animation.pulseMaxScale << '\n'
             << "motif " << scene.state.motif << '\n'
             << "palette " << scene.state.palette << '\n'
             << "base_angle_degrees " << kitenge::radiansToDegrees(base.transform.angleRadians) << '\n'
             << "base_scale " << base.transform.scale << '\n';

        std::cout << "Saved " << frameCount << " frames (" << frameCount / fps << " s at " << fps
                  << " fps, mode " << kitenge::animationModeLabel(animation.mode) << ") to "
                  << folder.string() << '\n';
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
