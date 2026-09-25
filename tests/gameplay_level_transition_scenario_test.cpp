#include "src/limbo/actor_system.hpp"
#include "src/limbo/game_canvas.hpp"
#include "src/game/game_runtime.hpp"
#include "tests/support/gameplay_scenario.hpp"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>

namespace {

struct Checkpoint {
    std::string name;
    uint64_t time_ms;
    voyage::RuntimeSnapshot state;
    std::string script_status;
    uint64_t frame_hash;
};

Checkpoint capture(const char *name, voyage::GameRuntime &game) {
    return Checkpoint{name,
                      game.simulation_time_ms(),
                      game.inspect_state(),
                      game.script_status(),
                      voyage::test::hash_logical_frame(game.copy_frame())};
}

std::string encode(const Checkpoint &checkpoint) {
    const voyage::RuntimeSnapshot &state = checkpoint.state;
    std::ostringstream line;
    line << checkpoint.name << '|' << checkpoint.time_ms << '|' << state.level_id << '|'
         << state.state << '|' << checkpoint.script_status << '|' << (state.player_present ? 1 : 0)
         << '|' << state.player_world_x << '|' << state.player_world_y << '|'
         << state.player_screen_x << '|' << state.player_screen_y << '|' << state.camera_offset_x
         << '|' << state.camera_offset_y << '|' << state.camera_target_actor << '|'
         << state.actors.size() << '|' << checkpoint.frame_hash;
    return line.str();
}

void drive_destination_opening(voyage::GameRuntime &game) {
    for (int step = 0; step < 90; ++step) {
        voyage::test::tap(game, voyage::Input::Action);
        voyage::test::advance(game, 1000);
        const voyage::RuntimeSnapshot state = game.inspect_state();
        if (state.camera_target_actor == 0 && game.script_status() == "script stack empty") return;
    }
}

}

int main(int argc, char **argv) {
    const bool emit = argc == 4 && std::string(argv[2]) == "--emit";
    if (argc != 3 && !emit) {
        std::fprintf(stderr, "usage: %s RESOURCE_DIR EXPECTED_TRACE\n"
                             "       %s RESOURCE_DIR --emit OUTPUT_TRACE\n",
                     argv[0], argv[0]);
        return 2;
    }

    voyage::RuntimeConfig config;
    config.resource_dir = argv[1];
    config.random_seed = 0x4f626c6976696f6eLL;
    if (const char *test_tmp = std::getenv("TEST_TMPDIR")) config.save_dir = test_tmp;
    config.save_dir += "/level-transition-scenario";

    voyage::GameRuntime game(config);
    game.start();
    SessionScope scope(game.session());
    voyage::test::drive_opening(game);

    std::vector<Checkpoint> observed;
    ActorSystem::set_position(GameCanvas::state().actors[0], 4100, 650);
    voyage::test::advance(game, 100);
    observed.push_back(capture("source-exit", game));

    voyage::test::tap(game, voyage::Input::Action);
    voyage::test::advance(game, 500);
    observed.push_back(capture("destination-loading", game));
    voyage::test::advance(game, 500);
    observed.push_back(capture("destination-menu", game));

    voyage::test::tap(game, voyage::Input::Right);
    voyage::test::tap(game, voyage::Input::Right);
    voyage::test::tap(game, voyage::Input::Action);
    voyage::test::advance(game, 1000);
    observed.push_back(capture("destination-script", game));

    drive_destination_opening(game);
    observed.push_back(capture("destination-ready", game));
    voyage::test::tap(game, voyage::Input::Right);
    voyage::test::advance(game, 500);
    observed.push_back(capture("destination-moved", game));

    bool valid = true;
    const Checkpoint &source = observed[0];
    const Checkpoint &loading = observed[1];
    const Checkpoint &menu = observed[2];
    const Checkpoint &script = observed[3];
    const Checkpoint &ready = observed[4];
    const Checkpoint &moved = observed[5];
    valid &= voyage::test::require(!game.has_error(), "level transition scenario",
                                   "the game entered its error screen");
    valid &= voyage::test::require(source.state.level_id == "/l01_1.scr" && source.state.state == 0,
                                   "level transition scenario", "source exit was not in level-one free play");
    valid &= voyage::test::require(source.state.player_world_x == 4100 && source.state.player_world_y == 650,
                                   "level transition scenario", "player was not staged on the real exit trigger");
    valid &= voyage::test::require(loading.state.level_id == "/l02_2_1.scr" && loading.state.state == 15,
                                   "level transition scenario", "Action did not enter destination loading");
    valid &= voyage::test::require(menu.state.level_id == "/l02_2_1.scr" && menu.state.state == 3,
                                   "level transition scenario", "destination prompt did not open");
    valid &= voyage::test::require(script.state.level_id == "/l02_2_1.scr" && script.state.state == 0 &&
                                       script.script_status != "script stack empty",
                                   "level transition scenario", "Continue did not start the destination script");
    valid &= voyage::test::require(ready.state.level_id == "/l02_2.scr" && ready.state.state == 0 &&
                                       ready.script_status == "script stack empty",
                                   "level transition scenario", "destination opening did not reach free play");
    valid &= voyage::test::require(ready.state.player_present && ready.state.camera_target_actor == 0,
                                   "level transition scenario", "destination camera did not return to the player");
    valid &= voyage::test::require(moved.state.level_id == ready.state.level_id &&
                                       moved.state.player_world_x > ready.state.player_world_x &&
                                       moved.state.player_world_y == ready.state.player_world_y,
                                   "level transition scenario", "player could not move in the destination level");

    std::vector<std::string> actual;
    for (const Checkpoint &checkpoint : observed) actual.push_back(encode(checkpoint));
    if (emit) {
        if (valid && !voyage::test::write_trace(
                         argv[3],
                         "# name|time_ms|level_id|state|script_status|player_present|player_world_x|"
                         "player_world_y|player_screen_x|player_screen_y|camera_offset_x|camera_offset_y|"
                         "camera_target_actor|active_actor_count|logical_frame_fnv1a64",
                         actual)) {
            std::fprintf(stderr, "level transition scenario: cannot write %s\n", argv[3]);
            valid = false;
        }
    } else {
        valid &= voyage::test::compare_trace(argv[2], actual, "level transition scenario");
    }

    game.stop();
    return valid ? 0 : 1;
}
