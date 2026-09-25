#ifndef OBLIVION_ACTOR_SYSTEM_HPP
#define OBLIVION_ACTOR_SYSTEM_HPP

#include "src/common/ui.hpp"
#include "src/limbo/actor.hpp"
#include "src/limbo/oblivion_state.hpp"

class GameData;

class ActorSystem {
public:
    static constexpr int32_t ACTION_QUICK_POTION_HEALTH = 0;
    static constexpr int32_t ACTION_QUICK_POTION_MAGICKA = 1;
    static constexpr int32_t ACTION_TOGGLE_HAND = 2;
    static constexpr int32_t ACTION_UP = 3;
    static constexpr int32_t ACTION_DOWN = 4;
    static constexpr int32_t ACTION_LEFT = 5;
    static constexpr int32_t ACTION_RIGHT = 6;
    static constexpr int32_t ACTION_USE = 7;
    static constexpr int32_t ACTION_UP_LEFT = 11;
    static constexpr int32_t ACTION_UP_RIGHT = 12;
    static constexpr int32_t ACTION_DOWN_LEFT = 13;
    static constexpr int32_t ACTION_DOWN_RIGHT = 14;

    static constexpr int32_t DIR_DOWN_LEFT = 1;
    static constexpr int32_t DIR_UP_RIGHT = 2;
    static constexpr int32_t DIR_DOWN_RIGHT = 3;
    static constexpr int32_t DIR_UP_LEFT = 4;

    static constexpr int8_t FACING_DOWN_LEFT = 1;
    static constexpr int8_t FACING_UP_RIGHT = 2;
    static constexpr int8_t FACING_DOWN_RIGHT = 3;
    static constexpr int8_t FACING_UP_LEFT = 4;

    static constexpr int8_t ANIM_STATE_IDLE = 0;
    static constexpr int8_t ANIM_STATE_WALK = 1;

    static ActorSystemStatics &S();

    static Actor *create_actor(const std::string &sprite_path, int8_t actor_type);
    static void reset_actor(Actor *actor);
    static void update_screen_position(Actor *actor);
    static void update_footprint_tiles(Actor *actor);
    static bool is_colliding(Actor *actor);
    static bool is_corner_colliding(Actor *actor, int8_t corner);
    static bool tile_index_out_of_range(const SharedArray<int8_t> &corner);
    static void move_direction(Actor *actor, int32_t direction, int64_t elapsed_ms);
    static void move_diagonal(Actor *actor, int32_t dx_dir, int32_t dy_dir, int64_t elapsed_ms);
    static void move_by(Actor *actor, int32_t dx, int32_t dy);
    static void set_position(Actor *actor, int32_t x, int32_t y);
    static void update_tile_position(Actor *actor);
    static void revert_movement(Actor *actor);
    static int8_t check_tile_triggers(Actor *actor, const SharedArray<int8_t> &enter_triggers, const SharedArray<int8_t> &exit_triggers);
    static void set_move_target(Actor *actor, int32_t target_x, int32_t target_y);
    static void update(Actor *actor, int64_t delta_ms, bool can_auto_attack);
    static void render_actor(Actor *actor, Graphics *graphics, const SharedArray<int32_t> &camera_offset);
    static void set_anim_state(Actor *actor, int8_t anim_state);
    static void recalculate_stats(Actor *actor);
    static int32_t distance(const SharedArray<int32_t> &first, const SharedArray<int32_t> &second);
    static Actor *find_nearest_enemy(Actor *actor);
    static void retreat_from(Actor *actor, Actor *target);
    static void face_actor(Actor *actor, Actor *target);
    static bool update_combat_target(Actor *actor);
    static void apply_definition(Actor *actor, const SharedArray<int32_t> &record);
    static bool apply_damage(int32_t damage, Actor *target, Actor *attacker, bool is_critical, bool ignore_defense);
    static bool handle_teleport(Actor *actor);
    static bool attack(Actor *attacker, Actor *defender, bool use_special_action);
    static void award_experience(Actor *actor, int32_t victim_level);
    static void apply_class_level_bonuses(Actor *actor);
    static int8_t check_tile_use_trigger(Actor *actor, const SharedArray<int8_t> &use_triggers);
    static void cast_spell(Actor *actor, bool pay_magicka_cost);
    static void set_death_script(Actor *actor, int32_t unused, int32_t script_id);
    static void clear_death_script(Actor *actor, int32_t unused);
    static void set_attribute(Actor *actor, int32_t attribute_id, int32_t value, GameData *game_data);
    static void add_inventory_item(Actor *actor, int32_t item_kind, const SharedArray<int32_t> &item_record);
    static void add_inventory_item(Actor *actor, int32_t item_kind, const SharedArray<int32_t> &item_record, bool force_equip);
    static void equip_best_weapon(Actor *actor);
    static void remove_inventory_item(Actor *actor, int32_t item_kind, const SharedArray<int32_t> &item_record);
    static int32_t frame_height(Actor *actor);
    static int32_t frame_width(Actor *actor);
    static void apply_class(Actor *actor, int8_t class_id, bool skip_starting_kit);
    static void set_status_icon(Actor *actor, int8_t icon_type);
    static void use_potion(Actor *actor, const SharedArray<int32_t> &potion_record);
    static void use_quick_potion(Actor *actor, bool is_health_potion);
    static bool equip_by_name(Actor *actor, const std::string &item_name);
    static void apply_damage_over_time(Actor *attacker, Actor *target, int32_t damage, int32_t duration_ms);
    static void apply_spell_damage(Actor *attacker, Actor *target, int32_t damage);
    static void equip_armour(Actor *actor, const SharedArray<int32_t> &armour_record);
    static bool is_armour_equipped(Actor *actor, int32_t armour_id);
    static bool is_weapon_or_spell_equipped(Actor *actor, int32_t weapon_id, bool is_spell);
    static bool is_weapon_or_spell_equipped(Actor *actor, const SharedArray<int32_t> &item_record, bool is_spell);
    static void set_loot_drop_enabled(Actor *actor, bool enabled);
    static void level_up_to(Actor *actor, int32_t target_level);
    static bool handle_input(Actor *actor, int32_t action, int64_t elapsed_ms, bool screen_aligned = false);
    static void update_hand_icon(Actor *actor);
    static bool can_use_item(Actor *actor, int32_t item_kind, const SharedArray<int32_t> &item_record);

    static void clinit();
};

#endif
