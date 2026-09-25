#include "src/limbo/actor_system.hpp"
#include "src/limbo/game_canvas.hpp"
#include "src/game/game_runtime.hpp"
#include "tests/support/gameplay_scenario.hpp"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
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
    const voyage::RuntimeActorSnapshot *player = voyage::test::find_actor(state, 0);
    std::ostringstream line;
    line << checkpoint.name << '|' << checkpoint.time_ms << '|' << state.level_id << '|'
         << state.state << '|' << checkpoint.script_status << '|' << (state.player_present ? 1 : 0)
         << '|' << state.player_world_x << '|' << state.player_world_y << '|'
         << (player == nullptr ? -1 : player->health) << '|'
         << (player == nullptr ? -1 : player->max_health) << '|' << state.camera_target_actor << '|'
         << checkpoint.frame_hash;
    return line.str();
}

void drive_to_title_menu(voyage::GameRuntime &game) {
    for (int step = 0; step < 30; ++step) {
        voyage::test::advance(game, 1000);
        const voyage::RuntimeSnapshot state = game.inspect_state();
        if (state.state == 3 && state.level_id == "/startup2.scr") return;
        voyage::test::tap(game, voyage::Input::Action);
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
    const char *test_tmp = std::getenv("TEST_TMPDIR");
    if (test_tmp == nullptr) {
        std::fprintf(stderr, "save/load scenario: TEST_TMPDIR must name an isolated save root\n");
        return 2;
    }
    config.save_dir = (std::filesystem::path(test_tmp) / "save-load-scenario").string();

    std::vector<Checkpoint> observed;
    {
        voyage::GameRuntime game(config);
        game.start();
        SessionScope scope(game.session());
        voyage::test::drive_opening(game);

        ActorSystem::set_position(GameCanvas::state().actors[0], 4100, 650);
        voyage::test::advance(game, 100);
        voyage::test::tap(game, voyage::Input::Action);
        voyage::test::advance(game, 1000);
        GameCanvas::state().actors[0]->strength =
            static_cast<int16_t>(GameCanvas::state().actors[0]->strength + 3);
        ActorSystem::reset_actor(GameCanvas::state().actors[0]);
        observed.push_back(capture("save-menu", game));

        voyage::test::tap(game, voyage::Input::Right);
        voyage::test::tap(game, voyage::Input::Action);
        voyage::test::advance(game, 500);
        observed.push_back(capture("saved", game));
        game.stop();
    }

    const bool store_written =
        std::filesystem::is_regular_file(std::filesystem::path(config.save_dir) / "ESO.rs");

    {
        voyage::GameRuntime game(config);
        game.start();
        SessionScope scope(game.session());
        drive_to_title_menu(game);
        observed.push_back(capture("fresh-title", game));

        voyage::test::tap(game, voyage::Input::Right);
        voyage::test::tap(game, voyage::Input::Action);
        voyage::test::advance(game, 500);
        observed.push_back(capture("load-confirm", game));

        voyage::test::tap(game, voyage::Input::Action);
        voyage::test::advance(game, 1000);
        observed.push_back(capture("restored", game));

        bool valid = true;
        const Checkpoint &save_menu = observed[0];
        const Checkpoint &saved = observed[1];
        const Checkpoint &fresh = observed[2];
        const Checkpoint &confirm = observed[3];
        const Checkpoint &restored = observed[4];
        const voyage::RuntimeActorSnapshot *saved_player = voyage::test::find_actor(saved.state, 0);
        const voyage::RuntimeActorSnapshot *restored_player = voyage::test::find_actor(restored.state, 0);
        valid &= voyage::test::require(store_written, "save/load scenario", "Save did not write the RMS store");
        valid &= voyage::test::require(save_menu.state.level_id == "/l02_2_1.scr" &&
                                           save_menu.state.state == 3,
                                       "save/load scenario", "level-end save menu did not open");
        valid &= voyage::test::require(saved.state.state == 13 && saved_player != nullptr &&
                                           saved_player->max_health == 120,
                                       "save/load scenario", "Save did not enter its acknowledgement state");
        valid &= voyage::test::require(fresh.state.level_id == "/startup2.scr" && fresh.state.state == 3,
                                       "save/load scenario", "fresh runtime did not reach the title menu");
        valid &= voyage::test::require(confirm.state.state == 14, "save/load scenario",
                                       "Load Game did not open its confirmation prompt");
        valid &= voyage::test::require(restored.state.level_id == saved.state.level_id &&
                                           restored.state.state == 3 && restored_player != nullptr,
                                       "save/load scenario", "fresh runtime did not restore the saved level");
        valid &= voyage::test::require(restored_player != nullptr && saved_player != nullptr &&
                                           restored_player->max_health == saved_player->max_health &&
                                           restored_player->health == restored_player->max_health,
                                       "save/load scenario", "restored player state differs from the saved state");
        valid &= voyage::test::require(!game.has_error(), "save/load scenario",
                                       "the restored game entered its error screen");

        std::vector<std::string> actual;
        for (const Checkpoint &checkpoint : observed) actual.push_back(encode(checkpoint));
        if (!valid) {
            for (const std::string &line : actual) std::fprintf(stderr, "observed: %s\n", line.c_str());
        }
        if (emit) {
            if (valid && !voyage::test::write_trace(
                             argv[3],
                             "# name|time_ms|level_id|state|script_status|player_present|player_world_x|"
                             "player_world_y|player_health|player_max_health|camera_target_actor|"
                             "logical_frame_fnv1a64",
                             actual)) {
                std::fprintf(stderr, "save/load scenario: cannot write %s\n", argv[3]);
                valid = false;
            }
        } else {
            valid &= voyage::test::compare_trace(argv[2], actual, "save/load scenario");
        }

        game.stop();
        return valid ? 0 : 1;
    }
}
