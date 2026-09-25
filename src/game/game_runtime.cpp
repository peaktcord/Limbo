#include "src/game/game_runtime.hpp"

#include <mutex>
#include <optional>
#include <utility>

#include "src/storage/resource_store.hpp"
#include "src/storage/save_store.hpp"
#include "src/common/render/render.hpp"
#include "src/limbo/crash_report.hpp"
#include "src/limbo/game_canvas.hpp"
#include "src/limbo/key_bindings.hpp"
#include "src/limbo/statics.hpp"
#include "src/game/input_mapper.hpp"
#include "src/game/state_inspector.hpp"

namespace voyage {

class GameRuntime::Impl {
public:
    SessionContext session;
    render::Surface screen{kSurfaceWidth, kScreenHeight};
    render::SoftRenderer renderer{&screen};
    std::optional<voyage::storage::DirectoryResourceStore> resources;
    std::optional<voyage::storage::DirectorySaveStore> saves;
    GameCanvas *canvas = nullptr;
    GameRuntime *runtime = nullptr;

    void clear() {
        session.reset();
        renderer.forgetImages();
        canvas = nullptr;
    }

    void boot(const RuntimeConfig &config, int32_t canvas_width) {
        resources.emplace(config.resource_dir);
        saves.emplace(config.save_dir, config.on_save_persisted);
        session.resources = &*resources;
        session.saves = &*saves;
        session.paint = &renderer;
        session.set_canvas_size(canvas_width, kScreenHeight - kChromeHeight);
        oblivion_init_statics();
        if (config.random_seed) GameCanvas::state().random->setSeed(*config.random_seed);

        canvas = session_new<GameCanvas>("/startup.scr", "/oh_menu.cml", "1.0.10");
        Display::getDisplay()->setCurrent(canvas);
        canvas->port_options = config.port_options;
        canvas->center_camera_on_target = config.centered_camera;
        canvas->modernized_dialogue_input = config.modernized_dialogue_input.value_or(config.viewport == Viewport::Wide);
        canvas->modern_movement = config.modern_movement;
        canvas->screen_aligned_movement = config.screen_aligned_movement;
        canvas->top_level_port_options = config.top_level_port_options;
        canvas->on_port_options_changed = [this](const PortOptions &options) {
            if (runtime != nullptr) {
                runtime->handle_canvas_port_options_changed(options);
            }
        };
        canvas->start_game_loop();
    }
};

GameRuntime::GameRuntime(RuntimeConfig config) : config_(std::move(config)), impl_(new Impl()) {
    impl_->runtime = this;
    if (config_.port_options.widescreen) config_.viewport = Viewport::Wide;
    if (config_.port_options.centered_camera) config_.centered_camera = true;
    if (config_.port_options.modern_movement) config_.modern_movement = true;
    if (config_.port_options.screen_aligned_movement) config_.screen_aligned_movement = true;

    if (config_.viewport == Viewport::Wide) config_.port_options.widescreen = true;
    if (config_.centered_camera) config_.port_options.centered_camera = true;
    if (config_.modern_movement) config_.port_options.modern_movement = true;
    if (config_.screen_aligned_movement) config_.port_options.screen_aligned_movement = true;
}

GameRuntime::~GameRuntime() {
    {
        SessionScope session(impl_->session);
        stop();
        Display::getDisplay()->setCurrent(nullptr);
    }
    delete impl_;
}

SessionContext &GameRuntime::session() { return impl_->session; }

void GameRuntime::start() {
    SessionScope session(impl_->session);
    if (running_) return;
    if (impl_->session.objects.size() != 0) impl_->clear();
    impl_->session.clock.pin(0);
    simulation_time_ms_ = 0;
    try {
        logging::write(logging::Level::Info, "Runtime", "Starting game; resources=" + config_.resource_dir +
                       "; saves=" + config_.save_dir + "; viewport=" + std::to_string(width()) + "x220");
        update_game_log_context(nullptr, "starting game");
        impl_->boot(config_, width());
        running_ = true;
        tick(0);
    } catch (...) {
        running_ = false;
        impl_->clear();
        throw;
    }
}

void GameRuntime::stop() {
    SessionScope session(impl_->session);
    if (!running_) return;
    impl_->canvas->stop_game_loop();
    Display::getDisplay()->setCurrent(nullptr);
    logging::write(logging::Level::Info, "Runtime", "Game stopped");
    running_ = false;
    impl_->session.clock.release();
}

void GameRuntime::pause() {
    SessionScope session(impl_->session);
    if (!running_ || impl_->canvas->is_terminal_state() || impl_->canvas->paused) return;
    impl_->canvas->paused = true;
    logging::write(logging::Level::Info, "Runtime", "Game paused");
}

void GameRuntime::resume() {
    SessionScope session(impl_->session);
    if (!running_ || impl_->canvas->is_terminal_state() || !impl_->canvas->paused) return;
    impl_->canvas->paused = false;
    logging::write(logging::Level::Info, "Runtime", "Game resumed");
}

bool GameRuntime::paused() const {
    SessionScope session(impl_->session);
    return running_ && impl_->canvas->paused;
}

void GameRuntime::tick(uint32_t elapsed_ms) {
    SessionScope session(impl_->session);
    if (!running_) return;
    simulation_time_ms_ += elapsed_ms;
    impl_->session.clock.advance((int64_t)elapsed_ms);
    update_game_log_context(impl_->canvas, "ticking");
    const int32_t previous_state = state();
    impl_->canvas->tick((int64_t)elapsed_ms);
    if (!impl_->canvas->game_loop_running) {
        stop();
        return;
    }
    if (state() != previous_state) {
        logging::write(logging::Level::Info, "Screen", voyage::state_name(previous_state) + " (" +
                       std::to_string(previous_state) + ") -> " + state_name() + " (" + std::to_string(state()) + ")");
    }
}

int32_t GameRuntime::width() const {
    return config_.viewport == Viewport::Wide ? kWideWidth : kPhoneWidth;
}

void GameRuntime::set_viewport(Viewport viewport) {
    SessionScope session(impl_->session);
    if (config_.viewport == viewport) return;
    config_.viewport = viewport;
    config_.port_options.widescreen = (viewport == Viewport::Wide);
    if (!running_) return;
    std::lock_guard<std::mutex> guard(impl_->renderer.frameMutex());
    impl_->session.set_canvas_size(width(), kScreenHeight - kChromeHeight);
    impl_->canvas->resize_canvas(width(), kScreenHeight);
    impl_->canvas->scene_dirty = true;
    if (config_.modernized_dialogue_input.has_value()) {
        impl_->canvas->modernized_dialogue_input = *config_.modernized_dialogue_input;
    } else {
        impl_->canvas->modernized_dialogue_input = (viewport == Viewport::Wide);
    }
    impl_->canvas->port_options.widescreen = (viewport == Viewport::Wide);
    impl_->canvas->dialogue_confirm_ready = !impl_->canvas->confirm_input_down;
}

void GameRuntime::set_centered_camera(bool centered) {
    SessionScope session(impl_->session);
    config_.centered_camera = centered;
    config_.port_options.centered_camera = centered;
    if (!running_) return;
    impl_->canvas->center_camera_on_target = centered;
    impl_->canvas->port_options.centered_camera = centered;
}

bool GameRuntime::modern_movement() const {
    return config_.modern_movement;
}

void GameRuntime::set_modern_movement(bool enabled) {
    SessionScope session(impl_->session);
    config_.modern_movement = enabled;
    config_.port_options.modern_movement = enabled;
    if (!running_) return;
    impl_->canvas->modern_movement = enabled;
    impl_->canvas->port_options.modern_movement = enabled;
}

bool GameRuntime::screen_aligned_movement() const {
    return config_.screen_aligned_movement;
}

void GameRuntime::set_screen_aligned_movement(bool enabled) {
    SessionScope session(impl_->session);
    config_.screen_aligned_movement = enabled;
    config_.port_options.screen_aligned_movement = enabled;
    if (!running_) return;
    impl_->canvas->screen_aligned_movement = enabled;
    impl_->canvas->port_options.screen_aligned_movement = enabled;
}

void GameRuntime::handle_canvas_port_options_changed(const PortOptions &options) {
    config_.port_options = options;
    config_.centered_camera = options.centered_camera;
    config_.modern_movement = options.modern_movement;
    config_.screen_aligned_movement = options.screen_aligned_movement;
    if (options.widescreen != (config_.viewport == Viewport::Wide)) {
        set_viewport(options.widescreen ? Viewport::Wide : Viewport::Phone);
    }
    if (port_options_listener_) {
        port_options_listener_(options);
    }
}

void GameRuntime::apply_port_options(const PortOptions &options) {
    SessionScope session(impl_->session);
    config_.port_options = options;
    config_.centered_camera = options.centered_camera;
    config_.modern_movement = options.modern_movement;
    config_.screen_aligned_movement = options.screen_aligned_movement;
    if (options.widescreen != (config_.viewport == Viewport::Wide)) {
        set_viewport(options.widescreen ? Viewport::Wide : Viewport::Phone);
    }
    set_centered_camera(options.centered_camera);
    set_modern_movement(options.modern_movement);
    set_screen_aligned_movement(options.screen_aligned_movement);
    if (running_ && impl_->canvas != nullptr) {
        impl_->canvas->port_options = options;
        impl_->canvas->center_camera_on_target = options.centered_camera;
        impl_->canvas->modern_movement = options.modern_movement;
        impl_->canvas->screen_aligned_movement = options.screen_aligned_movement;
        impl_->canvas->modernized_dialogue_input = true;
    }
    if (port_options_listener_) {
        port_options_listener_(options);
    }
}

void GameRuntime::set_port_options_listener(PortOptionsListener listener) {
    port_options_listener_ = std::move(listener);
}

void GameRuntime::press(Input input) {
    SessionScope session(impl_->session);
    if (!running_) return;
    crash_report().input = (int)input;
    update_game_log_context(impl_->canvas, "key press");
    if (input == Input::Action) impl_->canvas->set_confirm_input(true);
    int32_t code = midp_key_code(input, GameCanvas::state().game_state, key_bindings_state());
    if (code != 0) impl_->canvas->keyPressed(code);
}

void GameRuntime::release(Input input) {
    SessionScope session(impl_->session);
    if (!running_) return;
    crash_report().input = (int)input;
    update_game_log_context(impl_->canvas, "key release");
    if (input == Input::Action) impl_->canvas->set_confirm_input(false);
    int32_t code = midp_key_code(input, GameCanvas::state().game_state, key_bindings_state());
    if (code != 0) impl_->canvas->keyReleased(code);
}

bool GameRuntime::is_movement(Input input) {
    return input == Input::Up || input == Input::Down || input == Input::Left || input == Input::Right ||
           input == Input::UpLeft || input == Input::UpRight || input == Input::DownLeft || input == Input::DownRight;
}

Frame GameRuntime::copy_frame() const {
    SessionScope session(impl_->session);
    Frame frame;
    frame.logical_width = width();
    frame.height = kScreenHeight;
    frame.surface_width = kSurfaceWidth;
    std::lock_guard<std::mutex> guard(impl_->renderer.frameMutex());
    frame.pixels = impl_->screen.pixels;
    return frame;
}

int32_t GameRuntime::state() const {
    SessionScope session(impl_->session);
    return running_ ? GameCanvas::state().game_state : -1;
}

RuntimeSnapshot GameRuntime::inspect_state() const {
    SessionScope session(impl_->session);
    if (!running_ || impl_->canvas == nullptr) {
        RuntimeSnapshot snapshot;
        snapshot.state = state();
        return snapshot;
    }
    return inspect_canvas(*impl_->canvas, state());
}

std::string GameRuntime::state_name() const { return voyage::state_name(state()); }

std::string GameRuntime::script_status() const {
    SessionScope session(impl_->session);
    return running_ ? voyage::script_status(*impl_->canvas) : "not running";
}

bool GameRuntime::has_error() const {
    SessionScope session(impl_->session);
    return running_ && impl_->canvas->crash_context.has_value();
}

std::string GameRuntime::error_message() const {
    SessionScope session(impl_->session);
    if (!has_error()) return "";
    return impl_->canvas->crash_context.value_or("null") + " | " +
           crash_report().message;
}

}
