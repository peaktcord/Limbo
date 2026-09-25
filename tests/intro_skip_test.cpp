#include "src/game/game_runtime.hpp"
#include "tests/support/gameplay_scenario.hpp"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <string>

namespace {

constexpr int kCrawlState = 10;
constexpr int kPlayState = 0;

bool reach_crawl(voyage::GameRuntime &game, int &steps) {
    for (steps = 0; steps < 200; ++steps) {
        voyage::test::advance(game, 500);
        if (game.inspect_state().state == kCrawlState) return true;
        voyage::test::tap(game, voyage::Input::Action);
    }
    return false;
}

voyage::RuntimeConfig config_for(const char *resources, const char *suffix) {
    voyage::RuntimeConfig config;
    config.resource_dir = resources;
    config.random_seed = 0x4f626c6976696f6eLL;
    const char *test_tmp = std::getenv("TEST_TMPDIR");
    assert(test_tmp != nullptr);
    config.save_dir = std::string(test_tmp) + "/intro-skip-" + suffix;
    return config;
}

}

int main(int argc, char **argv) {
    if (argc != 2) {
        std::fprintf(stderr, "usage: %s RESOURCE_DIR\n", argv[0]);
        return 2;
    }

    uint64_t skipped_at = 0;
    {
        voyage::GameRuntime game(config_for(argv[1], "skip"));
        game.start();
        int steps = 0;
        assert(reach_crawl(game, steps));
        voyage::test::advance(game, 1000);
        assert(game.inspect_state().state == kCrawlState);

        voyage::test::tap(game, voyage::Input::Action);
        voyage::test::advance(game, 500);
        const voyage::RuntimeSnapshot state = game.inspect_state();
        assert(state.state == kPlayState);
        assert(state.player_present);
        skipped_at = game.simulation_time_ms();
        assert(!game.has_error());
        game.stop();
    }

    {
        voyage::GameRuntime game(config_for(argv[1], "wait"));
        game.start();
        int steps = 0;
        assert(reach_crawl(game, steps));

        bool reached_play = false;
        for (int tick = 0; tick < 200 && !reached_play; ++tick) {
            voyage::test::advance(game, 500);
            reached_play = game.inspect_state().state == kPlayState;
        }
        assert(reached_play);
        const voyage::RuntimeSnapshot state = game.inspect_state();
        assert(state.player_present);
        assert(!game.has_error());
        assert(game.simulation_time_ms() > skipped_at);
        game.stop();
    }

    return 0;
}
