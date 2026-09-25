#include "src/limbo/game_canvas.hpp"
#include "src/limbo/crash_report.hpp"

namespace {
bool tick_failed(GameCanvas *canvas, const char *message) noexcept {
    voyage::logging::write(voyage::logging::Level::Error, "Game loop", message);
    log_game_context(canvas);
    try {
        canvas->crash_context = std::string("Game loop failure");
        crash_report().message = std::string(message);
        canvas->repaint();
        canvas->serviceRepaints();
    } catch (const std::exception &nested) {
        voyage::logging::exception("Game loop", "Drawing the error screen", nested);
    } catch (...) {
        voyage::logging::write(voyage::logging::Level::Error, "Game loop", "Unknown exception drawing the error screen");
    }
    return false;
}
}

bool GameCanvas::tick(int64_t l) {
    int32_t n = 0;
    int32_t n2 = 0;
    SharedArray<int32_t> nArray(2);
    bool bl = false;
    if (!this->game_loop_running) return false;
    if (l < 0) l = 0;
    if (this->text_transition_delay_ms >= 0 && state().game_state != 4 && state().game_state != 9 && state().game_state != 10) {
        this->text_transition_delay_ms = -1;
    }
    if (this->text_transition_delay_ms > 0) {
        this->text_transition_delay_ms -= l;
        if (this->text_transition_delay_ms < 0) this->text_transition_delay_ms = 0;
    }
    if (this->exit_delay_ms >= 0) {
        this->exit_delay_ms -= l;
        if (this->exit_delay_ms > 0) {
            this->repaint();
            this->serviceRepaints();
            return true;
        }
        this->exit_delay_ms = -1;
        this->game_loop_running = false;
        return false;
    }
    if (this->crash_context.has_value()) return false;
    if (this->paused) return true;
    if (state().game_state == 12) return false;

    try {
        if (this->menu_screen->is_open == 1) {
            crash_report().phase = "menu animation";
            this->menu_screen->a(l);
        } else if (state().game_state != 3 && state().game_state != 10 && state().game_state != 9 && state().game_state != 13) {
            crash_report().phase = "script update";
            MagicEffects::update(l);
            state().game_data->update_scripts(l);
        }
        crash_report().phase = "input dispatch";
        this->dispatch_pending_input(l);
        if (state().hud_blink_ms >= 0 && (state().hud_blink_ms = (int32_t)((int64_t)state().hud_blink_ms - l)) <= 0) {
            state().hud_blink_ms = 500;
            state().hud_blink_on = !state().hud_blink_on;
        }
        crash_report().phase = "actor updates";
        this->update_status_message(l);
        for (n = 0; n <= this->highest_actor_slot && state().game_state == 0; ++n) {
            if (state().actors[n] == nullptr) continue;
            int8_t by = 0;
            int8_t by2 = 0;
            int8_t by3 = 0;
            if (n == 0) {
                by = GameCanvas::state().actors[n]->tile_enter_script_id;
                by2 = GameCanvas::state().actors[n]->tile_exit_script_id;
            }
            crash_report().actor_slot = n;
            update_game_log_context(this, "actor updates");
            ActorSystem::update(state().actors[n], l, !this->dialogue_open && this->player_input_enabled);
            if (state().actors[n] == nullptr || n != 0) continue;
            by3 = ActorSystem::check_tile_triggers(state().actors[n], this->tile_enter_script, this->tile_exit_script);
            if (by3 != by) {
                if (by2 != 0 && by2 != -1 && by2 != -2) {
                    GameData::call_script((uint16_t)by2);
                }
                if (by3 != 0 && by3 != -1 && by3 != -2) {
                    GameData::call_script((uint16_t)by3);
                } else if (by3 == -2) {
                    GameCanvas::set_status_message(GameCanvas::get_text(24), 60, 4, 0);
                }
            }
            if (!state().status_text.has_value() || (state().status_text == GameCanvas::get_text(363))) {
                bl = false;
            }
            for (n2 = 0; !bl && n2 < state().world_item_count; n2 += 3) {
                nArray[0] = shiftLeft32(state().world_item_entries[n2 + 0], 7);
                nArray[1] = shiftLeft32(state().world_item_entries[n2 + 1], 7);
                int32_t n3 = ActorSystem::distance(nArray, this->player->world_position);
                if (wrappingAbs(n3) >= 350) continue;
                GameCanvas::set_status_message(GameCanvas::get_text(363), 60, 4, 0);
                bl = true;
            }
            if (state().status_text.has_value() && !(state().status_text == GameCanvas::get_text(363)) || bl)
                continue;
            GameCanvas::set_status_message(std::nullopt, 0, 0, 0);
        }
        if (state().game_state == 9 || state().game_state == 10 || state().game_state == 4 || state().game_state == 21) {
            if (this->text_scroll_ms > 100) {
                this->text_scroll_y = (int16_t)(this->text_scroll_y - 1);
                this->text_scroll_ms = 0;
            }
            this->text_scroll_ms = (int16_t)((int64_t)this->text_scroll_ms + l);
        }
        if (state().game_state == 15) {
            this->loading_spinner_ms = (int16_t)((int64_t)this->loading_spinner_ms + l);
            if (this->loading_spinner_ms >= 200) {
                SpriteAtlas::advance_animation(state().loading_spinner_atlas, 5);
                this->loading_spinner_ms = 0;
            }
        }
        this->repaint();
        this->serviceRepaints();
        return true;
    } catch (const std::exception &exception) {
        return tick_failed(this, exception.what());
    } catch (...) {
        return tick_failed(this, "Unknown exception");
    }
}
void GameCanvas::update_status_message(int64_t l) {
    if (state().status_text.has_value()) {
        if (state().status_scroll_style == 1) {
            if (state().status_blink_ms >= 500) {
                state().status_blink_off = !state().status_blink_off;
                state().status_blink_ms = 0;
            }
            state().status_blink_ms = (int32_t)((int64_t)state().status_blink_ms + l);
        } else if (state().status_scroll_style == 2) {
            if (state().status_scroll_ms >= 50) {
                if ((state().status_x += 2) == -1) {
                    ++state().status_x;
                }
                state().status_scroll_ms = 0;
            }
            state().status_scroll_ms = (int32_t)((int64_t)state().status_scroll_ms + l);
        } else if (state().status_scroll_style == 3) {
            if (state().status_scroll_ms >= 50) {
                if ((state().status_x -= 2) == -1) {
                    --state().status_x;
                }
                state().status_scroll_ms = 0;
            }
            state().status_scroll_ms = (int32_t)((int64_t)state().status_scroll_ms + l);
        }
        if (state().status_elapsed_ms > state().status_duration_ms) {
            state().status_text = std::nullopt;
            state().status_elapsed_ms = 0;
            state().status_duration_ms = 0;
            state().status_color = 0;
            state().status_x = -1;
            state().status_y = -1;
            return;
        }
        state().status_elapsed_ms = (int32_t)((int64_t)state().status_elapsed_ms + l);
    }
}
