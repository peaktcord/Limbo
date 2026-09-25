#include "src/limbo/game_data.hpp"
#include "src/limbo/crash_report.hpp"

#include "src/limbo/actor_system.hpp"
#include "src/limbo/game_canvas.hpp"
#include "src/limbo/magic_effects.hpp"
#include "src/limbo/sprite_atlas.hpp"

void GameData::execute_script_step(int64_t elapsed_ms) {
    crash_report().phase = "script interpreter";
    int32_t current_pc = this->frame_pc[this->call_depth - 1];
    crash_report().script_pc = current_pc;
    crash_report().opcode = -1;
    update_game_log_context(GameCanvas::state().instance, "script interpreter");
    int8_t opcode = this->script_bytecode[current_pc];
    crash_report().opcode = (uint8_t)opcode;
    if (this->waiting_for_key) {
        return;
    }
    if (this->game_canvas->dialogue_open) {
        return;
    }
    if (this->pending_move_actor >= 0) {
        ActorSystem::set_status_icon(GameCanvas::state().actors[this->pending_move_actor], (int8_t)0);
        this->pending_move_actor = (int8_t)-1;
    }
    if (this->game_canvas->is_merchant_screen()) {
        return;
    }
    if (this->script_timer_target_ms >= 0) {
        this->script_timer_ms = (int32_t)((int64_t)this->script_timer_ms + elapsed_ms);
        if (this->script_timer_ms >= this->script_timer_target_ms) {
            this->script_timer_target_ms = -1;
        } else {
            return;
        }
    }
    if (!this->waiting_for_actors.isNull()) {
        for (int32_t i = 0; i < this->waiting_for_actors.length(); ++i) {
            if (GameCanvas::state().actors[this->waiting_for_actors[i]] == nullptr || GameCanvas::state().actors[this->waiting_for_actors[i]]->move_target[0] == -1) continue;
            return;
        }
        this->waiting_for_actors.setNull();
    }
    if (!this->scripted_walk_target.isNull()) {
        switch (this->interpreter_state) {
            case 0: {
                this->game_canvas->set_player_input_enabled(false);
                this->game_canvas->follow_actor(this->scripted_walk_actor);
                if (this->scripted_walk_axis == 0 || this->scripted_walk_axis == 1) {
                    ActorSystem::set_move_target(GameCanvas::state().actors[this->scripted_walk_actor], GameCanvas::state().actors[this->scripted_walk_actor]->world_position[0], this->scripted_walk_target[1]);
                } else {
                    ActorSystem::set_move_target(GameCanvas::state().actors[this->scripted_walk_actor], this->scripted_walk_target[0], GameCanvas::state().actors[this->scripted_walk_actor]->world_position[1]);
                }
                ActorSystem::set_anim_state(GameCanvas::state().actors[this->scripted_walk_actor], (int8_t)2);
                ActorSystem::set_attribute(GameCanvas::state().actors[this->scripted_walk_actor], 7, 900, this);
                this->interpreter_state = 1;
                return;
            }
            case 1: {
                if (GameCanvas::state().actors[this->scripted_walk_actor]->move_target[0] != -1) break;
                ActorSystem::set_anim_state(GameCanvas::state().actors[this->scripted_walk_actor], (int8_t)3);
                ActorSystem::set_attribute(GameCanvas::state().actors[this->scripted_walk_actor], 7, 400, this);
                ActorSystem::set_move_target(GameCanvas::state().actors[this->scripted_walk_actor], this->scripted_walk_target[0], this->scripted_walk_target[1]);
                this->interpreter_state = (int8_t)2;
                return;
            }
            case 2: {
                if (GameCanvas::state().actors[this->scripted_walk_actor]->move_target[0] != -1) break;
                this->game_canvas->set_player_input_enabled(true);
                this->scripted_walk_target.setNull();
            }
        }
        return;
    }
    int32_t active_frame = this->call_depth - 1;
    this->frame_pc[active_frame] = this->frame_pc[active_frame] + 1;
    switch (opcode) {
        case 0: {
            voyage::logging::write(voyage::logging::Level::Error, "Script", "Unexpected opcode 0");
            log_game_context(this->game_canvas);
            return;
        }
        case 1: {
            voyage::logging::write(voyage::logging::Level::Error, "Script", "Unexpected opcode 1");
            log_game_context(this->game_canvas);
            return;
        }
        case 2: {
            this->return_from_script();
            return;
        }
        case 3: {
            int32_t text_id = this->read_script_int16();
            GameCanvas::set_speaker_name(std::nullopt);
            std::string dialogue_text = (text_id & 0xF000) == 61440 ? GameCanvas::get_text(text_id & 0xFFF) : this->read_script_text(text_id);
            this->game_canvas->layout_dialogue(dialogue_text);
            this->game_canvas->open_dialogue();
            return;
        }
        case 4: {
            int32_t width = this->read_script_byte();
            int32_t height = this->read_script_byte();
            this->game_canvas->resize_canvas(width, height);
            return;
        }
        case 5: {
            this->read_script_byte();
            return;
        }
        case 6: {
            this->read_script_byte();
            return;
        }
        case 7: {
            GameCanvas::state().var_boolean_c = this->read_script_byte() == 1;
            return;
        }
        case 8: {
            try {
                this->game_canvas->load_level_map(this->read_script_text(this->read_script_byte()));
                this->game_canvas->load_tile_atlas(this->read_script_text(this->read_script_byte()));
                return;
            }
            catch (const std::exception &exception) {
                voyage::logging::exception("Script", "Loading the scripted level", exception);
                log_game_context(this->game_canvas);
                return;
            }
        }
        case 9: {
            this->read_script_text(this->read_script_byte());
            return;
        }
        case 10: {
            int32_t splash_id = this->read_script_byte();
            int32_t color_rgb = this->read_script_int24();
            this->game_canvas->show_splash_screen(splash_id, color_rgb);
            return;
        }
        case 11: {
            this->script_timer_target_ms = this->read_script_int16();
            this->script_timer_ms = 0;
            return;
        }
        case 12: {
            GameCanvas::set_game_state((int8_t)0);
            return;
        }
        case 13: {
            this->read_script_byte();
            return;
        }
        case 14: {
            switch (this->read_script_byte()) {
                case 0: {
                    this->input_trigger_scripts[TRIGGER_UP] = this->read_script_byte();
                    break;
                }
                case 1: {
                    this->input_trigger_scripts[TRIGGER_DOWN] = this->read_script_byte();
                    break;
                }
                case 2: {
                    this->input_trigger_scripts[TRIGGER_LEFT] = this->read_script_byte();
                    break;
                }
                case 3: {
                    this->input_trigger_scripts[TRIGGER_RIGHT] = this->read_script_byte();
                    break;
                }
                case 4: {
                    this->input_trigger_scripts[TRIGGER_ACTION] = this->read_script_byte();
                }
            }
            return;
        }
        case 15: {
            int32_t name_ref = this->read_script_int16();
            std::optional<std::string> actor_name;
            if (name_ref != 0) {
                actor_name = (name_ref & 0xF000) == 61440 ? GameCanvas::get_text(name_ref & 0xFFF) : this->read_script_text(name_ref);
            }
            int32_t slot = this->read_script_byte();
            int32_t record_id = this->read_script_byte();
            int32_t pos_x = this->read_script_int16();
            int32_t pos_y = this->read_script_int16();
            this->game_canvas->spawn_actor_in_slot(actor_name, this->string_pool[this->script_table[record_id][1]], (int8_t)slot, pos_x, pos_y, this->script_table[record_id]);
            return;
        }
        case 16: {
            int32_t tile_y = this->read_script_byte();
            int32_t tile_x = this->read_script_byte();
            int32_t enter_script = this->read_script_byte();
            int32_t exit_script = this->read_script_byte();
            int32_t use_script = this->read_script_byte();
            this->game_canvas->set_tile_scripts(tile_y, tile_x, enter_script, exit_script, use_script);
            return;
        }
        case 17: {
            int32_t actor_id = this->read_script_byte();
            int32_t target_x = this->read_script_int16();
            int32_t target_y = this->read_script_int16();
            if (GameCanvas::state().actors[actor_id] == nullptr) break;
            ActorSystem::set_move_target(GameCanvas::state().actors[actor_id], target_x, target_y);
            return;
        }
        case 18: {
            int32_t tile_y = this->read_script_byte();
            int32_t tile_x = this->read_script_byte();
            int32_t layer = this->read_script_byte();
            int32_t frame = this->read_script_byte();
            this->game_canvas->set_tile_frame(tile_y, tile_x, layer, frame);
            return;
        }
        case 19: {
            this->game_canvas->set_player_input_enabled(this->read_script_byte() == 1);
            return;
        }
        case 20: {
            GameCanvas::remove_actor(this->read_script_byte());
            return;
        }
        case 21: {
            this->waiting_for_actors = SharedArray<int32_t>(this->read_script_byte());
            for (int32_t i = 0; i < this->waiting_for_actors.length(); ++i) {
                this->waiting_for_actors[i] = this->read_script_byte();
            }
            break;
        }
        case 22: {
            int32_t tile_y = this->read_script_byte();
            int32_t tile_x = this->read_script_byte();
            int32_t blocked = this->read_script_byte();
            this->game_canvas->set_tile_blocked(tile_y, tile_x, blocked == 1);
            return;
        }
        case 23: {
            GameData::call_script(this->read_script_byte());
            return;
        }
        case 24: {
            int32_t actor_id = this->read_script_byte();
            int32_t anim_state = this->read_script_byte();
            ActorSystem::set_anim_state(GameCanvas::state().actors[actor_id], (int8_t)anim_state);
            return;
        }
        case 25: {
            int32_t world_x = this->read_script_int16();
            int32_t world_y = this->read_script_int16();
            this->game_canvas->center_camera_at(world_x, world_y);
            return;
        }
        case 26: {
            this->game_canvas->follow_actor(this->read_script_byte());
            return;
        }
        case 27: {
            int32_t tile_y = this->read_script_byte();
            int32_t tile_x = this->read_script_byte();
            this->game_canvas->set_tile_scripts(tile_y, tile_x, 255, 255, 255);
            return;
        }
        case 28: {
            switch (this->read_script_byte()) {
                case 0: {
                    this->input_trigger_scripts[TRIGGER_UP] = -1;
                    break;
                }
                case 1: {
                    this->input_trigger_scripts[TRIGGER_DOWN] = -1;
                    break;
                }
                case 2: {
                    this->input_trigger_scripts[TRIGGER_LEFT] = -1;
                    break;
                }
                case 3: {
                    this->input_trigger_scripts[TRIGGER_RIGHT] = -1;
                    break;
                }
                case 4: {
                    this->input_trigger_scripts[TRIGGER_ACTION] = -1;
                }
            }
            return;
        }
        case 29: {
            this->game_canvas->load_level_script(this->read_script_text(this->read_script_byte()));
            return;
        }
        case 32: {
            int32_t actor_id = this->read_script_byte();
            int32_t death_type = this->read_script_byte();
            int32_t script_id = this->read_script_byte();
            ActorSystem::set_death_script(GameCanvas::state().actors[actor_id], death_type, script_id);
            return;
        }
        case 33: {
            int32_t actor_id = this->read_script_byte();
            int32_t death_type = this->read_script_byte();
            ActorSystem::clear_death_script(GameCanvas::state().actors[actor_id], death_type);
            return;
        }
        case 34: {
            int32_t actor_id = this->read_script_byte();
            int32_t attr_id = this->read_script_byte();
            int32_t attr_val = 0;
            switch (attr_id) {
                case 2:
                case 3:
                case 4:
                case 5:
                case 6:
                case 8:
                case 9:
                case 10:
                case 11:
                case 12:
                case 13:
                case 18:
                case 19:
                case 20: {
                    attr_val = this->read_script_byte();
                    break;
                }
                case 7:
                case 14:
                case 15: {
                    attr_val = this->read_script_int16();
                }
            }
            if (GameCanvas::state().actors[actor_id] == nullptr) break;
            ActorSystem::set_attribute(GameCanvas::state().actors[actor_id], attr_id, attr_val, this);
            return;
        }
        case 35: {
            this->read_script_byte();
            return;
        }
        case 36: {
            int32_t actor_id = this->read_script_byte();
            int32_t pos_x = this->read_script_int16();
            int32_t pos_y = this->read_script_int16();
            if (GameCanvas::state().actors[actor_id] == nullptr) break;
            ActorSystem::set_position(GameCanvas::state().actors[actor_id], pos_x, pos_y);
            return;
        }
        case 37: {
            int32_t actor_id = this->read_script_byte();
            int32_t item_kind = this->read_script_byte();
            int32_t item_id = this->read_script_byte();
            if (GameCanvas::state().actors[actor_id] == nullptr) break;
            switch (item_kind) {
                case 1: {
                    ActorSystem::add_inventory_item(GameCanvas::state().actors[actor_id], item_kind, this->armour_table[item_id]);
                    break;
                }
                case 2: {
                    ActorSystem::add_inventory_item(GameCanvas::state().actors[actor_id], item_kind, this->potion_table[item_id]);
                    break;
                }
                case 0: {
                    ActorSystem::add_inventory_item(GameCanvas::state().actors[actor_id], item_kind, this->weapon_table[item_id]);
                }
            }
            return;
        }
        case 38: {
            int32_t actor_id = this->read_script_byte();
            int32_t item_kind = this->read_script_byte();
            int32_t item_id = this->read_script_byte();
            if (GameCanvas::state().actors[actor_id] == nullptr) break;
            switch (item_kind) {
                case 1: {
                    ActorSystem::remove_inventory_item(GameCanvas::state().actors[actor_id], item_kind, this->armour_table[item_id]);
                    break;
                }
                case 2: {
                    ActorSystem::remove_inventory_item(GameCanvas::state().actors[actor_id], item_kind, this->potion_table[item_id]);
                    break;
                }
                case 0: {
                    ActorSystem::remove_inventory_item(GameCanvas::state().actors[actor_id], item_kind, this->weapon_table[item_id]);
                }
            }
            return;
        }
        case 39: {
            int32_t text_id = this->read_script_int16();
            std::string message = (text_id & 0xF000) == 61440 ? GameCanvas::get_text(text_id & 0xFFF) : this->read_script_text(text_id);
            int32_t r = this->read_script_byte();
            int32_t g = this->read_script_byte();
            int32_t b = this->read_script_byte();
            GameCanvas::set_status_message(message, r, g, b);
            return;
        }
        case 40: {
            GameCanvas::set_status_message(std::nullopt, 0, 0, 0);
            return;
        }
        case 41: {
            int32_t actor_id = this->read_script_byte();
            int32_t target_x = this->read_script_int16();
            if (GameCanvas::state().actors[actor_id] == nullptr) break;
            ActorSystem::set_move_target(GameCanvas::state().actors[actor_id], target_x, GameCanvas::state().actors[actor_id]->world_position[1]);
            return;
        }
        case 42: {
            int32_t actor_id = this->read_script_byte();
            int32_t target_y = this->read_script_int16();
            if (GameCanvas::state().actors[actor_id] == nullptr) break;
            ActorSystem::set_move_target(GameCanvas::state().actors[actor_id], GameCanvas::state().actors[actor_id]->world_position[0], target_y);
            return;
        }
        case 43: {
            this->game_canvas->load_ui_atlas(this->read_script_text(this->read_script_byte()));
            return;
        }
        case 44: {
            this->game_canvas->show_main_menu();
            return;
        }
        case 45: {
            this->game_canvas->show_merchant_options();
            return;
        }
        case 46: {
            int32_t actor_id = this->read_script_byte();
            int32_t status_icon = this->read_script_byte();
            if (GameCanvas::state().actors[actor_id] == nullptr) break;
            ActorSystem::set_status_icon(GameCanvas::state().actors[actor_id], (int8_t)status_icon);
            return;
        }
        case 47: {
            int32_t record_id = this->read_script_byte();
            int32_t param1 = this->read_script_byte();
            int32_t param2 = this->read_script_byte();
            this->game_canvas->generate_level(this->get_record(9, record_id), this->dungeon_item_drops, param1, param2);
            return;
        }
        case 48: {
            this->game_canvas->clear_tile_layers();
            return;
        }
        case 49: {
            int32_t item_id = this->read_script_byte();
            int32_t tile_x = this->read_script_byte();
            int32_t tile_y = this->read_script_byte();
            this->game_canvas->place_world_item(item_id, true, tile_x, tile_y);
            return;
        }
        case 50: {
            int32_t start_y = this->read_script_byte();
            int32_t start_x = this->read_script_byte();
            int32_t end_y = this->read_script_byte();
            int32_t end_x = this->read_script_byte();
            int32_t enter_script = this->read_script_byte();
            int32_t exit_script = this->read_script_byte();
            int32_t use_script = this->read_script_byte();
            for (int32_t y = start_y; y <= end_y; ++y) {
                for (int32_t x = start_x; x <= end_x; ++x) {
                    this->game_canvas->set_tile_scripts(y, x, enter_script, exit_script, use_script);
                }
            }
            break;
        }
        case 51: {
            int32_t start_y = this->read_script_byte();
            int32_t start_x = this->read_script_byte();
            int32_t end_y = this->read_script_byte();
            int32_t end_x = this->read_script_byte();
            for (int32_t y = start_y; y <= end_y; ++y) {
                for (int32_t x = start_x; x <= end_x; ++x) {
                    this->game_canvas->set_tile_scripts(y, x, 255, 255, 255);
                }
            }
            break;
        }
        case 52: {
            this->scripted_walk_actor = (int8_t)this->read_script_byte();
            this->scripted_walk_axis = (int8_t)this->read_script_byte();
            int32_t target_x = this->read_script_int16();
            int32_t target_y = this->read_script_int16();
            this->scripted_walk_target = SharedArray<int32_t>{target_x, target_y};
            this->interpreter_state = 0;
            return;
        }
        case 53: {
            this->pending_move_actor = (int8_t)this->read_script_byte();
            int32_t status_icon = this->read_script_byte();
            int32_t text_id = this->read_script_int16();
            this->game_canvas->follow_actor(this->pending_move_actor);
            ActorSystem::set_status_icon(GameCanvas::state().actors[this->pending_move_actor], (int8_t)status_icon);
            if ((text_id & 0xF000) == 61440) {
                this->game_canvas->layout_dialogue(GameCanvas::get_text(text_id & 0xFFF));
            } else {
                this->game_canvas->layout_dialogue(this->read_script_text(text_id));
            }
            this->game_canvas->open_dialogue();
            return;
        }
        case 54: {
            return;
        }
        case 55: {
            return;
        }
        case 56: {
            std::string lang_name = this->read_script_text(this->read_script_byte());
            int32_t max_strings = this->read_script_byte();
            if (max_strings == 0) {
                max_strings = 65535;
            }
            GameCanvas::load_language(lang_name, max_strings);
            return;
        }
        case 57: {
            return;
        }
        case 58: {
            int32_t script_id = this->read_script_byte();
            int32_t multiplier = this->read_script_byte();
            this->script_table[script_id][2] = multiplier;
            for (int32_t stat_idx = 3; stat_idx <= 9; ++stat_idx) {
                SharedArray<int32_t> script_row = this->script_table[script_id];
                script_row[stat_idx] += multiplier * S().class_stat_growth_table[this->script_table[script_id][17]][stat_idx - 3];
            }
            break;
        }
        case 59: {
            int32_t actor_id = this->read_script_byte();
            int32_t enabled = this->read_script_byte();
            if (GameCanvas::state().actors[actor_id] == nullptr) break;
            ActorSystem::set_loot_drop_enabled(GameCanvas::state().actors[actor_id], enabled == 1);
            return;
        }
        case 60: {
            this->waiting_for_key = true;
            return;
        }
        case 61: {
            GameCanvas::set_game_state((int8_t)9);
            return;
        }
        case 62: {
            return;
        }
        case 63: {
            return;
        }
        case 64: {
            GameCanvas::state().scene_clear_color = this->read_script_int24();
            return;
        }
        case 65: {
            int32_t actor_id = this->read_script_byte();
            int32_t target_level = this->read_script_byte();
            if (GameCanvas::state().actors[actor_id] == nullptr) break;
            ActorSystem::level_up_to(GameCanvas::state().actors[actor_id], target_level);
            return;
        }
        case 66: {
            int32_t text_id = this->read_script_int16();
            if ((text_id & 0xF000) == 61440) {
                GameCanvas::show_text_screen(GameCanvas::get_text(text_id & 0xFFF));
                return;
            }
            GameCanvas::show_text_screen(this->read_script_text(text_id));
            return;
        }
        case 67: {
            int32_t script_id = this->read_script_byte();
            copySharedArray(this->default_script_table[script_id], 0, this->script_table[script_id], 0, this->script_table[script_id].length());
            return;
        }
        case 68: {
            int32_t effect_kind = this->read_script_byte();
            int32_t pos_x = this->read_script_int16();
            int32_t pos_y = this->read_script_int16();
            if (effect_kind == 0) {
                effect_kind = 8;
            } else if (effect_kind == 2) {
                effect_kind = 10;
            } else if (effect_kind == 1) {
                effect_kind = 9;
            }
            MagicEffects::spawn(effect_kind, pos_x, pos_y);
            return;
        }
        case 69: {
            int32_t effect_kind = this->read_script_byte();
            int32_t pos_x = this->read_script_int16();
            int32_t pos_y = this->read_script_int16();
            int32_t duration_sec = this->read_script_byte();
            if (effect_kind == 0) {
                effect_kind = 8;
            } else if (effect_kind == 2) {
                effect_kind = 10;
            } else if (effect_kind == 1) {
                effect_kind = 9;
            }
            MagicEffects::spawn(effect_kind, pos_x, pos_y, duration_sec * 1000);
            return;
        }
        case 70: {
            int32_t pos_x = this->read_script_int16();
            int32_t pos_y = this->read_script_int16();
            MagicEffects::remove_at_position(pos_x, pos_y);
            return;
        }
        case 71: {
            int32_t respawn_x = this->read_script_int16();
            int32_t respawn_y = this->read_script_int16();
            GameCanvas::set_respawn_position(respawn_x, respawn_y);
            return;
        }
        case 72: {
            SpriteAtlas::evict_image_prefix(this->read_script_text(this->read_script_int16()));
            return;
        }
        case 73: {
            GameCanvas::set_game_state((int8_t)15);
            GameCanvas::state().state_changes_enabled = false;
            return;
        }
        case 74: {
            GameCanvas::state().state_changes_enabled = true;
            return;
        }
        case 75: {
            int32_t actor_id = this->read_script_byte();
            if (GameCanvas::state().actors[actor_id] == nullptr) break;
            GameCanvas::state().actors[actor_id]->is_invulnerable = (int8_t)(GameCanvas::state().actors[actor_id]->is_invulnerable != 1 ? 1 : 0);
            return;
        }
        case 76: {
            GameCanvas::state().hud_visible = this->read_script_byte() == 1;
            return;
        }
        case 77: {
            GameCanvas::set_game_state((int8_t)4);
            GameCanvas::state().var_boolean_f = false;
            return;
        }
        case 78: {
            int32_t actor_id = this->read_script_byte();
            if (GameCanvas::state().actors[actor_id] == nullptr) break;
            GameCanvas::state().actors[actor_id]->auto_attack_enabled = (int8_t)this->read_script_byte();
            break;
        }
    }
}

void GameData::call_script(int32_t script_id) {
    if (GameData::S().instance->call_depth < GameData::S().instance->frame_script_id.length() - 2 && GameData::S().instance->call_depth < GameData::S().instance->frame_pc.length() - 2) {
        GameData::S().instance->frame_script_id[GameData::S().instance->call_depth] = script_id;
        GameData::S().instance->frame_pc[GameData::S().instance->call_depth++] = GameData::S().instance->script_entry_offsets[script_id];
    }
}

void GameData::return_from_script() {
    --this->call_depth;
}

void GameData::update_scripts(int64_t elapsed_ms) {
    if (this->call_depth > 0) {
        this->execute_script_step(elapsed_ms);
    }
}
