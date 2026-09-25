#include "src/limbo/actor_system.hpp"

#include "src/limbo/game_canvas.hpp"
#include "src/limbo/game_data.hpp"
#include "src/limbo/magic_effects.hpp"
#include "src/limbo/sprite_atlas.hpp"

bool ActorSystem::apply_damage(int32_t n, Actor *j2, Actor *j3, bool bl, bool bl2) {
    if (j2 == nullptr || j3 == nullptr && !bl2 || j2->is_invulnerable == 1) {
        return false;
    }
    int32_t n2 = j2->dodge_chance + (j2->dodge_chance >> 1);
    int32_t n3 = j2->block_chance + (j2->block_chance >> 1);
    int32_t n4 = ((j2->agility + j2->armour_rating + j2->armour_bonus) >> 3) + j2->defense_bonus;
    if (bl2) {
        n4 = 0;
        n3 = -1000;
        n2 = -1000;
    }
    int32_t n5 = n - n4;
    int32_t n6 = GameCanvas::state().random->nextInt() % 100;
    int32_t n7 = GameCanvas::state().random->nextInt() % 100;
    n2 *= j2->dodge_chance_multiplier_percent;
    n2 /= 100;
    n6 = n6 < 0 ? -n6 : n6;
    n7 = n7 < 0 ? -n7 : n7;
    if (j2->combat_target == nullptr) {
        j2->combat_target = j3;
    }
    if (n6 <= n2) {
        j2->combat_text = GameCanvas::get_text(471);
        j2->combat_text_y = 0;
    } else if (n7 <= n3) {
        j2->combat_text = GameCanvas::get_text(470);
        j2->combat_text_y = 0;
    } else if (n5 > 0) {
        if (j2->actor_kind != 1 && j2->combat_target != nullptr && !bl2) {
            j2->engage_radius = (int16_t)max32(ActorSystem::distance(j2->world_position, j2->combat_target->world_position), j2->engage_radius);
        }
        if (j3 != nullptr && j3->bow_equipped == 0) {
            GameCanvas::state().random->nextInt();
        }
        j2->health = (int16_t)(j2->health - n5);
        j2->combat_text = std::string(bl ? GameCanvas::get_text(472) : std::string("")) + std::to_string(n5);
        j2->combat_text_y = 0;
        j2->is_dead = (int8_t)(j2->health <= 0 ? 1 : 0);
        if (j2->is_dead == 1) {
            int32_t n9;
            if (j3 != nullptr) {
                j3->loot_pickup_delay_elapsed_ms = 0;
                ActorSystem::award_experience(j3, j2->level);
            }
            GameCanvas::state().random->nextInt();
            j2->anim_state = (int8_t)6;
            ActorSystem::update_tile_position(j2);
            if (j2->death_script_id >= 0) {
                GameData::call_script((uint16_t)j2->death_script_id);
            }
            if (j2->loot_drop_enabled == 1 && (n9 = GameCanvas::state().game_data->roll_loot_drop()) != 0) {
                GameCanvas::state().instance->place_world_item(n9, false, (int32_t)j2->footprint_center_tile[0], (int32_t)j2->footprint_center_tile[1]);
            }
        }
    }
    return j2->is_dead == 1;
}

bool ActorSystem::attack(Actor *j2, Actor *j3, bool bl) {
    if (j2 == nullptr || j3 == nullptr) {
        return false;
    }
    if (j2->actor_kind != 1 && (!j2->active_hand_record.isNull() || j2->bow_equipped == 1) && bl) {
        ActorSystem::cast_spell(j2, false);
        if (j2->ai_behavior == 3) {
            j2->active_hand_record.setNull();
            j2->disengage_radius = (int16_t)(j2->disengage_radius >> 1);
        } else if (j2->ai_behavior == 2 && j2->teleport_timer_ms <= 0 && ActorSystem::handle_teleport(j2)) {
            j2->teleport_timer_ms = (int16_t)(wrappingAbs(GameCanvas::state().random->nextInt()) % 2000 + 2000);
        }
        return false;
    }
    int32_t n = ((j2->strength + j2->strength_bonus + j2->weapon_damage_bonus) >> 1) + j2->attack_bonus + j2->damage_bonus;
    int32_t n2 = GameCanvas::state().random->nextInt() % 16;
    bool bl2 = false;
    n *= j2->attack_rating;
    n /= 100;
    if (!j2->active_hand_record.isNull()) {
        n = j2->level >= j2->active_hand_record[10]
                ? j2->active_hand_record[5]
                : (j2->level >= j2->active_hand_record[9] ? j2->active_hand_record[4] : j2->active_hand_record[3]);
        if (j2->actor_kind != 1 && j2->active_hand_record[2] != 4) {
            n = (n >> 1);
        }
    }
    if ((n2 = n2 < 0 ? -n2 : n2) == 1) {
        n = max32(j3->health >> 2, shiftLeft32(n, 1));
        bl2 = true;
    }
    return ActorSystem::apply_damage(n, j3, j2, bl2, false);
}

void ActorSystem::award_experience(Actor *j2, int32_t n) {
    if (j2->summoner != nullptr) {
        j2 = j2->summoner;
    }
    if (j2->actor_kind == 1) {
        j2->experience += S().level_xp_awards[n];
        if (j2->level < 25 && j2->experience >= S().level_xp_thresholds[j2->level + 1]) {
            int16_t s = j2->strength;
            int16_t s2 = j2->intelligence;
            int16_t s3 = j2->willpower;
            int16_t s4 = j2->agility;
            int16_t s5 = j2->endurance;
            int16_t s6 = j2->personality;
            std::string level_message;
            level_message += GameCanvas::get_text(41);
            j2->level = (int8_t)(j2->level + 1);
            j2->strength = (int16_t)(j2->strength + 1);
            j2->intelligence = (int16_t)(j2->intelligence + 1);
            j2->willpower = (int16_t)(j2->willpower + 1);
            j2->agility = (int16_t)(j2->agility + 1);
            j2->move_speed = (int16_t)(j2->move_speed + 1);
            j2->endurance = (int16_t)(j2->endurance + 1);
            j2->personality = (int16_t)(j2->personality + 1);
            ActorSystem::apply_class_level_bonuses(j2);
            level_message += " ";
            level_message += std::to_string((int32_t)j2->level);
            level_message += ": +";
            level_message += std::to_string((int32_t)(j2->strength - s));
            level_message += " ";
            level_message += GameCanvas::get_text(415);
            level_message += ", +";
            level_message += std::to_string((int32_t)(j2->intelligence - s2));
            level_message += " ";
            level_message += GameCanvas::get_text(416);
            level_message += ", +";
            level_message += std::to_string((int32_t)(j2->willpower - s3));
            level_message += " ";
            level_message += GameCanvas::get_text(417);
            level_message += ", +";
            level_message += std::to_string((int32_t)(j2->agility - s4));
            level_message += " ";
            level_message += GameCanvas::get_text(418);
            level_message += ", +";
            level_message += std::to_string((int32_t)(j2->endurance - s5));
            level_message += " ";
            level_message += GameCanvas::get_text(419);
            level_message += ", +";
            level_message += std::to_string((int32_t)(j2->personality - s6));
            level_message += " ";
            level_message += GameCanvas::get_text(420);
            j2->max_health =
                (int16_t)(j2->level * 4 + (j2->strength + j2->strength_bonus) * 2 + j2->endurance * 2 + j2->health_capacity_bonus);
            j2->health_regen_interval_ms = (int16_t)(40000 / j2->max_health);
            j2->max_magicka = (int16_t)(j2->level * 4 + j2->intelligence * 2 + j2->magicka_capacity_bonus);
            j2->magicka_regen_interval_ms = (int16_t)(40000 / j2->max_magicka);
            ActorSystem::recalculate_stats(j2);
            GameCanvas::set_status_message(level_message, 30, 4, 3);
            return;
        }
        GameCanvas::set_status_message(std::to_string((int32_t)S().level_xp_awards[n]) + " " + GameCanvas::get_text(42) + "!", 3, 4, 1);
    }
}

void ActorSystem::apply_class_level_bonuses(Actor *j2) {
    switch (j2->character_class) {
        case 1: {
            if (j2->level == 5) {
                j2->move_speed = (int16_t)(j2->move_speed + 25);
                return;
            }
            if (j2->level == 10) {
                j2->agility = (int16_t)(j2->agility + 1);
                return;
            }
            if (j2->level == 15) {
                j2->endurance = (int16_t)(j2->endurance + 2);
                return;
            }
            if (j2->level != 20) break;
            j2->strength = (int16_t)(j2->strength + 2);
            return;
        }
        case 2: {
            if (j2->level == 5) {
                j2->agility = (int16_t)(j2->agility + 1);
                return;
            }
            if (j2->level == 10) {
                j2->willpower = (int16_t)(j2->willpower + 1);
                return;
            }
            if (j2->level == 15) {
                j2->intelligence = (int16_t)(j2->intelligence + 2);
                return;
            }
            if (j2->level != 20) break;
            j2->agility = (int16_t)(j2->agility + 2);
            return;
        }
        case 3: {
            if (j2->level == 5) {
                j2->strength = (int16_t)(j2->strength + 1);
                return;
            }
            if (j2->level == 10) {
                j2->endurance = (int16_t)(j2->endurance + 1);
                return;
            }
            if (j2->level == 15) {
                j2->endurance = (int16_t)(j2->endurance + 2);
                return;
            }
            if (j2->level != 20) break;
            j2->strength = (int16_t)(j2->strength + 2);
            return;
        }
        case 4: {
            if (j2->level == 5) {
                j2->move_speed = (int16_t)(j2->move_speed + 25);
                return;
            }
            if (j2->level == 10) {
                j2->agility = (int16_t)(j2->agility + 2);
                return;
            }
            if (j2->level == 15) {
                j2->strength = (int16_t)(j2->strength + 1);
                return;
            }
            if (j2->level != 20) break;
            j2->strength = (int16_t)(j2->strength + 2);
            return;
        }
        case 5: {
            if (j2->level == 5) {
                j2->strength = (int16_t)(j2->strength + 1);
                return;
            }
            if (j2->level == 10) {
                j2->endurance = (int16_t)(j2->endurance + 1);
                return;
            }
            if (j2->level == 15) {
                j2->strength = (int16_t)(j2->strength + 2);
                return;
            }
            if (j2->level != 20) break;
            j2->endurance = (int16_t)(j2->endurance + 2);
            return;
        }
        case 6: {
            if (j2->level == 5) {
                j2->agility = (int16_t)(j2->agility + 1);
                return;
            }
            if (j2->level == 10) {
                j2->willpower = (int16_t)(j2->willpower + 1);
                return;
            }
            if (j2->level == 15) {
                j2->intelligence = (int16_t)(j2->intelligence + 2);
                return;
            }
            if (j2->level != 20) break;
            j2->willpower = (int16_t)(j2->willpower + 2);
            return;
        }
        case 7: {
            if (j2->level == 5) {
                j2->intelligence = (int16_t)(j2->intelligence + 1);
                return;
            }
            if (j2->level == 10) {
                j2->willpower = (int16_t)(j2->willpower + 1);
                return;
            }
            if (j2->level == 15) {
                j2->willpower = (int16_t)(j2->willpower + 2);
                return;
            }
            if (j2->level != 20) break;
            j2->intelligence = (int16_t)(j2->intelligence + 2);
            return;
        }
        case 8: {
            if (j2->level == 5) {
                j2->willpower = (int16_t)(j2->willpower + 1);
                return;
            }
            if (j2->level == 10) {
                j2->strength = (int16_t)(j2->strength + 1);
                return;
            }
            if (j2->level == 15) {
                j2->intelligence = (int16_t)(j2->intelligence + 2);
                return;
            }
            if (j2->level != 20) break;
            j2->willpower = (int16_t)(j2->willpower + 2);
        }
    }
}

int8_t ActorSystem::check_tile_use_trigger(Actor *j2, const SharedArray<int8_t> &byArray) {
    int32_t n = 0;
    j2->tile_use_script_id = (int8_t)-1;
    if (byArray.isNull()) {
        return j2->tile_use_script_id;
    }
    SharedArray<int32_t> nArray{j2->footprint_left_tile[0] * GameCanvas::state().level_cols + j2->footprint_left_tile[1],
                        j2->footprint_center_tile[0] * GameCanvas::state().level_cols + j2->footprint_center_tile[1],
                        j2->footprint_right_tile[0] * GameCanvas::state().level_cols + j2->footprint_right_tile[1]};
    for (n = 0; n < nArray.length(); ++n) {
        if (nArray[n] < 0 || nArray[n] >= byArray.length()) {
            return -1;
        }
        if (byArray[nArray[n]] < 0 || byArray[nArray[n]] >= 255) continue;
        j2->tile_use_script_id = byArray[nArray[n]];
        return byArray[nArray[n]];
    }
    if (j2->bow_equipped == 0 && j2->active_hand_record.isNull()) {
        j2->action_animation_remaining_ms = (int16_t)500;
        j2->anim_state = (int8_t)4;
    }
    if (j2->attack_elapsed_ms >= j2->attack_interval_ms) {
        j2->attack_elapsed_ms = 0;
        if (!j2->active_hand_record.isNull() || j2->bow_equipped == 1) {
            ActorSystem::cast_spell(j2, true);
        } else {
            ActorSystem::update_combat_target(j2);
            if (j2->combat_target != nullptr) {
                ActorSystem::face_actor(j2, j2->combat_target);
                if (ActorSystem::attack(j2, j2->combat_target, true)) {
                    j2->combat_target = nullptr;
                    j2->damage_over_time_source = nullptr;
                    j2->anim_state = 0;
                }
            }
        }
    }
    return -1;
}

void ActorSystem::cast_spell(Actor *j2, bool bl) {
    int32_t n = 0;
    int32_t n2 = 0;
    if (j2->bow_equipped == 1) {
        if (j2->actor_kind == 1) {
            j2->action_animation_remaining_ms = (int16_t)500;
            j2->anim_state = (int8_t)7;
        }
        MagicEffects::spawn_attached(11, (int32_t)j2->facing, j2);
        return;
    }
    if (j2->level >= j2->active_hand_record[10]) {
        if (j2->magicka < j2->active_hand_record[13] && bl) return;
        n = j2->active_hand_record[5];
        j2->magicka = (int16_t)(j2->magicka - j2->active_hand_record[13]);
    } else if (j2->level >= j2->active_hand_record[9]) {
        if (j2->magicka < j2->active_hand_record[12] && bl) return;
        n = j2->active_hand_record[4];
        j2->magicka = (int16_t)(j2->magicka - j2->active_hand_record[12]);
    } else {
        if (j2->magicka < j2->active_hand_record[11] && bl) return;
        n = j2->active_hand_record[3];
        j2->magicka = (int16_t)(j2->magicka - j2->active_hand_record[11]);
    }
    switch (j2->active_hand_record[2]) {
        case 0: {
            j2->spell_buff_remaining_ms = (int16_t)j2->active_hand_record[6];
            j2->armour_bonus = (int16_t)n;
            j2->offhand_icon_frame = (int8_t)-48;
            MagicEffects::clear_slot(j2->buff_effect_slot);
            j2->buff_effect_slot = (int8_t)MagicEffects::spawn_attached(9, j2, 5000);
            break;
        }
        case 1: {
            j2->spell_buff_remaining_ms = (int16_t)j2->active_hand_record[6];
            j2->damage_bonus = (int16_t)n;
            j2->offhand_icon_frame = (int8_t)-50;
            MagicEffects::clear_slot(j2->buff_effect_slot);
            j2->buff_effect_slot = (int8_t)MagicEffects::spawn_attached(9, j2, 5000);
            break;
        }
        case 5: {
            j2->spell_buff_remaining_ms = (int16_t)j2->active_hand_record[6];
            j2->dodge_chance_multiplier_percent = (int16_t)(n + 100);
            j2->offhand_icon_frame = (int8_t)-48;
            MagicEffects::clear_slot(j2->buff_effect_slot);
            j2->buff_effect_slot = (int8_t)MagicEffects::spawn_attached(9, j2, 5000);
            break;
        }
        case 2: {
            if (j2->summoned_actor != nullptr) {
                GameCanvas::remove_actor(j2->summoned_actor->actor_kind - 1);
            }
            j2->summoned_actor = GameCanvas::state().instance->spawn_actor(std::string("/oh_scamp.cml"), j2->world_position[0], j2->world_position[1],
                                        j2->definition_record);
            j2->summoned_actor->summoner = j2;
            ActorSystem::set_loot_drop_enabled(j2->summoned_actor, false);
        }
        case 4: {
            for (n2 = 0; n2 < GameCanvas::state().actors.length(); ++n2) {
                if (GameCanvas::state().actors[n2] == nullptr || GameCanvas::state().actors[n2] == j2 ||
                    GameCanvas::state().actors[n2]->faction_id == j2->faction_id ||
                    ActorSystem::distance(j2->world_position, GameCanvas::state().actors[n2]->world_position) > j2->active_hand_record[14])
                    continue;
                ActorSystem::apply_damage_over_time(j2, GameCanvas::state().actors[n2], n, j2->active_hand_record[6]);
            }
            break;
        }
        case 6: {
            MagicEffects::spawn_attached(8, j2);
            j2->damage_over_time_remaining_ms = 0;
            j2->damage_over_time_tick_remaining_ms = 0;
            j2->offhand_icon_frame = (int8_t)-1;
            break;
        }
        case 3: {
            if (j2->active_hand_record[1] == 61618) {
                for (n2 = 0; n2 < GameCanvas::state().actors.length(); ++n2) {
                    if (GameCanvas::state().actors[n2] == nullptr || GameCanvas::state().actors[n2] == j2 ||
                        GameCanvas::state().actors[n2]->faction_id == j2->faction_id ||
                        ActorSystem::distance(j2->world_position, GameCanvas::state().actors[n2]->world_position) > j2->active_hand_record[14])
                        continue;
                    ActorSystem::apply_spell_damage(j2, GameCanvas::state().actors[n2], n);
                }
                break;
            }
            if (j2->active_hand_record[1] == 61619) {
                MagicEffects::spawn_attached(8, j2);
                j2->health = (int16_t)min32(j2->max_health, j2->health + wrappingAbs(n));
                break;
            }
            MagicEffects::spawn_attached(0, (int32_t)j2->facing, j2);
        }
    }
    ActorSystem::recalculate_stats(j2);
}

void ActorSystem::set_death_script(Actor *j2, int32_t n, int32_t n2) {
    (void)n;
    j2->death_script_id = (int8_t)n2;
}

void ActorSystem::clear_death_script(Actor *j2, int32_t n) {
    (void)n;
    j2->death_script_id = (int8_t)-1;
}
