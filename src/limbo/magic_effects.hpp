#ifndef OBLIVION_MAGIC_EFFECTS_HPP
#define OBLIVION_MAGIC_EFFECTS_HPP

#include "src/common/ui.hpp"
#include "src/limbo/oblivion_state.hpp"

class Actor;

class MagicEffects {
public:
    static constexpr int32_t SLOT_FIELDS = 9;
    static constexpr int32_t MAX_SLOTS = 11;
    static constexpr int32_t TOTAL_SLOT_FIELDS = MAX_SLOTS * SLOT_FIELDS;

    static constexpr int32_t OFFSET_TYPE = 0;
    static constexpr int32_t OFFSET_WORLD_X = 1;
    static constexpr int32_t OFFSET_WORLD_Y = 2;
    static constexpr int32_t OFFSET_FRAME_TIME = 3;
    static constexpr int32_t OFFSET_FRAME = 4;
    static constexpr int32_t OFFSET_ORIGIN_X = 5;
    static constexpr int32_t OFFSET_ORIGIN_Y = 6;
    static constexpr int32_t OFFSET_LIFETIME = 7;
    static constexpr int32_t OFFSET_LIFETIME_ELAPSED = 8;

    static constexpr int16_t FREE_SLOT = -1;
    static constexpr int16_t ATTACHED_FLAG = static_cast<int16_t>(0xF000);
    static constexpr int32_t ATTACHED_SIGN_EXTENDED = -4096;
    static constexpr int32_t EFFECT_ID_MASK = 0xFF;
    static constexpr int32_t ACTOR_INDEX_MASK = 0x0FFF;
    static constexpr int16_t FRAME_HIDDEN_MASK = static_cast<int16_t>(0xFF00);

    static constexpr int32_t FRAME_INTERVAL_MS = 100;
    static constexpr int32_t SPELL_PROJECTILE_SPEED = 60;
    static constexpr int32_t LIGHTNING_PROJECTILE_SPEED = 150;
    static constexpr int32_t PROJECTILE_MAX_TRAVEL_DIST = 750;
    static constexpr int32_t PROXIMITY_HIT_RADIUS = 200;
    static constexpr int32_t MAX_COMBAT_TARGETS = 25;

    static MagicEffectsStatics &S();

    static int32_t find_free_slot();
    static void clear_all();
    static void clear_slot(int32_t slot);
    static void remove_at_position(int32_t x, int32_t y);
    static int32_t spawn_attached(int32_t effect_id, Actor *actor);
    static int32_t spawn_attached(int32_t effect_id, Actor *actor, int32_t lifetime);
    static int32_t spawn_attached(int32_t effect_id, int32_t direction, Actor *actor);
    static int32_t spawn_attached(int32_t effect_id, int32_t direction, Actor *actor, int32_t lifetime);
    static int32_t spawn(int32_t effect_id, int32_t x, int32_t y);
    static int32_t spawn(int32_t effect_id, int32_t x, int32_t y, int32_t lifetime);
    static bool hit_nearby_target(int32_t slot);
    static void update(int64_t elapsed_ms);
    static void render(Graphics *graphics, const SharedArray<int32_t> &camera_offset);

    static void clinit();
};

#endif
