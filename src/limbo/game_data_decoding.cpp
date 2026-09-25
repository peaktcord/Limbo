#include "src/limbo/game_data.hpp"

#include "src/limbo/actor_system.hpp"
#include "src/limbo/game_canvas.hpp"

#include "src/limbo/string_table.hpp"
#include "src/limbo/magic_effects.hpp"
#include "src/limbo/sprite_atlas.hpp"

void GameData::reset() {
    int32_t n = 0;
    int32_t n2 = 0;
    this->input_trigger_scripts[TRIGGER_UP] = -1;
    this->input_trigger_scripts[TRIGGER_DOWN] = -1;
    this->input_trigger_scripts[TRIGGER_LEFT] = -1;
    this->input_trigger_scripts[TRIGGER_RIGHT] = -1;
    this->input_trigger_scripts[TRIGGER_ACTION] = -1;
    this->merchant_inventory[0] = -1;
    this->call_depth = 0;
    this->script_timer_ms = 0;
    this->script_timer_target_ms = -1;
    this->string_pool_count = 0;
    this->waiting_for_actors.setNull();
    this->script_bytecode.setNull();
    this->dungeon_item_drop_count = 0;
    this->scripted_walk_target.setNull();
    this->pending_move_actor = (int8_t)-1;
    this->waiting_for_key = false;
    for (n = 0; n < this->dungeon_item_drops.length(); ++n) {
        this->dungeon_item_drops[n] = 0;
    }
    for (n = 0; n < 10; ++n) {
        this->frame_pc[n] = 0;
        this->frame_script_id[n] = 0;
    }
    for (n = 0; n < this->script_entry_offsets.length(); ++n) {
        this->script_entry_offsets[n] = 0;
    }
    for (n = 0; n < 30; ++n) {
        for (n2 = 0; n2 < 4; ++n2) {
            this->loot_drop_table[n][n2] = 0;
        }
    }
}

void GameData::load(const std::string &string) {
    int32_t n = 0;
    int32_t n2 = 0;
    this->reset();
    int32_t n3 = GameCanvas::load_resource(string);
    for (n = 1; n < current_string_table().resource_buffer[0] * 3; n += 3) {
        this->script_entry_offsets[current_string_table().resource_buffer[n]] = ((int32_t)(uint16_t)current_string_table().resource_buffer[n + 1] & 0xFF) << 8 | ((int32_t)(uint16_t)current_string_table().resource_buffer[n + 2] & 0xFF) << 0;
    }
    while (current_string_table().resource_buffer[n++] == 30) {
        switch (current_string_table().resource_buffer[n]) {
            case 0: n = this->decode_script_record(n + 1); break;
            case 1: n = this->decode_armour_record(n + 1); break;
            case 2: n = this->decode_potion_record(n + 1); break;
            case 4: n = this->decode_weapon_record(n + 1); break;
            case 5: n = this->decode_creature_record(n + 1); break;
            case 6: n = this->decode_world_item_record(n + 1); break;
            case 7: n = this->decode_merchant_inventory(n + 1); break;
            case 8: n = this->decode_spell_record(n + 1); break;
            case 9: n = this->decode_level_gen_record(n + 1); break;
            case 10: n = this->decode_loot_drop_record(n + 1); break;
        }
    }
    if (this->initial_load) {
        for (n = 0; n < this->script_table.length(); ++n) {
            copySharedArray(this->script_table[n], 0, this->default_script_table[n], 0, this->default_script_table[n].length());
        }
        this->initial_load = false;
    }
    n += 2;
    for (n2 = 0; n2 < this->script_entry_offsets.length(); ++n2) {
        if (this->script_entry_offsets[n2] == 0) continue;
        int32_t n4 = n2;
        this->script_entry_offsets[n4] = this->script_entry_offsets[n4] - n;
    }
    this->script_bytecode = SharedArray<int8_t>(n3 - n);
    copySharedArray(current_string_table().resource_buffer, n, this->script_bytecode, 0, this->script_bytecode.length());
    GameData::call_script(1);
}

int32_t GameData::decode_script_record(int32_t n) {
    int32_t n2 = 0;
    SharedArray<int32_t> nArray(21);
    while (current_string_table().resource_buffer[n] != 31) {
        if (current_string_table().resource_buffer[n] == 1) {
            this->string_pool[this->string_pool_count] = array_text(current_string_table().resource_buffer, n + 2, (int32_t)current_string_table().resource_buffer[n + 1]);
            nArray[current_string_table().resource_buffer[n]] = this->string_pool_count++;
            n += current_string_table().resource_buffer[n + 1] + 1;
        } else if (current_string_table().resource_buffer[n] == 7 || current_string_table().resource_buffer[n] == 14 || current_string_table().resource_buffer[n] == 15) {
            nArray[current_string_table().resource_buffer[n]] = ((int32_t)(uint16_t)current_string_table().resource_buffer[n + 1] & 0xFF) << 8 | ((int32_t)(uint16_t)current_string_table().resource_buffer[n + 2] & 0xFF) << 0;
            n += 2;
        } else {
            if (current_string_table().resource_buffer[n] == 0) {
                n2 = current_string_table().resource_buffer[n + 1];
            }
            int32_t slot = current_string_table().resource_buffer[n++];
            nArray[slot] = (int32_t)(uint16_t)(current_string_table().resource_buffer[n] & 0xFF);
        }
        ++n;
    }
    copySharedArray(nArray, 0, this->script_table[n2], 0, nArray.length());
    return ++n;
}

int32_t GameData::decode_armour_record(int32_t n) {
    int32_t n2 = 0;
    SharedArray<int32_t> nArray(10);
    while (current_string_table().resource_buffer[n] != 31) {
        if (current_string_table().resource_buffer[n] == 1) {
            int8_t by = current_string_table().resource_buffer[n + 1];
            if ((by & 0xF0) == 240) {
                nArray[current_string_table().resource_buffer[n]] = ((int32_t)(uint16_t)current_string_table().resource_buffer[n + 1] & 0xFF) << 8 | ((int32_t)(uint16_t)current_string_table().resource_buffer[n + 2] & 0xFF) << 0;
                n += 2;
            } else {
                this->string_pool[this->string_pool_count] = array_text(current_string_table().resource_buffer, n + 2, (int32_t)current_string_table().resource_buffer[n + 1]);
                nArray[current_string_table().resource_buffer[n]] = this->string_pool_count++;
                n += current_string_table().resource_buffer[n + 1] + 1;
            }
        } else if (current_string_table().resource_buffer[n] == 5) {
            nArray[current_string_table().resource_buffer[n]] = ((int32_t)(uint16_t)current_string_table().resource_buffer[n + 1] & 0xFF) << 16 | ((int32_t)(uint16_t)current_string_table().resource_buffer[n + 2] & 0xFF) << 8 | ((int32_t)(uint16_t)current_string_table().resource_buffer[n + 3] & 0xFF) << 0;
            n += 3;
        } else if (current_string_table().resource_buffer[n] == 9) {
            nArray[current_string_table().resource_buffer[n]] = ((int32_t)(uint16_t)current_string_table().resource_buffer[n + 1] & 0xFF) << 8 | ((int32_t)(uint16_t)current_string_table().resource_buffer[n + 2] & 0xFF) << 0;
            n += 2;
        } else if (current_string_table().resource_buffer[n] == 6) {
            nArray[current_string_table().resource_buffer[n]] = 1;
        } else {
            if (current_string_table().resource_buffer[n] == 0) {
                n2 = current_string_table().resource_buffer[n + 1];
            }
            int32_t slot = current_string_table().resource_buffer[n++];
            nArray[slot] = current_string_table().resource_buffer[n];
        }
        ++n;
    }
    copySharedArray(nArray, 0, this->armour_table[n2], 0, nArray.length());
    return ++n;
}

int32_t GameData::decode_potion_record(int32_t n) {
    int32_t n2 = 0;
    SharedArray<int32_t> nArray(14);
    while (current_string_table().resource_buffer[n] != 31) {
        if (current_string_table().resource_buffer[n] == 1) {
            int8_t by = current_string_table().resource_buffer[n + 1];
            if ((by & 0xF0) == 240) {
                nArray[current_string_table().resource_buffer[n]] = ((int32_t)(uint16_t)current_string_table().resource_buffer[n + 1] & 0xFF) << 8 | ((int32_t)(uint16_t)current_string_table().resource_buffer[n + 2] & 0xFF) << 0;
                n += 2;
            } else {
                this->string_pool[this->string_pool_count] = array_text(current_string_table().resource_buffer, n + 2, (int32_t)current_string_table().resource_buffer[n + 1]);
                nArray[current_string_table().resource_buffer[n]] = this->string_pool_count++;
                n += current_string_table().resource_buffer[n + 1] + 1;
            }
        } else if (current_string_table().resource_buffer[n] == 5) {
            nArray[current_string_table().resource_buffer[n]] = ((int32_t)(uint16_t)current_string_table().resource_buffer[n + 1] & 0xFF) << 16 | ((int32_t)(uint16_t)current_string_table().resource_buffer[n + 2] & 0xFF) << 8 | ((int32_t)(uint16_t)current_string_table().resource_buffer[n + 3] & 0xFF) << 0;
            n += 3;
        } else if (current_string_table().resource_buffer[n] == 13) {
            nArray[current_string_table().resource_buffer[n]] = ((int32_t)(uint16_t)current_string_table().resource_buffer[n + 1] & 0xFF) << 8 | ((int32_t)(uint16_t)current_string_table().resource_buffer[n + 2] & 0xFF) << 0;
            n += 2;
        } else if (current_string_table().resource_buffer[n] == 4) {
            nArray[current_string_table().resource_buffer[n]] = 1;
        } else {
            if (current_string_table().resource_buffer[n] == 0) {
                n2 = current_string_table().resource_buffer[n + 1];
            }
            int32_t slot = current_string_table().resource_buffer[n++];
            nArray[slot] = (int32_t)(uint16_t)current_string_table().resource_buffer[n] & 0xFF;
        }
        ++n;
    }
    copySharedArray(nArray, 0, this->potion_table[n2], 0, nArray.length());
    return ++n;
}

int32_t GameData::decode_weapon_record(int32_t n) {
    int32_t n2 = 0;
    SharedArray<int32_t> nArray(8);
    while (current_string_table().resource_buffer[n] != 31) {
        if (current_string_table().resource_buffer[n] == 1) {
            int8_t by = current_string_table().resource_buffer[n + 1];
            if ((by & 0xF0) == 240) {
                nArray[current_string_table().resource_buffer[n]] = ((int32_t)(uint16_t)current_string_table().resource_buffer[n + 1] & 0xFF) << 8 | ((int32_t)(uint16_t)current_string_table().resource_buffer[n + 2] & 0xFF) << 0;
                n += 2;
            } else {
                this->string_pool[this->string_pool_count] = array_text(current_string_table().resource_buffer, n + 2, (int32_t)current_string_table().resource_buffer[n + 1]);
                nArray[current_string_table().resource_buffer[n]] = this->string_pool_count++;
                n += current_string_table().resource_buffer[n + 1] + 1;
            }
        } else if (current_string_table().resource_buffer[n] == 7) {
            nArray[current_string_table().resource_buffer[n]] = ((int32_t)(uint16_t)current_string_table().resource_buffer[n + 1] & 0xFF) << 8 | ((int32_t)(uint16_t)current_string_table().resource_buffer[n + 2] & 0xFF) << 0;
            n += 2;
        } else {
            if (current_string_table().resource_buffer[n] == 0) {
                n2 = current_string_table().resource_buffer[n + 1];
            }
            int32_t slot = current_string_table().resource_buffer[n++];
            nArray[slot] = current_string_table().resource_buffer[n];
        }
        ++n;
    }
    copySharedArray(nArray, 0, this->weapon_table[n2], 0, nArray.length());
    return ++n;
}

int32_t GameData::decode_creature_record(int32_t n) {
    int32_t n2 = 0;
    SharedArray<int32_t> nArray(15);
    SharedArray<int32_t> nArray2(15);
    SharedArray<int32_t> nArray3(15);
    int32_t n3 = 0;
    int32_t n4 = 0;
    while (current_string_table().resource_buffer[n] != 31) {
        if (current_string_table().resource_buffer[n] == 1) {
            int8_t by = current_string_table().resource_buffer[n + 1];
            if ((by & 0xF0) == 240) {
                nArray[current_string_table().resource_buffer[n]] = ((int32_t)(uint16_t)current_string_table().resource_buffer[n + 1] & 0xFF) << 8 | ((int32_t)(uint16_t)current_string_table().resource_buffer[n + 2] & 0xFF) << 0;
                n += 2;
            } else {
                this->string_pool[this->string_pool_count] = array_text(current_string_table().resource_buffer, n + 2, (int32_t)current_string_table().resource_buffer[n + 1]);
                nArray[current_string_table().resource_buffer[n]] = this->string_pool_count++;
                n += current_string_table().resource_buffer[n + 1] + 1;
            }
        } else if (current_string_table().resource_buffer[n] == 6 || current_string_table().resource_buffer[n] == 13 || current_string_table().resource_buffer[n] == 14) {
            nArray[current_string_table().resource_buffer[n]] = ((int32_t)(uint16_t)current_string_table().resource_buffer[n + 1] & 0xFF) << 8 | ((int32_t)(uint16_t)current_string_table().resource_buffer[n + 2] & 0xFF) << 0;
            n += 2;
        } else if (current_string_table().resource_buffer[n] == 2) {
            nArray2[n3++] = current_string_table().resource_buffer[++n];
        } else if (current_string_table().resource_buffer[n] == 3) {
            nArray3[n4++] = current_string_table().resource_buffer[++n];
        } else {
            if (current_string_table().resource_buffer[n] == 0) {
                n2 = current_string_table().resource_buffer[n + 1];
            }
            int32_t slot = current_string_table().resource_buffer[n++];
            nArray[slot] = current_string_table().resource_buffer[n];
        }
        ++n;
    }
    nArray2[n3] = -1;
    nArray3[n4] = -1;
    copySharedArray(nArray, 0, this->creature_table[n2], 0, nArray.length());
    copySharedArray(nArray3, 0, this->class_spells_table[n2], 0, nArray3.length());
    copySharedArray(nArray2, 0, this->class_skills_table[n2], 0, nArray2.length());
    return ++n;
}

int32_t GameData::decode_world_item_record(int32_t n) {
    int32_t n2 = 0;
    SharedArray<int32_t> nArray(7);
    while (current_string_table().resource_buffer[n] != 31) {
        if (current_string_table().resource_buffer[n] == 2) {
            nArray[current_string_table().resource_buffer[n]] = ((int32_t)(uint16_t)current_string_table().resource_buffer[n + 1] & 0xFF) << 8 | ((int32_t)(uint16_t)current_string_table().resource_buffer[n + 2] & 0xFF) << 0;
            n += 2;
        } else {
            if (current_string_table().resource_buffer[n] == 0) {
                n2 = current_string_table().resource_buffer[n + 1];
            }
            int32_t slot = current_string_table().resource_buffer[n++];
            nArray[slot] = (int32_t)(uint16_t)current_string_table().resource_buffer[n] & 0xFF;
        }
        ++n;
    }
    copySharedArray(nArray, 0, this->world_item_table[n2], 0, nArray.length());
    return ++n;
}

int32_t GameData::decode_merchant_inventory(int32_t n) {
    int32_t n2 = 0;
    while (current_string_table().resource_buffer[n] != 31) {
        this->merchant_inventory[n2++] = current_string_table().resource_buffer[n++];
        this->merchant_inventory[n2++] = current_string_table().resource_buffer[n++];
    }
    this->merchant_inventory[n2] = -1;
    return ++n;
}

int32_t GameData::decode_spell_record(int32_t n) {
    int32_t n2 = 0;
    SharedArray<int32_t> nArray(15);
    while (current_string_table().resource_buffer[n] != 31) {
        if (current_string_table().resource_buffer[n] == 1) {
            int8_t by = current_string_table().resource_buffer[n + 1];
            if ((by & 0xF0) == 240) {
                nArray[current_string_table().resource_buffer[n]] = ((int32_t)(uint16_t)current_string_table().resource_buffer[n + 1] & 0xFF) << 8 | ((int32_t)(uint16_t)current_string_table().resource_buffer[n + 2] & 0xFF) << 0;
                n += 2;
            } else {
                this->string_pool[this->string_pool_count] = array_text(current_string_table().resource_buffer, n + 2, (int32_t)current_string_table().resource_buffer[n + 1]);
                nArray[current_string_table().resource_buffer[n]] = this->string_pool_count++;
                n += current_string_table().resource_buffer[n + 1] + 1;
            }
        } else if (current_string_table().resource_buffer[n] == 14) {
            nArray[current_string_table().resource_buffer[n]] = ((int32_t)(uint16_t)current_string_table().resource_buffer[n + 1] & 0xFF) << 8 | ((int32_t)(uint16_t)current_string_table().resource_buffer[n + 2] & 0xFF) << 0;
            n += 2;
        } else if (current_string_table().resource_buffer[n] == 6) {
            nArray[current_string_table().resource_buffer[n]] = ((int32_t)(uint16_t)current_string_table().resource_buffer[n + 1] & 0xFF) << 16 | ((int32_t)(uint16_t)current_string_table().resource_buffer[n + 2] & 0xFF) << 8 | ((int32_t)(uint16_t)current_string_table().resource_buffer[n + 3] & 0xFF) << 0;
            n += 3;
        } else {
            if (current_string_table().resource_buffer[n] == 0) {
                n2 = current_string_table().resource_buffer[n + 1];
            }
            int32_t slot = current_string_table().resource_buffer[n++];
            nArray[slot] = current_string_table().resource_buffer[n];
        }
        ++n;
    }
    copySharedArray(nArray, 0, this->spell_table[n2], 0, nArray.length());
    return ++n;
}

int32_t GameData::decode_level_gen_record(int32_t n) {
    int32_t n2 = 0;
    SharedArray<int32_t> nArray(21);
    while (current_string_table().resource_buffer[n] != 31) {
        if (current_string_table().resource_buffer[n] == 1 || current_string_table().resource_buffer[n] == 2) {
            nArray[current_string_table().resource_buffer[n]] = ((int32_t)(uint16_t)current_string_table().resource_buffer[n + 1] & 0xFF) << 8 | ((int32_t)(uint16_t)current_string_table().resource_buffer[n + 2] & 0xFF) << 0;
            n += 2;
        } else if (current_string_table().resource_buffer[n] == 20) {
            this->dungeon_item_drops[this->dungeon_item_drop_count++] = (int32_t)(uint16_t)current_string_table().resource_buffer[++n] & 0xFF;
        } else {
            if (current_string_table().resource_buffer[n] == 0) {
                n2 = current_string_table().resource_buffer[n + 1];
            }
            int32_t slot = current_string_table().resource_buffer[n++];
            nArray[slot] = (int32_t)(uint16_t)current_string_table().resource_buffer[n] & 0xFF;
        }
        ++n;
    }
    copySharedArray(nArray, 0, this->level_gen_table[n2], 0, nArray.length());
    return ++n;
}

int32_t GameData::decode_loot_drop_record(int32_t n) {
    int32_t n2 = 0;
    SharedArray<int32_t> nArray(4);
    while (current_string_table().resource_buffer[n] != 31) {
        if (current_string_table().resource_buffer[n] == 0) {
            n2 = current_string_table().resource_buffer[n + 1];
        }
        int32_t slot = current_string_table().resource_buffer[n++];
        nArray[slot] = (int32_t)(uint16_t)current_string_table().resource_buffer[n] & 0xFF;
        ++n;
    }
    copySharedArray(nArray, 0, this->loot_drop_table[n2], 0, nArray.length());
    return ++n;
}

int32_t GameData::read_script_byte() {
    int32_t n = (int32_t)(uint16_t)this->script_bytecode[this->frame_pc[this->call_depth - 1]] & 0xFF;
    int32_t n2 = this->call_depth - 1;
    this->frame_pc[n2] = this->frame_pc[n2] + 1;
    return n;
}

int32_t GameData::read_script_int24() {
    int32_t n = ((int32_t)(uint16_t)this->script_bytecode[this->frame_pc[this->call_depth - 1] + 0] & 0xFF) << 16 | ((int32_t)(uint16_t)this->script_bytecode[this->frame_pc[this->call_depth - 1] + 1] & 0xFF) << 8 | ((int32_t)(uint16_t)this->script_bytecode[this->frame_pc[this->call_depth - 1] + 2] & 0xFF) << 0;
    int32_t n2 = this->call_depth - 1;
    this->frame_pc[n2] = this->frame_pc[n2] + 3;
    return n;
}

int32_t GameData::read_script_int16() {
    int32_t n = ((int32_t)(uint16_t)this->script_bytecode[this->frame_pc[this->call_depth - 1] + 0] & 0xFF) << 8 | ((int32_t)(uint16_t)this->script_bytecode[this->frame_pc[this->call_depth - 1] + 1] & 0xFF) << 0;
    int32_t n2 = this->call_depth - 1;
    this->frame_pc[n2] = this->frame_pc[n2] + 2;
    return n;
}

std::string GameData::read_script_text(int32_t n) {
    std::string string = array_text(this->script_bytecode, this->frame_pc[this->call_depth - 1], n);
    int32_t n2 = this->call_depth - 1;
    this->frame_pc[n2] = this->frame_pc[n2] + n;
    return string;
}
