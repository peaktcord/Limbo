#ifndef OBLIVION_GAME_CANVAS_HPP
#define OBLIVION_GAME_CANVAS_HPP

#include <functional>
#include <vector>

#include "src/common/session.hpp"
#include "src/common/ui.hpp"
#include "src/game/port_settings.hpp"
#include "src/limbo/actor.hpp"
#include "src/limbo/actor_system.hpp"
#include "src/limbo/game_data.hpp"
#include "src/limbo/magic_effects.hpp"
#include "src/limbo/menu_node.hpp"
#include "src/limbo/menu_screen.hpp"
#include "src/limbo/oblivion_state.hpp"
#include "src/limbo/sprite_atlas.hpp"
#include "src/limbo/sprite_frame.hpp"
#include "src/limbo/text_resources.hpp"

class GameCanvas : public Canvas {
public:
    static constexpr int32_t original_interface_width = 176;
    static constexpr int32_t NO_KEY_CODE = GameCanvasStatics::NO_KEY_CODE;
    static constexpr int32_t ACTION_UP = 1;
    static constexpr int32_t ACTION_LEFT = 2;
    static constexpr int32_t ACTION_RIGHT = 5;
    static constexpr int32_t ACTION_DOWN = 6;
    static constexpr int32_t ACTION_FIRE = 8;
    static constexpr int32_t ACTION_UP_LEFT = 11;
    static constexpr int32_t ACTION_UP_RIGHT = 12;
    static constexpr int32_t ACTION_DOWN_LEFT = 13;
    static constexpr int32_t ACTION_DOWN_RIGHT = 14;
    static GameCanvasStatics &state();
    static void clear_pending_key();
    static bool has_pending_key();
    SharedArray<int8_t> visible_tile_min = SharedArray<int8_t>{0, 0};
    SharedArray<int8_t> visible_tile_max = SharedArray<int8_t>{0, 0};
    int8_t state_before_loading = -1;
    int8_t input_consumed = 0;
    int8_t camera_actor_slot = 0;
    int8_t loading_percent = 0;
    SharedArray<int32_t> camera_offset = SharedArray<int32_t>{0, 0};
    SharedArray<int16_t> tile_screen_positions;
    SharedArray<int8_t> tile_enter_script;
    SharedArray<int8_t> tile_exit_script;
    SharedArray<int32_t> var_int_arr_m = SharedArray<int32_t>(50);
    int8_t var_byte_s = 0;
    int8_t var_byte_t = 0;
    int8_t var_byte_u = 0;
    int16_t canvas_center_x = 0;
    int16_t canvas_center_y = 0;
    int16_t tile_world_step = 128;
    int16_t level_tile_count = 0;
    int32_t splash_frame = 0;
    int32_t splash_color = 0;
    int32_t highest_actor_slot = 0;
    bool player_input_enabled = true;
    bool game_loop_running = true;
    bool center_camera_on_target = false;
    bool modernized_dialogue_input = true;
    bool modern_movement = false;
    bool screen_aligned_movement = false;
    bool top_level_port_options = false;
    voyage::PortOptions port_options;
    std::function<void(const voyage::PortOptions &)> on_port_options_changed;
    bool confirm_input_down = false;
    bool dialogue_confirm_ready = true;
    int64_t text_transition_delay_ms = -1;
    int64_t exit_delay_ms = -1;
    bool scene_dirty = true;
    bool var_boolean_o = false;
    SpriteFrame *tile_atlas = nullptr;
    SpriteFrame *ui_atlas = nullptr;
    MenuScreen *menu_screen = nullptr;
    Actor *player = nullptr;
    Graphics *scene_graphics = nullptr;
    Image *scene_image = nullptr;
    Image *original_width_ui_image = nullptr;
    std::optional<std::string> crash_context;
    int32_t crash_key_presses = 0;
    std::vector<std::vector<std::string>> text_paragraphs;
    int16_t text_scroll_y = 0;
    int16_t text_scroll_ms = 0;
    bool text_scroll_at_end = false;
    bool paused = false;
    int16_t loading_spinner_ms = 0;
    SharedArray<int32_t> corner_world_tl = SharedArray<int32_t>{0, 0};
    SharedArray<int32_t> corner_world_tr = SharedArray<int32_t>{0, 0};
    SharedArray<int32_t> corner_world_bl = SharedArray<int32_t>{0, 0};
    SharedArray<int32_t> corner_world_br = SharedArray<int32_t>{0, 0};
    SharedArray<int32_t> corner_tile_tl = SharedArray<int32_t>{0, 0};
    SharedArray<int32_t> corner_tile_tr = SharedArray<int32_t>{0, 0};
    SharedArray<int32_t> corner_tile_bl = SharedArray<int32_t>{0, 0};
    SharedArray<int32_t> corner_tile_br = SharedArray<int32_t>{0, 0};
    bool dialogue_at_end = false;
    bool dialogue_open = false;
    std::vector<std::string> dialogue_lines;
    int32_t dialogue_box_width = 0;
    int32_t dialogue_text_x = 12;
    int32_t dialogue_text_y = 7;
    int32_t dialogue_scroll_y = 0;
    int32_t dialogue_wrap_width = 0;
    int32_t dialogue_box_height = 0;
    int64_t dialogue_opened_ms = 0L;

    GameCanvas(const std::string &startup_script, const std::string &menu_atlas,
               const std::string &version);

    static void reset_canvas_state();
    void clear_reference_pages();
    void build_frontend_menus();
    void load_level_script(const std::string &string);
    void resize_canvas(int32_t n, int32_t n2);
    void initialize_level_scene();
    void carve_floor_tile(const SharedArray<int8_t> &byArray, int32_t n, int32_t n2, int32_t n3);
    void carve_corridor(const SharedArray<int8_t> &byArray, const SharedArray<int32_t> &nArray, const SharedArray<int32_t> &nArray2, int32_t n, int32_t n2,
           const SharedArray<int32_t> &nArray3);
    void build_wall_layer(const SharedArray<int8_t> &byArray, const SharedArray<int8_t> &byArray2, const SharedArray<int32_t> &nArray);
    void carve_corridor_endpoints(const SharedArray<int8_t> &byArray, const SharedArray<int32_t> &nArray, const SharedArray<int32_t> &nArray2,
           const SharedArray<int32_t> &nArray3);
    void generate_level(const SharedArray<int32_t> &nArray, const SharedArray<int32_t> &nArray2, int32_t n, int32_t n2);
    void clear_tile_layers();
    void load_level_map(const std::string &string);
    void redraw_tile_background(Graphics *graphics);
    void paint(Graphics *graphics) override;
    void paint_legacy_contents(Graphics *graphics);
    static int32_t interface_width();
    static int32_t interface_left();
    bool tick(int64_t elapsed_ms);
    void update_status_message(int64_t l);
    void build_character_menu();
    void build_merchant_menu();
    void keyReleased(int32_t n) override;
    bool is_confirm_key_code(int32_t key_code);
    void set_confirm_input(bool down);
    static void set_game_state(int8_t by);
    static void layout_text_screen(const std::string &string);
    int32_t decode_input_action(int32_t key_or_action);
    void keyPressed(int32_t n) override;
    void reset_frontend_menu();
    void dispatch_pending_input(int64_t elapsed_ms);
    int32_t handle_gameplay_input(int32_t action, int64_t elapsed_ms);
    int32_t handle_merchant_menu_input(int32_t action);
    int32_t handle_character_menu_input(int32_t action);
    int32_t handle_frontend_menu_input(int32_t action);
    int32_t handle_control_menu_input(int32_t action);
    int32_t handle_port_options_menu_input(int32_t action);
    void apply_port_options(const voyage::PortOptions &options);
    int32_t handle_game_over_input(int32_t action);
    int32_t handle_load_confirmation_input(int32_t action);
    int32_t handle_acknowledgement_input(int32_t action);
    int32_t handle_new_game_confirmation_input(int32_t action);
    int32_t handle_reference_screen_input(int32_t action);
    int32_t handle_exit_confirmation_input(int32_t action);
    int32_t handle_scrollable_text_input(int32_t action, int64_t elapsed_ms);
    int32_t handle_invalid_control_input(int32_t action);
    int32_t handle_terminal_prompt_input(int32_t action);
    bool can_bind_key(int32_t n, int32_t n2);
    void update_visible_tile_bounds();
    void request_exit();
    void start_game_loop();
    void stop_game_loop();
    Actor *spawn_actor(const std::string &string, int32_t n, int32_t n2, const SharedArray<int32_t> &nArray);
    Actor *spawn_actor_in_slot(const std::optional<std::string> &string, const std::string &string2, int8_t by, int32_t n, int32_t n2, const SharedArray<int32_t> &nArray);
    static void player_died();
    static void remove_actor(int32_t n);
    void set_player_input_enabled(bool bl);
    void load_tile_atlas(const std::string &string);
    void show_splash_screen(int32_t n, int32_t n2);
    void set_tile_blocked(int32_t n, int32_t n2, bool bl);
    void set_tile_frame(int32_t n, int32_t n2, int32_t n3, int32_t n4);
    void center_camera_at(int32_t n, int32_t n2);
    void follow_actor(int32_t n);
    void update_camera();
    void set_tile_scripts(int32_t n, int32_t n2, int32_t n3, int32_t n4, int32_t n5);
    static int32_t load_resource(const std::string &path);
    static void update_loading_progress(int32_t n);
    void show_main_menu();
    static void set_status_message(const std::optional<std::string> &string, int32_t n, int32_t n2, int32_t n3);
    void load_ui_atlas(const std::string &string);
    void show_merchant_options();
    bool is_merchant_screen();
    void handle_menu_selection(MenuNode *c2);
    bool save_game();
    bool has_saved_player();
    bool load_game(bool enter_level);
    std::string persistence_error;
    void place_world_item(int32_t n, bool bl, int32_t n2, int32_t n3);
    static void show_text_screen(const std::string &string);
    static void set_respawn_position(int32_t n, int32_t n2);
    static void load_language(const std::string &string, int32_t n);
    static std::string get_text(int32_t string_id);
    static int32_t find_text_id(const std::string &string);
    void layout_dialogue(const std::string &string);
    static void format_wrapped_text(const std::string &string, std::vector<std::string> &lines, int32_t n);
    void open_dialogue();
    void draw_dialogue(Graphics *graphics);
    void handle_dialogue_input(int32_t n);
    static void set_speaker_name(const std::optional<std::string> &string);
    void hideNotify() override;
    void showNotify() override;
    bool is_terminal_state();

    static void initialize_session_state();
};

#endif
