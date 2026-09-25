#ifndef OBLIVION_GAME_DATA_HPP
#define OBLIVION_GAME_DATA_HPP

#include "src/common/session.hpp"
#include "src/limbo/oblivion_state.hpp"

class GameCanvas;

class GameData {
public:
    static constexpr int32_t TRIGGER_UP = 3;
    static constexpr int32_t TRIGGER_DOWN = 4;
    static constexpr int32_t TRIGGER_LEFT = 5;
    static constexpr int32_t TRIGGER_RIGHT = 6;
    static constexpr int32_t TRIGGER_ACTION = 7;

    static GameDataStatics &S();
    SharedArray<int32_t> frame_pc = SharedArray<int32_t>(10);
    SharedArray<int32_t> frame_script_id = SharedArray<int32_t>(10);
    SharedArray<int32_t> input_trigger_scripts = SharedArray<int32_t>(8);
    SharedArray<int32_t> script_entry_offsets = SharedArray<int32_t>(256);
    SharedArray<int32_t> waiting_for_actors;
    SharedArray<int32_t> merchant_inventory = SharedArray<int32_t>(100);
    SharedArray<int32_t> dungeon_item_drops = SharedArray<int32_t>(10);
    SharedArray<int32_t> scripted_walk_target;
    SharedArray<SharedArray<int32_t>> script_table = makeSharedArray2D<int32_t>(25, 21);
    SharedArray<SharedArray<int32_t>> default_script_table = makeSharedArray2D<int32_t>(25, 21);
    SharedArray<SharedArray<int32_t>> weapon_table = makeSharedArray2D<int32_t>(37, 8);
    SharedArray<SharedArray<int32_t>> armour_table = makeSharedArray2D<int32_t>(42, 10);
    SharedArray<SharedArray<int32_t>> potion_table = makeSharedArray2D<int32_t>(11, 14);
    SharedArray<SharedArray<int32_t>> level_gen_table = makeSharedArray2D<int32_t>(10, 21);
    SharedArray<SharedArray<int32_t>> world_item_table = makeSharedArray2D<int32_t>(25, 7);
    SharedArray<SharedArray<int32_t>> creature_table = makeSharedArray2D<int32_t>(9, 15);
    SharedArray<SharedArray<int32_t>> class_skills_table = makeSharedArray2D<int32_t>(9, 15);
    SharedArray<SharedArray<int32_t>> class_spells_table = makeSharedArray2D<int32_t>(9, 15);
    SharedArray<SharedArray<int32_t>> spell_table = makeSharedArray2D<int32_t>(10, 15);
    SharedArray<SharedArray<int32_t>> loot_drop_table = makeSharedArray2D<int32_t>(30, 4);
    int32_t call_depth = 0;
    int32_t script_timer_ms = 0;
    int32_t script_timer_target_ms = -1;
    int32_t string_pool_count = 0;
    int32_t dungeon_item_drop_count = 0;
    GameCanvas *game_canvas = nullptr;
    SharedArray<std::string> string_pool = SharedArray<std::string>(255);
    SharedArray<int8_t> script_bytecode;
    int8_t scripted_walk_axis = 0;
    int8_t scripted_walk_actor = 0;
    int8_t interpreter_state = 0;
    int8_t pending_move_actor = -1;
    bool waiting_for_key = false;
    bool initial_load = true;

    explicit GameData(GameCanvas *game);

    void reset();
    void load(const std::string &path);
    int32_t decode_script_record(int32_t offset);
    int32_t decode_armour_record(int32_t offset);
    int32_t decode_potion_record(int32_t offset);
    int32_t decode_weapon_record(int32_t offset);
    int32_t decode_creature_record(int32_t offset);
    int32_t decode_world_item_record(int32_t offset);
    int32_t decode_merchant_inventory(int32_t offset);
    int32_t decode_spell_record(int32_t offset);
    int32_t decode_level_gen_record(int32_t offset);
    int32_t decode_loot_drop_record(int32_t offset);
    int32_t read_script_byte();
    int32_t read_script_int24();
    int32_t read_script_int16();
    std::string read_script_text(int32_t n);
    void execute_script_step(int64_t elapsed_ms);
    static void call_script(int32_t script_id);
    void return_from_script();
    void update_scripts(int64_t elapsed_ms);
    SharedArray<int32_t> get_record(int32_t table_id, int32_t record_id);
    SharedArray<int32_t> get_item_record(int32_t item_kind, int32_t record_id);
    void handle_input_trigger(int32_t key_action);
    std::string get_string(int32_t string_id);
    int32_t resolve_string_id(const std::string &name);
    SharedArray<int32_t> find_record(const std::string &name);
    int32_t find_item_kind_by_name(const std::string &name);
    int32_t roll_loot_drop();
    bool class_has_skill(int32_t class_id, int32_t skill_id);
    static std::optional<std::string> effect_name(int32_t n);

    static void clinit();
};

#endif
