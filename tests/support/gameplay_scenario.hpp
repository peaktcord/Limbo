#ifndef VOYAGE_TESTS_GAMEPLAY_SCENARIO_HPP
#define VOYAGE_TESTS_GAMEPLAY_SCENARIO_HPP

#include <cstdint>
#include <string>
#include <vector>

#include "src/game/game_runtime.hpp"

namespace voyage::test {

void advance(GameRuntime &game, int elapsed_ms);
void tap(GameRuntime &game, Input input);
void drive_opening(GameRuntime &game);
uint64_t hash_logical_frame(const Frame &frame);

bool require(bool condition, const char *scenario, const char *message);
std::vector<std::string> read_trace(const char *path);
bool write_trace(const char *path, const std::string &header, const std::vector<std::string> &lines);
bool compare_trace(const char *path, const std::vector<std::string> &actual, const char *scenario);

const RuntimeActorSnapshot *find_actor(const RuntimeSnapshot &snapshot, int32_t slot);

}

#endif
