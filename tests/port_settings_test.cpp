#include "src/game/port_settings.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

void test_labels_and_toggles() {
    voyage::PortOptions options;
    assert(!options.widescreen);
    assert(!options.fullscreen);
    assert(!options.centered_camera);
    assert(!options.modern_movement);
    assert(!options.screen_aligned_movement);

    auto labels = voyage::port_option_labels(options);
    assert(labels.size() == voyage::ROW_COUNT);
    assert(labels[voyage::ROW_VIEWPORT] == "View: Classic Phone");
    assert(labels[voyage::ROW_MOVEMENT] == "Movement: Classic 4-Way");
    assert(labels[voyage::ROW_DIRECTION] == "Direction: Isometric");
    assert(labels[voyage::ROW_CAMERA] == "Camera: Classic");
    assert(labels[voyage::ROW_WINDOW] == "Window: Windowed");
    assert(labels[voyage::ROW_HANDSET_DEFAULTS] == "Handset Defaults");

    voyage::toggle_port_option(voyage::ROW_VIEWPORT, options);
    assert(options.widescreen);
    voyage::toggle_port_option(voyage::ROW_MOVEMENT, options);
    assert(options.modern_movement);
    voyage::toggle_port_option(voyage::ROW_DIRECTION, options);
    assert(options.screen_aligned_movement);
    voyage::toggle_port_option(voyage::ROW_CAMERA, options);
    assert(options.centered_camera);
    voyage::toggle_port_option(voyage::ROW_WINDOW, options);
    assert(options.fullscreen);

    labels = voyage::port_option_labels(options);
    assert(labels[voyage::ROW_VIEWPORT] == "View: Wide 16:9");
    assert(labels[voyage::ROW_MOVEMENT] == "Movement: 8-Way");
    assert(labels[voyage::ROW_DIRECTION] == "Direction: Screen");
    assert(labels[voyage::ROW_CAMERA] == "Camera: Centered");
    assert(labels[voyage::ROW_WINDOW] == "Window: Fullscreen");

    voyage::toggle_port_option(voyage::ROW_HANDSET_DEFAULTS, options);
    assert(!options.widescreen);
    assert(!options.fullscreen);
    assert(!options.centered_camera);
    assert(!options.modern_movement);
    assert(!options.screen_aligned_movement);
}

void test_load_save(const fs::path &temp_dir) {
    voyage::PortOptions options;
    options.widescreen = true;
    options.modern_movement = true;
    options.screen_aligned_movement = true;
    options.centered_camera = true;
    options.fullscreen = true;

    voyage::savePortOptions(temp_dir.string(), options);
    assert(fs::exists(temp_dir / "portoptions.txt"));

    voyage::PortOptions loaded;
    voyage::loadPortOptions(temp_dir.string(), &loaded);
    assert(loaded.widescreen);
    assert(loaded.modern_movement);
    assert(loaded.screen_aligned_movement);
    assert(loaded.centered_camera);
    assert(loaded.fullscreen);

    loaded.widescreen = false;
    loaded.fullscreen = false;
    voyage::savePortOptions(temp_dir.string(), loaded);

    voyage::PortOptions reloaded;
    voyage::loadPortOptions(temp_dir.string(), &reloaded);
    assert(!reloaded.widescreen);
    assert(!reloaded.fullscreen);
    assert(reloaded.modern_movement);
    assert(reloaded.screen_aligned_movement);
    assert(reloaded.centered_camera);
}

int main(int argc, char **argv) {
    test_labels_and_toggles();

    fs::path temp_dir = "test_port_settings_tmp";
    if (const char *env_tmp = std::getenv("TEST_TMPDIR")) {
        temp_dir = fs::path(env_tmp) / "port_settings";
    }
    std::error_code ec;
    fs::remove_all(temp_dir, ec);
    fs::create_directories(temp_dir, ec);

    test_load_save(temp_dir);

    fs::remove_all(temp_dir, ec);
    return 0;
}
