#include "src/game/game_runtime.hpp"

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

namespace {

struct Observation {
    uint64_t frame_hash;
    uint64_t simulation_time_ms;
    int32_t state;
    std::string script_status;
};

uint64_t hash_frame(const voyage::Frame &frame) {
    uint64_t hash = 1469598103934665603ULL;
    for (uint32_t pixel : frame.pixels) {
        hash ^= pixel;
        hash *= 1099511628211ULL;
    }
    return hash;
}

uint64_t hash_trace(const std::vector<Observation> &observations) {
    uint64_t hash = 1469598103934665603ULL;
    auto mix = [&hash](uint64_t value) {
        hash ^= value;
        hash *= 1099511628211ULL;
    };
    for (const Observation &observation : observations) {
        mix(observation.frame_hash);
        mix(observation.simulation_time_ms);
        mix((uint32_t)observation.state);
        for (unsigned char byte : observation.script_status) mix(byte);
        mix(0xff);
    }
    return hash;
}

void advance(voyage::GameRuntime &game, int elapsed_ms) {
    while (elapsed_ms > 0) {
        int step = std::min(elapsed_ms, 16);
        game.tick((uint32_t)step);
        elapsed_ms -= step;
    }
}

std::vector<Observation> replay(const voyage::RuntimeConfig &config) {
    voyage::GameRuntime game(config);
    game.start();
    std::vector<Observation> result;
    for (int frame = 0; frame < 50; ++frame) {
        advance(game, 1000);
        voyage::Input input = (frame % 2 == 0) ? voyage::Input::Action : voyage::Input::Down;
        game.press(input);
        advance(game, 120);
        game.release(input);
        result.push_back(Observation{hash_frame(game.copy_frame()), game.simulation_time_ms(), game.state(),
                                     game.script_status()});
    }
    assert(!game.has_error());
    assert(game.simulation_time_ms() == 56000);
    bool saw_play = false;
    bool saw_dialogue_wait = false;
    for (const Observation &observation : result) {
        saw_play = saw_play || observation.state == 0;
        saw_dialogue_wait = saw_dialogue_wait ||
                            observation.script_status ==
                                "dialogue box open -- press the action key to advance";
    }
    assert(saw_play);
    assert(saw_dialogue_wait);
    game.stop();
    return result;
}

}

int main(int argc, char **argv) {
    assert(argc == 2);
    voyage::RuntimeConfig config;
    config.resource_dir = argv[1];
    config.random_seed = 0x4f626c6976696f6eLL;
    if (const char *test_tmp = std::getenv("TEST_TMPDIR")) config.save_dir = test_tmp;

    config.save_dir += "/replay";
    std::vector<Observation> trace = replay(config);
    uint64_t digest = hash_trace(trace);
    std::fprintf(stderr, "deterministic opening trace digest: %llu\n", (unsigned long long)digest);
    assert(digest == 8427914607596559517ULL);
    return 0;
}
