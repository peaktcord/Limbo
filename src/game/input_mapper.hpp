#ifndef VOYAGE_INPUT_MAPPER_HPP
#define VOYAGE_INPUT_MAPPER_HPP

#include <cstdint>

#include "src/game/game_runtime.hpp"

class KeyBindings;

namespace voyage {

int32_t midp_key_code(Input input, int32_t game_state, const KeyBindings &bindings);

}

#endif
