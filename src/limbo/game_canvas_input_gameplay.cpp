#include "src/limbo/game_canvas.hpp"

namespace {

constexpr int32_t ACTION_QUICK_POTION_HEALTH = ActorSystem::ACTION_QUICK_POTION_HEALTH;
constexpr int32_t ACTION_QUICK_POTION_MAGICKA = ActorSystem::ACTION_QUICK_POTION_MAGICKA;

constexpr int8_t SCRIPT_NONE = -1;
constexpr int8_t SCRIPT_LOCKED_TILE = -2;

constexpr int8_t CHEST_TILE_CLOSED = -45;
constexpr int8_t CHEST_TILE_OPEN = -44;

constexpr int32_t WORLD_ITEM_STRIDE = 3;

inline bool is_executable_script(int8_t script_id) {
    return script_id != 0 && script_id != SCRIPT_NONE && script_id != SCRIPT_LOCKED_TILE;
}

}

int32_t GameCanvas::handle_gameplay_input(int32_t raw_key_code, int64_t elapsed_ms) {
    if (state().pending_key_code == state().character_softkey_code) {
        if (this->player != nullptr && state().hud_visible) {
            this->build_character_menu();
            GameCanvas::set_game_state((int8_t)2);
            clear_pending_key();
        }
        return raw_key_code;
    }

    if (state().pending_key_code == state().menu_softkey_code) {
        this->build_frontend_menus();
        state().menu_page = (int8_t)5;
        for (int32_t i = 0; i < state().menu_selection.length(); ++i) {
            GameCanvas::state().menu_selection[i] = 0;
        }
        GameCanvas::set_game_state((int8_t)3);
        clear_pending_key();
        return raw_key_code;
    }

    const int32_t action = this->decode_input_action(raw_key_code);
    if (this->dialogue_open || this->menu_screen->is_open != 0 || !this->player_input_enabled ||
        this->player == nullptr || this->player->is_dead != 0) {
        return action;
    }

    const int8_t prev_enter_script_id = this->player->tile_enter_script_id;
    const int8_t prev_exit_script_id = this->player->tile_exit_script_id;
    const int8_t prev_use_script_id = this->player->tile_use_script_id;

    if (ActorSystem::handle_input(this->player, action, elapsed_ms, this->screen_aligned_movement)) {
        clear_pending_key();
        this->input_consumed = 0;
    }

    if (prev_use_script_id != this->player->tile_use_script_id &&
        is_executable_script(this->player->tile_use_script_id)) {
        GameData::call_script((uint16_t)this->player->tile_use_script_id);
    }

    if (action == ACTION_QUICK_POTION_HEALTH) {
        ActorSystem::use_quick_potion(this->player, true);
        clear_pending_key();
        this->input_consumed = 0;
    } else if (action == ACTION_QUICK_POTION_MAGICKA) {
        ActorSystem::use_quick_potion(this->player, false);
        clear_pending_key();
        this->input_consumed = 0;
    } else if (action == GameData::TRIGGER_ACTION && this->player->loot_pickup_delay_elapsed_ms > 1000) {
        SharedArray<int32_t> item_world_pos(2);
        for (int32_t entry_idx = 0; entry_idx < state().world_item_count; entry_idx += WORLD_ITEM_STRIDE) {
            const int32_t tile_y = state().world_item_entries[entry_idx + 0];
            const int32_t tile_x = state().world_item_entries[entry_idx + 1];
            const int32_t world_item_id = state().world_item_entries[entry_idx + 2];

            item_world_pos[0] = shiftLeft32(tile_y, 7);
            item_world_pos[1] = shiftLeft32(tile_x, 7);

            const int32_t dist = ActorSystem::distance(item_world_pos, this->player->world_position);
            if (dist >= 350) continue;

            SharedArray<int32_t> world_item_rec = state().game_data->get_record(6, world_item_id);
            if (world_item_rec.isNull()) continue;

            MagicEffects::spawn(8, item_world_pos[0], item_world_pos[1] + 128);

            const int32_t tile_index = tile_y * state().level_cols + tile_x;
            int8_t &ground_tile = state().tile_layers.back()[tile_index];
            ground_tile = (ground_tile == CHEST_TILE_CLOSED) ? CHEST_TILE_OPEN : 0;

            for (int32_t shift_idx = entry_idx;
                 shift_idx < state().world_item_count && shift_idx < state().world_item_entries.length() - 3;
                 shift_idx += WORLD_ITEM_STRIDE) {
                state().world_item_entries[shift_idx + 0] = state().world_item_entries[shift_idx + 3];
                state().world_item_entries[shift_idx + 1] = state().world_item_entries[shift_idx + 4];
                state().world_item_entries[shift_idx + 2] = state().world_item_entries[shift_idx + 5];
            }
            state().world_item_count = (int8_t)(state().world_item_count - WORLD_ITEM_STRIDE);

            const int32_t gold_amount = world_item_rec[2];
            const int32_t armour_id = world_item_rec[4];
            const int32_t weapon_id = world_item_rec[3];
            const int32_t potion_id = world_item_rec[5];

            if (gold_amount > 0) {
                GameCanvas::set_status_message(std::to_string(gold_amount) + " " + GameCanvas::get_text(38), 3, 4, 0);
                state().player_gold += gold_amount;
                break;
            }
            if (armour_id > 0) {
                SharedArray<int32_t> armour_rec = state().game_data->get_record(1, armour_id);
                GameCanvas::set_status_message(state().game_data->get_string(armour_rec[1]), 3, 4, 0);
                ActorSystem::add_inventory_item(this->player, 1, armour_rec);
                break;
            }
            if (weapon_id > 0) {
                SharedArray<int32_t> weapon_rec = state().game_data->get_record(4, weapon_id);
                GameCanvas::set_status_message(state().game_data->get_string(weapon_rec[1]), 3, 4, 0);
                ActorSystem::add_inventory_item(this->player, 0, weapon_rec);
                break;
            }
            if (potion_id > 0) {
                SharedArray<int32_t> potion_rec = state().game_data->get_record(2, potion_id);
                GameCanvas::set_status_message(state().game_data->get_string(potion_rec[1]), 3, 4, 0);
                ActorSystem::add_inventory_item(this->player, 2, potion_rec);
                break;
            }
            break;
        }
    }

    ActorSystem::check_tile_triggers(this->player, this->tile_enter_script, this->tile_exit_script);
    if (this->player->tile_enter_script_id == prev_enter_script_id) {
        return action;
    }

    if (is_executable_script(prev_exit_script_id)) {
        GameData::call_script((uint16_t)prev_exit_script_id);
    } else if (prev_exit_script_id == SCRIPT_LOCKED_TILE) {
        GameCanvas::set_status_message(std::nullopt, 0, 0, 0);
    }

    if (is_executable_script(this->player->tile_enter_script_id)) {
        GameData::call_script((uint16_t)this->player->tile_enter_script_id);
    } else if (this->player->tile_enter_script_id == SCRIPT_LOCKED_TILE) {
        GameCanvas::set_status_message(GameCanvas::get_text(24), 60, 4, 0);
    }

    return action;
}
