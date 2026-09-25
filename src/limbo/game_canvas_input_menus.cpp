#include "src/limbo/game_canvas.hpp"
#include "src/limbo/key_bindings.hpp"
#include "src/limbo/reference_pages.hpp"

int32_t GameCanvas::handle_merchant_menu_input(int32_t n) {
    do {
        n = this->decode_input_action(n);
        if (state().pending_key_code == state().menu_softkey_code) {
            this->menu_screen->is_open = 0;
            GameCanvas::set_game_state((int8_t)3);
        } else if (state().pending_key_code != state().character_softkey_code && this->menu_screen->is_open == 1) {
            this->menu_screen->handle_input((uint16_t)n);
        }
        this->input_consumed = 1;
        break;
    } while (false);
    return n;
}

int32_t GameCanvas::handle_character_menu_input(int32_t n) {
    do {
        n = this->decode_input_action(n);
        if (state().pending_key_code == state().menu_softkey_code) {
            if (!this->menu_screen->navigate_back()) {
                this->menu_screen->is_open = 0;
                GameCanvas::set_game_state((int8_t)0);
            } else {
                clear_pending_key();
            }
        } else if (state().pending_key_code != state().character_softkey_code && this->menu_screen->is_open == 1) {
            this->menu_screen->handle_input((uint16_t)n);
        }
        this->input_consumed = 1;
        break;
    } while (false);
    return n;
}

int32_t GameCanvas::handle_frontend_menu_input(int32_t n) {
    int32_t n4 = 0;
    do {
        n = this->decode_input_action(n);
        if (n == 5) {
            int8_t by4 = state().menu_page;
            state().menu_selection[by4] = (int8_t)(state().menu_selection[by4] - 1);
            if (state().menu_selection[by4] == -1) {
                GameCanvas::state().menu_selection[GameCanvas::state().menu_page] =
                    (int8_t)(state().menu_pages[state().menu_page].length() - 1);
            }
        } else if (n == 6) {
            int8_t by5 = state().menu_page;
            state().menu_selection[by5] = (int8_t)(state().menu_selection[by5] + 1);
            if (state().menu_selection[by5] == state().menu_pages[state().menu_page].length()) {
                GameCanvas::state().menu_selection[GameCanvas::state().menu_page] = 0;
            }
        } else if (state().pending_key_code == state().menu_softkey_code && state().game_state == 3) {
            if (state().menu_page == 6) {
                state().menu_page = state().menu_page_before_options;
            } else if (state().menu_page == 1) {
                state().menu_page = state().var_boolean_f ? (int8_t)5 : (int8_t)0;
            }
        } else if (n == 7) {
            if (state().menu_page == 2) {
                this->loading_percent = 0;
                GameCanvas::set_game_state((int8_t)6);
                this->repaint();
                this->serviceRepaints();
                for (n4 = 0; n4 < state().actors.length(); ++n4) {
                    GameCanvas::state().actors[n4] = nullptr;
                    this->player = nullptr;
                }
                this->load_level_script(state().menu_pages[3][state().menu_selection[state().menu_page]]);
            } else if (voyage::text::starts_with(state().menu_pages[state().menu_page][state().menu_selection[state().menu_page]],
                           GameCanvas::get_text(4))) {
                this->var_boolean_o = !this->var_boolean_o;
                this->build_frontend_menus();
                this->save_game();
            } else if ((state().menu_pages[state().menu_page][state().menu_selection[state().menu_page]] ==
                           GameCanvas::get_text(19))) {
                this->save_game();
                GameCanvas::state().menu_selection[GameCanvas::state().menu_page] = 2;
                GameCanvas::set_game_state((int8_t)13);
                clear_pending_key();
                this->input_consumed = 0;
            } else if ((state().menu_pages[state().menu_page][state().menu_selection[state().menu_page]] ==
                           GameCanvas::get_text(3))) {
                this->clear_reference_pages();
                GameCanvas::set_game_state((int8_t)14);
                clear_pending_key();
                this->input_consumed = 0;
            } else if ((state().menu_pages[state().menu_page][state().menu_selection[state().menu_page]] ==
                           GameCanvas::get_text(21))) {
                GameCanvas::set_game_state((int8_t)0);
                this->paused = false;
            } else if ((state().menu_pages[state().menu_page][state().menu_selection[state().menu_page]] ==
                           GameCanvas::get_text(2))) {
                if (this->has_saved_player()) {
                    GameCanvas::set_game_state((int8_t)16);
                } else {
                    state().menu_page = 1;
                }
            } else if ((state().menu_pages[state().menu_page][state().menu_selection[state().menu_page]] ==
                           GameCanvas::get_text(6))) {
                GameCanvas::set_game_state((int8_t)4);
            } else if ((state().menu_pages[state().menu_page][state().menu_selection[state().menu_page]] ==
                           GameCanvas::get_text(456))) {
                state().menu_page_before_options = state().menu_page;
                state().menu_page = (int8_t)6;
            } else if ((state().menu_pages[state().menu_page][state().menu_selection[state().menu_page]] ==
                           GameCanvas::get_text(457))) {
                state().reference_title_id = (int16_t)457;
                state().reference_scroll_row = 0;
                state().reference_page = 0;
                GameCanvas::set_game_state((int8_t)17);
            } else if ((state().menu_pages[state().menu_page][state().menu_selection[state().menu_page]] ==
                           GameCanvas::get_text(458))) {
                for (n4 = 0; n4 < key_bindings_state().pending.length(); ++n4) {
                    key_bindings_state().pending[n4] = key_bindings_state().codes[n4];
                }
                clear_pending_key();
                this->input_consumed = 0;
                GameCanvas::set_game_state((int8_t)5);
            } else if ((state().menu_pages[state().menu_page][state().menu_selection[state().menu_page]] ==
                           "Port Options")) {
                state().port_options_row = 0;
                clear_pending_key();
                this->input_consumed = 0;
                GameCanvas::set_game_state((int8_t)24);
            } else if ((state().menu_pages[state().menu_page][state().menu_selection[state().menu_page]] ==
                           GameCanvas::get_text(573))) {
                state().reference_title_id = (int16_t)573;
                state().reference_scroll_row = 0;
                state().reference_page = 0;
                state().reference_pages = reference_pages_cache().build_game_overview();
                GameCanvas::set_game_state((int8_t)23);
            } else if ((state().menu_pages[state().menu_page][state().menu_selection[state().menu_page]] ==
                           GameCanvas::get_text(522))) {
                state().reference_title_id = (int16_t)522;
                state().reference_scroll_row = 0;
                state().reference_page = 0;
                state().reference_pages = reference_pages_cache().build_classes_overview();
                GameCanvas::set_game_state((int8_t)18);
            } else if ((state().menu_pages[state().menu_page][state().menu_selection[state().menu_page]] ==
                           GameCanvas::get_text(459))) {
                state().reference_title_id = (int16_t)459;
                state().reference_scroll_row = 0;
                state().reference_page = 0;
                state().reference_pages = reference_pages_cache().build_weapons_overview();
                GameCanvas::set_game_state((int8_t)18);
            } else if ((state().menu_pages[state().menu_page][state().menu_selection[state().menu_page]] ==
                           GameCanvas::get_text(460))) {
                state().reference_title_id = (int16_t)460;
                state().reference_scroll_row = 0;
                state().reference_page = 0;
                state().reference_pages = reference_pages_cache().build_armour_overview();
                GameCanvas::set_game_state((int8_t)18);
            } else if ((state().menu_pages[state().menu_page][state().menu_selection[state().menu_page]] ==
                           GameCanvas::get_text(461))) {
                state().reference_title_id = (int16_t)461;
                state().reference_scroll_row = 0;
                state().reference_page = 0;
                state().reference_pages = reference_pages_cache().build_spells_overview();
                GameCanvas::set_game_state((int8_t)18);
            } else if ((state().menu_pages[state().menu_page][state().menu_selection[state().menu_page]] ==
                           GameCanvas::get_text(462))) {
                state().reference_title_id = (int16_t)462;
                state().reference_scroll_row = 0;
                state().reference_page = 0;
                state().reference_pages = reference_pages_cache().build_items_overview();
                GameCanvas::set_game_state((int8_t)18);
            } else if ((state().menu_pages[state().menu_page][state().menu_selection[state().menu_page]] ==
                           GameCanvas::get_text(18))) {
                this->build_merchant_menu();
                GameCanvas::set_game_state((int8_t)1);
            } else if ((state().menu_pages[state().menu_page][state().menu_selection[state().menu_page]] ==
                           GameCanvas::get_text(20))) {
                GameCanvas::set_game_state((int8_t)0);
            } else if (state().menu_page == 1) {
                this->clear_reference_pages();
                this->loading_percent = 0;
                GameCanvas::set_game_state((int8_t)6);
                this->repaint();
                this->serviceRepaints();
                for (n4 = 0; n4 < state().actors.length(); ++n4) {
                    GameCanvas::state().actors[n4] = nullptr;
                    this->player = nullptr;
                }
                this->load_level_script(std::string("/l01_1.scr"));
                state().player_gold = 100;
            } else if ((state().menu_pages[state().menu_page][state().menu_selection[state().menu_page]] ==
                           GameCanvas::get_text(22))) {
                GameCanvas::set_game_state((int8_t)19);
            }
        }
        this->input_consumed = 1;
        break;
    } while (false);
    return n;
}

int32_t GameCanvas::handle_control_menu_input(int32_t n) {
    int32_t n4 = 0;
    do {
        n = this->decode_input_action(n);
        if (state().awaiting_key_bind) {
            if (state().pending_key_code != state().menu_softkey_code && state().pending_key_code != state().character_softkey_code) {
                if (this->can_bind_key(state().pending_key_code, n)) {
                    key_bindings_state().pending[GameCanvas::state().control_row] =
                        state().pending_key_code >= 0 ? (int8_t)state().pending_key_code : (int8_t)this->getGameAction(state().pending_key_code);
                } else {
                    GameCanvas::set_game_state((int8_t)20);
                }
                state().awaiting_key_bind = false;
            }
        } else if (state().pending_key_code == state().menu_softkey_code) {
            GameCanvas::set_game_state((int8_t)3);
        } else if (n == 3) {
            state().control_row = max32(0, state().control_row - 1);
        } else if (n == 4) {
            state().control_row = min32(state().control_rows.length() - 1, state().control_row + 1);
        } else if (n == 7) {
            if ((state().control_rows[state().control_row] == GameCanvas::get_text(294))) {
                for (n4 = 0; n4 < key_bindings_state().pending.length(); ++n4) {
                    key_bindings_state().codes[n4] = key_bindings_state().pending[n4];
                }
                state().awaiting_key_bind = false;
                GameCanvas::set_game_state((int8_t)3);
                this->save_game();
            } else {
                state().awaiting_key_bind = true;
            }
        }
        this->input_consumed = 1;
        break;
    } while (false);
    return n;
}

int32_t GameCanvas::handle_port_options_menu_input(int32_t n) {
    do {
        n = this->decode_input_action(n);
        if (state().pending_key_code == state().menu_softkey_code || state().pending_key_code == 57) {
            state().menu_page = state().menu_page_before_options;
            GameCanvas::set_game_state((int8_t)3);
        } else if (n == 3) {
            state().port_options_row = (state().port_options_row + voyage::ROW_COUNT - 1) % voyage::ROW_COUNT;
        } else if (n == 4) {
            state().port_options_row = (state().port_options_row + 1) % voyage::ROW_COUNT;
        } else if (n == 7 || n == 5 || n == 6) {
            voyage::toggle_port_option(state().port_options_row, this->port_options);
            this->apply_port_options(this->port_options);
        }
        this->input_consumed = 1;
        break;
    } while (false);
    return n;
}
