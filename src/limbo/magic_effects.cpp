#include "src/limbo/magic_effects.hpp"

#include <algorithm>

#include "src/limbo/actor_system.hpp"
#include "src/limbo/game_canvas.hpp"
#include "src/limbo/iso_transform.hpp"
#include "src/limbo/sprite_atlas.hpp"

MagicEffectsStatics &MagicEffects::S() { return oblivion_state().magic; }

namespace {

constexpr int32_t DIR_NORTH = 1;
constexpr int32_t DIR_SOUTH = 2;
constexpr int32_t DIR_EAST  = 3;
constexpr int32_t DIR_WEST  = 4;

struct EffectSlot {
    int16_t &type;
    int16_t &world_x;
    int16_t &world_y;
    int16_t &frame_time;
    int16_t &frame;
    int16_t &origin_x;
    int16_t &origin_y;
    int16_t &lifetime;
    int16_t &lifetime_elapsed;

    explicit EffectSlot(int16_t *base)
        : type(base[MagicEffects::OFFSET_TYPE]),
          world_x(base[MagicEffects::OFFSET_WORLD_X]),
          world_y(base[MagicEffects::OFFSET_WORLD_Y]),
          frame_time(base[MagicEffects::OFFSET_FRAME_TIME]),
          frame(base[MagicEffects::OFFSET_FRAME]),
          origin_x(base[MagicEffects::OFFSET_ORIGIN_X]),
          origin_y(base[MagicEffects::OFFSET_ORIGIN_Y]),
          lifetime(base[MagicEffects::OFFSET_LIFETIME]),
          lifetime_elapsed(base[MagicEffects::OFFSET_LIFETIME_ELAPSED]) {}

    bool is_active() const { return type != MagicEffects::FREE_SLOT; }
    bool is_attached() const { return (type & 0xFFFFF000) == MagicEffects::ATTACHED_SIGN_EXTENDED; }
    bool is_hidden() const { return (frame & MagicEffects::FRAME_HIDDEN_MASK) == MagicEffects::FRAME_HIDDEN_MASK; }
    int32_t effect_id() const { return type & MagicEffects::EFFECT_ID_MASK; }
    int32_t attached_actor_id() const { return (type & MagicEffects::ACTOR_INDEX_MASK) >> 8; }
};

}

void MagicEffects::clinit() {
    auto &state = S();
    if (state.clinit_done) return;
    state.clinit_done = true;
    state.effect_atlas = SpriteAtlas::load_atlas(std::string("/oh_magic.cml"));
    state.effect_slots = SharedArray<int16_t>(TOTAL_SLOT_FIELDS);
    for (int32_t i = 0; i < state.effect_slots.length(); ++i) {
        state.effect_slots[i] = FREE_SLOT;
    }
}

int32_t MagicEffects::find_free_slot() {
    clinit();
    auto &slots = S().effect_slots;
    for (int32_t slot_idx = 0; slot_idx < slots.length() - SLOT_FIELDS; slot_idx += SLOT_FIELDS) {
        if (slots[slot_idx + OFFSET_TYPE] == FREE_SLOT) {
            return slot_idx;
        }
    }
    return -1;
}

void MagicEffects::clear_all() {
    clinit();
    auto &slots = S().effect_slots;
    for (int32_t slot_idx = 0; slot_idx < slots.length(); slot_idx += SLOT_FIELDS) {
        clear_slot(slot_idx);
    }
}

void MagicEffects::clear_slot(int32_t slot_idx) {
    clinit();
    auto &slots = S().effect_slots;
    if (slot_idx < 0 || slot_idx + SLOT_FIELDS > slots.length()) {
        return;
    }
    for (int32_t i = 0; i < SLOT_FIELDS; ++i) {
        slots[slot_idx + i] = FREE_SLOT;
    }
}

void MagicEffects::remove_at_position(int32_t x, int32_t y) {
    clinit();
    auto &slots = S().effect_slots;
    for (int32_t slot_idx = 0; slot_idx < slots.length(); slot_idx += SLOT_FIELDS) {
        if (slots[slot_idx + OFFSET_TYPE] == FREE_SLOT) continue;
        EffectSlot slot(&slots[slot_idx]);
        if (slot.world_x == x && slot.world_y == y) {
            clear_slot(slot_idx);
            return;
        }
    }
}

int32_t MagicEffects::spawn_attached(int32_t effect_id, Actor *actor) {
    return spawn_attached(effect_id, 0, actor, 0);
}

int32_t MagicEffects::spawn_attached(int32_t effect_id, Actor *actor, int32_t lifetime) {
    return spawn_attached(effect_id, 0, actor, lifetime);
}

int32_t MagicEffects::spawn_attached(int32_t effect_id, int32_t direction, Actor *actor) {
    return spawn_attached(effect_id, direction, actor, 0);
}

int32_t MagicEffects::spawn_attached(int32_t effect_id, int32_t direction, Actor *actor, int32_t lifetime) {
    clinit();
    int32_t slot_idx = find_free_slot();
    if (slot_idx == -1) {
        return -1;
    }

    if (effect_id == 0) {
        if (direction == DIR_SOUTH) {
            effect_id = 0;
        } else if (direction == DIR_NORTH) {
            effect_id = 2;
        } else if (direction == DIR_EAST) {
            effect_id = 4;
        } else if (direction == DIR_WEST) {
            effect_id = 6;
        }
    } else if (effect_id == 11) {
        if (direction == DIR_SOUTH) {
            effect_id = 11;
        } else if (direction == DIR_NORTH) {
            effect_id = 12;
        } else if (direction == DIR_EAST) {
            effect_id = 13;
        } else if (direction == DIR_WEST) {
            effect_id = 14;
        }
    }

    auto &slots = S().effect_slots;
    EffectSlot slot(&slots[slot_idx]);
    slot.type = (int16_t)(ATTACHED_SIGN_EXTENDED | (actor->actor_kind << 8) | effect_id);
    slot.world_x = (int16_t)actor->world_position[0];
    slot.world_y = (int16_t)actor->world_position[1];
    slot.frame_time = 0;
    slot.frame = 0;
    slot.origin_x = (int16_t)actor->world_position[0];
    slot.origin_y = (int16_t)actor->world_position[1];
    slot.lifetime = (int16_t)lifetime;
    slot.lifetime_elapsed = 0;

    return slot_idx;
}

int32_t MagicEffects::spawn(int32_t effect_id, int32_t x, int32_t y) {
    return spawn(effect_id, x, y, 0);
}

int32_t MagicEffects::spawn(int32_t effect_id, int32_t x, int32_t y, int32_t lifetime) {
    clinit();
    int32_t slot_idx = find_free_slot();
    if (slot_idx == -1) {
        return -1;
    }

    auto &slots = S().effect_slots;
    EffectSlot slot(&slots[slot_idx]);
    slot.type = (int16_t)effect_id;
    slot.world_x = (int16_t)x;
    slot.world_y = (int16_t)y;
    slot.frame_time = 0;
    slot.frame = 0;
    slot.origin_x = (int16_t)x;
    slot.origin_y = (int16_t)y;
    slot.lifetime = (int16_t)lifetime;
    slot.lifetime_elapsed = 0;

    return slot_idx;
}

bool MagicEffects::hit_nearby_target(int32_t slot_idx) {
    auto &slots = S().effect_slots;
    EffectSlot slot(&slots[slot_idx]);

    int32_t source_actor_kind = slot.attached_actor_id();
    auto &actors = GameCanvas::state().actors;
    if (source_actor_kind <= 0 || source_actor_kind >= actors.length()) {
        clear_slot(slot_idx);
        return false;
    }

    Actor *source_actor = actors[source_actor_kind - 1];
    if (source_actor == nullptr) {
        clear_slot(slot_idx);
        return false;
    }

    SharedArray<int32_t> effect_pos{slot.world_x, slot.world_y};
    int32_t closest_target_index = -1;
    int32_t min_distance = 0xFFFFFF;

    const int32_t max_targets = std::min<int32_t>(MAX_COMBAT_TARGETS, actors.length());
    for (int32_t i = 0; i < max_targets; ++i) {
        Actor *candidate = actors[i];
        if (candidate == nullptr || candidate->is_dead == 1 || candidate == source_actor) {
            continue;
        }
        if (candidate->faction_id == source_actor->faction_id) {
            continue;
        }
        int32_t dist = ActorSystem::distance(effect_pos, candidate->world_position);
        if (dist >= PROXIMITY_HIT_RADIUS || dist >= min_distance) {
            continue;
        }
        closest_target_index = i;
        min_distance = dist;
    }

    if (closest_target_index != -1) {
        ActorSystem::attack(source_actor, actors[closest_target_index], false);
        return true;
    }
    return false;
}

void MagicEffects::update(int64_t elapsed_ms) {
    clinit();
    SharedArray<int32_t> current_pos{0, 0};
    SharedArray<int32_t> origin_pos{0, 0};

    auto &slots = S().effect_slots;
    for (int32_t slot_idx = 0; slot_idx < slots.length(); slot_idx += SLOT_FIELDS) {
        if (slots[slot_idx + OFFSET_TYPE] == FREE_SLOT) {
            continue;
        }

        EffectSlot slot(&slots[slot_idx]);
        slot.frame_time = (int16_t)((int64_t)slot.frame_time + elapsed_ms);
        slot.lifetime_elapsed = (int16_t)((int64_t)slot.lifetime_elapsed + elapsed_ms);

        if (slot.lifetime > 0 && slot.lifetime_elapsed >= slot.lifetime) {
            slot.frame = 0;
            slot.lifetime_elapsed = 0;
        }

        if (slot.is_hidden() || slot.frame_time <= FRAME_INTERVAL_MS) {
            continue;
        }

        slot.frame_time = 0;
        slot.frame = (int16_t)(slot.frame + 1);

        int32_t effect_id = slot.effect_id();

        if ((effect_id >= 0 && effect_id <= 6) || (effect_id >= 11 && effect_id <= 14)) {
            switch (effect_id) {
            case 0:  slot.world_y -= SPELL_PROJECTILE_SPEED; break;
            case 2:  slot.world_y += SPELL_PROJECTILE_SPEED; break;
            case 4:  slot.world_x += SPELL_PROJECTILE_SPEED; break;
            case 6:  slot.world_x -= SPELL_PROJECTILE_SPEED; break;
            case 11: slot.world_y -= LIGHTNING_PROJECTILE_SPEED; break;
            case 12: slot.world_y += LIGHTNING_PROJECTILE_SPEED; break;
            case 13: slot.world_x += LIGHTNING_PROJECTILE_SPEED; break;
            case 14: slot.world_x -= LIGHTNING_PROJECTILE_SPEED; break;
            default: break;
            }

            if (SpriteAtlas::set_animation_frame(S().effect_atlas, effect_id, slot.frame)) {
                if (effect_id == 1 || effect_id == 3 || effect_id == 5 || effect_id == 7) {
                    clear_slot(slot_idx);
                } else {
                    slot.frame = 0;
                    SpriteAtlas::set_animation_frame(S().effect_atlas, effect_id, 0);
                }
            }

            current_pos[0] = slot.world_x;
            current_pos[1] = slot.world_y;
            origin_pos[0] = slot.origin_x;
            origin_pos[1] = slot.origin_y;

            if (ActorSystem::distance(current_pos, origin_pos) <= PROJECTILE_MAX_TRAVEL_DIST &&
                !hit_nearby_target(slot_idx)) {
                continue;
            }

            if (effect_id >= 11 && effect_id <= 14) {
                clear_slot(slot_idx);
                continue;
            }

            if (effect_id == 0 || effect_id == 2 || effect_id == 4 || effect_id == 6) {
                slot.type = (int16_t)(slot.type + 1);
            }
            continue;
        }

        if (slot.is_attached()) {
            int32_t actor_idx = slot.attached_actor_id() - 1;
            auto &actors = GameCanvas::state().actors;
            if (actor_idx < 0 || actor_idx >= actors.length() || actors[actor_idx] == nullptr) {
                clear_slot(slot_idx);
                continue;
            }
            slot.world_x = (int16_t)actors[actor_idx]->world_position[0];
            slot.world_y = (int16_t)actors[actor_idx]->world_position[1];
        }

        if (!SpriteAtlas::set_animation_frame(S().effect_atlas, effect_id, slot.frame)) {
            continue;
        }

        if (slot.lifetime <= 0) {
            clear_slot(slot_idx);
        } else {
            slot.frame = (int16_t)(slot.frame | FRAME_HIDDEN_MASK);
        }
    }
}

void MagicEffects::render(Graphics *graphics, const SharedArray<int32_t> &camera_offset) {
    clinit();
    SharedArray<int32_t> world_pos{0, 0};
    SharedArray<int32_t> screen_pos{0, 0};

    auto &slots = S().effect_slots;
    const int32_t canvas_width = GameCanvas::state().canvas_width;
    const int32_t canvas_height = GameCanvas::state().canvas_height;
    SpriteFrame *atlas = S().effect_atlas;

    for (int32_t slot_idx = 0; slot_idx < slots.length(); slot_idx += SLOT_FIELDS) {
        if (slots[slot_idx + OFFSET_TYPE] == FREE_SLOT) {
            continue;
        }

        EffectSlot slot(&slots[slot_idx]);
        if (slot.is_hidden()) {
            continue;
        }

        world_pos[0] = slot.world_x;
        world_pos[1] = slot.world_y;
        IsoTransform::world_to_screen(world_pos, screen_pos);

        int32_t draw_x = screen_pos[0] + camera_offset[0];
        int32_t draw_y = screen_pos[1] + camera_offset[1];

        if (draw_x < 0 || draw_x > canvas_width || draw_y < 0 || draw_y > canvas_height) {
            continue;
        }

        int32_t effect_id = slot.effect_id();
        SpriteAtlas::set_animation_frame(atlas, effect_id, slot.frame);
        SpriteAtlas::draw_frame(graphics, atlas, effect_id, draw_x, draw_y);
    }
}
