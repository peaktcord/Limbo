#include "src/limbo/game_canvas.hpp"

#include "src/limbo/crash_report.hpp"
#include "src/limbo/key_bindings.hpp"
#include "src/limbo/reference_pages.hpp"
#include "src/limbo/string_table.hpp"
#include "src/limbo/ui_fonts.hpp"

GameCanvasStatics &GameCanvas::state() { return oblivion_state().canvas; }

void GameCanvas::clear_pending_key() { state().clear_pending_key(); }
bool GameCanvas::has_pending_key() { return state().has_pending_key(); }

GameCanvas::GameCanvas(const std::string &startup_script, const std::string &menu_atlas, const std::string &version) {
    this->setFullScreenMode(true);
    state().version_text = version;
    state().instance = this;
    GameCanvas::reset_canvas_state();
    state().game_data = session_new<GameData>(this);
    this->resize_canvas(this->getWidth(), this->getHeight() + 25);
    SpriteAtlas::clear_image_cache();
    this->text_scroll_y = (int16_t)(state().canvas_height - shiftLeft32(ui_fonts().bold_small->getHeight(), 3));
    this->text_scroll_ms = 0;
    state().world_item_count = 0;
    this->menu_screen = session_new<MenuScreen>(menu_atlas, this);
    state().loading_spinner_atlas = SpriteAtlas::load_atlas(std::string("/oh_pc.cml"));
    this->load_level_script(startup_script);
    this->load_game(false);
}

void GameCanvas::reset_canvas_state() {
    int32_t n = 0;
    key_bindings_state().reset();
    state().menu_page = (int8_t)-1;
    state().status_duration_ms = 0;
    state().status_elapsed_ms = 0;
    state().status_color = 0;
    state().status_scroll_style = 0;
    state().status_x = -1;
    state().status_y = -1;
    state().status_blink_ms = 0;
    state().status_scroll_ms = 0;
    state().control_row = 0;
    state().port_options_row = 0;
    state().hud_blink_ms = -1;
    state().player_gold = 100;
    state().canvas_width = 0;
    state().canvas_height = 0;
    state().level_rows = 0;
    state().level_cols = 0;
    state().tile_walkable.setNull();
    state().splash_image = nullptr;
    state().awaiting_key_bind = false;
    state().status_blink_off = false;
    state().game_data = nullptr;
    state().menu_pages.setNull();
    state().control_rows.setNull();
    state().status_text = std::nullopt;
    state().level_script_path = std::nullopt;
    for (n = 0; n < state().menu_selection.length(); ++n) {
        GameCanvas::state().menu_selection[n] = 0;
    }
    for (n = 0; n < state().actors.length(); ++n) {
        GameCanvas::state().actors[n] = nullptr;
    }
    state().tile_layers.clear();
}

void GameCanvas::apply_port_options(const voyage::PortOptions &options) {
    this->port_options = options;
    this->center_camera_on_target = options.centered_camera;
    this->modern_movement = options.modern_movement;
    this->screen_aligned_movement = options.screen_aligned_movement;
    this->modernized_dialogue_input = true;
    if (this->on_port_options_changed) {
        this->on_port_options_changed(options);
    }
}

void GameCanvas::clear_reference_pages() {
    state().reference_pages.setNull();
    reference_pages_cache().clear();
}

void GameCanvas::build_frontend_menus() {
    int32_t n = 0;
    int32_t n2 = 0;
    state().menu_pages = SharedArray<SharedArray<std::string>>{
        SharedArray<std::string>(),
        SharedArray<std::string>(),
        SharedArray<std::string>{std::string("Level 1"), std::string("Level 2"), std::string("Level 3"), std::string("Level 4"), std::string("Level 5"),
                       std::string("Level 6"), std::string("Level 7"), std::string("Level 8"), std::string("Level 9"), std::string("Level 10"),
                       std::string("Level 11"), std::string("Level 12")},
        SharedArray<std::string>{std::string("/l01_1.scr"), std::string("/l02_2_1.scr"), std::string("/l03_3.scr"), std::string("/l04_4.scr"),
                       std::string("/l05_5.scr"), std::string("/l06_6_cr.scr"), std::string("/l07_7_cr.scr"), std::string("/l08_8_cr.scr"),
                       std::string("/l09_9_cr.scr"), std::string("/l10_10_cr.scr"), std::string("/l11_11_cr.scr"),
                       std::string("/l12_12.scr")},
        SharedArray<std::string>{GameCanvas::get_text(18), GameCanvas::get_text(19), GameCanvas::get_text(20)},
        SharedArray<std::string>(),
        SharedArray<std::string>{std::string("Port Options"),
                       GameCanvas::get_text(457), GameCanvas::get_text(458),
                       GameCanvas::get_text(573),
                       GameCanvas::get_text(522), GameCanvas::get_text(459), GameCanvas::get_text(460),
                       GameCanvas::get_text(461), GameCanvas::get_text(462)}};
    const int32_t page0_count = (this->has_saved_player() ? 5 : 4) + (this->top_level_port_options ? 1 : 0);
    const int32_t page5_count = (this->has_saved_player() ? 6 : 5) + (this->top_level_port_options ? 1 : 0);
    GameCanvas::state().menu_pages[0] = SharedArray<std::string>(page0_count);
    GameCanvas::state().menu_pages[5] = SharedArray<std::string>(page5_count);
    int32_t n3 = 0;
    GameCanvas::state().menu_pages[0][n3++] = GameCanvas::get_text(2);
    if (this->has_saved_player()) {
        GameCanvas::state().menu_pages[0][n3++] = GameCanvas::get_text(3);
    }
    if (this->top_level_port_options) {
        GameCanvas::state().menu_pages[0][n3++] = "Port Options";
    }
    GameCanvas::state().menu_pages[0][n3++] = GameCanvas::get_text(456);
    GameCanvas::state().menu_pages[0][n3++] = GameCanvas::get_text(6);
    GameCanvas::state().menu_pages[0][n3++] = GameCanvas::get_text(22);
    n3 = 0;
    GameCanvas::state().menu_pages[5][n3++] = GameCanvas::get_text(21);
    GameCanvas::state().menu_pages[5][n3++] = GameCanvas::get_text(2);
    if (this->has_saved_player()) {
        GameCanvas::state().menu_pages[5][n3++] = GameCanvas::get_text(3);
    }
    if (this->top_level_port_options) {
        GameCanvas::state().menu_pages[5][n3++] = "Port Options";
    }
    GameCanvas::state().menu_pages[5][n3++] = GameCanvas::get_text(456);
    GameCanvas::state().menu_pages[5][n3++] = GameCanvas::get_text(6);
    GameCanvas::state().menu_pages[5][n3++] = GameCanvas::get_text(22);
    for (n2 = 0; n2 < GameCanvas::state().game_data->creature_table.length(); ++n2) {
        if (GameCanvas::state().game_data->creature_table[n2][0] <= 0) continue;
        ++n;
    }
    GameCanvas::state().menu_pages[1] = SharedArray<std::string>(n);
    n = 0;
    for (n2 = 0; n2 < GameCanvas::state().game_data->creature_table.length(); ++n2) {
        if (GameCanvas::state().game_data->creature_table[n2][0] <= 0) continue;
        GameCanvas::state().menu_pages[1][n++] = state().game_data->get_string(GameCanvas::state().game_data->creature_table[n2][1]);
    }
    state().control_rows = SharedArray<std::string>{GameCanvas::get_text(292), GameCanvas::get_text(293),
                                                GameCanvas::get_text(463), GameCanvas::get_text(294)};
    key_bindings_state().load_names();
}

void GameCanvas::load_level_script(const std::string &string) {
    voyage::logging::write(voyage::logging::Level::Info, "Level", "Loading " + string);
    crash_report().resource = string;
    int32_t n = 0;
    GameCanvas::set_game_state((int8_t)6);
    state().level_script_path = string;
    state().tile_walkable.setNull();
    this->tile_enter_script.setNull();
    this->tile_exit_script.setNull();
    state().tile_use_script.setNull();
    this->tile_screen_positions.setNull();
    this->player_input_enabled = true;
    this->game_loop_running = true;
    state().var_boolean_c = true;
    this->highest_actor_slot = 0;
    this->loading_percent = (int8_t)-1;
    this->camera_offset[0] = 0;
    this->camera_offset[1] = 0;
    state().menu_page = (int8_t)-1;
    clear_pending_key();
    this->input_consumed = 0;
    this->text_scroll_y = (int16_t)(state().canvas_height - shiftLeft32(ui_fonts().bold_small->getHeight(), 3));
    this->text_scroll_ms = 0;
    this->dialogue_open = false;
    MagicEffects::clear_all();
    if (this->player != nullptr) {
        this->player->summoned_actor = nullptr;
    }
    for (n = 0; n < 25; ++n) {
        GameCanvas::state().actors[n] = nullptr;
    }
    state().game_data->load(string);
}

void GameCanvas::resize_canvas(int32_t n, int32_t n2) {
    state().canvas_width = (int16_t)n;
    state().canvas_height = (int16_t)n2;
    this->canvas_center_x = (int16_t)((state().canvas_width) >> (1));
    this->canvas_center_y = (int16_t)((state().canvas_height) >> (1));
    this->scene_image = Image::createImage((int32_t)state().canvas_width, (int32_t)state().canvas_height);
    this->scene_graphics = this->scene_image->getGraphics();
}

void GameCanvas::paint(Graphics *graphics) {
    update_game_log_context(this, "painting");
    if (state().canvas_width <= original_interface_width || state().game_state == 0) {
        this->paint_legacy_contents(graphics);
        return;
    }

    const int16_t viewport_width = state().canvas_width;
    const int16_t viewport_center = this->canvas_center_x;
    if (this->original_width_ui_image == nullptr ||
        this->original_width_ui_image->getHeight() != state().canvas_height) {
        this->original_width_ui_image = Image::createImage(original_interface_width, (int32_t)state().canvas_height);
    }

    Graphics *ui_graphics = this->original_width_ui_image->getGraphics();
    graphics->setColor(0);
    graphics->fillRect(0, 0, (int32_t)viewport_width, (int32_t)state().canvas_height);
    state().canvas_width = (int16_t)original_interface_width;
    this->canvas_center_x = (int16_t)(original_interface_width / 2);
    try {
        this->paint_legacy_contents(ui_graphics);
    } catch (...) {
        state().canvas_width = viewport_width;
        this->canvas_center_x = viewport_center;
        throw;
    }
    state().canvas_width = viewport_width;
    this->canvas_center_x = viewport_center;
    graphics->drawImage(this->original_width_ui_image, this->interface_left(), 0, 0);
}

int32_t GameCanvas::interface_width() {
    return state().canvas_width < original_interface_width ? state().canvas_width : original_interface_width;
}

int32_t GameCanvas::interface_left() {
    return ((int32_t)state().canvas_width - interface_width()) / 2;
}

void GameCanvas::paint_legacy_contents(Graphics *graphics) {
    SharedArray<int8_t> byArray;
    int32_t n = 0;
    int32_t n2 = 0;
    int32_t n3 = 0;
    int32_t n4 = 0;
    int32_t n5 = 0;
    int32_t n6 = 0;
    std::string string;
    if (this->crash_context.has_value()) {
        graphics->setColor(0xFFFFFF);
        graphics->fillRect(0, 0, (int32_t)state().canvas_width, (int32_t)state().canvas_height);
        graphics->setColor(0);
        graphics->drawString(*this->crash_context, 5, 10, 0);
        graphics->drawString(crash_report().message, 5, 25, 0);
        graphics->drawString(std::string("Press any key 3x to quit..."), 5, 40, 0);
        return;
    }
    if (state().splash_image != nullptr) {
        graphics->setColor(0);
        graphics->fillRect(0, 0, (int32_t)state().canvas_width, (int32_t)state().canvas_height);
        graphics->drawImage(state().splash_image,
                            this->canvas_center_x - ((state().splash_image->getWidth()) >> (1)),
                            this->canvas_center_y - ((state().splash_image->getHeight()) >> (1)), 0);
        return;
    }
    switch (state().game_state) {
        case 0: {
            const int32_t ui_left = interface_left();
            const int32_t ui_width = interface_width();
            this->update_camera();
            this->update_visible_tile_bounds();
            this->redraw_tile_background(graphics);
            graphics->drawImage(this->scene_image, 0, 0, 0);
            if (state().tile_layers.empty()) break;
            byArray = state().tile_layers.back();
            for (n4 = this->visible_tile_min[0]; n4 <= this->visible_tile_max[0] && n4 < state().level_rows; ++n4) {
                for (n5 = this->visible_tile_min[1]; n5 <= this->visible_tile_max[1] && n5 < state().level_cols; ++n5) {
                    if (n4 < 0 || n5 < 0) continue;
                    n2 = this->tile_screen_positions[n4 * state().level_cols + 0 * this->level_tile_count + n5] + this->camera_offset[0];
                    n3 = this->tile_screen_positions[n4 * state().level_cols + 1 * this->level_tile_count + n5] + this->camera_offset[1];
                    if (byArray[n4 * state().level_cols + n5] != 0) {
                        n = SpriteAtlas::frame_height(this->tile_atlas, byArray[n4 * state().level_cols + n5]);
                    }
                    if (n2 <= -state().tile_width || n2 >= state().canvas_width || n3 <= -state().tile_height || n3 >= state().canvas_height + n) continue;
                    if (byArray[n4 * state().level_cols + n5] != 0) {
                        SpriteAtlas::draw_frame(graphics, this->tile_atlas, byArray[n4 * state().level_cols + n5], n2, n3);
                    }
                    for (n6 = 0; n6 < state().actors.length(); ++n6) {
                        if (state().actors[n6] == nullptr || GameCanvas::state().actors[n6]->tile_position[0] != n4 ||
                            GameCanvas::state().actors[n6]->tile_position[1] != n5)
                            continue;
                        ActorSystem::render_actor(state().actors[n6], graphics, this->camera_offset);
                    }
                }
            }
            if (state().hud_visible && this->player != nullptr) {
                if (this->player->is_dead == 0) {
                    int32_t n7 = min32(70, 70 * this->player->health / this->player->max_health);
                    int32_t n8 = min32(70, 70 * this->player->magicka / this->player->max_magicka);
                    graphics->setColor(0xFF0000);
                    graphics->fillRect(ui_left + 18, 10, n7, 7);
                    graphics->setColor(255);
                    graphics->fillRect(ui_left + 18, 18, n8, 7);
                }
                SpriteAtlas::draw_frame(graphics, this->tile_atlas, -56, ui_left, 0);
                if (this->player->hand_icon_frame != -1) {
                    SpriteAtlas::draw_frame(graphics, this->ui_atlas, this->player->hand_icon_frame,
                         ui_left + ui_width - SpriteAtlas::frame_width(this->ui_atlas, this->player->hand_icon_frame) - 2, 2);
                }
                if (this->player->offhand_icon_frame != -1) {
                    SpriteAtlas::draw_frame(graphics, this->ui_atlas, this->player->offhand_icon_frame,
                         ui_left + ui_width - shiftLeft32(SpriteAtlas::frame_width(this->ui_atlas, this->player->hand_icon_frame), 1) - 4, 2);
                }
            }
            graphics->setFont(ui_fonts().bold_small);
            graphics->setColor(0xFFFFFF);
            graphics->drawString(voyage::text::upper_ascii(GameCanvas::get_text(422)), ui_left + 2,
                                 state().canvas_height - ui_fonts().bold_small->getHeight() - 2, 0);
            if (state().hud_visible) {
                graphics->drawString(
                    voyage::text::upper_ascii(GameCanvas::get_text(421)),
                    ui_left + ui_width - ui_fonts().bold_small->stringWidth(GameCanvas::get_text(421)) - 2,
                    state().canvas_height - ui_fonts().bold_small->getHeight() - 2, 0);
            }
            graphics->setFont(ui_fonts().small);
            if (state().status_text.has_value()) {
                state().status_y = state().canvas_height - ui_fonts().plain->getHeight() - 5;
                graphics->setFont(ui_fonts().plain);
                graphics->setColor(0);
                graphics->fillRect(ui_left, state().status_y - 5, ui_width, (int32_t)state().canvas_height);
                if (!state().status_blink_off) {
                    if (state().status_x == -1) {
                        if (state().status_scroll_style == 0 || state().status_scroll_style == 1) {
                            state().status_x = ui_left + ((ui_width) >> (1)) -
                                        ((ui_fonts().plain->stringWidth(*state().status_text)) >> (1));
                        } else if (state().status_scroll_style == 2) {
                            state().status_x = ui_left - ui_fonts().plain->stringWidth(*state().status_text);
                        } else if (state().status_scroll_style == 3) {
                            state().status_x = ui_left + ui_width;
                        }
                    }
                    graphics->setColor(state().status_color);
                    graphics->drawString(*state().status_text, state().status_x, state().status_y, 0);
                }
            }
            MagicEffects::render(graphics, this->camera_offset);
            if (!this->dialogue_open || this->menu_screen->is_open != 0) break;
            this->draw_dialogue(graphics);
            break;
        }
        case 1: {
            break;
        }
        case 2: {
            break;
        }
        case 3: {
            graphics->setColor(0);
            graphics->fillRect(0, 0, (int32_t)state().canvas_width, (int32_t)state().canvas_height);
            graphics->setColor(0);
            graphics->setFont(ui_fonts().bold_medium);
            if (state().title_image != nullptr) {
                graphics->drawImage(state().title_image,
                                    this->canvas_center_x - ((state().title_image->getWidth()) >> (1)), 0,
                                    0);
                if (state().menu_page == 1) {
                    graphics->setColor(1044480);
                    graphics->drawString(
                        GameCanvas::get_text(423),
                        ((state().canvas_width) >> (1)) -
                            ((ui_fonts().bold_medium->stringWidth(GameCanvas::get_text(423))) >> (1)),
                        state().title_image->getHeight() + 1, 0);
                }
            }
            graphics->setColor(0xFF0000);
            graphics->drawString(
                std::string("<<"), 0,
                (state().title_image != nullptr ? state().title_image->getHeight()
                                                                : 0) +
                    25,
                0);
            graphics->drawString(
                std::string(">>"), state().canvas_width - ui_fonts().bold_medium->stringWidth(std::string(">>")),
                (state().title_image != nullptr ? state().title_image->getHeight()
                                                                : 0) +
                    25,
                0);
            graphics->setColor(0xFFFFFF);
            int32_t n9 = state().canvas_width - ui_fonts().bold_medium->stringWidth(std::string("<<  >>"));
            int32_t n10 = ui_fonts().bold_medium->stringWidth(
                state().menu_pages[state().menu_page][state().menu_selection[state().menu_page]]);
            if (n10 < n9 || voyage::text::index_of(state().menu_pages[state().menu_page][state().menu_selection[state().menu_page]], 32) == -1) {
                graphics->drawString(
                    state().menu_pages[state().menu_page][state().menu_selection[state().menu_page]],
                    ((state().canvas_width) >> (1)) -
                        ((ui_fonts().bold_medium->stringWidth(
                                 state().menu_pages[state().menu_page][state().menu_selection[state().menu_page]])) >> (1)),
                    (state().title_image != nullptr ? state().title_image->getHeight()
                                                                    : 0) +
                        25,
                    0);
            } else {
                std::string string2 = voyage::text::slice(state().menu_pages[state().menu_page][state().menu_selection[state().menu_page]],
                    0, voyage::text::index_of(state().menu_pages[state().menu_page][state().menu_selection[state().menu_page]], 32));
                std::string string3 = voyage::text::slice(state().menu_pages[state().menu_page][state().menu_selection[state().menu_page]],
                    voyage::text::index_of(state().menu_pages[state().menu_page][state().menu_selection[state().menu_page]], 32) + 1);
                graphics->drawString(
                    string2, ((state().canvas_width) >> (1)) - ((ui_fonts().bold_medium->stringWidth(string2)) >> (1)),
                    (state().title_image != nullptr ? state().title_image->getHeight()
                                                                    : 0) +
                        25 - ((ui_fonts().bold_medium->getHeight()) >> (1)),
                    0);
                graphics->drawString(
                    string3, ((state().canvas_width) >> (1)) - ((ui_fonts().bold_medium->stringWidth(string3)) >> (1)),
                    (state().title_image != nullptr ? state().title_image->getHeight()
                                                                    : 0) +
                        25 + ((ui_fonts().bold_medium->getHeight()) >> (1)),
                    0);
            }
            if (state().menu_page == 0 || state().menu_page == 5 || state().menu_page == 4) break;
            graphics->setFont(ui_fonts().bold_small);
            graphics->drawString(voyage::text::upper_ascii(GameCanvas::get_text(449)), 2,
                                 state().canvas_height - ui_fonts().bold_small->getHeight() - 2, 0);
            graphics->setFont(ui_fonts().bold_medium);
            break;
        }
        case 5: {
            graphics->setColor(0);
            graphics->setFont(ui_fonts().bold_small);
            graphics->fillRect(0, 0, (int32_t)state().canvas_width, (int32_t)state().canvas_height);
            graphics->setColor(0xFFFFFF);
            graphics->fillRect(5, 5, state().canvas_width - 10, 20);
            graphics->setColor(0xDD0000);
            string = state().awaiting_key_bind ? GameCanvas::get_text(424) : GameCanvas::get_text(425);
            graphics->drawString(string,
                                 ((state().canvas_width) >> (1)) -
                                     ((ui_fonts().bold_small->stringWidth(string)) >> (1)),
                                 7, 0);
            graphics->setColor(0xFFFFFF);
            n3 = 30;
            for (n4 = 0; n4 < state().control_rows.length(); ++n4) {
                n2 = 5;
                if (n4 != state().control_row) {
                    n2 = 5 + ui_fonts().bold_small->stringWidth(std::string("> "));
                }
                if ((state().control_rows[n4] == GameCanvas::get_text(294))) {
                    n3 += ui_fonts().bold_small->getHeight();
                }
                graphics->drawString(std::string(n4 == state().control_row ? "> " : "") + state().control_rows[n4], n2, n3, 0);
                n3 += ui_fonts().bold_small->getHeight();
            }
            if (!(state().control_rows[state().control_row] == GameCanvas::get_text(294))) {
                if (state().awaiting_key_bind) {
                    graphics->setColor(0xFF0000);
                }
                graphics->drawString(key_bindings_state().names[key_bindings_state().pending[state().control_row]].value_or(""),
                                     ((state().canvas_width) >> (1)) -
                                         ((ui_fonts().bold_small->stringWidth(
                                                  key_bindings_state().names[key_bindings_state().pending[state().control_row]].value_or(""))) >> (1)),
                                     state().canvas_height - ui_fonts().bold_small->getHeight() - 2, 0);
            }
            graphics->drawString(voyage::text::upper_ascii(GameCanvas::get_text(449)), 2,
                                 state().canvas_height - ui_fonts().bold_small->getHeight() - 2, 0);
            break;
        }
        case 24: {
            graphics->setColor(0);
            graphics->setFont(ui_fonts().bold_small);
            graphics->fillRect(0, 0, (int32_t)state().canvas_width, (int32_t)state().canvas_height);
            graphics->setColor(0xFFFFFF);
            graphics->fillRect(5, 5, state().canvas_width - 10, 20);
            graphics->setColor(0xDD0000);
            string = "PORT OPTIONS";
            graphics->drawString(string,
                                 ((state().canvas_width) >> 1) -
                                     ((ui_fonts().bold_small->stringWidth(string)) >> 1),
                                 7, 0);
            graphics->setColor(0xFFFFFF);
            n3 = 30;
            const std::vector<std::string> opt_labels = voyage::port_option_labels(this->port_options);
            for (n4 = 0; n4 < (int32_t)opt_labels.size(); ++n4) {
                n2 = 5;
                if (n4 != state().port_options_row) {
                    n2 = 5 + ui_fonts().bold_small->stringWidth(std::string("> "));
                }
                if (n4 == voyage::ROW_HANDSET_DEFAULTS) {
                    n3 += ui_fonts().bold_small->getHeight() / 2;
                }
                graphics->drawString(std::string(n4 == state().port_options_row ? "> " : "") + opt_labels[(size_t)n4], n2, n3, 0);
                n3 += ui_fonts().bold_small->getHeight() + 2;
            }
            graphics->drawString(voyage::text::upper_ascii(GameCanvas::get_text(449)), 2,
                                 state().canvas_height - ui_fonts().bold_small->getHeight() - 2, 0);
            break;
        }
        case 6:
        case 7: {
            int32_t n11 = state().canvas_width - 20;
            int32_t n12 = this->loading_percent * n11 / 100;
            graphics->setColor(0);
            graphics->fillRect(0, 0, (int32_t)state().canvas_width, (int32_t)state().canvas_height);
            graphics->setColor(0xFF0000);
            graphics->fillRect(10, 30, n12, 10);
            graphics->setColor(0xFFFFFF);
            graphics->drawRect(10, 30, n11, 10);
            string = GameCanvas::get_text(39);
            if (voyage::text::length(string) == 0) {
                TextResources *text_resources = session_new<TextResources>();
                string = text_resources->load_intro_segment((int8_t)0);
            }
            graphics->drawString(string + "...", 10, 10, 0);
            break;
        }
        case 8: {
            graphics->setColor(this->splash_color);
            graphics->fillRect(0, 0, (int32_t)state().canvas_width, (int32_t)state().canvas_height);
            SpriteAtlas::draw_frame(graphics, this->ui_atlas, this->splash_frame,
                 this->canvas_center_x - ((SpriteAtlas::frame_width(this->ui_atlas, this->splash_frame)) >> (1)),
                 this->canvas_center_y - ((SpriteAtlas::frame_height(this->ui_atlas, this->splash_frame)) >> (1)));
            if (!GameCanvas::state().game_data->waiting_for_key) break;
            if (state().hud_blink_on) {
                TextResources *text_resources = session_new<TextResources>();
                string = text_resources->load_intro_segment((int8_t)1);
                graphics->setFont(ui_fonts().bold_small);
                graphics->setColor(0);
                graphics->drawString(string,
                                     ((state().canvas_width) >> (1)) -
                                         ((ui_fonts().bold_small->stringWidth(string)) >> (1)),
                                     this->canvas_center_y + ((SpriteAtlas::frame_height(this->ui_atlas, this->splash_frame)) >> (1)) + 12, 0);
            }
            if (state().hud_blink_ms != -1) break;
            state().hud_blink_ms = 500;
            break;
        }
        case 4:
        case 9:
        case 10:
        case 17:
        case 21:
        case 23: {
            int32_t n13;
            graphics->setFont(ui_fonts().bold_small);
            graphics->setColor(state().game_state == 4 || state().game_state == 21 || state().game_state == 23 || state().game_state == 17 ? 0
                                                                                                          : 15327683);
            graphics->fillRect(0, 0, (int32_t)state().canvas_width, (int32_t)state().canvas_height);
            graphics->setColor(state().game_state == 4 || state().game_state == 21 || state().game_state == 23 || state().game_state == 17 ? 0xFFFFFF
                                                                                                          : 0);
            bool bl = false;
            if (state().game_state == 21) {
                bl = true;
                graphics->setFont(ui_fonts().small);
            }
            if ((state().game_state == 23 || state().game_state == 17) && this->text_scroll_y > 20) {
                this->text_scroll_y = (int16_t)20;
            }
            if (state().game_state == 21) {
                this->text_scroll_y = (int16_t)(this->text_scroll_y - 1);
            }
            n3 = 3 + this->text_scroll_y;
            for (n4 = 0; n4 < (int32_t)this->text_paragraphs.size(); ++n4) {
                for (n5 = 0; n5 < (int32_t)this->text_paragraphs[(size_t)n4].size(); ++n5) {
                    n13 = state().game_state == 4 || state().game_state == 21 || state().game_state == 23 || state().game_state == 17 ? 0xFFFFFF : 0;
                    n2 = 2;
                    if (!bl) {
                        n3 += ui_fonts().bold_small->getHeight() + 1;
                    } else {
                        bl = false;
                    }
                    string = this->text_paragraphs[(size_t)n4][(size_t)n5];
                    if (voyage::text::byte_at(string, 1) == '~') {
                        if (voyage::text::byte_at(string, 0) == '1') {
                            graphics->setFont(ui_fonts().bold_small);
                            n13 = 0xAAAA00;
                        } else if (voyage::text::byte_at(string, 0) == '3') {
                            n13 = 0xAA0000;
                            n2 = ((state().canvas_width) >> (1)) -
                                 ((ui_fonts().bold_small->stringWidth(
                                          voyage::text::slice(string, voyage::text::index_of(string, 126) + 1))) >> (1));
                        }
                        string = voyage::text::slice(string, voyage::text::index_of(string, 126) + 1);
                    }
                    graphics->setColor(n13);
                    graphics->drawString(string, n2, n3, 0);
                }
                if (!this->text_paragraphs[(size_t)n4].empty()) continue;
                n3 += ui_fonts().bold_small->getHeight() + 1;
            }
            if (state().game_state == 23 || state().game_state == 17) {
                graphics->setColor(0);
                graphics->setFont(ui_fonts().bold_small);
                graphics->fillRect(0, 0, (int32_t)state().canvas_width, ui_fonts().bold_small->getHeight() + 20);
                graphics->setColor(0xFFFFFF);
                graphics->fillRect(5, 5, state().canvas_width - 10, ui_fonts().bold_small->getHeight() + 10);
                graphics->setColor(0xDD0000);
                graphics->drawString(GameCanvas::get_text(state().reference_title_id),
                                     ((state().canvas_width) >> (1)) -
                                         ((ui_fonts().bold_small->stringWidth(
                                                  GameCanvas::get_text(state().reference_title_id))) >> (1)),
                                     10, 0);
                graphics->setColor(0xFFFFFF);
            }
            n13 = state().canvas_height - ui_fonts().bold_small->getHeight();
            if (state().game_state == 10 || state().game_state == 23 || state().game_state == 4 || state().game_state == 17) {
                n13 -= ui_fonts().bold_small->getHeight() * 3;
            }
            if (state().game_state == 4 || state().game_state == 23 || state().game_state == 17) {
                graphics->setColor(0);
                graphics->fillRect(0, state().canvas_height - ui_fonts().bold_small->getHeight() - 8,
                                   (int32_t)state().canvas_width, ui_fonts().bold_small->getHeight() + 8);
                graphics->setColor(0xFFFFFF);
                graphics->setFont(ui_fonts().bold_small);
                if (state().game_state == 4 || state().game_state == 23 || state().game_state == 17) {
                    graphics->drawString(voyage::text::upper_ascii(GameCanvas::get_text(449)), 2,
                                         state().canvas_height - ui_fonts().bold_small->getHeight() - 2, 0);
                } else {
                    graphics->drawString(voyage::text::upper_ascii(GameCanvas::get_text(21)), 2,
                                         state().canvas_height - ui_fonts().bold_small->getHeight() - 2, 0);
                }
                graphics->setFont(ui_fonts().bold_medium);
                int16_t s = 0;
                if (state().game_state == 4) {
                    s = state().canvas_height - shiftLeft32(ui_fonts().bold_small->getHeight(), 2);
                } else if (state().game_state == 23 || state().game_state == 17) {
                    s = 10;
                }
                if (this->text_scroll_y < s) {
                    SpriteAtlas::draw_frame(graphics, this->ui_atlas, 54, this->canvas_center_x - ((SpriteAtlas::frame_width(this->ui_atlas, 54)) >> (1)),
                         state().canvas_height - ui_fonts().bold_small->getHeight() - 7);
                }
                if (n3 > n13) {
                    SpriteAtlas::draw_frame(graphics, this->ui_atlas, 53, this->canvas_center_x - ((SpriteAtlas::frame_width(this->ui_atlas, 53)) >> (1)),
                         state().canvas_height - ui_fonts().bold_small->getHeight() - 5 +
                             SpriteAtlas::frame_height(this->ui_atlas, 54));
                }
            }
            if (state().game_state == 10) {
                graphics->setColor(15327683);
                graphics->fillRect(0, state().canvas_height - ui_fonts().bold_small->getHeight() - 8,
                                   (int32_t)state().canvas_width, ui_fonts().bold_small->getHeight() + 8);
                graphics->setColor(0);
                if (this->text_scroll_y < state().canvas_height - shiftLeft32(ui_fonts().bold_small->getHeight(), 2)) {
                    SpriteAtlas::draw_frame(graphics, this->ui_atlas, 54, this->canvas_center_x - ((SpriteAtlas::frame_width(this->ui_atlas, 54)) >> (1)),
                         state().canvas_height - ui_fonts().bold_small->getHeight() - 7);
                }
                if (n3 > n13) {
                    SpriteAtlas::draw_frame(graphics, this->ui_atlas, 53, this->canvas_center_x - ((SpriteAtlas::frame_width(this->ui_atlas, 53)) >> (1)),
                         state().canvas_height - ui_fonts().bold_small->getHeight() - 5 +
                             SpriteAtlas::frame_height(this->ui_atlas, 54));
                }
            }
            if (n3 < n13) {
                if (this->text_transition_delay_ms >= 0) {
                    if (this->text_transition_delay_ms > 0) break;
                    this->text_transition_delay_ms = -1;
                } else {
                    if (state().text_screen_dwelled) {
                        state().text_screen_dwelled = false;
                        break;
                    }
                    state().text_screen_dwelled = true;
                    if (state().game_state != 21 && state().game_state != 23 && state().game_state != 17) {
                        this->text_transition_delay_ms = 3000;
                        break;
                    }
                }
                if (state().game_state == 23 || state().game_state == 17) {
                    this->text_scroll_at_end = true;
                }
                if (state().game_state == 9) {
                    this->text_scroll_y =
                        (int16_t)(state().canvas_height - shiftLeft32(ui_fonts().bold_small->getHeight(), 3));
                    this->text_scroll_ms = 0;
                    state().menu_selection = SharedArray<int8_t>(7);
                    state().menu_page = state().var_boolean_f ? (int8_t)5 : (int8_t)0;
                    for (n4 = 0; n4 < state().actors.length(); ++n4) {
                        GameCanvas::state().actors[n4] = nullptr;
                    }
                    GameCanvas::set_game_state((int8_t)4);
                    break;
                }
                if (state().game_state == 10) {
                    GameCanvas::set_game_state((int8_t)0);
                    break;
                }
                if (state().game_state != 4) break;
                GameCanvas::set_game_state((int8_t)3);
                if (state().var_boolean_f) {
                    state().menu_page = (int8_t)5;
                    break;
                }
                state().menu_page = 0;
                break;
            }
            if (state().game_state != 23 && state().game_state != 17) break;
            this->text_scroll_at_end = false;
            break;
        }
        case 11: {
            graphics->setColor(0);
            graphics->fillRect(0, 0, (int32_t)state().canvas_width, (int32_t)state().canvas_height);
            graphics->setColor(0xFFFFFF);
            graphics->setFont(ui_fonts().bold_medium);
            graphics->drawString(
                GameCanvas::get_text(428),
                ((state().canvas_width) >> (1)) -
                    ((ui_fonts().bold_medium->stringWidth(GameCanvas::get_text(428))) >> (1)),
                ((state().canvas_height) >> (1)) - ((ui_fonts().bold_medium->getHeight()) >> (1)), 0);
            graphics->drawString(voyage::text::upper_ascii(GameCanvas::get_text(427)), 2,
                                 state().canvas_height - ui_fonts().bold_medium->getHeight() - 2, 0);
            graphics->drawString(
                voyage::text::upper_ascii(GameCanvas::get_text(426)),
                state().canvas_width - ui_fonts().bold_medium->stringWidth(GameCanvas::get_text(426)) - 2,
                state().canvas_height - ui_fonts().bold_medium->getHeight() - 2, 0);
            break;
        }
        case 13: {
            const std::string message = this->persistence_error.empty() ? GameCanvas::get_text(450) : std::string(this->persistence_error);
            n3 = ((state().canvas_height) >> (1)) - ((ui_fonts().bold_medium->getHeight()) >> (1));
            graphics->setColor(0);
            graphics->fillRect(0, 0, (int32_t)state().canvas_width, (int32_t)state().canvas_height);
            graphics->setColor(0xFFFFFF);
            graphics->setFont(ui_fonts().bold_medium);
            graphics->drawString(
                message,
                ((state().canvas_width) >> (1)) -
                    ((ui_fonts().bold_medium->stringWidth(message)) >> (1)),
                n3, 0);
            graphics->setFont(ui_fonts().bold_small);
            graphics->drawString(
                GameCanvas::get_text(401),
                ((state().canvas_width) >> (1)) -
                    ((ui_fonts().bold_small->stringWidth(GameCanvas::get_text(401))) >> (1)),
                n3 + shiftLeft32(ui_fonts().bold_medium->getHeight(), 1), 0);
            break;
        }
        case 14: {
            graphics->setColor(0);
            graphics->fillRect(0, 0, (int32_t)state().canvas_width, (int32_t)state().canvas_height);
            graphics->setColor(0xFFFFFF);
            graphics->setFont(ui_fonts().bold_medium);
            graphics->drawString(
                GameCanvas::get_text(451),
                ((state().canvas_width) >> (1)) -
                    ((ui_fonts().bold_medium->stringWidth(GameCanvas::get_text(451))) >> (1)),
                ((state().canvas_height) >> (1)) - ((ui_fonts().bold_medium->getHeight()) >> (1)), 0);
            graphics->drawString(voyage::text::upper_ascii(GameCanvas::get_text(427)), 2,
                                 state().canvas_height - ui_fonts().bold_medium->getHeight() - 2, 0);
            graphics->drawString(
                voyage::text::upper_ascii(GameCanvas::get_text(426)),
                state().canvas_width - ui_fonts().bold_medium->stringWidth(GameCanvas::get_text(426)) - 2,
                state().canvas_height - ui_fonts().bold_medium->getHeight() - 2, 0);
            break;
        }
        case 15: {
            graphics->setColor(0);
            graphics->fillRect(0, 0, (int32_t)state().canvas_width, (int32_t)state().canvas_height);
            graphics->setColor(0xFFFFFF);
            graphics->setFont(ui_fonts().bold_medium);
            graphics->drawString(
                std::string("Please Wait..."),
                ((state().canvas_width) >> (1)) - ((ui_fonts().bold_medium->stringWidth(std::string("Please Wait..."))) >> (1)),
                ((state().canvas_height) >> (1)) - ((ui_fonts().bold_medium->getHeight()) >> (1)), 0);
            SpriteAtlas::draw_frame(graphics, state().loading_spinner_atlas, 5, ((state().canvas_width) >> (1)) - ((SpriteAtlas::frame_width(state().loading_spinner_atlas, 5)) >> (1)),
                 ((state().canvas_height) >> (1)) + ui_fonts().bold_medium->getHeight());
            break;
        }
        case 16: {
            graphics->setColor(0);
            graphics->fillRect(0, 0, (int32_t)state().canvas_width, (int32_t)state().canvas_height);
            graphics->setColor(0xFFFFFF);
            graphics->setFont(ui_fonts().bold_medium);
            graphics->drawString(
                GameCanvas::get_text(455),
                ((state().canvas_width) >> (1)) -
                    ((ui_fonts().bold_medium->stringWidth(GameCanvas::get_text(455))) >> (1)),
                ((state().canvas_height) >> (1)) - ((ui_fonts().bold_medium->getHeight()) >> (1)), 0);
            graphics->drawString(
                GameCanvas::get_text(464),
                ((state().canvas_width) >> (1)) -
                    ((ui_fonts().bold_medium->stringWidth(GameCanvas::get_text(464))) >> (1)),
                ((state().canvas_height) >> (1)) - ((ui_fonts().bold_medium->getHeight()) >> (1)) +
                    ui_fonts().bold_medium->getHeight(),
                0);
            graphics->drawString(voyage::text::upper_ascii(GameCanvas::get_text(427)), 2,
                                 state().canvas_height - ui_fonts().bold_medium->getHeight() - 2, 0);
            graphics->drawString(
                voyage::text::upper_ascii(GameCanvas::get_text(426)),
                state().canvas_width - ui_fonts().bold_medium->stringWidth(GameCanvas::get_text(426)) - 2,
                state().canvas_height - ui_fonts().bold_medium->getHeight() - 2, 0);
            break;
        }
        case 18: {
            graphics->setColor(0);
            graphics->setFont(ui_fonts().bold_small);
            graphics->fillRect(0, 0, (int32_t)state().canvas_width, (int32_t)state().canvas_height);
            graphics->setColor(0xFFFFFF);
            graphics->fillRect(5, 5, state().canvas_width - 10, ui_fonts().bold_small->getHeight() + 10);
            graphics->setColor(0xDD0000);
            graphics->drawString(
                GameCanvas::get_text(state().reference_title_id),
                ((state().canvas_width) >> (1)) -
                    ((ui_fonts().bold_small->stringWidth(GameCanvas::get_text(state().reference_title_id))) >> (1)),
                10, 0);
            graphics->setColor(0xFFFFFF);
            n3 = 35;
            for (n4 = (int32_t)state().reference_scroll_row; n4 < state().reference_pages[state().reference_page].length() &&
                                        state().reference_pages[state().reference_page][n4].has_value();
                 ++n4) {
                std::string string4;
                std::string string5 = string4 = *state().reference_pages[state().reference_page][n4];
                int32_t n14 = voyage::text::length(string4);
                while (ui_fonts().bold_small->stringWidth(string4) > state().canvas_width - 20) {
                    n14 = voyage::text::last_index_of(string4, 32, n14 - 1);
                    string4 = voyage::text::slice(string4, 0, n14);
                }
                graphics->drawString(string4, 10, n3, 0);
                n3 += ui_fonts().bold_small->getHeight();
                if (n14 < voyage::text::length(string5)) {
                    if (n3 + shiftLeft32(ui_fonts().bold_small->getHeight(), 1) >= state().canvas_height) break;
                    graphics->drawString(voyage::text::slice(string5, n14), 15, n3, 0);
                    n3 += ui_fonts().bold_small->getHeight();
                }
                if (n3 + shiftLeft32(ui_fonts().bold_small->getHeight(), 1) >= state().canvas_height) break;
            }
            graphics->drawString(voyage::text::upper_ascii(GameCanvas::get_text(449)), 2,
                                 state().canvas_height - ui_fonts().bold_small->getHeight() - 2, 0);
            if (state().reference_scroll_row != 0) {
                SpriteAtlas::draw_frame(graphics, this->ui_atlas, 54, state().canvas_width - SpriteAtlas::frame_width(this->ui_atlas, 54) - 2, 35);
            }
            if (n4 < state().reference_pages[state().reference_page].length() &&
                state().reference_pages[state().reference_page][n4].has_value()) {
                SpriteAtlas::draw_frame(graphics, this->ui_atlas, 53, state().canvas_width - SpriteAtlas::frame_width(this->ui_atlas, 53) - 2,
                     state().canvas_height - ui_fonts().bold_small->getHeight() - SpriteAtlas::frame_height(this->ui_atlas, 53) -
                         SpriteAtlas::frame_height(this->ui_atlas, 55) - 4);
                state().reference_has_more = true;
            } else {
                state().reference_has_more = false;
            }
            if (state().reference_title_id == 573) break;
            SpriteAtlas::draw_frame(graphics, this->ui_atlas, 56, 2,
                 state().canvas_height - ui_fonts().bold_small->getHeight() - SpriteAtlas::frame_height(this->ui_atlas, 53) - 4);
            SpriteAtlas::draw_frame(graphics, this->ui_atlas, 55, state().canvas_width - SpriteAtlas::frame_width(this->ui_atlas, 55) - 2,
                 state().canvas_height - ui_fonts().bold_small->getHeight() - SpriteAtlas::frame_height(this->ui_atlas, 53) - 4);
            break;
        }
        case 19: {
            graphics->setColor(0);
            graphics->fillRect(0, 0, (int32_t)state().canvas_width, (int32_t)state().canvas_height);
            graphics->setColor(0xFFFFFF);
            graphics->setFont(ui_fonts().bold_medium);
            graphics->drawString(
                GameCanvas::get_text(475),
                ((state().canvas_width) >> (1)) -
                    ((ui_fonts().bold_medium->stringWidth(GameCanvas::get_text(451))) >> (1)),
                ((state().canvas_height) >> (1)) - ((ui_fonts().bold_medium->getHeight()) >> (1)), 0);
            graphics->drawString(voyage::text::upper_ascii(GameCanvas::get_text(427)), 2,
                                 state().canvas_height - ui_fonts().bold_medium->getHeight() - 2, 0);
            graphics->drawString(
                voyage::text::upper_ascii(GameCanvas::get_text(426)),
                state().canvas_width - ui_fonts().bold_medium->stringWidth(GameCanvas::get_text(426)) - 2,
                state().canvas_height - ui_fonts().bold_medium->getHeight() - 2, 0);
            break;
        }
        case 20: {
            graphics->setColor(0);
            graphics->fillRect(0, 0, (int32_t)state().canvas_width, (int32_t)state().canvas_height);
            graphics->setColor(0xFFFFFF);
            graphics->setFont(ui_fonts().bold_small);
            graphics->drawString(
                GameCanvas::get_text(566),
                ((state().canvas_width) >> (1)) -
                    ((ui_fonts().bold_small->stringWidth(GameCanvas::get_text(566))) >> (1)),
                ((state().canvas_height) >> (1)) - ((ui_fonts().bold_small->getHeight()) >> (1)), 0);
            graphics->drawString(voyage::text::upper_ascii(GameCanvas::get_text(567)), 2,
                                 state().canvas_height - ui_fonts().bold_small->getHeight() - 2, 0);
            break;
        }
        case 22: {
            graphics->setColor(0);
            graphics->fillRect(0, 0, (int32_t)state().canvas_width, (int32_t)state().canvas_height);
            graphics->setColor(0xFFFFFF);
            graphics->setFont(ui_fonts().bold_medium);
            if (GameCanvas::get_text(571).empty()) {
                TextResources *text_resources = session_new<TextResources>();
                std::string string6 = text_resources->load_intro_segment((int8_t)2);
                graphics->drawString(string6,
                                     ((state().canvas_width) >> (1)) -
                                         ((ui_fonts().bold_medium->stringWidth(string6)) >> (1)),
                                     ((state().canvas_height) >> (1)) - ((ui_fonts().bold_medium->getHeight()) >> (1)),
                                     0);
                string6 = text_resources->load_intro_segment((int8_t)4);
                graphics->drawString(voyage::text::upper_ascii(string6), 2,
                                     state().canvas_height - ui_fonts().bold_medium->getHeight() - 2, 0);
                string6 = text_resources->load_intro_segment((int8_t)3);
                graphics->drawString(voyage::text::upper_ascii(string6),
                                     state().canvas_width - ui_fonts().bold_medium->stringWidth(string6) - 2,
                                     state().canvas_height - ui_fonts().bold_medium->getHeight() - 2, 0);
                break;
            }
            graphics->drawString(
                GameCanvas::get_text(571),
                ((state().canvas_width) >> (1)) -
                    ((ui_fonts().bold_medium->stringWidth(GameCanvas::get_text(571))) >> (1)),
                ((state().canvas_height) >> (1)) - ((ui_fonts().bold_medium->getHeight()) >> (1)), 0);
            graphics->drawString(voyage::text::upper_ascii(GameCanvas::get_text(22)), 2,
                                 state().canvas_height - ui_fonts().bold_medium->getHeight() - 2, 0);
            graphics->drawString(
                voyage::text::upper_ascii(GameCanvas::get_text(426)),
                state().canvas_width - ui_fonts().bold_medium->stringWidth(GameCanvas::get_text(426)) - 2,
                state().canvas_height - ui_fonts().bold_medium->getHeight() - 2, 0);
        }
    }
    if (this->menu_screen->is_open == 1) {
        this->menu_screen->render(graphics);
    }
}

bool GameCanvas::can_bind_key(int32_t n, int32_t n2) {
    int32_t n3 = 0;
    n = n >= 0 ? n : this->getGameAction(n);
    for (n3 = 0; n3 < key_bindings_state().pending.length(); ++n3) {
        if (n3 == state().control_row || n != key_bindings_state().pending[n3]) continue;
        return false;
    }
    if (n2 == 3) {
        return false;
    }
    if (n2 == 4) {
        return false;
    }
    if (n2 == 5) {
        return false;
    }
    if (n2 == 6) {
        return false;
    }
    if (n2 == 7) {
        return false;
    }
    if (n == state().character_softkey_code) {
        return false;
    }
    return n != state().menu_softkey_code;
}







bool GameCanvas::is_merchant_screen() { return state().game_state == 1; }

void GameCanvas::place_world_item(int32_t n, bool bl, int32_t n2, int32_t n3) {
    int32_t n4 = n2 * state().level_cols + n3;
    if (state().world_item_count >= state().world_item_entries.length() - 4 || n4 >= this->level_tile_count || n4 < 0) {
        return;
    }
    GameCanvas::state().tile_layers.back()[n4] = bl ? -45 : 22;
    int8_t by = state().world_item_count;
    state().world_item_count = (int8_t)(by + 1);
    GameCanvas::state().world_item_entries[by] = (int8_t)n2;
    int8_t by2 = state().world_item_count;
    state().world_item_count = (int8_t)(by2 + 1);
    GameCanvas::state().world_item_entries[by2] = (int8_t)n3;
    int8_t by3 = state().world_item_count;
    state().world_item_count = (int8_t)(by3 + 1);
    GameCanvas::state().world_item_entries[by3] = (int8_t)n;
}

void GameCanvas::show_text_screen(const std::string &string) {
    GameCanvas::layout_text_screen(string);
    GameCanvas::set_game_state((int8_t)10);
}

void GameCanvas::set_respawn_position(int32_t n, int32_t n2) {
    state().respawn_x = (int16_t)n;
    state().respawn_y = (int16_t)n2;
}

void GameCanvas::load_language(const std::string &string, int32_t n) {
    (void)string;
    current_string_table().load_language(n);
}

std::string GameCanvas::get_text(int32_t n) { return current_string_table().get_text(n); }

int32_t GameCanvas::find_text_id(const std::string &string) { return current_string_table().find_text_id(string); }

void GameCanvas::layout_dialogue(const std::string &string) {
    this->dialogue_box_width = interface_width() - 10;
    this->dialogue_wrap_width = this->dialogue_box_width - SpriteAtlas::frame_width(this->ui_atlas, 54) - 13;
    this->dialogue_box_height = min32(((state().canvas_height) >> (1)), SpriteAtlas::frame_height(this->ui_atlas, 51)) - 4;
    GameCanvas::format_wrapped_text(string, this->dialogue_lines, this->dialogue_wrap_width);
}

void GameCanvas::format_wrapped_text(const std::string &string, std::vector<std::string> &lines, int32_t n) {
    int32_t n2 = 0;
    int32_t n3 = 0;
    int32_t n4 = 0;
    int32_t n5 = 0;
    uint16_t c2 = 0;
    std::string text = string;
    text = key_bindings_state().substitute(text);
    if (state().speaker_name.has_value()) {
        text = *state().speaker_name + ": " + text;
    } else if (voyage::text::index_of(text, std::string(":")) != -1) {
        state().speaker_name = voyage::text::slice(text, 0, voyage::text::index_of(text, std::string(":")));
    }
    lines.clear();
    for (n2 = 0; n2 < voyage::text::length(text) - 1; ++n2) {
        c2 = voyage::text::byte_at(text, n2);
        n3 = ui_fonts().bold_small->substringWidth(text, n5, n2 - n5 + 1);
        if (c2 == ' ') {
            n4 = n2;
        }
        if (n3 < n || n4 <= 0) continue;
        lines.push_back(voyage::text::slice(text, n5, n4));
        n5 = n2 = n4 + 1;
        n4 = 0;
    }
    if (n2 > n5) {
        lines.push_back(voyage::text::slice(text, n5, n2 + 1));
    }
}

void GameCanvas::open_dialogue() {
    this->dialogue_scroll_y = -1;
    this->dialogue_at_end = true;
    this->dialogue_open = true;
    this->dialogue_opened_ms = currentSession().clock.now();
    this->dialogue_confirm_ready = !this->confirm_input_down;
}

void GameCanvas::draw_dialogue(Graphics *graphics) {
    bool bl = false;
    bool bl2 = true;
    int32_t n = this->dialogue_text_x;
    int32_t n2 = this->dialogue_text_y;
    int32_t n3 = 0;
    int32_t n4 = 0;
    const int32_t ui_left = interface_left();
    SpriteAtlas::draw_frame(graphics, this->ui_atlas, 51, ui_left + 5, 5);
    for (n3 = 0;
         n3 <= (this->dialogue_box_width - 5 - SpriteAtlas::frame_width(this->ui_atlas, 51) - SpriteAtlas::frame_width(this->ui_atlas, 50)) /
                   SpriteAtlas::frame_width(this->ui_atlas, 52);
         ++n3) {
        SpriteAtlas::draw_frame(graphics, this->ui_atlas, 52, ui_left + 5 + SpriteAtlas::frame_width(this->ui_atlas, 51) + n3 * SpriteAtlas::frame_width(this->ui_atlas, 52), 5);
    }
    SpriteAtlas::draw_frame(graphics, this->ui_atlas, 50, ui_left + 5 + SpriteAtlas::frame_width(this->ui_atlas, 51) + n3 * SpriteAtlas::frame_width(this->ui_atlas, 52), 5);
    n4 = n3;
    graphics->setColor(0xFFFFFF);
    graphics->setFont(ui_fonts().bold_small);
    for (n3 = 0; n3 < (int32_t)this->dialogue_lines.size() && bl2; ++n3) {
        std::string string = this->dialogue_lines[(size_t)n3];
        if (n2 - this->dialogue_scroll_y >= this->dialogue_text_y &&
            n2 - this->dialogue_scroll_y <=
                this->dialogue_text_y + this->dialogue_box_height - ui_fonts().bold_small->getHeight()) {
            bl |= n3 == 0;
            if (state().speaker_name.has_value() && voyage::text::starts_with(string, *state().speaker_name) && n3 == 0) {
                string = voyage::text::slice(string, voyage::text::length(*state().speaker_name) + 2);
                graphics->setColor(0x660000);
                graphics->drawString(*state().speaker_name + ": ", ui_left + n, n2 - this->dialogue_scroll_y, 0);
                graphics->setColor(0xFFFFFF);
                graphics->drawString(
                    string, ui_left + n + ui_fonts().bold_small->stringWidth(*state().speaker_name + ": "),
                    n2 - this->dialogue_scroll_y, 0);
            } else {
                graphics->drawString(string, ui_left + n, n2 - this->dialogue_scroll_y, 0);
            }
        }
        bl2 = (n2 += ui_fonts().bold_small->getHeight() + 1) - this->dialogue_scroll_y <
              this->dialogue_text_y + this->dialogue_box_height - ui_fonts().bold_small->getHeight() - 1;
    }
    this->dialogue_at_end = n3 == (int32_t)this->dialogue_lines.size() && bl2;
    if (!bl) {
        SpriteAtlas::draw_frame(graphics, this->ui_atlas, 54,
             ui_left + 5 + SpriteAtlas::frame_width(this->ui_atlas, 51) + n4 * SpriteAtlas::frame_width(this->ui_atlas, 52) - SpriteAtlas::frame_width(this->ui_atlas, 54) + 3, 8);
    }
    if (!this->dialogue_at_end) {
        SpriteAtlas::draw_frame(graphics, this->ui_atlas, 53,
             ui_left + 5 + SpriteAtlas::frame_width(this->ui_atlas, 51) + n4 * SpriteAtlas::frame_width(this->ui_atlas, 52) - SpriteAtlas::frame_width(this->ui_atlas, 53) + 3,
             5 + SpriteAtlas::frame_height(this->ui_atlas, 51) - SpriteAtlas::frame_height(this->ui_atlas, 53) - 3);
    }
}

void GameCanvas::handle_dialogue_input(int32_t n) {
    if (!this->dialogue_open) {
        return;
    }
    if (n == 3 && this->dialogue_scroll_y > -1) {
        this->dialogue_scroll_y -= 4;
        return;
    }
    if (n == 4 && !this->dialogue_at_end) {
        this->dialogue_scroll_y += 4;
        return;
    }
    const bool can_dismiss = this->modernized_dialogue_input
                                 ? this->dialogue_confirm_ready
                                 : currentSession().clock.now() - this->dialogue_opened_ms >= 1000L;
    if (n == 7 && can_dismiss) {
        this->dialogue_open = false;
        this->dialogue_confirm_ready = false;
        this->player->tile_use_script_id = 0;
        this->input_consumed = 1;
    }
}

void GameCanvas::set_speaker_name(const std::optional<std::string> &string) { state().speaker_name = string; }

void GameCanvas::initialize_session_state() {
    state().tile_width = (int8_t)32;
    state().tile_height = (int8_t)16;
    state().character_softkey_code = (int8_t)22;
    state().menu_softkey_code = (int8_t)21;
    state().var_byte_c = (int8_t)23;
    state().var_byte_d = (int8_t)-104;
    state().var_byte_e = (int8_t)-105;
    currentSession().random = session_new<GameRandom>();
    state().random = currentSession().random;
    state().menu_selection = SharedArray<int8_t>(8);
    state().menu_page = (int8_t)-1;
    state().status_scroll_style = 0;
    state().status_duration_ms = 0;
    state().status_elapsed_ms = 0;
    state().status_color = 0;
    state().status_x = 0;
    state().status_y = 0;
    state().status_blink_ms = 0;
    state().status_scroll_ms = 0;
    state().control_row = 0;
    state().port_options_row = 0;
    state().hud_blink_ms = -1;
    clear_pending_key();
    state().hud_blink_on = true;
    state().canvas_width = 0;
    state().canvas_height = 0;
    state().level_rows = 0;
    state().level_cols = 0;
    state().state_changes_enabled = true;
    state().game_state = (int8_t)-1;
    state().suspended_game_state = (int8_t)-1;
    state().tile_walkable.setNull();
    current_string_table().resource_buffer.setNull();
    state().text_screen_dwelled = true;
    state().awaiting_key_bind = false;
    state().status_blink_off = false;
    state().var_boolean_c = true;
    state().actors = SharedArray<Actor *>(25);
    state().instance = nullptr;
    state().game_data = nullptr;
    state().tile_layers.clear();
    state().menu_pages.setNull();
    state().control_rows.setNull();
    state().status_text = std::nullopt;
    state().level_script_path = std::nullopt;
    state().tile_use_script.setNull();
    state().player_gold = 100;
    state().version_text.clear();
    state().scene_clear_color = 0;
    state().respawn_x = 0;
    state().respawn_y = 0;
    state().loading_spinner_atlas = nullptr;
    state().world_item_entries = SharedArray<int8_t>(75);
    state().world_item_count = 0;
    state().hud_visible = true;
    state().reference_pages.setNull();
    state().reference_page = 0;
    state().reference_scroll_row = 0;
    state().reference_title_id = 0;
    state().reference_has_more = false;
    state().menu_page_before_options = 0;
    state().equipped_weapon_node = nullptr;
    state().equipped_spell_node = nullptr;
    state().splash_image = nullptr;
    state().title_image = nullptr;
    state().var_boolean_f = false;
    state().speaker_name = std::nullopt;
}
