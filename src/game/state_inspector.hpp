#ifndef VOYAGE_STATE_INSPECTOR_HPP
#define VOYAGE_STATE_INSPECTOR_HPP

#include <cstdint>
#include <string>

#include "src/game/game_runtime.hpp"

class GameCanvas;

namespace voyage {

RuntimeSnapshot inspect_canvas(const GameCanvas &canvas, int32_t state);

std::string state_name(int32_t state);

std::string script_status(const GameCanvas &canvas);

}

#endif
