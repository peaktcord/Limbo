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

void encode_actor(std::ostringstream &line, const voyage::RuntimeActorSnapshot *actor) {
    if (actor == nullptr) {
        line << "|0|-1|-1|-1|-1|-1|-1";
        return;
    }
    line << "|1|" << (actor->alive ? 1 : 0) << '|' << (actor->attacking ? 1 : 0) << '|'
         << actor->health << '|' << actor->world_x << '|' << actor->world_y << '|'
         << actor->target_actor;
}

std::string encode(const Checkpoint &checkpoint) {
    const voyage::RuntimeSnapshot &state = checkpoint.state;
    std::ostringstream line;
    line << checkpoint.name << '|' << checkpoint.time_ms << '|' << state.state << '|'
         << checkpoint.script_status << '|' << state.camera_target_actor;
    encode_actor(line, voyage::test::find_actor(state, 0));
    encode_actor(line, voyage::test::find_actor(state, 6));
    encode_actor(line, voyage::test::find_actor(state, 7));
    line << '|' << checkpoint.frame_hash;
    return line.str();
}

void attack(voyage::GameRuntime &game, int count) {
    for (int attempt = 0; attempt < count; ++attempt) {
        voyage::test::tap(game, voyage::Input::Action);
        voyage::test::advance(game, 300);
    }
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
    config.save_dir += "/combat-scenario";

    voyage::GameRuntime game(config);
    game.start();
    voyage::test::drive_opening(game);

    std::vector<Checkpoint> observed;
    observed.push_back(capture("gameplay-start", game));

    for (int attempt = 0; attempt < 40; ++attempt) {
        voyage::test::tap(game, voyage::Input::Down);
        voyage::test::advance(game, 200);
    }
    for (int attempt = 0; attempt < 12; ++attempt) {
        voyage::test::tap(game, voyage::Input::Left);
        voyage::test::advance(game, 200);
    }
    observed.push_back(capture("guards-engaged", game));

    attack(game, 1);
    observed.push_back(capture("first-strike", game));
    attack(game, 6);
    observed.push_back(capture("first-guard-defeated", game));
    attack(game, 3);
    observed.push_back(capture("second-guard-wounded", game));
    attack(game, 3);
    observed.push_back(capture("courtyard-clear", game));

    bool valid = true;
    const Checkpoint &start = observed[0];
    const Checkpoint &engaged = observed[1];
    const Checkpoint &struck = observed[2];
    const Checkpoint &first_defeated = observed[3];
    const Checkpoint &second_wounded = observed[4];
    const Checkpoint &clear = observed[5];
    const voyage::RuntimeActorSnapshot *start_player = voyage::test::find_actor(start.state, 0);
    const voyage::RuntimeActorSnapshot *engaged_player = voyage::test::find_actor(engaged.state, 0);
    const voyage::RuntimeActorSnapshot *engaged_first = voyage::test::find_actor(engaged.state, 7);
    const voyage::RuntimeActorSnapshot *engaged_second = voyage::test::find_actor(engaged.state, 6);
    const voyage::RuntimeActorSnapshot *struck_player = voyage::test::find_actor(struck.state, 0);
    const voyage::RuntimeActorSnapshot *struck_first = voyage::test::find_actor(struck.state, 7);
    const voyage::RuntimeActorSnapshot *wounded_player = voyage::test::find_actor(second_wounded.state, 0);
    const voyage::RuntimeActorSnapshot *wounded_second = voyage::test::find_actor(second_wounded.state, 6);

    valid &= voyage::test::require(!game.has_error(), "combat scenario", "the game entered its error screen");
    valid &= voyage::test::require(start.state.state == 0 && start.script_status == "script stack empty",
                                   "combat scenario", "opening did not reach free play");
    valid &= voyage::test::require(start_player != nullptr && start_player->alive, "combat scenario",
                                   "player is absent at gameplay start");
    valid &= voyage::test::require(engaged_player != nullptr && start_player != nullptr &&
                                       engaged_player->health < start_player->health,
                                   "combat scenario", "guards did not damage the player");
    valid &= voyage::test::require(engaged_player != nullptr && engaged_player->target_actor == 7,
                                   "combat scenario", "player did not acquire the nearer guard");
    valid &= voyage::test::require(engaged_first != nullptr && engaged_first->target_actor == 0,
                                   "combat scenario", "nearer guard did not acquire the player");
    valid &= voyage::test::require(engaged_first != nullptr && engaged_second != nullptr &&
                                       engaged_first->hostile_to_player && engaged_second->hostile_to_player,
                                   "combat scenario", "engaged actors were not hostile to the player");
    valid &= voyage::test::require(struck_player != nullptr && struck_player->attacking,
                                   "combat scenario", "Action did not enter the player's attack state");
    valid &= voyage::test::require(struck_first != nullptr && engaged_first != nullptr &&
                                       struck_first->health < engaged_first->health,
                                   "combat scenario", "Action did not damage the nearer guard");
    valid &= voyage::test::require(voyage::test::find_actor(first_defeated.state, 7) == nullptr,
                                   "combat scenario", "nearer guard was not defeated");
    valid &= voyage::test::require(wounded_second != nullptr && wounded_second->health < wounded_second->max_health,
                                   "combat scenario", "player did not transfer attacks to the second guard");
    valid &= voyage::test::require(wounded_player != nullptr && wounded_player->target_actor == 6,
                                   "combat scenario", "player did not acquire the remaining guard");
    valid &= voyage::test::require(voyage::test::find_actor(clear.state, 6) == nullptr,
                                   "combat scenario", "second guard was not defeated");
    valid &= voyage::test::require(voyage::test::find_actor(clear.state, 0) != nullptr &&
                                       voyage::test::find_actor(clear.state, 0)->alive,
                                   "combat scenario", "player did not survive the encounter");
    for (const Checkpoint &checkpoint : observed) {
        valid &= voyage::test::require(checkpoint.state.camera_target_actor == 0, "combat scenario",
                                       "camera left the player during combat");
    }

    std::vector<std::string> actual;
    for (const Checkpoint &checkpoint : observed) actual.push_back(encode(checkpoint));
    if (emit) {
        if (valid && !voyage::test::write_trace(
                         argv[3],
                         "# name|time_ms|state|script_status|camera_target|"
                         "player_present|alive|attacking|health|x|y|target|"
                         "guard6_present|alive|attacking|health|x|y|target|"
                         "guard7_present|alive|attacking|health|x|y|target|logical_frame_fnv1a64",
                         actual)) {
            std::fprintf(stderr, "combat scenario: cannot write %s\n", argv[3]);
            valid = false;
        }
    } else {
        valid &= voyage::test::compare_trace(argv[2], actual, "combat scenario");
    }

    game.stop();
    return valid ? 0 : 1;
}
