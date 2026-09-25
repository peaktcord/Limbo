#ifndef VOYAGE_GAME_PORT_SETTINGS_HPP
#define VOYAGE_GAME_PORT_SETTINGS_HPP

#include <cstdint>
#include <string>
#include <vector>

namespace voyage {

enum PortOptionRow {
    ROW_VIEWPORT = 0,
    ROW_MOVEMENT,
    ROW_DIRECTION,
    ROW_CAMERA,
    ROW_WINDOW,
    ROW_HANDSET_DEFAULTS,
    ROW_COUNT
};

struct PortOptions {
    bool widescreen = false;
    bool fullscreen = false;
    bool centered_camera = false;
    bool modern_movement = false;
    bool screen_aligned_movement = false;

    void useHandsetDefaults() {
        widescreen = false;
        fullscreen = false;
        centered_camera = false;
        modern_movement = false;
        screen_aligned_movement = false;
    }
};

std::vector<std::string> port_option_labels(const PortOptions &options);
void toggle_port_option(int32_t row, PortOptions &options);
void loadPortOptions(const std::string &settingsDir, PortOptions *options);
void savePortOptions(const std::string &settingsDir, const PortOptions &options);

}

#endif
