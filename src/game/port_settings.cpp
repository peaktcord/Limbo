#include "src/game/port_settings.hpp"

#include <cstdio>
#include <cstring>
#include <filesystem>

namespace fs = std::filesystem;

namespace voyage {

namespace {

const char *kFileName = "portoptions.txt";

std::string filePath(const std::string &settingsDir) {
    if (settingsDir.empty()) return kFileName;
    return settingsDir + "/" + kFileName;
}

void apply(PortOptions *options, const std::string &key, bool value) {
    if (key == "widescreen") options->widescreen = value;
    else if (key == "fullscreen") options->fullscreen = value;
    else if (key == "centered_camera") options->centered_camera = value;
    else if (key == "modern_movement") options->modern_movement = value;
    else if (key == "screen_aligned_movement") options->screen_aligned_movement = value;
}

}

std::vector<std::string> port_option_labels(const PortOptions &options) {
    return {
        options.widescreen ? "View: Wide 16:9" : "View: Classic Phone",
        options.modern_movement ? "Movement: 8-Way" : "Movement: Classic 4-Way",
        options.screen_aligned_movement ? "Direction: Screen" : "Direction: Isometric",
        options.centered_camera ? "Camera: Centered" : "Camera: Classic",
        options.fullscreen ? "Window: Fullscreen" : "Window: Windowed",
        "Handset Defaults"
    };
}

void toggle_port_option(int32_t row, PortOptions &options) {
    switch (row) {
        case ROW_VIEWPORT: options.widescreen = !options.widescreen; break;
        case ROW_MOVEMENT: options.modern_movement = !options.modern_movement; break;
        case ROW_DIRECTION: options.screen_aligned_movement = !options.screen_aligned_movement; break;
        case ROW_CAMERA: options.centered_camera = !options.centered_camera; break;
        case ROW_WINDOW: options.fullscreen = !options.fullscreen; break;
        case ROW_HANDSET_DEFAULTS: options.useHandsetDefaults(); break;
        default: break;
    }
}

void loadPortOptions(const std::string &settingsDir, PortOptions *options) {
    if (options == nullptr) return;
    std::FILE *f = std::fopen(filePath(settingsDir).c_str(), "rb");
    if (f == nullptr) return;

    char line[128];
    while (std::fgets(line, (int)sizeof(line), f) != nullptr) {
        size_t len = std::strlen(line);
        while (len > 0 && (line[len - 1] == '\r' || line[len - 1] == '\n')) {
            line[--len] = '\0';
        }
        char *eq = std::strchr(line, '=');
        if (eq == nullptr) continue;
        *eq = '\0';
        const std::string key(line);
        const char value = eq[1];
        if (value == '0' || value == '1') apply(options, key, value == '1');
    }
    std::fclose(f);
}

void savePortOptions(const std::string &settingsDir, const PortOptions &options) {
    std::error_code ec;
    if (!settingsDir.empty()) {
        fs::create_directories(settingsDir, ec);
    }
    std::FILE *f = std::fopen(filePath(settingsDir).c_str(), "wb");
    if (f == nullptr) return;
    std::fprintf(f, "widescreen=%d\n", options.widescreen ? 1 : 0);
    std::fprintf(f, "fullscreen=%d\n", options.fullscreen ? 1 : 0);
    std::fprintf(f, "centered_camera=%d\n", options.centered_camera ? 1 : 0);
    std::fprintf(f, "modern_movement=%d\n", options.modern_movement ? 1 : 0);
    std::fprintf(f, "screen_aligned_movement=%d\n", options.screen_aligned_movement ? 1 : 0);
    std::fclose(f);
}

}
