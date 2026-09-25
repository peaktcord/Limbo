#include "src/game/game_runtime.hpp"
#include "tests/support/gameplay_scenario.hpp"

#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

namespace {

struct Observation {
    uint64_t frame_hash;
    uint64_t simulation_time_ms;
    int32_t state;
    std::string level_id;
    std::string script_status;
    int32_t player_world_x;
    int32_t player_world_y;

    bool operator==(const Observation &other) const {
        return frame_hash == other.frame_hash && simulation_time_ms == other.simulation_time_ms &&
               state == other.state && level_id == other.level_id && script_status == other.script_status &&
               player_world_x == other.player_world_x && player_world_y == other.player_world_y;
    }
};

void drive_step(voyage::GameRuntime &game, int step) {
    voyage::test::advance(game, 1000);
    voyage::test::tap(game, step % 2 == 0 ? voyage::Input::Action : voyage::Input::Down);
}

Observation capture(voyage::GameRuntime &game) {
    voyage::RuntimeSnapshot state = game.inspect_state();
    return Observation{voyage::test::hash_logical_frame(game.copy_frame()),
                       game.simulation_time_ms(),
                       state.state,
                       state.level_id,
                       game.script_status(),
                       state.player_world_x,
                       state.player_world_y};
}

std::vector<Observation> run_isolated(const voyage::RuntimeConfig &config, int steps) {
    voyage::GameRuntime game(config);
    game.start();
    std::vector<Observation> observations;
    for (int step = 0; step < steps; ++step) {
        drive_step(game, step);
        observations.push_back(capture(game));
    }
    assert(!game.has_error());
    return observations;
}

}

int main(int argc, char **argv) {
    assert(argc == 2);
    const char *test_tmp = std::getenv("TEST_TMPDIR");
    assert(test_tmp != nullptr);
    const std::filesystem::path temp_root(test_tmp);

    voyage::RuntimeConfig phone;
    phone.resource_dir = argv[1];
    phone.save_dir = (temp_root / "phone-rms").string();
    phone.random_seed = 0x1111222233334444LL;

    voyage::RuntimeConfig wide;
    wide.resource_dir = argv[1];
    wide.save_dir = (temp_root / "wide-rms").string();
    wide.random_seed = 0x5555666677778888LL;
    wide.viewport = voyage::Viewport::Wide;

    constexpr int kSteps = 12;
    const std::vector<Observation> isolated_phone = run_isolated(phone, kSteps);
    const std::vector<Observation> isolated_wide = run_isolated(wide, kSteps);

    voyage::GameRuntime live_phone(phone);
    voyage::GameRuntime live_wide(wide);
    live_phone.start();
    live_wide.start();
    for (int step = 0; step < kSteps; ++step) {
        drive_step(live_phone, step);
        assert(capture(live_phone) == isolated_phone[(size_t)step]);
        drive_step(live_wide, step);
        assert(capture(live_wide) == isolated_wide[(size_t)step]);
    }
    assert(!live_phone.has_error());
    assert(!live_wide.has_error());
    return 0;
}
