#include "src/limbo/game_canvas.hpp"

void GameCanvas::request_exit() {
    GameCanvas::set_game_state((int8_t)12);
    this->repaint();
    this->serviceRepaints();
    this->exit_delay_ms = 2000;
}

void GameCanvas::start_game_loop() {
    this->text_transition_delay_ms = -1;
    this->exit_delay_ms = -1;
    this->game_loop_running = true;
}

void GameCanvas::stop_game_loop() { this->game_loop_running = false; }

void GameCanvas::hideNotify() {
    if (!this->is_terminal_state()) {
        Canvas::hideNotify();
        this->paused = true;
        if (state().game_state != 8 && state().game_state != 21 && state().game_state != 15 && state().game_state != 10) {
            if (state().game_state != 22) {
                state().suspended_game_state = state().game_state;
            }
            state().game_state = (int8_t)22;
            ::MenuScreen::S().needs_rebuild = true;
        }
    }
}

void GameCanvas::showNotify() {
    if (!this->is_terminal_state()) {
        Canvas::showNotify();
        this->paused = false;
    }
}

bool GameCanvas::is_terminal_state() { return state().game_state == 6 || state().game_state == 7 || state().game_state == 15; }
