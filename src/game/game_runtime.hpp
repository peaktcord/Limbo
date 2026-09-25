#ifndef VOYAGE_GAME_RUNTIME_HPP
#define VOYAGE_GAME_RUNTIME_HPP

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "src/game/port_settings.hpp"

struct SessionContext;

namespace voyage {

constexpr int32_t kChromeHeight = 25;
constexpr int32_t kScreenHeight = 220;
constexpr int32_t kPhoneWidth = 176;
constexpr int32_t kWideWidth = 391;
constexpr int32_t kSurfaceWidth = kWideWidth;

enum class Viewport { Phone, Wide };

enum class Input {
    Up,
    Down,
    Left,
    Right,
    UpLeft,
    UpRight,
    DownLeft,
    DownRight,
    Action,
    Menu,
    Character,
    QuickHealth,
    QuickMagicka,
    ToggleWeapon,
    Digit0,
    Digit1,
    Digit2,
    Digit3,
    Digit4,
    Digit5,
    Digit6,
    Digit7,
    Digit8,
    Digit9,
};

struct RuntimeConfig {
    std::string resource_dir = "artifacts/extracted/v1010";
    std::string save_dir = "saves/rms";
    Viewport viewport = Viewport::Phone;
    bool centered_camera = false;
    bool modern_movement = false;
    bool screen_aligned_movement = false;
    std::optional<int64_t> random_seed;
    std::function<void()> on_save_persisted;
    std::string settings_dir = "settings";
    PortOptions port_options;
    std::optional<bool> modernized_dialogue_input;
    bool top_level_port_options = false;
};

struct Frame {
    int32_t logical_width = 0;
    int32_t height = 0;
    int32_t surface_width = 0;
    std::vector<uint32_t> pixels;
};

struct RuntimeActorSnapshot {
    int32_t slot = -1;
    bool alive = false;
    bool hostile_to_player = false;
    bool attacking = false;
    int32_t world_x = 0;
    int32_t world_y = 0;
    int32_t health = 0;
    int32_t max_health = 0;
    int32_t target_actor = -1;
};

struct RuntimeSnapshot {
    int32_t state = -1;
    std::string level_id;
    bool player_present = false;
    int32_t player_world_x = 0;
    int32_t player_world_y = 0;
    int32_t player_screen_x = 0;
    int32_t player_screen_y = 0;
    int32_t camera_offset_x = 0;
    int32_t camera_offset_y = 0;
    int32_t camera_target_actor = -1;
    std::vector<RuntimeActorSnapshot> actors;
};

class GameRuntime final {
public:
    explicit GameRuntime(RuntimeConfig config);
    ~GameRuntime();

    GameRuntime(const GameRuntime &) = delete;
    GameRuntime &operator=(const GameRuntime &) = delete;

    void start();
    void stop();
    bool running() const { return running_; }
    void pause();
    void resume();
    bool paused() const;

    void tick(uint32_t elapsed_ms);
    uint64_t simulation_time_ms() const { return simulation_time_ms_; }

    void press(Input input);
    void release(Input input);
    void set_viewport(Viewport viewport);
    void set_centered_camera(bool centered);
    void set_modern_movement(bool enabled);
    void set_screen_aligned_movement(bool enabled);

    int32_t width() const;
    int32_t height() const { return kScreenHeight; }
    bool centered_camera() const { return config_.centered_camera; }
    bool modern_movement() const;
    bool screen_aligned_movement() const;
    Frame copy_frame() const;

    int32_t state() const;
    RuntimeSnapshot inspect_state() const;
    std::string state_name() const;
    std::string script_status() const;
    bool has_error() const;
    std::string error_message() const;

    static bool is_movement(Input input);

    SessionContext &session();

    const PortOptions &port_options() const { return config_.port_options; }
    void apply_port_options(const PortOptions &options);
    using PortOptionsListener = std::function<void(const PortOptions &)>;
    void set_port_options_listener(PortOptionsListener listener);

private:
    void handle_canvas_port_options_changed(const PortOptions &options);

    RuntimeConfig config_;
    class Impl;
    Impl *impl_ = nullptr;
    bool running_ = false;
    uint64_t simulation_time_ms_ = 0;
    PortOptionsListener port_options_listener_;
};

}

#endif
