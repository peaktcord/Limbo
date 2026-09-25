#ifndef OBLIVION_CRASH_REPORT_HPP
#define OBLIVION_CRASH_REPORT_HPP

#include "src/common/session.hpp"

class GameCanvas;

class CrashReport {
public:
    std::string message;
    const char *phase = "startup";
    int32_t script_pc = -1;
    int32_t opcode = -1;
    int32_t actor_slot = -1;
    int32_t input = -1;
    std::optional<std::string> resource;
};

CrashReport &crash_report();
std::string game_failure_context(const GameCanvas *canvas);
void log_game_context(const GameCanvas *canvas) noexcept;
void update_game_log_context(const GameCanvas *canvas, const char *phase) noexcept;

#endif
