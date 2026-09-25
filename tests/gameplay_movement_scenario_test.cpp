#include "src/game/game_runtime.hpp"
#include "src/platform/held_input.hpp"
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
    line << checkpoint.name << '|' << checkpoint.time_ms << '|' << state.state << '|'
         << checkpoint.script_status << '|' << (state.player_present ? 1 : 0) << '|'
         << state.player_world_x << '|' << state.player_world_y << '|' << state.player_screen_x << '|'
         << state.player_screen_y << '|' << state.camera_offset_x << '|' << state.camera_offset_y << '|'
         << state.camera_target_actor << '|' << checkpoint.frame_hash;
    return line.str();
}

}

int main(int argc, char **argv) {
    bool emit = argc == 4 && std::string(argv[2]) == "--emit";
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
    config.save_dir += "/movement-scenario";

    voyage::GameRuntime game(config);
    game.start();

    voyage::test::drive_opening(game);

    std::vector<Checkpoint> observed;
    observed.push_back(capture("gameplay-start", game));
    for (int attempt = 0; attempt < 40; ++attempt) {
        voyage::test::tap(game, voyage::Input::Up);
        voyage::test::advance(game, 200);
    }
    observed.push_back(capture("up-approach", game));

    for (int attempt = 0; attempt < 10; ++attempt) {
        voyage::test::tap(game, voyage::Input::Up);
        voyage::test::advance(game, 200);
    }
    observed.push_back(capture("blocked-up", game));

    for (int attempt = 0; attempt < 10; ++attempt) {
        voyage::test::tap(game, voyage::Input::Right);
        voyage::test::advance(game, 200);
    }
    observed.push_back(capture("moved-right", game));

    bool valid = true;
    const Checkpoint &start = observed[0];
    const Checkpoint &approach = observed[1];
    const Checkpoint &blocked = observed[2];
    const Checkpoint &moved = observed[3];
    valid &= voyage::test::require(!game.has_error(), "movement scenario", "the game entered its error screen");
    valid &= voyage::test::require(start.state.state == 0, "movement scenario", "opening did not reach gameplay");
    valid &= voyage::test::require(start.script_status == "script stack empty", "movement scenario",
                                   "opening script did not finish");
    valid &= voyage::test::require(start.state.player_present, "movement scenario",
                                   "player is absent at gameplay start");
    valid &= voyage::test::require(start.state.camera_target_actor == 0, "movement scenario",
                                   "camera did not return to the player");
    valid &= voyage::test::require(approach.state.player_world_y < start.state.player_world_y, "movement scenario",
                                   "Up did not move the player");
    valid &= voyage::test::require(blocked.state.player_world_x == approach.state.player_world_x &&
                                       blocked.state.player_world_y == approach.state.player_world_y,
                                   "movement scenario", "the player crossed the upper collision boundary");
    valid &= voyage::test::require(moved.state.player_world_x > blocked.state.player_world_x &&
                                       moved.state.player_world_y == blocked.state.player_world_y,
                                   "movement scenario", "Right did not move the player along the wall");

    voyage::platform::HeldInput held;
    auto advance_held = [&](int elapsed_ms) {
        while (elapsed_ms > 0) {
            int step = elapsed_ms < 16 ? elapsed_ms : 16;
            if (std::optional<voyage::Input> input = held.repeat(game.state())) {
                game.press(*input);
            }
            game.tick((uint32_t)step);
            elapsed_ms -= step;
        }
    };
    game.press(voyage::Input::Left);
    held.press(voyage::Input::Left);
    voyage::RuntimeSnapshot held_start = game.inspect_state();
    voyage::RuntimeSnapshot reset_origin = held_start;
    for (int slice = 0; slice < 5 && reset_origin.player_world_x == held_start.player_world_x; ++slice) {
        advance_held(16);
        reset_origin = game.inspect_state();
    }
    valid &= voyage::test::require(
        reset_origin.player_world_x < held_start.player_world_x,
        "movement scenario", "held movement did not produce an initial timed step");
    advance_held(50);
    voyage::RuntimeSnapshot at_threshold = game.inspect_state();
    advance_held(14);
    voyage::RuntimeSnapshot first_held_step = game.inspect_state();
    advance_held(160);
    voyage::RuntimeSnapshot repeated_held_step = game.inspect_state();
    held.release(voyage::Input::Left);
    game.release(voyage::Input::Left);
    voyage::test::advance(game, 16);
    voyage::RuntimeSnapshot released = game.inspect_state();
    voyage::test::advance(game, 160);
    voyage::RuntimeSnapshot after_release = game.inspect_state();
    valid &= voyage::test::require(
        at_threshold.player_world_x == reset_origin.player_world_x &&
            at_threshold.player_world_y == reset_origin.player_world_y,
        "movement scenario", "held movement stepped before the 50 ms threshold");
    valid &= voyage::test::require(
        first_held_step.player_world_x < at_threshold.player_world_x,
        "movement scenario", "held movement did not step on the first host slice after 50 ms");
    valid &= voyage::test::require(
        repeated_held_step.player_world_x < first_held_step.player_world_x,
        "movement scenario", "the adapter did not repeat a held movement key");
    valid &= voyage::test::require(
        after_release.player_world_x == released.player_world_x &&
            after_release.player_world_y == released.player_world_y,
        "movement scenario", "movement continued after key release was consumed");

    if (emit) {
        if (!valid) {
            game.stop();
            return 1;
        }
        std::vector<std::string> lines;
        for (const Checkpoint &checkpoint : observed) lines.push_back(encode(checkpoint));
        if (!voyage::test::write_trace(
                argv[3],
                "# name|time_ms|state|script_status|player_present|player_world_x|player_world_y|"
                "player_screen_x|player_screen_y|camera_offset_x|camera_offset_y|camera_target_actor|"
                "logical_frame_fnv1a64",
                lines)) {
            std::fprintf(stderr, "gameplay scenario: cannot write %s\n", argv[3]);
            game.stop();
            return 1;
        }
        game.stop();
        return 0;
    }

    std::vector<std::string> actual;
    for (const Checkpoint &checkpoint : observed) actual.push_back(encode(checkpoint));
    valid &= voyage::test::compare_trace(argv[2], actual, "movement scenario");

    game.stop();
    return valid ? 0 : 1;
}
