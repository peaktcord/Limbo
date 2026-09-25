#include "src/limbo/actor_system.hpp"

#include "src/limbo/game_canvas.hpp"
#include "src/limbo/game_data.hpp"
#include "src/limbo/magic_effects.hpp"
#include "src/limbo/sprite_atlas.hpp"

int32_t ActorSystem::distance(const SharedArray<int32_t> &nArray, const SharedArray<int32_t> &nArray2) {
    int32_t n = 0;
    int32_t n2 = 0;
    int32_t n3 = 0;
    int32_t n4 = 0;
    int32_t n5 = 0;
    int32_t n6 = 0;
    n5 = nArray[0] - nArray2[0];
    n6 = nArray[1] - nArray2[1];
    n5 = n5 < 0 ? -n5 : n5;
    n6 = n6 < 0 ? -n6 : n6;
    if (n5 < n6) {
        n3 = n5;
        n4 = n6;
    } else {
        n3 = n6;
        n4 = n5;
    }
    n2 = n4 * 1007 + n3 * 441;
    if (n4 < shiftLeft32(n3, 4)) {
        n2 -= n4 * 40;
    }
    n = (n2 + 512) >> 10;
    return wrappingAbs(n);
}

Actor *ActorSystem::find_nearest_enemy(Actor *j2) {
    Actor *j3 = nullptr;
    int32_t n = 0xFFFFFF;
    int32_t n2 = 0;
    int32_t n3 = 0;
    for (n3 = 0; n3 < GameCanvas::state().actors.length(); ++n3) {
        if (GameCanvas::state().actors[n3] == nullptr || GameCanvas::state().actors[n3]->is_dead == 1 ||
            GameCanvas::state().actors[n3]->faction_id == j2->faction_id || GameCanvas::state().actors[n3]->actor_kind == j2->actor_kind ||
            (n2 = ActorSystem::distance(j2->world_position, GameCanvas::state().actors[n3]->world_position)) >= n)
            continue;
        n = n2;
        j3 = GameCanvas::state().actors[n3];
    }
    return j3;
}

void ActorSystem::retreat_from(Actor *j2, Actor *j3) {
    int32_t n = j2->world_position[0] - j3->world_position[0];
    int32_t n2 = j2->world_position[1] - j3->world_position[1];
    if (wrappingAbs(n) > wrappingAbs(n2)) {
        if (n > 0) {
            ActorSystem::set_move_target(j2, j2->world_position[0] - 20, j2->world_position[1]);
            return;
        }
        ActorSystem::set_move_target(j2, j2->world_position[0] + 20, j2->world_position[1]);
        return;
    }
    if (n2 > 0) {
        ActorSystem::set_move_target(j2, j2->world_position[0], j2->world_position[1] - 20);
        return;
    }
    ActorSystem::set_move_target(j2, j2->world_position[0], j2->world_position[1] + 20);
}

void ActorSystem::face_actor(Actor *j2, Actor *j3) {
    if (j2->screen_position[0] < j3->screen_position[0] && j2->screen_position[1] > j3->screen_position[1]) {
        j2->facing = (int8_t)2;
        return;
    }
    if (j2->screen_position[0] > j3->screen_position[0] && j2->screen_position[1] < j3->screen_position[1]) {
        j2->facing = 1;
        return;
    }
    if (j2->screen_position[0] < j3->screen_position[0] && j2->screen_position[1] < j3->screen_position[1]) {
        j2->facing = (int8_t)3;
        return;
    }
    if (j2->screen_position[0] > j3->screen_position[0] && j2->screen_position[1] > j3->screen_position[1]) {
        j2->facing = (int8_t)4;
    }
}

bool ActorSystem::update_combat_target(Actor *j2) {
    Actor *j3 = ActorSystem::find_nearest_enemy(j2);
    int32_t n = 0;
    if (j3 != nullptr) {
        n = ActorSystem::distance(j2->world_position, j3->world_position);
        if (n <= j2->engage_radius) {
            if (n >= j2->disengage_radius) {
                if (j2->actor_kind != 1) {
                    ActorSystem::retreat_from(j2, j3);
                    return false;
                }
            } else {
                j2->move_target[0] = -1;
                j2->combat_target = j3;
                j2->anim_state = (int8_t)4;
                ActorSystem::face_actor(j2, j3);
            }
        } else if (j2->combat_target != nullptr && j2->ai_behavior != 2) {
            j2->move_target[0] = -1;
            j2->combat_target = nullptr;
            j2->anim_state = 0;
        }
    } else if (j2->combat_target != nullptr) {
        j2->combat_target = nullptr;
        j2->anim_state = 0;
    }
    return true;
}

void ActorSystem::apply_definition(Actor *j2, const SharedArray<int32_t> &nArray) {
    int32_t n = 0;
    if (j2 == nullptr || nArray.isNull()) {
        return;
    }
    j2->definition_record = nArray;
    j2->level = (int8_t)nArray[2];
    if (j2->actor_kind != 1) {
        j2->strength = (int16_t)nArray[3];
        j2->intelligence = (int16_t)nArray[4];
        j2->willpower = (int16_t)nArray[5];
        j2->agility = (int16_t)nArray[6];
        j2->move_speed = (int16_t)nArray[7];
        j2->endurance = (int16_t)nArray[8];
        j2->personality = (int16_t)nArray[9];
        j2->engage_radius = (int16_t)nArray[14];
        j2->disengage_radius = (int16_t)nArray[15];
        j2->equipped_weapon_id = (int8_t)nArray[10];
        j2->ai_behavior = (int8_t)nArray[18];
        n = nArray[11];
        j2->active_hand_record = GameCanvas::state().game_data->spell_table[nArray[19]];
        j2->bow_equipped = (int8_t)(j2->ai_behavior == 4 ? 1 : 0);
        if (nArray[20] > 0) {
            j2->attack_interval_ms = (int16_t)(nArray[20] * 1000);
        }
        if (j2->bow_equipped == 1 || j2->ai_behavior == 0) {
            j2->active_hand_record.setNull();
        }
        if (j2->equipped_weapon_id > 0) {
            ActorSystem::add_inventory_item(j2, 0, GameCanvas::state().game_data->get_record(4, j2->equipped_weapon_id));
        }
        if (n > 0) {
            ActorSystem::add_inventory_item(j2, 1, GameCanvas::state().game_data->get_record(1, n));
        }
    }
    j2->faction_id = (int8_t)nArray[13];
    j2->health = j2->max_health =
        (int16_t)(j2->level * 4 + (j2->strength + j2->strength_bonus) * 2 + j2->endurance * 2 + j2->health_capacity_bonus);
    j2->health_regen_interval_ms = (int16_t)(40000 / j2->max_health);
    j2->magicka = j2->max_magicka = (int16_t)(j2->level * 4 + j2->intelligence * 2 + j2->magicka_capacity_bonus);
    j2->magicka_regen_interval_ms = (int16_t)(40000 / j2->max_magicka);
    if (j2->engage_radius == 0) {
        j2->engage_radius = S().var_short_a;
    }
    if (j2->disengage_radius == 0) {
        j2->disengage_radius = S().var_short_b;
    }
    ActorSystem::recalculate_stats(j2);
}

bool ActorSystem::handle_teleport(Actor *j2) {
    int32_t n = 0;
    int32_t n2 = 0;
    int32_t n3 = 0;
    bool bl = false;
    int32_t n4 = 0;
    int32_t n5 = 0;
    SharedArray<int8_t> byArray;
    SharedArray<SharedArray<int8_t>> byArrayArray{SharedArray<int8_t>{-1, 0}, SharedArray<int8_t>{0, -1}, SharedArray<int8_t>{0, 0},
                                       SharedArray<int8_t>{0, 1}, SharedArray<int8_t>{1, 0}};
    if (j2->teleport_hidden == 1) {
        if (j2->teleport_timer_ms <= -1000 && GameCanvas::state().actors[0]->is_dead == 0) {
            byArray = GameCanvas::state().tile_layers[0];
            for (int32_t k = 0; !byArray.isNull() && !bl && k < 100; ++k) {
                bl = true;
                n = wrappingAbs(GameCanvas::state().actors[0]->world_position[0] + GameCanvas::state().random->nextInt() % 500);
                n2 = wrappingAbs(GameCanvas::state().actors[0]->world_position[1] + GameCanvas::state().random->nextInt() % 500);
                n4 = (n >> 7);
                n5 = (n2 >> 7);
                for (n3 = 0; n3 < byArrayArray.length(); ++n3) {
                    int32_t n6 = (n4 + byArrayArray[n3][0]) * GameCanvas::state().level_cols + (n5 + byArrayArray[n3][1]);
                    if (n6 < 0 || n6 >= byArray.length()) {
                        bl = false;
                        continue;
                    }
                    if (GameCanvas::state().tile_walkable[n6] == 0 && byArray[n6] != 0) continue;
                    bl = false;
                    goto continue_block0;
                }
            continue_block0:;
            }
            ActorSystem::set_position(j2, n, n2);
            MagicEffects::spawn(8, j2->world_position[0], j2->world_position[1]);
            if (bl) {
                j2->teleport_hidden = 0;
                return true;
            }
        }
    } else if (GameCanvas::state().actors[0]->is_dead == 0) {
        MagicEffects::spawn(8, j2->world_position[0], j2->world_position[1]);
        j2->move_target[0] = -1;
        j2->move_target[1] = -1;
        ActorSystem::set_position(j2, -10000, -10000);
        j2->teleport_hidden = 1;
    }
    return false;
}
