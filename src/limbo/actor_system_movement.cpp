#include "src/limbo/actor_system.hpp"

#include "src/limbo/game_canvas.hpp"
#include "src/limbo/iso_transform.hpp"
#include "src/limbo/game_data.hpp"
#include "src/limbo/magic_effects.hpp"
#include "src/limbo/sprite_atlas.hpp"

void ActorSystem::reset_actor(Actor *j2) {
    int32_t n = 0;
    for (n = 0; n < 25; ++n) {
        if (GameCanvas::state().actors[n] == nullptr) continue;
        GameCanvas::state().actors[n]->combat_target = nullptr;
    }
    j2->tile_position[0] = 0;
    j2->tile_position[1] = 0;
    j2->tile_enter_script_id = (int8_t)-1;
    j2->tile_exit_script_id = (int8_t)-1;
    j2->tile_use_script_id = (int8_t)-1;
    j2->is_dead = 0;
    j2->status_icon_frame = (int8_t)-1;
    j2->combat_target = nullptr;
    j2->death_elapsed_ms = 0;
    j2->combat_text = std::nullopt;
    j2->combat_text_y = 0;
    j2->combat_text_color = 0xFF0000;
    j2->anim_state = 0;
    j2->move_target[0] = -1;
    j2->move_target[1] = -1;
    j2->damage_over_time_remaining_ms = 0;
    j2->damage_over_time_tick_remaining_ms = 0;
    j2->damage_over_time_source = nullptr;
    j2->health = j2->max_health =
        (int16_t)(j2->level * 4 + (j2->strength + j2->strength_bonus) * 2 + j2->endurance * 2 + j2->health_capacity_bonus);
    j2->health_regen_interval_ms = (int16_t)(40000 / j2->max_health);
    j2->magicka = j2->max_magicka = (int16_t)(j2->level * 4 + j2->intelligence * 2 + j2->magicka_capacity_bonus);
    j2->magicka_regen_interval_ms = (int16_t)(40000 / j2->max_magicka);
    ActorSystem::recalculate_stats(j2);
    ActorSystem::update_tile_position(j2);
}

void ActorSystem::update_screen_position(Actor *j2) {
    j2->screen_position[0] = (j2->world_position[0] - j2->world_position[1]) >> 3;
    j2->screen_position[1] = (j2->world_position[0] + j2->world_position[1]) >> 4;
}

void ActorSystem::update_footprint_tiles(Actor *j2) {
    j2->footprint_left_tile[0] = (int8_t)(j2->world_position[0] >> 7);
    j2->footprint_left_tile[1] = (int8_t)(j2->world_position[1] >> 7);
    j2->footprint_center_tile[0] = (int8_t)(j2->footprint_center_position[0] >> 7);
    j2->footprint_center_tile[1] = (int8_t)(j2->footprint_center_position[1] >> 7);
    j2->footprint_right_tile[0] = (int8_t)(j2->footprint_right_position[0] >> 7);
    j2->footprint_right_tile[1] = (int8_t)(j2->footprint_right_position[1] >> 7);
    ActorSystem::update_tile_position(j2);
}

bool ActorSystem::is_colliding(Actor *j2) {
    if (j2 == nullptr) {
        return false;
    }
    if (j2->collision_enabled == 0) {
        return false;
    }
    if (j2->footprint_left_tile[0] < 0) {
        return true;
    }
    if (j2->footprint_left_tile[1] >= GameCanvas::state().level_cols) {
        return true;
    }
    if (j2->footprint_right_tile[0] >= GameCanvas::state().level_rows) {
        return true;
    }
    if (j2->footprint_right_tile[1] < 0) {
        return true;
    }
    if (ActorSystem::is_corner_colliding(j2, (int8_t)1)) {
        return true;
    }
    if (ActorSystem::is_corner_colliding(j2, (int8_t)2)) {
        return true;
    }
    return ActorSystem::is_corner_colliding(j2, (int8_t)3);
}

bool ActorSystem::tile_index_out_of_range(const SharedArray<int8_t> &corner) {
    const int32_t index = corner[0] * GameCanvas::state().level_cols + corner[1];
    return index < 0 || index >= GameCanvas::state().tile_walkable.length();
}

bool ActorSystem::is_corner_colliding(Actor *j2, int8_t by) {
    int32_t n = 0;
    int32_t n2 = 0;
    int32_t n3 = 0;
    if (GameCanvas::state().tile_walkable.isNull()) {
        return false;
    }
    if (j2 == nullptr) {
        return false;
    }
    if (ActorSystem::tile_index_out_of_range(j2->footprint_left_tile)) {
        return true;
    }
    if (ActorSystem::tile_index_out_of_range(j2->footprint_center_tile)) {
        return true;
    }
    if (ActorSystem::tile_index_out_of_range(j2->footprint_right_tile)) {
        return true;
    }
    switch (by) {
        case 1: {
            n = GameCanvas::state().tile_walkable[j2->footprint_left_tile[0] * GameCanvas::state().level_cols + j2->footprint_left_tile[1]];
            n2 = j2->world_position[0] % 128;
            n3 = j2->world_position[1] % 128;
            break;
        }
        case 2: {
            n = GameCanvas::state().tile_walkable[j2->footprint_center_tile[0] * GameCanvas::state().level_cols + j2->footprint_center_tile[1]];
            n2 = j2->footprint_center_position[0] % 128;
            n3 = j2->footprint_center_position[1] % 128;
            break;
        }
        case 3: {
            n = GameCanvas::state().tile_walkable[j2->footprint_right_tile[0] * GameCanvas::state().level_cols + j2->footprint_right_tile[1]];
            n2 = j2->footprint_right_position[0] % 128;
            n3 = j2->footprint_right_position[1] % 128;
        }
    }
    if (n == 0) {
        return false;
    }
    switch (n) {
        case 1: {
            return true;
        }
        case 4: {
            return n3 <= n2;
        }
        case 3: {
            return n3 >= n2;
        }
        case 2: {
            return n2 <= n3;
        }
        case 5: {
            return n2 >= n3;
        }
    }
    return false;
}

void ActorSystem::move_direction(Actor *j2, int32_t n, int64_t l) {
    if (j2 == nullptr) {
        return;
    }
    j2->movement_elapsed_ms = (int16_t)((int64_t)j2->movement_elapsed_ms + l);
    if (j2->movement_elapsed_ms > 50) {
        if (j2->movement_elapsed_ms > 400) {
            j2->movement_elapsed_ms = (int16_t)50;
        }
        int32_t n2 = j2->move_speed / (1000 / j2->movement_elapsed_ms);
        switch (n) {
            case 2: {
                ActorSystem::move_by(j2, 0, -n2);
                break;
            }
            case 1: {
                ActorSystem::move_by(j2, 0, n2);
                break;
            }
            case 4: {
                ActorSystem::move_by(j2, -n2, 0);
                break;
            }
            case 3: {
                ActorSystem::move_by(j2, n2, 0);
            }
        }
        if (ActorSystem::is_colliding(j2)) {
            ActorSystem::revert_movement(j2);
        }
        j2->movement_elapsed_ms = 0;
    }
}

void ActorSystem::move_diagonal(Actor *j2, int32_t dx_dir, int32_t dy_dir, int64_t elapsed_ms) {
    if (j2 == nullptr) {
        return;
    }
    j2->movement_elapsed_ms = (int16_t)((int64_t)j2->movement_elapsed_ms + elapsed_ms);
    if (j2->movement_elapsed_ms > 50) {
        if (j2->movement_elapsed_ms > 400) {
            j2->movement_elapsed_ms = (int16_t)50;
        }
        const int32_t base_step = j2->move_speed / (1000 / j2->movement_elapsed_ms);
        int32_t diag_step = (base_step * 724) / 1024;
        if (diag_step == 0 && base_step > 0) {
            diag_step = 1;
        }

        const int32_t start_x = j2->world_position[0];
        const int32_t start_y = j2->world_position[1];
        const int8_t start_facing = j2->facing;
        const int32_t step_x = dx_dir * diag_step;
        const int32_t step_y = dy_dir * diag_step;

        ActorSystem::move_by(j2, step_x, 0);
        if (ActorSystem::is_colliding(j2)) {
            ActorSystem::revert_movement(j2);
        }

        ActorSystem::move_by(j2, 0, step_y);
        if (ActorSystem::is_colliding(j2)) {
            ActorSystem::revert_movement(j2);
        }

        const bool moved_x = (j2->world_position[0] != start_x);
        const bool moved_y = (j2->world_position[1] != start_y);

        if (moved_x && moved_y) {
            j2->facing = (int8_t)(dx_dir > 0 ? 3 : 4);
        } else if (moved_x) {
            j2->facing = (int8_t)(dx_dir > 0 ? 3 : 4);
        } else if (moved_y) {
            j2->facing = (int8_t)(dy_dir > 0 ? 1 : 2);
        } else {
            j2->facing = start_facing;
        }

        if (moved_x || moved_y) {
            j2->action_animation_remaining_ms = (int16_t)500;
            j2->previous_world_position[0] = start_x;
            j2->previous_world_position[1] = start_y;
        }

        j2->movement_elapsed_ms = 0;
    }
}

void ActorSystem::move_by(Actor *j2, int32_t n, int32_t n2) {
    j2->previous_world_position[0] = j2->world_position[0];
    j2->previous_world_position[1] = j2->world_position[1];
    j2->world_position[0] = j2->world_position[0] + n;
    j2->world_position[1] = j2->world_position[1] + n2;
    j2->footprint_center_position[0] = j2->footprint_center_position[0] + n;
    j2->footprint_center_position[1] = j2->footprint_center_position[1] + n2;
    j2->footprint_right_position[0] = j2->footprint_right_position[0] + n;
    j2->footprint_right_position[1] = j2->footprint_right_position[1] + n2;
    ActorSystem::update_screen_position(j2);
    ActorSystem::update_footprint_tiles(j2);
    if (n > 0) {
        j2->facing = (int8_t)3;
    } else if (n < 0) {
        j2->facing = (int8_t)4;
    } else if (n2 > 0) {
        j2->facing = 1;
    } else if (n2 < 0) {
        j2->facing = (int8_t)2;
    }
    j2->action_animation_remaining_ms = (int16_t)500;
}

void ActorSystem::set_position(Actor *j2, int32_t n, int32_t n2) {
    ActorSystem::S().var_int_arr_a[0] = j2->footprint_half_width;
    ActorSystem::S().var_int_arr_a[1] = 0;
    ActorSystem::S().var_int_arr_b[0] = j2->footprint_width;
    ActorSystem::S().var_int_arr_b[1] = 0;
    ActorSystem::S().var_int_arr_c[0] = 0;
    ActorSystem::S().var_int_arr_c[1] = 0;
    ActorSystem::S().var_int_arr_d[0] = 0;
    ActorSystem::S().var_int_arr_d[1] = 0;
    IsoTransform::screen_to_world(S().var_int_arr_c, S().var_int_arr_a);
    IsoTransform::screen_to_world(S().var_int_arr_d, S().var_int_arr_b);
    j2->world_position[0] = n;
    j2->world_position[1] = n2;
    j2->footprint_center_position[0] = n + S().var_int_arr_c[0];
    j2->footprint_center_position[1] = n2 + S().var_int_arr_c[1];
    j2->footprint_right_position[0] = n + S().var_int_arr_d[0];
    j2->footprint_right_position[1] = n2 + S().var_int_arr_d[1];
    ActorSystem::update_screen_position(j2);
    ActorSystem::update_footprint_tiles(j2);
}

void ActorSystem::update_tile_position(Actor *j2) {
    if (j2->footprint_right_tile[0] > j2->footprint_center_tile[0]) {
        j2->tile_position[0] = j2->footprint_right_tile[0];
        j2->tile_position[1] = j2->footprint_right_tile[1];
        return;
    }
    if (j2->footprint_center_tile[1] > j2->footprint_right_tile[1] || j2->footprint_center_tile[0] > j2->footprint_left_tile[0]) {
        j2->tile_position[0] = j2->footprint_center_tile[0];
        j2->tile_position[1] = j2->footprint_center_tile[1];
        return;
    }
    j2->tile_position[0] = j2->footprint_left_tile[0];
    j2->tile_position[1] = j2->footprint_left_tile[1];
}

void ActorSystem::revert_movement(Actor *j2) { ActorSystem::set_position(j2, j2->previous_world_position[0], j2->previous_world_position[1]); }

int8_t ActorSystem::check_tile_triggers(Actor *j2, const SharedArray<int8_t> &byArray, const SharedArray<int8_t> &byArray2) {
    int32_t n = 0;
    j2->tile_enter_script_id = (int8_t)-1;
    j2->tile_exit_script_id = (int8_t)-1;
    if (byArray.isNull() || byArray2.isNull()) {
        return -1;
    }
    ActorSystem::S().var_int_arr_e[0] = j2->footprint_left_tile[0] * GameCanvas::state().level_cols + j2->footprint_left_tile[1];
    ActorSystem::S().var_int_arr_e[1] = j2->footprint_center_tile[0] * GameCanvas::state().level_cols + j2->footprint_center_tile[1];
    ActorSystem::S().var_int_arr_e[2] = j2->footprint_right_tile[0] * GameCanvas::state().level_cols + j2->footprint_right_tile[1];
    for (n = 0; n < S().var_int_arr_e.length(); ++n) {
        if (S().var_int_arr_e[n] < 0 || S().var_int_arr_e[n] >= byArray.length()) {
            return -1;
        }
        if (byArray[S().var_int_arr_e[n]] == 0 || byArray[S().var_int_arr_e[n]] == -1) continue;
        j2->tile_enter_script_id = byArray[S().var_int_arr_e[n]];
        j2->tile_exit_script_id = byArray2[S().var_int_arr_e[n]];
        return byArray[S().var_int_arr_e[n]];
    }
    return -1;
}

void ActorSystem::set_move_target(Actor *j2, int32_t n, int32_t n2) {
    j2->move_target[0] = n;
    j2->move_target[1] = n2;
    j2->anim_state = 1;
}

void ActorSystem::update(Actor *j2, int64_t l, bool bl) {
    int32_t n = 0;
    int32_t n2 = 0;
    int32_t n3 = 0;
    if (j2 == nullptr) {
        return;
    }
    j2->animation_frame_elapsed_ms = (int16_t)((int64_t)j2->animation_frame_elapsed_ms + l);
    j2->loot_pickup_delay_elapsed_ms = (int32_t)((int64_t)j2->loot_pickup_delay_elapsed_ms + l);
    j2->attack_elapsed_ms = (int32_t)((int64_t)j2->attack_elapsed_ms + l);
    if (j2->animation_frame_elapsed_ms > 125 && j2->is_dead == 0) {
        SpriteAtlas::advance_animation(j2->sprite_atlas, j2->facing + S().anim_frame_offsets[j2->anim_state]);
        j2->animation_frame_elapsed_ms = 0;
    }
    if (j2->is_dead == 0) {
        if (j2->move_target[0] != -1) {
            j2->movement_elapsed_ms = (int16_t)((int64_t)j2->movement_elapsed_ms + l);
            if (j2->movement_elapsed_ms >= 50) {
                if (j2->movement_elapsed_ms > 100) {
                    j2->movement_elapsed_ms = (int16_t)100;
                }
                n3 = j2->move_speed / (1000 / j2->movement_elapsed_ms);
                if (j2->world_position[0] < j2->move_target[0]) {
                    n = min32(n3, j2->move_target[0] - j2->world_position[0]);
                } else if (j2->world_position[0] > j2->move_target[0]) {
                    n = max32(-n3, j2->move_target[0] - j2->world_position[0]);
                } else if (j2->world_position[1] < j2->move_target[1]) {
                    n2 = min32(n3, j2->move_target[1] - j2->world_position[1]);
                } else if (j2->world_position[1] > j2->move_target[1]) {
                    n2 = max32(-n3, j2->move_target[1] - j2->world_position[1]);
                } else {
                    j2->move_target[0] = -1;
                    if (j2->anim_state != 2) {
                        j2->anim_state = 0;
                    }
                }
                ActorSystem::move_by(j2, n, n2);
                j2->movement_elapsed_ms = 0;
            }
        } else if (j2->actor_kind == 1 && j2->action_animation_remaining_ms > 0) {
            j2->action_animation_remaining_ms = (int16_t)((int64_t)j2->action_animation_remaining_ms - l);
            if (j2->action_animation_remaining_ms <= 0) {
                j2->anim_state = 0;
            }
        }
        if (j2->damage_over_time_remaining_ms > 0) {
            j2->damage_over_time_remaining_ms = (int16_t)((int64_t)j2->damage_over_time_remaining_ms - l);
            j2->damage_over_time_tick_remaining_ms = (int16_t)((int64_t)j2->damage_over_time_tick_remaining_ms - l);
            if (j2->damage_over_time_tick_remaining_ms <= 0) {
                MagicEffects::spawn_attached(8, j2);
                j2->damage_over_time_tick_remaining_ms = (int16_t)1000;
                ActorSystem::apply_damage(j2->damage_over_time_amount, j2, j2->damage_over_time_source, false, true);
            }
        } else if (j2->offhand_icon_frame == -47) {
            j2->offhand_icon_frame = (int8_t)-1;
        }
        if (j2->ai_behavior == 2) {
            j2->teleport_timer_ms = (int16_t)((int64_t)j2->teleport_timer_ms - l);
        }
        if (j2->actor_kind == 1) {
            if (j2->health < j2->max_health) {
                j2->health_regen_elapsed_ms = (int16_t)((int64_t)j2->health_regen_elapsed_ms + l);
                if (j2->health_regen_elapsed_ms >= j2->health_regen_interval_ms) {
                    if (j2->health < j2->max_health) {
                        j2->health = (int16_t)(j2->health + 1);
                    }
                    j2->health_regen_elapsed_ms = 0;
                    ActorSystem::recalculate_stats(j2);
                }
            }
            if (j2->magicka < j2->max_magicka) {
                j2->magicka_regen_elapsed_ms = (int16_t)((int64_t)j2->magicka_regen_elapsed_ms + l);
                if (j2->magicka_regen_elapsed_ms >= j2->magicka_regen_interval_ms) {
                    if (j2->magicka < j2->max_magicka) {
                        j2->magicka = (int16_t)(j2->magicka + 1);
                    }
                    j2->magicka_regen_elapsed_ms = 0;
                    ActorSystem::recalculate_stats(j2);
                }
            }
            if (j2->potion_buff_duration_ms > 0) {
                if (j2->potion_buff_elapsed_ms >= j2->potion_buff_duration_ms) {
                    j2->potion_buff_elapsed_ms = 0;
                    j2->potion_buff_duration_ms = 0;
                    j2->magicka_capacity_bonus = 0;
                    j2->armour_bonus = 0;
                    j2->attack_bonus = 0;
                    j2->defense_bonus = 0;
                    j2->damage_bonus = 0;
                    j2->dodge_chance_multiplier_percent = 0;
                    j2->offhand_icon_frame = (int8_t)-1;
                    MagicEffects::clear_slot(j2->buff_effect_slot);
                    if (j2->magicka_capacity_bonus != 0) {
                        j2->magicka = j2->max_magicka = (int16_t)(j2->level * 4 + j2->intelligence * 2 + j2->magicka_capacity_bonus);
                        j2->magicka_regen_interval_ms = (int16_t)(40000 / j2->max_magicka);
                    }
                    if (j2->strength_bonus != 0) {
                        j2->strength_bonus = 0;
                        j2->max_health =
                            (int16_t)(j2->level * 4 + (j2->strength + j2->strength_bonus) * 2 + j2->endurance * 2 + j2->health_capacity_bonus);
                        j2->health_regen_interval_ms = (int16_t)(40000 / j2->max_health);
                    }
                    ActorSystem::recalculate_stats(j2);
                }
                j2->potion_buff_elapsed_ms = (int16_t)((int64_t)j2->potion_buff_elapsed_ms + l);
            }
        } else if (j2->auto_attack_enabled == 1 && ActorSystem::update_combat_target(j2) && j2->combat_target != nullptr &&
                   j2->attack_elapsed_ms >= j2->attack_interval_ms) {
            if ((bl || j2->combat_target->actor_kind != 1) && ActorSystem::attack(j2, j2->combat_target, true)) {
                j2->combat_target = nullptr;
                j2->damage_over_time_source = nullptr;
                j2->anim_state = 0;
            }
            j2->attack_elapsed_ms = 0;
        }
        if (j2->combat_text.has_value()) {
            j2->combat_text_step_elapsed_ms = (int16_t)((int64_t)j2->combat_text_step_elapsed_ms + l);
            if (j2->combat_text_step_elapsed_ms > 50) {
                j2->combat_text_y = (int16_t)(j2->combat_text_y - 2);
                j2->combat_text_color -= j2->combat_text_fade_step;
                if (j2->combat_text_color <= 0 || wrappingAbs(j2->combat_text_start_y - j2->combat_text_y) > 20) {
                    j2->combat_text_color = 0;
                    j2->combat_text_y = 0;
                    j2->combat_text_start_y = 0;
                    j2->combat_text = std::nullopt;
                }
                j2->combat_text_step_elapsed_ms = 0;
            }
        }
        if (j2->spell_buff_remaining_ms > 0) {
            j2->spell_buff_remaining_ms = (int16_t)((int64_t)j2->spell_buff_remaining_ms - l);
            if (j2->spell_buff_remaining_ms <= 0) {
                j2->potion_buff_elapsed_ms = 0;
                j2->potion_buff_duration_ms = 0;
                j2->magicka_capacity_bonus = 0;
                j2->armour_bonus = 0;
                j2->attack_bonus = 0;
                j2->defense_bonus = 0;
                j2->damage_bonus = 0;
                j2->dodge_chance_multiplier_percent = 0;
                j2->offhand_icon_frame = (int8_t)-1;
                MagicEffects::clear_slot(j2->buff_effect_slot);
                if (j2->magicka_capacity_bonus != 0) {
                    j2->magicka = j2->max_magicka = (int16_t)(j2->level * 4 + j2->intelligence * 2 + j2->magicka_capacity_bonus);
                    j2->magicka_regen_interval_ms = (int16_t)(40000 / j2->max_magicka);
                }
                if (j2->strength_bonus != 0) {
                    j2->strength_bonus = 0;
                    j2->max_health =
                        (int16_t)(j2->level * 4 + (j2->strength + j2->strength_bonus) * 2 + j2->endurance * 2 + j2->health_capacity_bonus);
                    j2->health_regen_interval_ms = (int16_t)(40000 / j2->max_health);
                }
                ActorSystem::recalculate_stats(j2);
                return;
            }
        }
    } else {
        if (j2->death_elapsed_ms >= 250) {
            GameCanvas::remove_actor(j2->actor_kind - 1);
        }
        j2->death_elapsed_ms = (int16_t)((int64_t)j2->death_elapsed_ms + l);
    }
}
