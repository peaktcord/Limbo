#include "src/game/game_runtime.hpp"

#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <stdexcept>


int main() {
    using voyage::GameRuntime;
    using voyage::Input;
    assert(voyage::kPhoneWidth == 176);
    assert(voyage::kWideWidth == 391);
    assert(voyage::kScreenHeight == 220);
    assert(GameRuntime::is_movement(Input::Up));
    assert(GameRuntime::is_movement(Input::Right));
    assert(GameRuntime::is_movement(Input::UpLeft));
    assert(GameRuntime::is_movement(Input::UpRight));
    assert(GameRuntime::is_movement(Input::DownLeft));
    assert(GameRuntime::is_movement(Input::DownRight));
    assert(!GameRuntime::is_movement(Input::Action));
    assert(!GameRuntime::is_movement(Input::QuickHealth));
    voyage::RuntimeConfig config;
    assert(!config.top_level_port_options);
    const char *temp = std::getenv("TEST_TMPDIR");
    assert(temp != nullptr);
    config.resource_dir = (std::filesystem::path(temp) / "missing-resources").string();
    config.save_dir = (std::filesystem::path(temp) / "saves").string();
    GameRuntime game(config);
    assert(!game.modern_movement());
    game.set_modern_movement(true);
    assert(game.modern_movement());
    game.set_modern_movement(false);
    assert(!game.modern_movement());
    game.set_viewport(voyage::Viewport::Wide);
    assert(!game.modern_movement());
    game.set_viewport(voyage::Viewport::Phone);
    assert(!game.modern_movement());
    assert(!game.screen_aligned_movement());
    game.set_screen_aligned_movement(true);
    assert(game.screen_aligned_movement());
    game.set_screen_aligned_movement(false);
    assert(!game.screen_aligned_movement());
    game.set_viewport(voyage::Viewport::Wide);
    assert(!game.screen_aligned_movement());
    game.set_viewport(voyage::Viewport::Phone);
    voyage::RuntimeSnapshot snapshot = game.inspect_state();
    assert(snapshot.state == -1);
    assert(!snapshot.player_present);
    assert(snapshot.camera_target_actor == -1);
    game.pause();
    game.resume();
    game.stop();
    assert(!game.running() && !game.paused());
    for (int attempt = 0; attempt < 2; ++attempt) {
        bool failed = false;
        try { game.start(); } catch (const std::exception &) { failed = true; }
        assert(failed);
        assert(!game.running() && !game.paused());
        assert(game.inspect_state().state == -1);
        assert(game.script_status() == "not running");
    }
    auto tick = &GameRuntime::tick;
    auto inspect = &GameRuntime::inspect_state;
    (void)tick;
    (void)inspect;
    return 0;
}
