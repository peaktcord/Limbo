#ifndef OBLIVION_STATE_HPP
#define OBLIVION_STATE_HPP

#include <vector>

#include "src/common/session.hpp"

class Actor;
class GameCanvas;
class GameData;
class Image;
class MenuNode;
class SpriteFrame;

struct GameCanvasStatics {
    int8_t tile_width = 0;
    int8_t tile_height = 0;
    int8_t character_softkey_code = 0;
    int8_t menu_softkey_code = 0;
    int8_t var_byte_c = 0;
    int8_t var_byte_d = 0;
    int8_t var_byte_e = 0;
    GameRandom *random = nullptr;
    SharedArray<int8_t> menu_selection;
    int8_t menu_page = 0;
    int8_t status_scroll_style = 0;
    int32_t status_duration_ms = 0;
    int32_t status_elapsed_ms = 0;
    int32_t status_color = 0;
    int32_t status_x = 0;
    int32_t status_y = 0;
    int32_t status_blink_ms = 0;
    int32_t status_scroll_ms = 0;
    int32_t control_row = 0;
    int32_t port_options_row = 0;
    int32_t hud_blink_ms = 0;
    static constexpr int32_t NO_KEY_CODE = 0xEEEEEEEE;
    int32_t pending_key_code = NO_KEY_CODE;
    void clear_pending_key() { pending_key_code = NO_KEY_CODE; }
    bool has_pending_key() const { return pending_key_code != NO_KEY_CODE; }
    bool hud_blink_on = false;
    int16_t canvas_width = 0;
    int16_t canvas_height = 0;
    int8_t level_rows = 0;
    int8_t level_cols = 0;
    bool state_changes_enabled = false;
    int8_t game_state = 0;
    int8_t suspended_game_state = 0;
    SharedArray<int8_t> tile_walkable;
    bool text_screen_dwelled = false;
    bool awaiting_key_bind = false;
    bool status_blink_off = false;
    bool var_boolean_c = false;
    SharedArray<Actor *> actors;
    GameCanvas *instance = nullptr;
    GameData *game_data = nullptr;
    std::vector<SharedArray<int8_t>> tile_layers;
    SharedArray<SharedArray<std::string>> menu_pages;
    SharedArray<std::string> control_rows;
    std::optional<std::string> status_text;
    std::optional<std::string> level_script_path;
    SharedArray<int8_t> tile_use_script;
    int32_t player_gold = 0;
    std::string version_text;
    int32_t scene_clear_color = 0;
    int16_t respawn_x = 0;
    int16_t respawn_y = 0;
    SpriteFrame *loading_spinner_atlas = nullptr;
    SharedArray<int8_t> world_item_entries;
    int8_t world_item_count = 0;
    bool hud_visible = false;
    SharedArray<SharedArray<std::optional<std::string>>> reference_pages;
    int8_t reference_page = 0;
    int8_t reference_scroll_row = 0;
    int16_t reference_title_id = 0;
    bool reference_has_more = false;
    int8_t menu_page_before_options = 0;
    MenuNode *equipped_weapon_node = nullptr;
    MenuNode *equipped_spell_node = nullptr;
    Image *splash_image = nullptr;
    Image *title_image = nullptr;
    bool var_boolean_f = false;
    std::optional<std::string> speaker_name;
};

struct ActorSystemStatics {
    int8_t var_byte_a = 0;
    int8_t var_byte_b = 0;
    int16_t var_short_a = 0;
    int16_t var_short_b = 0;
    SharedArray<int8_t> anim_frame_offsets;
    SharedArray<int16_t> level_xp_thresholds;
    SharedArray<int16_t> level_xp_awards;
    SharedArray<int32_t> var_int_arr_a;
    SharedArray<int32_t> var_int_arr_b;
    SharedArray<int32_t> var_int_arr_c;
    SharedArray<int32_t> var_int_arr_d;
    SharedArray<int32_t> var_int_arr_e;
};

struct GameDataStatics {
    SharedArray<SharedArray<int8_t>> class_stat_growth_table;
    GameData *instance = nullptr;
};

struct MenuScreenStatics {
    int8_t row_height = 0;
    bool needs_rebuild = false;
};

struct MagicEffectsStatics {
    SpriteFrame *effect_atlas = nullptr;
    SharedArray<int16_t> effect_slots;
    bool clinit_done = false;
};

struct SpriteAtlasStatics {
    int8_t var_byte_a = 0;
    int8_t var_byte_b = 0;
};

struct TextResourcesStatics {
    SharedArray<uint16_t> language_data;
    bool default_language_loaded = false;
};

struct OblivionState {
    GameCanvasStatics canvas;
    ActorSystemStatics actors;
    GameDataStatics data;
    MenuScreenStatics menu;
    MagicEffectsStatics magic;
    SpriteAtlasStatics atlas;
    TextResourcesStatics text;
    bool initialized = false;
};

OblivionState &oblivion_state();

#endif
