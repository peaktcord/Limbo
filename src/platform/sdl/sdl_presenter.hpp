#ifndef VOYAGE_PLATFORM_SDL_PRESENTER_HPP
#define VOYAGE_PLATFORM_SDL_PRESENTER_HPP

#include <SDL3/SDL.h>

#include <cstdint>
#include <optional>

#include "src/game/game_runtime.hpp"
#include "src/platform/adapter_policy.hpp"
#include "src/platform/held_input.hpp"
#include "src/platform/sdl/key_map.hpp"
#include "src/diagnostics/log.hpp"

namespace voyage::platform {

class SdlPresenter {
public:
    SdlPresenter() = default;
    ~SdlPresenter() { destroy(); }
    SdlPresenter(const SdlPresenter &) = delete;
    SdlPresenter &operator=(const SdlPresenter &) = delete;

    bool create(const char *title, int32_t logical_width, int scale, bool resize_window) {
        scale_ = scale;
        logical_width_ = logical_width;
        resize_window_ = resize_window;
        window_ = SDL_CreateWindow(title, logical_width * scale, kScreenHeight * scale,
                                   SDL_WINDOW_RESIZABLE);
        if (window_ == nullptr) return false;
        renderer_ = SDL_CreateRenderer(window_, nullptr);
        if (renderer_ == nullptr) return false;
        const char *renderer_name = SDL_GetRendererName(renderer_);
        logging::write(logging::Level::Info, "Display", std::string("Renderer=") +
                       (renderer_name ? renderer_name : "unknown"));
        texture_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_ARGB8888,
                                     SDL_TEXTUREACCESS_STREAMING, kSurfaceWidth, kScreenHeight);
        if (texture_ == nullptr) return false;
        SDL_SetTextureScaleMode(texture_, SDL_SCALEMODE_NEAREST);
        applyPresentation();
        return true;
    }

    void destroy() {
        if (texture_ != nullptr) SDL_DestroyTexture(texture_);
        if (renderer_ != nullptr) SDL_DestroyRenderer(renderer_);
        if (window_ != nullptr) SDL_DestroyWindow(window_);
        texture_ = nullptr;
        renderer_ = nullptr;
        window_ = nullptr;
    }

    int32_t logicalWidth() const { return logical_width_; }
    bool created() const { return texture_ != nullptr; }
    void setLogicalWidth(int32_t width) {
        logical_width_ = width;
        applyPresentation();
    }

    bool fullscreen() const { return fullscreen_; }
    void setFullscreen(bool enable) {
        if (fullscreen_ == enable) return;
        fullscreen_ = enable;
        if (window_ != nullptr) SDL_SetWindowFullscreen(window_, fullscreen_);
        applyPresentation();
    }
    void toggleFullscreen() {
        setFullscreen(!fullscreen_);
    }

    void present(const Frame *frame) {
        if (frame != nullptr) {
            SDL_UpdateTexture(texture_, nullptr, frame->pixels.data(),
                              frame->surface_width * (int)sizeof(uint32_t));
        }
        SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
        SDL_RenderClear(renderer_);
        if (frame != nullptr) {
            SDL_FRect source = {0.0f, 0.0f, (float)logical_width_, (float)kScreenHeight};
            SDL_RenderTexture(renderer_, texture_, &source, nullptr);
        }
        SDL_RenderPresent(renderer_);
    }

private:
    void applyPresentation() {
        Presentation policy = presentation(logical_width_, kScreenHeight, scale_, fullscreen_);
        SDL_SetRenderLogicalPresentation(renderer_, policy.logical_width, policy.logical_height,
                                         policy.mode == PresentationMode::Letterbox
                                             ? SDL_LOGICAL_PRESENTATION_LETTERBOX
                                             : SDL_LOGICAL_PRESENTATION_INTEGER_SCALE);
        if (resize_window_ && policy.resize_window) {
            SDL_SetWindowSize(window_, policy.window_width, policy.window_height);
        }
    }

    SDL_Window *window_ = nullptr;
    SDL_Renderer *renderer_ = nullptr;
    SDL_Texture *texture_ = nullptr;
    int scale_ = 1;
    int32_t logical_width_ = kPhoneWidth;
    bool fullscreen_ = false;
    bool resize_window_ = true;
};

inline bool dispatchShellEvent(const SDL_Event &event, GameRuntime &game, HeldInput &held,
                               SdlPresenter &presenter, bool &wide) {
    switch (event.type) {
        case SDL_EVENT_KEY_DOWN: {
            if (event.key.repeat) return true;
            if ((event.key.key == SDLK_RETURN || event.key.key == SDLK_KP_ENTER) &&
                (event.key.mod & SDL_KMOD_ALT) != 0) {
                presenter.toggleFullscreen();
                voyage::PortOptions opts = game.port_options();
                opts.fullscreen = presenter.fullscreen();
                game.apply_port_options(opts);
                return true;
            }
            if (event.key.key == SDLK_F11) {
                wide = !wide;
                voyage::PortOptions opts = game.port_options();
                opts.widescreen = wide;
                game.apply_port_options(opts);
                presenter.setLogicalWidth(game.width());
                return true;
            }
            if (std::optional<Input> input = map_sdl_key(event.key.key)) {
                game.press(*input);
                held.press(*input);
            }
            return true;
        }
        case SDL_EVENT_KEY_UP: {
            if (std::optional<Input> input = map_sdl_key(event.key.key)) {
                game.release(*input);
                held.release(*input);
            }
            return true;
        }
        case SDL_EVENT_MOUSE_BUTTON_DOWN: {
            if (std::optional<Input> input = map_sdl_mouse_button(event.button.button)) {
                game.press(*input);
                held.press(*input);
            }
            return true;
        }
        case SDL_EVENT_MOUSE_BUTTON_UP: {
            if (std::optional<Input> input = map_sdl_mouse_button(event.button.button)) {
                game.release(*input);
                held.release(*input);
            }
            return true;
        }
        default:
            return false;
    }
}

}

#endif
