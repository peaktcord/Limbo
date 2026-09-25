#include "src/limbo/game_data.hpp"

GameDataStatics &GameData::S() { return oblivion_state().data; }

#include "src/limbo/actor_system.hpp"
#include "src/limbo/game_canvas.hpp"
#include "src/limbo/magic_effects.hpp"
#include "src/limbo/sprite_atlas.hpp"


void GameData::clinit() {
    S().class_stat_growth_table = SharedArray<SharedArray<int8_t>>{SharedArray<int8_t>{3, 1, 1, 2, 0, 1, 1},
                                                SharedArray<int8_t>{2, 2, 2, 1, 0, 1, 1},
                                                SharedArray<int8_t>{1, 3, 3, 1, 0, 2, 3}};
}

GameData::GameData(GameCanvas *b2) {
    this->game_canvas = b2;
    S().instance = this;
    this->reset();
}

SharedArray<int32_t> GameData::get_record(int32_t n, int32_t n2) {
    if (n2 < 0) {
        return SharedArray<int32_t>();
    }
    switch (n) {
        case 0: {
            if (n2 < this->script_table.length()) {
                return this->script_table[n2];
            }
        }
        case 1: {
            if (n2 < this->armour_table.length()) {
                return this->armour_table[n2];
            }
        }
        case 2: {
            if (n2 < this->potion_table.length()) {
                return this->potion_table[n2];
            }
        }
        case 4: {
            if (n2 < this->weapon_table.length()) {
                return this->weapon_table[n2];
            }
        }
        case 7: {
            return this->merchant_inventory;
        }
        case 9: {
            if (n2 < this->level_gen_table.length()) {
                return this->level_gen_table[n2];
            }
        }
        case 6: {
            if (n2 < this->world_item_table.length()) {
                return this->world_item_table[n2];
            }
        }
        case 5: {
            if (n2 < this->creature_table.length()) {
                return this->creature_table[n2];
            }
        }
        case 8: {
            if (n2 < this->spell_table.length()) {
                return this->spell_table[n2];
            }
        }
        case 10: {
            if (n2 >= this->loot_drop_table.length()) break;
            return this->loot_drop_table[n2];
        }
    }
    return SharedArray<int32_t>();
}

SharedArray<int32_t> GameData::get_item_record(int32_t n, int32_t n2) {
    switch (n) {
        case 1: {
            return this->armour_table[n2];
        }
        case 2: {
            return this->potion_table[n2];
        }
        case 0: {
            return this->weapon_table[n2];
        }
    }
    return SharedArray<int32_t>();
}

void GameData::handle_input_trigger(int32_t key_action) {
    if (this->waiting_for_key) {
        this->waiting_for_key = false;
        GameCanvas::clear_pending_key();
        return;
    }
    if (key_action == TRIGGER_UP && this->input_trigger_scripts[TRIGGER_UP] >= 0) {
        GameData::call_script(this->input_trigger_scripts[TRIGGER_UP]);
        this->input_trigger_scripts[TRIGGER_UP] = -1;
        return;
    }
    if (key_action == TRIGGER_DOWN && this->input_trigger_scripts[TRIGGER_DOWN] >= 0) {
        GameData::call_script(this->input_trigger_scripts[TRIGGER_DOWN]);
        this->input_trigger_scripts[TRIGGER_DOWN] = -1;
        return;
    }
    if (key_action == TRIGGER_LEFT && this->input_trigger_scripts[TRIGGER_LEFT] >= 0) {
        GameData::call_script(this->input_trigger_scripts[TRIGGER_LEFT]);
        this->input_trigger_scripts[TRIGGER_LEFT] = -1;
        return;
    }
    if (key_action == TRIGGER_RIGHT && this->input_trigger_scripts[TRIGGER_RIGHT] >= 0) {
        GameData::call_script(this->input_trigger_scripts[TRIGGER_RIGHT]);
        this->input_trigger_scripts[TRIGGER_RIGHT] = -1;
        return;
    }
    if (key_action == TRIGGER_ACTION && this->input_trigger_scripts[TRIGGER_ACTION] >= 0) {
        GameData::call_script(this->input_trigger_scripts[TRIGGER_ACTION]);
        this->input_trigger_scripts[TRIGGER_ACTION] = -1;
    }
}

std::string GameData::get_string(int32_t n) {
    if ((n & 0xF000) == 61440) {
        return GameCanvas::get_text(n & 0xFFF);
    }
    return this->string_pool[n];
}

int32_t GameData::resolve_string_id(const std::string &string) {
    int32_t n = 0;
    int32_t n2 = 0;
    for (n2 = 0; n2 < this->string_pool_count; ++n2) {
        if (!(this->string_pool[n2] == string)) continue;
        return n2;
    }
    n = GameCanvas::find_text_id(string);
    if (n == -1) {
        return n;
    }
    return 0xF000 | n;
}

SharedArray<int32_t> GameData::find_record(const std::string &string) {
    int32_t n = this->resolve_string_id(string);
    int32_t n2 = 0;
    if (n != -1) {
        for (n2 = 0; n2 < this->weapon_table.length(); ++n2) {
            if (this->weapon_table[n2].isNull() || this->weapon_table[n2][1] != n) continue;
            return this->weapon_table[n2];
        }
        for (n2 = 0; n2 < this->potion_table.length(); ++n2) {
            if (this->potion_table[n2].isNull() || this->potion_table[n2][1] != n) continue;
            return this->potion_table[n2];
        }
        for (n2 = 0; n2 < this->armour_table.length(); ++n2) {
            if (this->armour_table[n2].isNull() || this->armour_table[n2][1] != n) continue;
            return this->armour_table[n2];
        }
        for (n2 = 0; n2 < this->creature_table.length(); ++n2) {
            if (this->creature_table[n2].isNull() || this->creature_table[n2][1] != n) continue;
            return this->creature_table[n2];
        }
        for (n2 = 0; n2 < this->spell_table.length(); ++n2) {
            if (this->spell_table[n2].isNull() || this->spell_table[n2][1] != n) continue;
            return this->spell_table[n2];
        }
    }
    return SharedArray<int32_t>();
}

int32_t GameData::find_item_kind_by_name(const std::string &string) {
    int32_t n = this->resolve_string_id(string);
    int32_t n2 = 0;
    if (n != -1) {
        for (n2 = 0; n2 < this->weapon_table.length(); ++n2) {
            if (this->weapon_table[n2].isNull() || this->weapon_table[n2][1] != n) continue;
            return 0;
        }
        for (n2 = 0; n2 < this->potion_table.length(); ++n2) {
            if (this->potion_table[n2].isNull() || this->potion_table[n2][1] != n) continue;
            return 2;
        }
        for (n2 = 0; n2 < this->armour_table.length(); ++n2) {
            if (this->armour_table[n2].isNull() || this->armour_table[n2][1] != n) continue;
            return 1;
        }
    }
    return -1;
}

int32_t GameData::roll_loot_drop() {
    int32_t n = GameCanvas::state().random->nextInt();
    int32_t n2 = 1;
    while (true) {
        if (this->loot_drop_table[n2][1] != 0) {
            if (n % this->loot_drop_table[n2][2] == 0 && this->loot_drop_table[n2][3] > 0) {
                SharedArray<int32_t> nArray = this->loot_drop_table[n2];
                nArray[3] = nArray[3] - 1;
                return this->loot_drop_table[n2][1];
            }
        } else {
            return 0;
        }
        ++n2;
    }
}

bool GameData::class_has_skill(int32_t n, int32_t n2) {
    int32_t n3 = 0;
    for (n3 = 0; n3 < this->class_skills_table[n].length(); ++n3) {
        if (this->class_skills_table[n][n3] != n2) continue;
        return true;
    }
    return false;
}

std::optional<std::string> GameData::effect_name(int32_t n) {
    switch (n) {
        case 0: {
            return GameCanvas::get_text(523);
        }
        case 1: {
            return GameCanvas::get_text(524);
        }
        case 2: {
            return GameCanvas::get_text(525);
        }
        case 3: {
            return GameCanvas::get_text(526);
        }
        case 4: {
            return GameCanvas::get_text(527);
        }
        case 5: {
            return GameCanvas::get_text(528);
        }
        case 6: {
            return GameCanvas::get_text(529);
        }
        case 7: {
            return GameCanvas::get_text(530);
        }
        case 8: {
            return GameCanvas::get_text(531);
        }
        case 9: {
            return GameCanvas::get_text(532);
        }
        case 10: {
            return GameCanvas::get_text(533);
        }
        case 11: {
            return GameCanvas::get_text(534);
        }
        case 12: {
            return GameCanvas::get_text(535);
        }
        case 13: {
            return GameCanvas::get_text(536);
        }
        case 14: {
            return GameCanvas::get_text(537);
        }
    }
    return std::nullopt;
}
