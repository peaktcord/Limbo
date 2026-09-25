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
    int32_t health;
    int32_t max_health;
    bool dead;
    uint64_t frame_hash;
};

Checkpoint capture(const char *name, voyage::GameRuntime &game) {
    Actor *player = GameCanvas::state().actors[0];
    return Checkpoint{name,
                      game.simulation_time_ms(),
                      game.inspect_state(),
                      player == nullptr ? -1 : player->health,
                      player == nullptr ? -1 : player->max_health,
                      player == nullptr || player->is_dead != 0,
                      voyage::test::hash_logical_frame(game.copy_frame())};
}

std::string encode(const Checkpoint &checkpoint) {
    std::ostringstream line;
    line << checkpoint.name << '|' << checkpoint.time_ms << '|' << checkpoint.state.state << '|'
         << checkpoint.state.level_id << '|' << (checkpoint.state.player_present ? 1 : 0) << '|'
         << checkpoint.health << '|' << checkpoint.max_health << '|' << (checkpoint.dead ? 1 : 0) << '|'
         << checkpoint.state.player_world_x << '|' << checkpoint.state.player_world_y << '|'
         << checkpoint.state.camera_target_actor << '|' << checkpoint.frame_hash;
    return line.str();
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
    config.save_dir += "/death-restart-scenario";

    voyage::GameRuntime game(config);
    game.start();
    SessionScope scope(game.session());
    voyage::test::drive_opening(game);

    std::vector<Checkpoint> observed;
    observed.push_back(capture("gameplay-start", game));

    Actor *player = GameCanvas::state().actors[0];
    ActorSystem::apply_damage(player->health + 1, player, nullptr, false, true);
    observed.push_back(capture("lethal-hit", game));
    voyage::test::advance(game, 300);
    observed.push_back(capture("game-over", game));

    voyage::test::tap(game, voyage::Input::Action);
    observed.push_back(capture("restarted", game));
    for (int step = 0; step < 12; ++step) {
        voyage::test::tap(game, voyage::Input::Up);
        voyage::test::advance(game, 200);
    }
    observed.push_back(capture("movement-restored", game));

    bool valid = true;
    valid &= voyage::test::require(!game.has_error(), "death/restart scenario", "the game entered its error screen");
    valid &= voyage::test::require(observed[0].state.state == 0 && !observed[0].dead,
                                   "death/restart scenario", "opening did not reach live free play");
    valid &= voyage::test::require(observed[1].state.state == 0 && observed[1].dead && observed[1].health <= 0,
                                   "death/restart scenario", "lethal damage did not mark the player dead");
    valid &= voyage::test::require(observed[2].state.state == 11 && !observed[2].dead &&
                                       observed[2].health == observed[2].max_health,
                                   "death/restart scenario", "death did not enter game-over with a reset player");
    valid &= voyage::test::require(observed[3].state.state == 0 && !observed[3].dead &&
                                       observed[3].state.camera_target_actor == 0,
                                   "death/restart scenario", "left softkey did not restore player-controlled gameplay");
    valid &= voyage::test::require(observed[4].state.player_world_x == observed[3].state.player_world_x &&
                                       observed[4].state.player_world_y < observed[3].state.player_world_y,
                                   "death/restart scenario", "movement did not resume after restart");

    std::vector<std::string> actual;
    for (const Checkpoint &checkpoint : observed) actual.push_back(encode(checkpoint));
    if (emit) {
        if (valid && !voyage::test::write_trace(
                         argv[3],
                         "# name|time_ms|state|level_id|player_present|health|max_health|dead|player_world_x|"
                         "player_world_y|camera_target_actor|logical_frame_fnv1a64",
                         actual)) {
            std::fprintf(stderr, "death/restart scenario: cannot write %s\n", argv[3]);
            valid = false;
        }
    } else {
        valid &= voyage::test::compare_trace(argv[2], actual, "death/restart scenario");
    }

    game.stop();
    return valid ? 0 : 1;
}
