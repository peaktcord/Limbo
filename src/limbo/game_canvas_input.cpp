#include "src/limbo/game_canvas.hpp"
#include "src/limbo/crash_report.hpp"
#include "src/limbo/key_bindings.hpp"
#include "src/limbo/ui_fonts.hpp"
#include "src/limbo/iso_transform.hpp"

void GameCanvas::keyReleased(int32_t n) {
    if (this->is_confirm_key_code(n)) this->set_confirm_input(false);
    this->input_consumed = 1;
}

void GameCanvas::set_confirm_input(bool down) {
    this->confirm_input_down = down;
    if (!down && this->modernized_dialogue_input && this->dialogue_open) {
        this->dialogue_confirm_ready = true;
    }
}

bool GameCanvas::is_confirm_key_code(int32_t key_code) {
    int32_t action = key_code;
    if (key_code < 0) {
        try {
            action = this->getGameAction(key_code);
        } catch (const std::exception &error) {
            voyage::logging::exception("Input", "Mapping a confirmation key", error);
            log_game_context(this);
            return false;
        }
    }
    return action == 8 || action == 20 || action == 53;
}

void GameCanvas::set_game_state(int8_t by) {
    if (!state().state_changes_enabled) {
        return;
    }
    if (state().game_state == 12) {
        return;
    }
    if (state().game_state == 0) {
        state().var_boolean_f = true;
    }
    int8_t by2 = state().game_state;
    state().game_state = by;
    if (state().game_state == 3 && state().title_image == nullptr) {
        if (!state().var_boolean_f && state().menu_page != 4) {
            try {
                state().title_image = Image::createImage(std::string("/main.png"));
            } catch (const std::runtime_error &error) {
                voyage::logging::exception("Resources", "Loading title image /main.png", error);
            }
        }
    } else {
        if (by2 != 22) {
            state().title_image = nullptr;
        }
    }
    if (state().game_state == 9) {
        state().var_boolean_f = false;
        GameCanvas::layout_text_screen(GameCanvas::get_text(547));
        return;
    }
    if (state().game_state == 4) {
        GameCanvas::layout_text_screen(GameCanvas::get_text(548));
        GameCanvas::state().instance->text_scroll_y = (int16_t)(state().canvas_height - shiftLeft32(ui_fonts().bold_small->getHeight(), 2));
        GameCanvas::state().instance->text_scroll_ms = 0;
        return;
    }
    if (state().game_state == 21) {
        TextResources *text_resources = session_new<TextResources>();
        GameCanvas::layout_text_screen(text_resources->load_copyright_text());
        GameCanvas::state().instance->text_scroll_y = 0;
        return;
    }
    if (state().game_state == 23) {
        GameCanvas::layout_text_screen(GameCanvas::get_text(574));
        GameCanvas::state().instance->text_scroll_y = (int16_t)15;
        GameCanvas::state().instance->text_scroll_at_end = false;
        return;
    }
    if (state().game_state == 17) {
        GameCanvas::layout_text_screen(GameCanvas::get_text(465));
        GameCanvas::state().instance->text_scroll_y = (int16_t)15;
        GameCanvas::state().instance->text_scroll_at_end = false;
    }
}

void GameCanvas::layout_text_screen(const std::string &string) {
    int32_t n = 0;
    int32_t n2 = 0;
    int32_t n3 = 0;
    std::string text = string;
    std::vector<std::string> paragraphs;
    n2 = voyage::text::index_of(text, std::string("VERSION"));
    if (n2 != -1) {
        text = voyage::text::slice(text, 0, n2) + state().version_text + voyage::text::slice(text, n2 + 7);
    }
    while ((n2 = voyage::text::index_of(text, std::string("\\n"), n3)) != -1) {
        paragraphs.push_back(voyage::text::slice(text, n3, n2));
        n3 = n2 + 2;
    }
    paragraphs.push_back(voyage::text::slice(text, n3));
    GameCanvas::state().instance->text_paragraphs = std::vector<std::vector<std::string>>(paragraphs.size());
    GameCanvas::set_speaker_name(std::nullopt);
    for (n = 0; n < (int32_t)paragraphs.size(); ++n) {
        GameCanvas::format_wrapped_text(paragraphs[(size_t)n], GameCanvas::state().instance->text_paragraphs[(size_t)n],
                      GameCanvas::interface_width() - 10);
    }
}


int32_t GameCanvas::decode_input_action(int32_t n) {
    int32_t n2 = -1122868;
    if (n == 1 || n == 50) {
        n2 = ActorSystem::ACTION_UP;
    } else if (n == 6 || n == 56) {
        n2 = ActorSystem::ACTION_DOWN;
    } else if (n == 2 || n == 52) {
        n2 = ActorSystem::ACTION_LEFT;
    } else if (n == 5 || n == 54) {
        n2 = ActorSystem::ACTION_RIGHT;
    } else if (n == 8 || n == 20 || n == 53) {
        n2 = ActorSystem::ACTION_USE;
    } else if (n == key_bindings_state().codes[0]) {
        n2 = ActorSystem::ACTION_QUICK_POTION_HEALTH;
    } else if (n == key_bindings_state().codes[1]) {
        n2 = ActorSystem::ACTION_QUICK_POTION_MAGICKA;
    } else if (n == key_bindings_state().codes[2]) {
        n2 = ActorSystem::ACTION_TOGGLE_HAND;
    } else if (this->modern_movement && n >= ActorSystem::ACTION_UP_LEFT && n <= ActorSystem::ACTION_DOWN_RIGHT) {
        n2 = n;
    }
    return n2;
}

void GameCanvas::keyPressed(int32_t n) {
    if (this->is_confirm_key_code(n)) this->set_confirm_input(true);
    state().pending_key_code = n;
    if (this->crash_context.has_value() && ++this->crash_key_presses == 3) {
        this->request_exit();
        return;
    }
}

void GameCanvas::reset_frontend_menu() {
    int32_t n = 0;
    for (n = 0; n < state().menu_selection.length(); ++n) {
        GameCanvas::state().menu_selection[n] = 0;
    }
    if (state().var_boolean_f) {
        state().menu_page = (int8_t)5;
        return;
    }
    state().menu_page = 0;
}

void GameCanvas::dispatch_pending_input(int64_t l) {
    int32_t n;
    if (state().pending_key_code == -state().var_byte_c || state().pending_key_code == -state().character_softkey_code || state().pending_key_code == -state().menu_softkey_code) {
        state().pending_key_code = -state().pending_key_code;
    }
    int32_t n2 = state().pending_key_code;
    int32_t n3 = 0;
    try {
        n3 = this->getGameAction(n2);
    } catch (const std::exception &error) {
        voyage::logging::exception("Input", "Mapping a pending key", error);
        log_game_context(this);
    }
    const bool is_action = (n3 == 8 || n3 == 1 || n3 == 6 || n3 == 2 || n3 == 5 ||
                            (this->modern_movement && n3 >= 11 && n3 <= 14));
    if (state().pending_key_code != state().var_byte_c && state().pending_key_code != state().character_softkey_code &&
        state().pending_key_code != state().menu_softkey_code && state().pending_key_code != state().var_byte_d &&
        state().pending_key_code != state().var_byte_e && (state().pending_key_code < 48 || state().pending_key_code > 57) &&
        !is_action) {
        return;
    }
    if (state().pending_key_code == state().var_byte_d || state().pending_key_code == state().var_byte_e) {
        if (state().game_state != 8) return;
        n = 0;
    } else {
        if (!has_pending_key() || state().pending_key_code == state().var_byte_c) {
            this->input_consumed = 0;
            return;
        }
        n = state().pending_key_code >= 0 ? state().pending_key_code : this->getGameAction(state().pending_key_code);
        switch (state().game_state) {
            case 0: n = this->handle_gameplay_input(n, l); break;
            case 1: n = this->handle_merchant_menu_input(n); break;
            case 2: n = this->handle_character_menu_input(n); break;
            case 3: n = this->handle_frontend_menu_input(n); break;
            case 5: n = this->handle_control_menu_input(n); break;
            case 6:
            case 7:
            case 8: break;
            case 11: n = this->handle_game_over_input(n); break;
            case 13: n = this->handle_acknowledgement_input(n); break;
            case 14: n = this->handle_load_confirmation_input(n); break;
            case 16: n = this->handle_new_game_confirmation_input(n); break;
            case 18: n = this->handle_reference_screen_input(n); break;
            case 19: n = this->handle_exit_confirmation_input(n); break;
            case 4:
            case 9:
            case 10:
            case 17:
            case 23: n = this->handle_scrollable_text_input(n, l); break;
            case 20: n = this->handle_invalid_control_input(n); break;
            case 22: n = this->handle_terminal_prompt_input(n); break;
            case 24: n = this->handle_port_options_menu_input(n); break;
        }
        if (!has_pending_key()) {
            return;
        }
    }
    if (this->menu_screen->is_open == 0) {
        state().game_data->handle_input_trigger((int32_t)(uint16_t)n);
        this->handle_dialogue_input((int32_t)(uint16_t)n);
    }
    if (this->input_consumed == 0) return;
    clear_pending_key();
    this->input_consumed = 0;
}

void GameCanvas::update_visible_tile_bounds() {
    this->corner_world_tl[0] = -this->camera_offset[0];
    this->corner_world_tl[1] = -this->camera_offset[1];
    this->corner_world_tr[0] = -this->camera_offset[0] + state().canvas_width;
    this->corner_world_tr[1] = -this->camera_offset[1];
    this->corner_world_bl[0] = -this->camera_offset[0];
    this->corner_world_bl[1] = -this->camera_offset[1] + state().canvas_height;
    this->corner_world_br[0] = -this->camera_offset[0] + state().canvas_width;
    this->corner_world_br[1] = -this->camera_offset[1] + state().canvas_height;
    IsoTransform::screen_to_tile(this->corner_world_tl, this->corner_tile_tl);
    IsoTransform::screen_to_tile(this->corner_world_tr, this->corner_tile_tr);
    IsoTransform::screen_to_tile(this->corner_world_bl, this->corner_tile_bl);
    IsoTransform::screen_to_tile(this->corner_world_br, this->corner_tile_br);
    this->visible_tile_min[0] = (int8_t)this->corner_tile_tl[0];
    this->visible_tile_min[1] = (int8_t)this->corner_tile_tr[1];
    this->visible_tile_max[0] = (int8_t)(this->corner_tile_br[0] + 3);
    this->visible_tile_max[1] = (int8_t)(this->corner_tile_bl[1] + 3);
}
