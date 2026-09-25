#include "src/game/game_runtime.hpp"

#include <cassert>
#include <cstdlib>

int main(int argc, char **argv) {
    assert(argc == 2);
    voyage::RuntimeConfig config;
    config.resource_dir = argv[1];
    if (const char *test_tmp = std::getenv("TEST_TMPDIR")) config.save_dir = test_tmp;

    voyage::GameRuntime game(config);
    game.start();
    assert(game.running());
    assert(game.simulation_time_ms() == 0);
    game.tick(16);
    game.tick(34);
    assert(game.simulation_time_ms() == 50);
    assert(game.width() == voyage::kPhoneWidth);
    assert(!game.centered_camera());
    voyage::Frame phone = game.copy_frame();
    assert(phone.logical_width == voyage::kPhoneWidth);
    assert(phone.height == voyage::kScreenHeight);
    assert(phone.surface_width == voyage::kSurfaceWidth);
    assert(phone.pixels.size() ==
           (size_t)voyage::kSurfaceWidth * (size_t)voyage::kScreenHeight);

    game.press(voyage::Input::Action);
    game.release(voyage::Input::Action);
    game.set_viewport(voyage::Viewport::Wide);
    game.set_centered_camera(true);
    assert(game.width() == voyage::kWideWidth);
    assert(game.centered_camera());
    assert(game.copy_frame().logical_width == voyage::kWideWidth);

    game.stop();
    assert(!game.running());
    return 0;
}
