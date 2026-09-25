#include "src/game/game_runtime.hpp"
#include "src/limbo/game_canvas.hpp"

#include <cassert>
#include <cstdlib>
#include <filesystem>

namespace {
bool completed = false;
void require_completion() {
    if (!completed) std::_Exit(99);
}
}

int main(int argc, char **argv) {
    std::atexit(require_completion);
    assert(argc == 2);
    const char *temp = std::getenv("TEST_TMPDIR");
    assert(temp != nullptr);
    voyage::RuntimeConfig config;
    config.resource_dir = argv[1];
    config.save_dir = (std::filesystem::path(temp) / "lifecycle-saves").string();
    config.random_seed = 42;
    auto missing = config;
    missing.resource_dir = (std::filesystem::path(temp) / "missing-resources").string();
    voyage::GameRuntime failed_game(missing);
    bool failed = false;
    try { failed_game.start(); } catch (const std::exception &) { failed = true; }
    assert(failed);
    assert(failed_game.session().objects.size() == 0);
    assert(failed_game.session().current_display == nullptr);
    assert(failed_game.session().game == nullptr);
    assert(!failed_game.session().clock.pinned);
    voyage::GameRuntime game(config);
    game.start();
    SessionScope scope(game.session());
    GameCanvas *canvas = GameCanvas::state().instance;
    assert(game.running() && !game.paused());
    assert(Display::getDisplay()->getCurrent() == canvas);
    assert(GameCanvas::state().version_text == "1.0.10");
    const auto initial_frame = game.copy_frame().pixels;
    const auto initial_state = game.state();
    const auto initial_objects = game.session().objects.size();
    game.start();
    assert(GameCanvas::state().instance == canvas);
    assert(game.session().objects.size() == initial_objects);

    for (int i = 0; i < 100 && canvas->is_terminal_state(); ++i) game.tick(16);
    assert(!canvas->is_terminal_state());
    const auto pause_state = game.state();
    const auto pause_frame = game.copy_frame().pixels;
    game.pause();
    game.pause();
    assert(game.paused());
    const auto script = game.script_status();
    game.tick(1000);
    assert(game.running() && game.paused());
    assert(game.state() == pause_state);
    assert(game.script_status() == script);
    assert(game.copy_frame().pixels == pause_frame);
    game.resume();
    game.resume();
    assert(!game.paused() && GameCanvas::state().instance == canvas);

    for (int state : {6, 7, 15}) {
        GameCanvas::state().game_state = static_cast<int8_t>(state);
        game.pause();
        assert(!game.paused());
    }
    GameCanvas::state().game_state = static_cast<int8_t>(pause_state);
    game.stop();
    assert(!game.running() && !game.paused());
    assert(!game.session().clock.pinned);
    assert(Display::getDisplay()->getCurrent() == nullptr);
    const auto stopped_time = game.simulation_time_ms();
    game.stop();
    game.pause();
    game.resume();
    game.tick(500);
    game.press(voyage::Input::Action);
    game.release(voyage::Input::Action);
    assert(game.state() == -1 && game.simulation_time_ms() == stopped_time);

    game.start();
    canvas = GameCanvas::state().instance;
    assert(game.running() && !game.paused());
    assert(game.simulation_time_ms() == 0);
    assert(game.state() == initial_state);
    assert(game.copy_frame().pixels == initial_frame);
    assert(game.session().objects.size() == initial_objects);

    voyage::GameRuntime other(config);
    other.start();
    const auto other_frame = other.copy_frame().pixels;
    canvas->request_exit();
    assert(game.state() == 12);
    game.tick(1999);
    assert(game.running() && canvas->exit_delay_ms == 1);
    game.tick(1);
    assert(!game.running());
    assert(Display::getDisplay()->getCurrent() == nullptr);
    assert(other.running() && other.copy_frame().pixels == other_frame);
    other.tick(16);
    assert(other.simulation_time_ms() == 16);

    game.start();
    canvas = GameCanvas::state().instance;
    canvas->crash_context = "controlled error screen";
    game.tick(50);
    assert(game.running() && game.has_error());
    for (int i = 0; i < 3; ++i) {
        game.press(voyage::Input::Action);
        game.release(voyage::Input::Action);
    }
    assert(canvas->exit_delay_ms == 2000);
    game.tick(2000);
    assert(!game.running());
    game.start();
    assert(game.running() && !game.has_error());
    assert(game.copy_frame().pixels == initial_frame);
    completed = true;
    return 0;
}
