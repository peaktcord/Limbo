#include "tests/support/gameplay_scenario.hpp"

#include <algorithm>
#include <cstdio>
#include <fstream>

namespace voyage::test {

void advance(GameRuntime &game, int elapsed_ms) {
    while (elapsed_ms > 0) {
        int step = std::min(elapsed_ms, 16);
        game.tick((uint32_t)step);
        elapsed_ms -= step;
    }
}

void tap(GameRuntime &game, Input input) {
    game.press(input);
    advance(game, 120);
    game.release(input);
}

void drive_opening(GameRuntime &game) {
    for (int step = 0; step < 120; ++step) {
        advance(game, 1000);
        const Input input = step % 2 == 0 ? Input::Action : Input::Down;
        if (input == Input::Action && game.inspect_state().state == 10) {
            advance(game, 120);
        } else {
            tap(game, input);
        }
        RuntimeSnapshot state = game.inspect_state();
        if (step >= 89 && state.state == 0 && state.player_present && state.camera_target_actor == 0 &&
            game.script_status() == "script stack empty") {
            return;
        }
    }
}

uint64_t hash_logical_frame(const Frame &frame) {
    uint64_t hash = 1469598103934665603ULL;
    for (int32_t y = 0; y < frame.height; ++y) {
        for (int32_t x = 0; x < frame.logical_width; ++x) {
            hash ^= frame.pixels[(size_t)y * (size_t)frame.surface_width + (size_t)x];
            hash *= 1099511628211ULL;
        }
    }
    return hash;
}

bool require(bool condition, const char *scenario, const char *message) {
    if (condition) return true;
    std::fprintf(stderr, "%s: %s\n", scenario, message);
    return false;
}

std::vector<std::string> read_trace(const char *path) {
    std::ifstream input(path);
    if (!input) return {};
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (!line.empty() && line[0] != '#') lines.push_back(line);
    }
    return lines;
}

bool write_trace(const char *path, const std::string &header, const std::vector<std::string> &lines) {
    std::ofstream output(path, std::ios::binary);
    if (!output) return false;
    output << header << '\n';
    for (const std::string &line : lines) output << line << '\n';
    return output.good();
}

bool compare_trace(const char *path, const std::vector<std::string> &actual, const char *scenario) {
    std::vector<std::string> expected = read_trace(path);
    bool valid = true;
    if (expected.size() != actual.size()) {
        std::fprintf(stderr, "%s: expected %zu checkpoints, found %zu\n", scenario, expected.size(), actual.size());
        valid = false;
    }
    for (size_t index = 0; index < actual.size(); ++index) {
        if (index >= expected.size() || actual[index] != expected[index]) {
            std::fprintf(stderr, "checkpoint %zu mismatch\n  expected: %s\n  actual:   %s\n", index,
                         index < expected.size() ? expected[index].c_str() : "<missing>", actual[index].c_str());
            valid = false;
        }
    }
    return valid;
}

const RuntimeActorSnapshot *find_actor(const RuntimeSnapshot &snapshot, int32_t slot) {
    for (const RuntimeActorSnapshot &actor : snapshot.actors) {
        if (actor.slot == slot) return &actor;
    }
    return nullptr;
}

}
