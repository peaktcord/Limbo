#include "src/limbo/crash_report.hpp"

#include <sstream>
#include "src/limbo/game_canvas.hpp"

CrashReport &crash_report() {
    SessionContext &session = currentSession();
    if (session.crash_report == nullptr) {
        session.crash_report = session_new<CrashReport>();
    }
    return *session.crash_report;
}

std::string game_failure_context(const GameCanvas *canvas) {
    const CrashReport &report = crash_report();
    std::ostringstream out;
    out << "phase=" << report.phase << " time_ms=" << currentSession().clock.now();
    if (currentSession().game != nullptr) {
        const auto &state = GameCanvas::state();
        out << " state=" << (int)state.game_state << " level=" << state.level_script_path.value_or("null")
            << " menu=" << (int)state.menu_page << " pending_key=" << (int)state.pending_key_code;
        if (const GameData *script = state.game_data) {
            out << " script_depth=" << script->call_depth << " script_frames=[";
            if (!script->frame_pc.isNull() && !script->frame_script_id.isNull()) {
                const int32_t count = std::min(script->call_depth,
                    std::min(script->frame_pc.length(), script->frame_script_id.length()));
                for (int32_t i = 0; i < count; ++i) {
                    if (i != 0) out << ',';
                    out << script->frame_script_id[i] << '@' << script->frame_pc[i];
                }
            }
            out << ']';
        }
    }
    out << " opcode=" << report.opcode << " opcode_pc=" << report.script_pc
        << " actor_slot=" << report.actor_slot << " input=" << report.input
        << " resource=" << report.resource.value_or("null");
    if (canvas != nullptr) {
        out << " dialogue=" << canvas->dialogue_open;
        if (const Actor *player = canvas->player) {
            if (!player->world_position.isNull() && player->world_position.length() >= 2) {
                out << " player=(" << player->world_position[0] << ',' << player->world_position[1] << ')';
            }
            out << " health=" << player->health << '/' << player->max_health;
        }
    }
    return out.str();
}

void log_game_context(const GameCanvas *canvas) noexcept {
    try {
        voyage::logging::write(voyage::logging::Level::Error, "Game context", game_failure_context(canvas));
    } catch (...) {
        voyage::logging::write(voyage::logging::Level::Error, "Game context", "Unable to collect game state after failure");
    }
}

void update_game_log_context(const GameCanvas *canvas, const char *phase) noexcept {
    try {
        crash_report().phase = phase;
        voyage::logging::set_context(game_failure_context(canvas));
    } catch (...) {
        voyage::logging::set_context(phase);
    }
}
