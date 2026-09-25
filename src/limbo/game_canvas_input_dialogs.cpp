#include <algorithm>
#include "src/limbo/game_canvas.hpp"
#include "src/limbo/ui_fonts.hpp"

int32_t GameCanvas::handle_game_over_input(int32_t n) {
    do {
        if (state().pending_key_code == state().character_softkey_code) {
            GameCanvas::set_game_state((int8_t)0);
        } else if (state().pending_key_code == state().menu_softkey_code) {
            this->reset_frontend_menu();
            GameCanvas::set_game_state((int8_t)3);
        }
        this->input_consumed = 1;
        break;
    } while (false);
    return n;
}

int32_t GameCanvas::handle_load_confirmation_input(int32_t n) {
    do {
        if (state().pending_key_code == state().character_softkey_code) {
            if (!this->load_game(true)) GameCanvas::set_game_state((int8_t)13);
        } else if (state().pending_key_code == state().menu_softkey_code) {
            this->reset_frontend_menu();
            GameCanvas::set_game_state((int8_t)3);
            GameCanvas::state().menu_selection[0] = 1;
        }
        this->input_consumed = 1;
        break;
    } while (false);
    return n;
}

int32_t GameCanvas::handle_acknowledgement_input(int32_t n) {
    do {
        GameCanvas::set_game_state((int8_t)3);
        this->input_consumed = 1;
        break;
    } while (false);
    return n;
}

int32_t GameCanvas::handle_new_game_confirmation_input(int32_t n) {
    do {
        if (state().pending_key_code == state().character_softkey_code) {
            state().menu_page = 1;
            GameCanvas::set_game_state((int8_t)3);
            clear_pending_key();
            this->input_consumed = 0;
            break;
        }
        if (state().pending_key_code != state().menu_softkey_code) break;
        GameCanvas::set_game_state((int8_t)3);
        clear_pending_key();
        this->input_consumed = 0;
        break;
    } while (false);
    return n;
}

int32_t GameCanvas::handle_reference_screen_input(int32_t n) {
    do {
        n = this->decode_input_action(n);
        if (state().pending_key_code == state().menu_softkey_code) {
            GameCanvas::set_game_state((int8_t)3);
        } else if (n == 5) {
            state().reference_scroll_row = 0;
            if ((state().reference_page = (int8_t)(state().reference_page - 1)) < 0) {
                state().reference_page = (int8_t)(state().reference_pages.length() - 1);
            }
        } else if (n == 6) {
            state().reference_scroll_row = 0;
            if ((state().reference_page = (int8_t)(state().reference_page + 1)) > state().reference_pages.length() - 1) {
                state().reference_page = 0;
            }
        } else if (n == 3) {
            if ((state().reference_scroll_row = (int8_t)(state().reference_scroll_row - 1)) < 0) {
                state().reference_scroll_row = 0;
            }
        } else if (n == 4 && state().reference_has_more) {
            state().reference_scroll_row = (int8_t)(state().reference_scroll_row + 1);
        }
        clear_pending_key();
        this->input_consumed = 0;
        break;
    } while (false);
    return n;
}

int32_t GameCanvas::handle_exit_confirmation_input(int32_t n) {
    do {
        if (state().pending_key_code == state().character_softkey_code) {
            this->request_exit();
            break;
        }
        if (state().pending_key_code != state().menu_softkey_code) break;
        GameCanvas::set_game_state((int8_t)3);
        clear_pending_key();
        this->input_consumed = 0;
        break;
    } while (false);
    return n;
}

int32_t GameCanvas::handle_scrollable_text_input(int32_t n, int64_t elapsed_ms) {
    do {
        n = this->decode_input_action(n);
        int64_t l2 = elapsed_ms / 10L;
        if (n == 3) {
            this->text_scroll_ms = 0;
            this->text_scroll_y = (int16_t)std::min<int64_t>(
                (int64_t)(state().canvas_height - shiftLeft32(ui_fonts().bold_small->getHeight(), 2)),
                (int64_t)this->text_scroll_y + l2);
        } else if (!(n != 4 || state().game_state == 23 && this->text_scroll_at_end ||
                     state().game_state == 17 && this->text_scroll_at_end)) {
            this->text_scroll_ms = 0;
            this->text_scroll_y = (int16_t)((int64_t)this->text_scroll_y - l2);
        }
        if (state().game_state == 10) {
            if (state().pending_key_code != state().menu_softkey_code) break;
            state().menu_page = state().var_boolean_f ? (int8_t)5 : (int8_t)0;
            GameCanvas::set_game_state((int8_t)0);
            clear_pending_key();
            this->input_consumed = 0;
            break;
        }
        if (state().game_state != 4 && state().game_state != 23 && state().game_state != 17 || state().pending_key_code != state().menu_softkey_code) break;
        if (state().game_state != 23 && state().game_state != 17) {
            state().menu_page = state().var_boolean_f ? (int8_t)5 : (int8_t)0;
        } else {
            state().menu_page = (int8_t)6;
            clear_pending_key();
            this->input_consumed = 0;
        }
        GameCanvas::set_game_state((int8_t)3);
        break;
    } while (false);
    return n;
}

int32_t GameCanvas::handle_invalid_control_input(int32_t n) {
    do {
        if (state().pending_key_code != state().menu_softkey_code) break;
        GameCanvas::set_game_state((int8_t)5);
        clear_pending_key();
        this->input_consumed = 0;
        break;
    } while (false);
    return n;
}

int32_t GameCanvas::handle_terminal_prompt_input(int32_t n) {
    do {
        if (state().pending_key_code == state().character_softkey_code) {
            GameCanvas::set_game_state(state().suspended_game_state);
            state().suspended_game_state = (int8_t)-1;
            ::MenuScreen::S().needs_rebuild = false;
            clear_pending_key();
            this->input_consumed = 0;
            break;
        }
        if (state().pending_key_code != state().menu_softkey_code) break;
        this->request_exit();
    } while (false);
    return n;
}
