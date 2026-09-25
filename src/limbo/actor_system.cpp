#include "src/limbo/actor_system.hpp"

ActorSystemStatics &ActorSystem::S() { return oblivion_state().actors; }

#include "src/limbo/game_canvas.hpp"
#include "src/limbo/game_data.hpp"
#include "src/limbo/magic_effects.hpp"
#include "src/limbo/sprite_atlas.hpp"


Actor *ActorSystem::create_actor(const std::string &string, int8_t by) {
    Actor *j2 = session_new<Actor>();
    int32_t n = 0;
    j2->sprite_path = string;
    j2->actor_kind = by;
    j2->tile_position[0] = 0;
    j2->tile_position[1] = 0;
    j2->sprite_atlas = SpriteAtlas::load_atlas(string);
    for (n = 0; n < j2->equipped_armour_ids.length(); ++n) {
        j2->equipped_armour_ids[n] = -1;
    }
    j2->footprint_width = (int8_t)SpriteAtlas::frame_width(j2->sprite_atlas, 1);
    j2->footprint_half_width = (int8_t)(j2->footprint_width >> 1);
    return j2;
}

void ActorSystem::render_actor(Actor *j2, Graphics *graphics, const SharedArray<int32_t> &nArray) {
    int32_t n;
    int32_t n2;
    if (j2->is_dead == 1 || j2->anim_state == 6) {
        SpriteAtlas::draw_frame(graphics, j2->sprite_atlas, -55,
             j2->screen_position[0] + nArray[0] +
                 (SpriteAtlas::frame_width(j2->sprite_atlas, j2->facing + S().anim_frame_offsets[0]) >> 1) -
                 (SpriteAtlas::frame_width(j2->sprite_atlas, -55) >> 1),
             j2->screen_position[1] + nArray[1] - SpriteAtlas::frame_height(j2->sprite_atlas, -55) + 3);
        return;
    }
    SpriteAtlas::draw_frame(graphics, j2->sprite_atlas, -56,
         j2->screen_position[0] + nArray[0] + (SpriteAtlas::frame_width(j2->sprite_atlas, j2->facing + S().anim_frame_offsets[0]) >> 1) -
             (SpriteAtlas::frame_width(j2->sprite_atlas, -56) >> 1),
         j2->screen_position[1] + nArray[1] - SpriteAtlas::frame_height(j2->sprite_atlas, -56) + 3 +
             (j2->anim_state == 2 || j2->anim_state == 3 ? 3 : 0));
    SpriteAtlas::draw_frame(graphics, j2->sprite_atlas, j2->facing + S().anim_frame_offsets[j2->anim_state], j2->screen_position[0] + nArray[0],
         j2->screen_position[1] + nArray[1] - SpriteAtlas::frame_height(j2->sprite_atlas, j2->facing + S().anim_frame_offsets[j2->anim_state]));
    if (j2->is_dead == 0 && j2->actor_kind != 1 && j2->faction_id == 0) {
        n2 = j2->screen_position[0] + nArray[0] + (SpriteAtlas::frame_width(j2->sprite_atlas, j2->facing) >> 1) - 10;
        n = j2->screen_position[1] + nArray[1] - SpriteAtlas::frame_height(j2->sprite_atlas, j2->facing) - 6;
        graphics->setColor(0xFFFFFF);
        graphics->drawRect(n2, n, 20, 3);
        graphics->setColor(0xFF0000);
        graphics->fillRect(n2 + 1, n + 1, 19 * j2->health / j2->max_health, 2);
    }
    if (j2->is_dead == 0 && j2->combat_text.has_value()) {
        if (j2->combat_text_y == 0) {
            j2->combat_text_start_y = j2->combat_text_y = (int16_t)(j2->screen_position[1] - SpriteAtlas::frame_height(j2->sprite_atlas, j2->facing) -
                                     (j2->actor_kind == 1 ? 6 : 10));
            if ((j2->combat_text == GameCanvas::get_text(471))) {
                j2->combat_text_color = 65280;
                j2->combat_text_fade_step = 8704;
            } else if ((j2->combat_text == GameCanvas::get_text(470))) {
                j2->combat_text_color = 255;
                j2->combat_text_fade_step = 34;
            } else {
                j2->combat_text_color = 0xFF0000;
                j2->combat_text_fade_step = 0x220000;
            }
        }
        graphics->setColor(j2->combat_text_color);
        graphics->drawString(*j2->combat_text,
                             j2->screen_position[0] + nArray[0] + (SpriteAtlas::frame_width(j2->sprite_atlas, j2->facing) >> 1) - 10,
                             j2->combat_text_y + nArray[1], 0);
    }
    if (j2->status_icon_frame != -1) {
        n2 = j2->screen_position[0] + nArray[0] + SpriteAtlas::frame_width(j2->sprite_atlas, j2->facing) - 4;
        n = j2->screen_position[1] + nArray[1] - SpriteAtlas::frame_height(j2->sprite_atlas, j2->facing + S().anim_frame_offsets[j2->anim_state]) -
            SpriteAtlas::frame_width(j2->sprite_atlas, -54) - 4;
        if (j2->actor_kind != 1) {
            n -= 8;
        }
        SpriteAtlas::draw_frame(graphics, j2->sprite_atlas, -54, n2, n);
        if (j2->status_icon_frame != -2) {
            SpriteAtlas::draw_frame(graphics, j2->sprite_atlas, j2->status_icon_frame, n2, n);
        }
    }
}

void ActorSystem::set_anim_state(Actor *j2, int8_t by) {
    if (by == 6) {
        j2->is_dead = 1;
    } else if (j2->anim_state != by) {
        SpriteAtlas::reset_animation(j2->sprite_atlas, j2->facing + S().anim_frame_offsets[by]);
    }
    j2->anim_state = by;
}

void ActorSystem::recalculate_stats(Actor *j2) {
    int32_t n = 0;
    SharedArray<int32_t> nArray = GameCanvas::state().game_data->get_record(4, j2->equipped_weapon_id);
    j2->weapon_damage_bonus = (int8_t)nArray[3];
    j2->armour_rating = 0;
    for (n = 0; n < j2->equipped_armour_ids.length(); ++n) {
        if (j2->equipped_armour_ids[n] == -1) continue;
        j2->armour_rating = (int16_t)(j2->armour_rating + GameCanvas::state().game_data->get_record(1, j2->equipped_armour_ids[n])[4]);
    }
    switch (j2->character_class) {
        case 4: {
            if (j2->level == 1) {
                j2->defense_rating = (int16_t)100;
            }
            if (j2->level == 10) {
                j2->defense_rating = (int16_t)115;
            }
            if (j2->level == 20) {
                j2->defense_rating = (int16_t)130;
            }
            if (nArray[2] == 1) {
                if (j2->level == 1) {
                    j2->attack_rating = (int16_t)100;
                }
                if (j2->level == 8) {
                    j2->attack_rating = (int16_t)110;
                }
                if (j2->level != 18) break;
                j2->attack_rating = (int16_t)125;
                return;
            }
            if (nArray[2] != 4) break;
            if (j2->level == 1) {
                j2->attack_rating = (int16_t)100;
            }
            if (j2->level == 7) {
                j2->attack_rating = (int16_t)110;
            }
            if (j2->level != 16) break;
            j2->attack_rating = (int16_t)125;
            return;
        }
        case 3: {
            if (j2->level == 1) {
                j2->block_chance = 0;
            }
            if (j2->level == 5) {
                j2->block_chance = (int16_t)3;
            }
            if (j2->level == 17) {
                j2->block_chance = (int16_t)10;
            }
            if (j2->level == 1) {
                j2->defense_rating = (int16_t)100;
            }
            if (j2->level == 10) {
                j2->defense_rating = (int16_t)115;
            }
            if (j2->level == 20) {
                j2->defense_rating = (int16_t)130;
            }
            if (nArray[2] == 1) {
                if (j2->level == 1) {
                    j2->attack_rating = (int16_t)100;
                }
                if (j2->level == 8) {
                    j2->attack_rating = (int16_t)110;
                }
                if (j2->level != 18) break;
                j2->attack_rating = (int16_t)125;
                return;
            }
            if (nArray[2] == 2) {
                if (j2->level == 1) {
                    j2->attack_rating = (int16_t)100;
                }
                if (j2->level == 8) {
                    j2->attack_rating = (int16_t)110;
                }
                if (j2->level != 18) break;
                j2->attack_rating = (int16_t)125;
                return;
            }
            if (nArray[2] == 3) {
                if (j2->level == 1) {
                    j2->attack_rating = (int16_t)100;
                }
                if (j2->level == 6) {
                    j2->attack_rating = (int16_t)110;
                }
                if (j2->level != 15) break;
                j2->attack_rating = (int16_t)125;
                return;
            }
            if (j2->equipped_weapon_id == 0) {
                if (j2->level == 1) {
                    j2->attack_rating = (int16_t)100;
                }
                if (j2->level == 5) {
                    j2->attack_rating = (int16_t)110;
                }
                if (j2->level != 15) break;
                j2->attack_rating = (int16_t)125;
                return;
            }
            if (nArray[2] != 0) break;
            if (j2->level == 1) {
                j2->attack_rating = (int16_t)100;
            }
            if (j2->level == 8) {
                j2->attack_rating = (int16_t)110;
            }
            if (j2->level != 18) break;
            j2->attack_rating = (int16_t)125;
            return;
        }
        case 8: {
            if (j2->level == 1) {
                j2->defense_rating = (int16_t)100;
            }
            if (j2->level == 10) {
                j2->defense_rating = (int16_t)115;
            }
            if (j2->level == 20) {
                j2->defense_rating = (int16_t)130;
            }
            if (j2->level == 1) {
                j2->defense_rating = (int16_t)100;
            }
            if (j2->level == 7) {
                j2->defense_rating = (int16_t)115;
            }
            if (j2->level == 17) {
                j2->defense_rating = (int16_t)130;
            }
            if (nArray[2] == 1) {
                if (j2->level == 1) {
                    j2->attack_rating = (int16_t)100;
                }
                if (j2->level == 8) {
                    j2->attack_rating = (int16_t)110;
                }
                if (j2->level != 18) break;
                j2->attack_rating = (int16_t)125;
                return;
            }
            if (nArray[2] == 2) {
                if (j2->level == 1) {
                    j2->attack_rating = (int16_t)100;
                }
                if (j2->level == 8) {
                    j2->attack_rating = (int16_t)110;
                }
                if (j2->level != 18) break;
                j2->attack_rating = (int16_t)125;
                return;
            }
            if (nArray[2] != 0) break;
            if (j2->level == 1) {
                j2->attack_rating = (int16_t)100;
            }
            if (j2->level == 8) {
                j2->attack_rating = (int16_t)110;
            }
            if (j2->level != 18) break;
            j2->attack_rating = (int16_t)125;
            return;
        }
        case 5: {
            if (j2->level == 1) {
                j2->block_chance = 0;
            }
            if (j2->level == 5) {
                j2->block_chance = (int16_t)3;
            }
            if (j2->level == 17) {
                j2->block_chance = (int16_t)10;
            }
            if (j2->level == 1) {
                j2->defense_rating = (int16_t)100;
            }
            if (j2->level == 10) {
                j2->defense_rating = (int16_t)115;
            }
            if (j2->level == 20) {
                j2->defense_rating = (int16_t)130;
            }
            if (j2->level == 1) {
                j2->defense_rating = (int16_t)100;
            }
            if (j2->level == 7) {
                j2->defense_rating = (int16_t)115;
            }
            if (j2->level == 17) {
                j2->defense_rating = (int16_t)130;
            }
            if (nArray[2] == 1) {
                if (j2->level == 1) {
                    j2->attack_rating = (int16_t)100;
                }
                if (j2->level == 8) {
                    j2->attack_rating = (int16_t)110;
                }
                if (j2->level != 18) break;
                j2->attack_rating = (int16_t)125;
                return;
            }
            if (nArray[2] == 2) {
                if (j2->level == 1) {
                    j2->attack_rating = (int16_t)100;
                }
                if (j2->level == 8) {
                    j2->attack_rating = (int16_t)110;
                }
                if (j2->level != 18) break;
                j2->attack_rating = (int16_t)125;
                return;
            }
            if (nArray[2] != 0) break;
            if (j2->level == 1) {
                j2->attack_rating = (int16_t)100;
            }
            if (j2->level == 8) {
                j2->attack_rating = (int16_t)110;
            }
            if (j2->level != 18) break;
            j2->attack_rating = (int16_t)125;
            return;
        }
        case 1: {
            if (j2->level == 1) {
                j2->dodge_chance = (int16_t)5;
            }
            if (j2->level == 7) {
                j2->dodge_chance = (int16_t)10;
            }
            if (j2->level == 15) {
                j2->dodge_chance = (int16_t)15;
            }
            if (nArray[2] == 4) {
                if (j2->level == 1) {
                    j2->attack_rating = (int16_t)100;
                }
                if (j2->level == 7) {
                    j2->attack_rating = (int16_t)110;
                }
                if (j2->level != 16) break;
                j2->attack_rating = (int16_t)125;
                return;
            }
            if (j2->equipped_weapon_id != 0) break;
            if (j2->level == 1) {
                j2->attack_rating = (int16_t)100;
            }
            if (j2->level == 5) {
                j2->attack_rating = (int16_t)110;
            }
            if (j2->level == 15) {
                j2->attack_rating = (int16_t)125;
            }
            if (j2->level == 1) {
                j2->defense_rating = (int16_t)110;
            }
            if (j2->level == 5) {
                j2->defense_rating = (int16_t)125;
            }
            if (j2->level != 15) break;
            j2->defense_rating = (int16_t)140;
            return;
        }
        case 2: {
            if (j2->level == 1) {
                j2->dodge_chance = (int16_t)5;
            }
            if (j2->level == 7) {
                j2->dodge_chance = (int16_t)10;
            }
            if (j2->level == 15) {
                j2->dodge_chance = (int16_t)15;
            }
            if (j2->level == 1) {
                j2->defense_rating = (int16_t)100;
            }
            if (j2->level == 10) {
                j2->defense_rating = (int16_t)115;
            }
            if (j2->level == 20) {
                j2->defense_rating = (int16_t)130;
            }
            if (nArray[2] == 2) {
                if (j2->level == 1) {
                    j2->attack_rating = (int16_t)100;
                }
                if (j2->level == 8) {
                    j2->attack_rating = (int16_t)110;
                }
                if (j2->level != 18) break;
                j2->attack_rating = (int16_t)125;
                return;
            }
            if (nArray[2] != 3) break;
            if (j2->level == 1) {
                j2->attack_rating = (int16_t)100;
            }
            if (j2->level == 6) {
                j2->attack_rating = (int16_t)110;
            }
            if (j2->level != 15) break;
            j2->attack_rating = (int16_t)125;
            return;
        }
        case 7: {
            if (j2->level == 1) {
                j2->dodge_chance = (int16_t)5;
            }
            if (j2->level == 7) {
                j2->dodge_chance = (int16_t)10;
            }
            if (j2->level == 15) {
                j2->dodge_chance = (int16_t)15;
            }
            if (j2->level == 1) {
                j2->defense_rating = (int16_t)100;
            }
            if (j2->level == 10) {
                j2->defense_rating = (int16_t)115;
            }
            if (j2->level != 20) break;
            j2->defense_rating = (int16_t)130;
            return;
        }
        case 6: {
            if (j2->level == 1) {
                j2->block_chance = 0;
            }
            if (j2->level == 5) {
                j2->block_chance = (int16_t)3;
            }
            if (j2->level == 17) {
                j2->block_chance = (int16_t)10;
            }
            if (j2->level == 1) {
                j2->defense_rating = (int16_t)100;
            }
            if (j2->level == 10) {
                j2->defense_rating = (int16_t)115;
            }
            if (j2->level == 20) {
                j2->defense_rating = (int16_t)130;
            }
            if (nArray[2] == 2) {
                if (j2->level == 1) {
                    j2->attack_rating = (int16_t)100;
                }
                if (j2->level == 8) {
                    j2->attack_rating = (int16_t)110;
                }
                if (j2->level != 18) break;
                j2->attack_rating = (int16_t)125;
                return;
            }
            if (nArray[2] != 4) break;
            if (j2->level == 1) {
                j2->attack_rating = (int16_t)100;
            }
            if (j2->level == 7) {
                j2->attack_rating = (int16_t)110;
            }
            if (j2->level != 16) break;
            j2->attack_rating = (int16_t)125;
        }
    }
}

void ActorSystem::set_attribute(Actor *j2, int32_t n, int32_t n2, GameData *e2) {
    switch (n) {
        case 2: {
            j2->level = (int8_t)n2;
            break;
        }
        case 3: {
            j2->strength = (int16_t)n2;
            break;
        }
        case 4: {
            j2->intelligence = (int16_t)n2;
            break;
        }
        case 5: {
            j2->willpower = (int16_t)n2;
            break;
        }
        case 6: {
            j2->agility = (int16_t)n2;
            break;
        }
        case 8: {
            j2->endurance = (int16_t)n2;
            break;
        }
        case 9: {
            j2->personality = (int16_t)n2;
            break;
        }
        case 10: {
            j2->equipped_weapon_id = (int8_t)n2;
            break;
        }
        case 11: {
            break;
        }
        case 12: {
            break;
        }
        case 13: {
            j2->faction_id = (int8_t)n2;
            break;
        }
        case 7: {
            j2->move_speed = (int16_t)n2;
            break;
        }
        case 14: {
            j2->engage_radius = (int16_t)n2;
            break;
        }
        case 15: {
            j2->disengage_radius = (int16_t)n2;
            break;
        }
        case 19: {
            j2->active_hand_record = GameCanvas::state().game_data->spell_table[n2];
            break;
        }
        case 18: {
            j2->ai_behavior = (int8_t)n2;
            j2->bow_equipped = (int8_t)(j2->ai_behavior == 4 ? 1 : 0);
            if (j2->bow_equipped != 1 && j2->ai_behavior != 0) break;
            j2->active_hand_record.setNull();
            break;
        }
        case 20: {
            j2->attack_interval_ms = (int16_t)(n2 * 1000);
        }
    }
    j2->max_health = (int16_t)(j2->level * 4 + (j2->strength + j2->strength_bonus) * 2 + j2->endurance * 2 + j2->health_capacity_bonus);
    j2->health = (int16_t)min32(j2->health, j2->max_health);
    j2->health_regen_interval_ms = (int16_t)(40000 / j2->max_health);
    j2->max_magicka = (int16_t)(j2->level * 4 + j2->intelligence * 2 + j2->magicka_capacity_bonus);
    j2->magicka = (int16_t)min32(j2->magicka, j2->max_magicka);
    j2->magicka_regen_interval_ms = (int16_t)(40000 / j2->max_magicka);
    if (j2->equipped_weapon_id > 0) {
        j2->weapon_damage_bonus = (int8_t)e2->get_record(4, j2->equipped_weapon_id)[3];
    }
    if (j2->engage_radius == 0) {
        j2->engage_radius = S().var_short_a;
    }
    if (j2->disengage_radius == 0) {
        j2->disengage_radius = S().var_short_b;
    }
    ActorSystem::recalculate_stats(j2);
}

void ActorSystem::add_inventory_item(Actor *j2, int32_t n, const SharedArray<int32_t> &nArray) { ActorSystem::add_inventory_item(j2, n, nArray, false); }

void ActorSystem::add_inventory_item(Actor *j2, int32_t n, const SharedArray<int32_t> &nArray, bool bl) {
    int32_t n2;
    for (n2 = 0; n2 < j2->inventory_entries.length() && j2->inventory_entries[n2] != 0; ++n2) {
    }
    if (n2 >= j2->inventory_entries.length()) {
        return;
    }
    switch (n) {
        case 1: {
            if (j2->equipped_armour_ids[nArray[3]] == -1 || bl) {
                ActorSystem::equip_armour(j2, nArray);
            }
            j2->inventory_entries[n2] = 0x100 | nArray[0];
            return;
        }
        case 2: {
            j2->inventory_entries[n2] = 0x200 | nArray[0];
            if (nArray[5] != 0) break;
            if (j2->quick_health_potion_record.isNull() && nArray[2] > 0) {
                j2->quick_health_potion_record = nArray;
                return;
            }
            if (!j2->quick_magicka_potion_record.isNull() || nArray[3] <= 0) break;
            j2->quick_magicka_potion_record = nArray;
            return;
        }
        case 0: {
            j2->inventory_entries[n2] = 0 | nArray[0];
            if ((j2->equipped_weapon_id != 0 || !j2->active_hand_record.isNull() || !ActorSystem::can_use_item(j2, 0, nArray)) && !bl) break;
            j2->equipped_weapon_id = (int8_t)nArray[0];
        }
    }
}

void ActorSystem::equip_best_weapon(Actor *j2) {
    int32_t n = 0;
    SharedArray<int32_t> nArray;
    SharedArray<int32_t> nArray2;
    for (n = 0; n < j2->inventory_entries.length() && j2->inventory_entries[n] != 0; ++n) {
        if (j2->inventory_entries[n] > 255 ||
            !ActorSystem::can_use_item(j2, 0, nArray2 = GameCanvas::state().game_data->get_record(4, j2->inventory_entries[n] & 0xFF)) ||
            !nArray.isNull() && nArray2[3] <= nArray[3])
            continue;
        nArray = nArray2;
    }
    if (!nArray.isNull()) {
        j2->equipped_weapon_id = (int8_t)nArray[0];
        j2->bow_equipped = (int8_t)(nArray[2] == 4 ? 1 : 0);
    }
}

void ActorSystem::remove_inventory_item(Actor *j2, int32_t n, const SharedArray<int32_t> &nArray) {
    int32_t n2 = 0;
    int32_t n3 = 0;
    int32_t n4 = 0;
    bool bl = false;
    switch (n) {
        case 1: {
            n4 = 0x100 | nArray[0];
            break;
        }
        case 2: {
            n4 = 0x200 | nArray[0];
            break;
        }
        case 0: {
            n4 = 0 | nArray[0];
            if (j2->equipped_weapon_id != nArray[0]) break;
            j2->equipped_weapon_id = 0;
            j2->bow_equipped = 0;
            bl = true;
        }
    }
    while (n2 < j2->inventory_entries.length() && j2->inventory_entries[n2] != 0) {
        if (j2->inventory_entries[n2] == n4) {
            for (n3 = n2; n3 < j2->inventory_entries.length() - 1; ++n3) {
                j2->inventory_entries[n3] = j2->inventory_entries[n3 + 1];
            }
            break;
        }
        ++n2;
    }
    if (bl) {
        ActorSystem::equip_best_weapon(j2);
    }
    ActorSystem::recalculate_stats(j2);
}

int32_t ActorSystem::frame_height(Actor *j2) {
    if (j2->anim_state == 6) {
        return 0;
    }
    return SpriteAtlas::frame_height(j2->sprite_atlas, j2->facing + S().anim_frame_offsets[j2->anim_state]);
}

int32_t ActorSystem::frame_width(Actor *j2) {
    if (j2->anim_state == 6) {
        return 0;
    }
    return SpriteAtlas::frame_width(j2->sprite_atlas, j2->facing + S().anim_frame_offsets[j2->anim_state]);
}

void ActorSystem::apply_class(Actor *j2, int8_t by, bool bl) {
    j2->character_class = by;
    if (j2->character_class == 4) {
        j2->bow_equipped = 1;
    }
    j2->class_record = GameCanvas::state().game_data->get_record(5, by);
    j2->available_spell_ids = GameCanvas::state().game_data->class_spells_table[by];
    if (!bl) {
        ActorSystem::add_inventory_item(j2, 0, GameCanvas::state().game_data->get_record(4, j2->class_record[4]));
        ActorSystem::add_inventory_item(j2, 1, GameCanvas::state().game_data->get_record(1, j2->class_record[5]));
        j2->strength = (int16_t)GameCanvas::state().game_data->creature_table[by][7];
        j2->intelligence = (int16_t)GameCanvas::state().game_data->creature_table[by][8];
        j2->willpower = (int16_t)GameCanvas::state().game_data->creature_table[by][9];
        j2->agility = (int16_t)GameCanvas::state().game_data->creature_table[by][10];
        j2->move_speed = (int16_t)GameCanvas::state().game_data->creature_table[by][6];
        j2->endurance = (int16_t)GameCanvas::state().game_data->creature_table[by][11];
        j2->personality = (int16_t)GameCanvas::state().game_data->creature_table[by][12];
        j2->disengage_radius = (int16_t)GameCanvas::state().game_data->creature_table[by][13];
        j2->engage_radius = (int16_t)GameCanvas::state().game_data->creature_table[by][14];
    }
    ActorSystem::recalculate_stats(j2);
}

void ActorSystem::set_status_icon(Actor *j2, int8_t by) {
    switch (by) {
        case 2: {
            j2->status_icon_frame = (int8_t)-52;
            return;
        }
        case 1: {
            j2->status_icon_frame = (int8_t)-53;
            return;
        }
        case 3: {
            j2->status_icon_frame = (int8_t)-51;
            return;
        }
        case 4: {
            j2->status_icon_frame = (int8_t)-2;
            return;
        }
        case 0: {
            j2->status_icon_frame = (int8_t)-1;
        }
    }
}

void ActorSystem::use_potion(Actor *j2, const SharedArray<int32_t> &nArray) {
    if (nArray[5] == 0) {
        if ((GameCanvas::state().game_data->get_string(nArray[1]) == GameCanvas::get_text(158))) {
            j2->health = (int16_t)min32(j2->max_health, j2->health + nArray[2]);
            j2->magicka = (int16_t)min32(j2->max_magicka, j2->magicka + nArray[3]);
            ActorSystem::remove_inventory_item(j2, 2, nArray);
            ActorSystem::recalculate_stats(j2);
        }
        if (nArray[2] > 0) {
            j2->quick_health_potion_record = nArray;
            return;
        }
        if (nArray[3] > 0) {
            j2->quick_magicka_potion_record = nArray;
            return;
        }
        if (nArray[4] > 0) {
            j2->damage_over_time_remaining_ms = 0;
            j2->damage_over_time_tick_remaining_ms = 0;
            j2->offhand_icon_frame = (int8_t)-1;
            ActorSystem::remove_inventory_item(j2, 2, nArray);
            ActorSystem::recalculate_stats(j2);
            return;
        }
    } else {
        if (nArray[3] > 0) {
            j2->magicka_capacity_bonus = (int16_t)nArray[3];
            j2->magicka = j2->max_magicka = (int16_t)(j2->level * 4 + j2->intelligence * 2 + j2->magicka_capacity_bonus);
            j2->magicka_regen_interval_ms = (int16_t)(40000 / j2->max_magicka);
        }
        if (nArray[6] > 0) {
            j2->attack_bonus = (int16_t)nArray[6];
        }
        if (nArray[7] > 0) {
            j2->armour_bonus = (int16_t)nArray[7];
        }
        if (nArray[8] > 0) {
            j2->defense_bonus = (int16_t)nArray[8];
        }
        if (nArray[10] > 0) {
            j2->damage_bonus = (int16_t)nArray[10];
        }
        if (nArray[11] > 0) {
            j2->strength_bonus = (int16_t)nArray[11];
            j2->max_health =
                (int16_t)(j2->level * 4 + (j2->strength + j2->strength_bonus) * 2 + j2->endurance * 2 + j2->health_capacity_bonus);
            j2->health_regen_interval_ms = (int16_t)(40000 / j2->max_health);
        }
        if (nArray[4] > 0) {
            j2->damage_over_time_remaining_ms = 0;
            j2->damage_over_time_tick_remaining_ms = 0;
            j2->offhand_icon_frame = (int8_t)-1;
        }
        j2->potion_buff_elapsed_ms = 0;
        j2->potion_buff_duration_ms = (int16_t)nArray[5];
        ActorSystem::remove_inventory_item(j2, 2, nArray);
        ActorSystem::recalculate_stats(j2);
    }
}

void ActorSystem::use_quick_potion(Actor *j2, bool bl) {
    {
        int32_t n;
        {
            n = 0;
            if (!bl) goto end_block3;
            if (j2->quick_health_potion_record.isNull() || j2->health >= j2->max_health) goto end_block4;
            ActorSystem::remove_inventory_item(j2, 2, j2->quick_health_potion_record);
            j2->health = (int16_t)min32(j2->max_health, j2->health + j2->quick_health_potion_record[2]);
            ActorSystem::recalculate_stats(j2);
            j2->quick_health_potion_record.setNull();
            for (n = 0; n < j2->inventory_entries.length(); ++n) {
                SharedArray<int32_t> nArray;
                int32_t n2 = (j2->inventory_entries[n] >> 8) & 0xFF;
                int32_t n3 = (j2->inventory_entries[n] >> 0) & 0xFF;
                if (n2 != 2 || (nArray = GameCanvas::state().game_data->get_record(2, n3))[2] <= 0) continue;
                j2->quick_health_potion_record = nArray;
            }
            goto end_block4;
        }
    end_block3:
        if (!j2->quick_magicka_potion_record.isNull() && j2->magicka < j2->max_magicka) {
            ActorSystem::remove_inventory_item(j2, 2, j2->quick_magicka_potion_record);
            j2->magicka = (int16_t)min32(j2->max_magicka, j2->magicka + j2->quick_magicka_potion_record[3]);
            ActorSystem::recalculate_stats(j2);
            j2->quick_magicka_potion_record.setNull();
            for (n = 0; n < j2->inventory_entries.length(); ++n) {
                SharedArray<int32_t> nArray;
                int32_t n4 = (j2->inventory_entries[n] >> 8) & 0xFF;
                int32_t n5 = (j2->inventory_entries[n] >> 0) & 0xFF;
                if (n4 != 2 || (nArray = GameCanvas::state().game_data->get_record(2, n5))[3] <= 0) continue;
                j2->quick_magicka_potion_record = nArray;
            }
        }
    }
end_block4:;
}

bool ActorSystem::equip_by_name(Actor *j2, const std::string &string) {
    std::string text = string;
    if (voyage::text::starts_with(text, GameCanvas::get_text(304))) {
        text = voyage::text::slice(text, voyage::text::length(GameCanvas::get_text(304)));
        j2->stowed_hand_record = GameCanvas::state().game_data->find_record(text);
        j2->bow_equipped = 0;
        if (!j2->active_hand_record.isNull()) {
            j2->active_hand_record = j2->stowed_hand_record;
            ActorSystem::update_hand_icon(j2);
        }
        return true;
    }
    if (voyage::text::starts_with(text, GameCanvas::get_text(400))) {
        text = voyage::text::slice(text, voyage::text::length(GameCanvas::get_text(400)));
        j2->bow_equipped = 1;
    } else {
        text = voyage::text::slice(text, voyage::text::length(GameCanvas::get_text(305)));
        j2->bow_equipped = 0;
    }
    SharedArray<int32_t> nArray = GameCanvas::state().game_data->find_record(text);
    j2->equipped_weapon_id = (int8_t)nArray[0];
    return false;
}

void ActorSystem::apply_damage_over_time(Actor *j2, Actor *j3, int32_t n, int32_t n2) {
    j3->damage_over_time_amount = (int8_t)n;
    j3->damage_over_time_remaining_ms = (int16_t)n2;
    j3->damage_over_time_source = j2;
    j3->offhand_icon_frame = (int8_t)-47;
    MagicEffects::spawn_attached(8, j3);
    ActorSystem::apply_damage(n, j3, j2, false, true);
}

void ActorSystem::apply_spell_damage(Actor *j2, Actor *j3, int32_t n) {
    MagicEffects::spawn_attached(10, j3);
    ActorSystem::apply_damage(n, j3, j2, false, false);
}

void ActorSystem::equip_armour(Actor *j2, const SharedArray<int32_t> &nArray) {
    if (nArray.isNull() || !ActorSystem::can_use_item(j2, 1, nArray)) {
        return;
    }
    j2->equipped_armour_ids[nArray[3]] = nArray[0];
    ActorSystem::recalculate_stats(j2);
}

bool ActorSystem::is_armour_equipped(Actor *j2, int32_t n) {
    int32_t n2 = 0;
    for (n2 = 0; n2 < j2->equipped_armour_ids.length(); ++n2) {
        if (j2->equipped_armour_ids[n2] != n) continue;
        return true;
    }
    return false;
}

bool ActorSystem::is_weapon_or_spell_equipped(Actor *j2, int32_t n, bool bl) { return ActorSystem::is_weapon_or_spell_equipped(j2, GameCanvas::state().game_data->get_record(4, n), bl); }

bool ActorSystem::is_weapon_or_spell_equipped(Actor *j2, const SharedArray<int32_t> &nArray, bool bl) {
    if (!j2->stowed_hand_record.isNull() && bl) {
        return nArray[0] == j2->stowed_hand_record[0];
    }
    if (!bl) {
        return j2->equipped_weapon_id == nArray[0];
    }
    return false;
}

void ActorSystem::set_loot_drop_enabled(Actor *j2, bool bl) { j2->loot_drop_enabled = (int8_t)(bl ? 1 : 0); }

void ActorSystem::level_up_to(Actor *j2, int32_t n) {
    while (j2->level < n) {
        j2->level = (int8_t)(j2->level + 1);
        j2->strength = (int16_t)(j2->strength + 1);
        j2->intelligence = (int16_t)(j2->intelligence + 1);
        j2->willpower = (int16_t)(j2->willpower + 1);
        j2->agility = (int16_t)(j2->agility + 1);
        j2->move_speed = (int16_t)(j2->move_speed + 1);
        j2->endurance = (int16_t)(j2->endurance + 1);
        j2->personality = (int16_t)(j2->personality + 1);
        ActorSystem::apply_class_level_bonuses(j2);
        j2->max_health = (int16_t)(j2->level * 4 + (j2->strength + j2->strength_bonus) * 2 + j2->endurance * 2 + j2->health_capacity_bonus);
        j2->health_regen_interval_ms = (int16_t)(40000 / j2->max_health);
        j2->max_magicka = (int16_t)(j2->level * 4 + j2->intelligence * 2 + j2->magicka_capacity_bonus);
        j2->magicka_regen_interval_ms = (int16_t)(40000 / j2->max_magicka);
        ActorSystem::recalculate_stats(j2);
    }
}

bool ActorSystem::handle_input(Actor *actor, int32_t action, int64_t elapsed_ms, bool screen_aligned) {
    if (screen_aligned) {
        switch (action) {
            case ACTION_UP:
                action = ACTION_UP_LEFT;
                break;
            case ACTION_DOWN:
                action = ACTION_DOWN_RIGHT;
                break;
            case ACTION_LEFT:
                action = ACTION_DOWN_LEFT;
                break;
            case ACTION_RIGHT:
                action = ACTION_UP_RIGHT;
                break;
            case ACTION_UP_LEFT:
                action = ACTION_LEFT;
                break;
            case ACTION_UP_RIGHT:
                action = ACTION_UP;
                break;
            case ACTION_DOWN_LEFT:
                action = ACTION_DOWN;
                break;
            case ACTION_DOWN_RIGHT:
                action = ACTION_RIGHT;
                break;
            default:
                break;
        }
    }

    switch (action) {
        case ACTION_UP: {
            ActorSystem::set_anim_state(actor, ANIM_STATE_WALK);
            ActorSystem::move_direction(actor, DIR_UP_RIGHT, elapsed_ms);
            break;
        }
        case ACTION_DOWN: {
            ActorSystem::set_anim_state(actor, ANIM_STATE_WALK);
            ActorSystem::move_direction(actor, DIR_DOWN_LEFT, elapsed_ms);
            break;
        }
        case ACTION_LEFT: {
            ActorSystem::set_anim_state(actor, ANIM_STATE_WALK);
            ActorSystem::move_direction(actor, DIR_UP_LEFT, elapsed_ms);
            break;
        }
        case ACTION_RIGHT: {
            ActorSystem::set_anim_state(actor, ANIM_STATE_WALK);
            ActorSystem::move_direction(actor, DIR_DOWN_RIGHT, elapsed_ms);
            break;
        }
        case ACTION_UP_LEFT: {
            ActorSystem::set_anim_state(actor, ANIM_STATE_WALK);
            ActorSystem::move_diagonal(actor, -1, -1, elapsed_ms);
            break;
        }
        case ACTION_UP_RIGHT: {
            ActorSystem::set_anim_state(actor, ANIM_STATE_WALK);
            ActorSystem::move_diagonal(actor, 1, -1, elapsed_ms);
            break;
        }
        case ACTION_DOWN_LEFT: {
            ActorSystem::set_anim_state(actor, ANIM_STATE_WALK);
            ActorSystem::move_diagonal(actor, -1, 1, elapsed_ms);
            break;
        }
        case ACTION_DOWN_RIGHT: {
            ActorSystem::set_anim_state(actor, ANIM_STATE_WALK);
            ActorSystem::move_diagonal(actor, 1, 1, elapsed_ms);
            break;
        }
        case ACTION_USE: {
            ActorSystem::check_tile_use_trigger(actor, GameCanvas::state().tile_use_script);
            break;
        }
        case ACTION_TOGGLE_HAND: {
            actor->active_hand_record = (actor->active_hand_record.isNull() && !actor->stowed_hand_record.isNull()
                                             ? actor->stowed_hand_record
                                             : SharedArray<int32_t>());
            ActorSystem::update_hand_icon(actor);
            return true;
        }
    }
    return false;
}

void ActorSystem::update_hand_icon(Actor *j2) {
    if (!j2->active_hand_record.isNull()) {
        switch (j2->active_hand_record[2]) {
            case 0: {
                j2->hand_icon_frame = (int8_t)-48;
                break;
            }
            case 1: {
                j2->hand_icon_frame = (int8_t)-50;
                break;
            }
            case 2: {
                j2->hand_icon_frame = (int8_t)-46;
                break;
            }
            case 3: {
                if (j2->active_hand_record[1] == 61618) {
                    j2->hand_icon_frame = (int8_t)-44;
                    break;
                }
                if (j2->active_hand_record[1] == 61619) {
                    j2->hand_icon_frame = (int8_t)-43;
                    break;
                }
                j2->hand_icon_frame = (int8_t)-50;
                break;
            }
            case 4: {
                j2->hand_icon_frame = (int8_t)-47;
                break;
            }
            case 5: {
                j2->hand_icon_frame = (int8_t)-48;
                break;
            }
            case 6: {
                j2->hand_icon_frame = (int8_t)-43;
            }
        }
        return;
    }
    j2->hand_icon_frame = (int8_t)-45;
}

bool ActorSystem::can_use_item(Actor *j2, int32_t n, const SharedArray<int32_t> &nArray) {
    if (j2 == nullptr || nArray.isNull() || j2->character_class == -1) {
        return false;
    }
    if (n == 0) {
        if (nArray[2] == 1) {
            return GameCanvas::state().game_data->class_has_skill(j2->character_class, 5);
        }
        if (nArray[2] == 2) {
            return GameCanvas::state().game_data->class_has_skill(j2->character_class, 6);
        }
        if (nArray[2] == 3) {
            return GameCanvas::state().game_data->class_has_skill(j2->character_class, 7);
        }
        if (nArray[2] == 4) {
            return GameCanvas::state().game_data->class_has_skill(j2->character_class, 8);
        }
        if (nArray[2] == 0) {
            return GameCanvas::state().game_data->class_has_skill(j2->character_class, 14);
        }
    } else if (n == 1) {
        if (nArray[2] == 2) {
            return GameCanvas::state().game_data->class_has_skill(j2->character_class, 4);
        }
        if (nArray[2] == 1) {
            return GameCanvas::state().game_data->class_has_skill(j2->character_class, 3);
        }
        if (nArray[2] == 0) {
            return GameCanvas::state().game_data->class_has_skill(j2->character_class, 1);
        }
    }
    return true;
}

void ActorSystem::clinit() {
    S().var_byte_a = (int8_t)-52;
    S().var_byte_b = (int8_t)-39;
    S().var_short_a = (int16_t)300;
    S().var_short_b = (int16_t)200;
    S().anim_frame_offsets = SharedArray<int8_t>{0, 4, 8, 12, 16, 20, 24, 25};
    S().level_xp_thresholds = SharedArray<int16_t>{0,    0,    100,  210,  340,  500,  700,   950,   1260,  1640,  2100, 2650, 3300,
                                     4060, 4940, 5950, 7100, 8400, 9860, 11490, 13300, 15300, 17500, 19910, 22540,
                                     25400};
    S().level_xp_awards = SharedArray<int16_t>{0,  10, 12, 15, 19, 24, 30,  37,  45,  54,  64,  75, 87,
                                     100, 114, 129, 145, 162, 180, 199, 219, 240, 262, 285, 309, 334};
    S().var_int_arr_a = SharedArray<int32_t>(2);
    S().var_int_arr_b = SharedArray<int32_t>(2);
    S().var_int_arr_c = SharedArray<int32_t>(2);
    S().var_int_arr_d = SharedArray<int32_t>(2);
    S().var_int_arr_e = SharedArray<int32_t>(3);
}
