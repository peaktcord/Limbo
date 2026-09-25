#ifndef VOYAGE_PLATFORM_SDL_JAR_INTAKE_SCREEN_HPP
#define VOYAGE_PLATFORM_SDL_JAR_INTAKE_SCREEN_HPP

#include <SDL3/SDL.h>

#include <filesystem>
#include <optional>
#include <string>
#include <utility>

#include "src/game/game_install.hpp"
#include "src/game/jar_prompt.hpp"
#include "src/diagnostics/log.hpp"

namespace voyage::platform {

class JarIntakeScreen {
public:
    explicit JarIntakeScreen(std::filesystem::path cache, std::string error = {})
        : cache_(std::move(cache)), error_(std::move(error)), frame_(jar_prompt_frame(error_)) {}

    void handleEvent(const SDL_Event &event) {
        if (closed_ || imported_) return;
        if (event.type == SDL_EVENT_QUIT || event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED ||
            (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat && event.key.key == SDLK_ESCAPE)) {
            closed_ = true;
        } else if (event.type == SDL_EVENT_DROP_FILE && event.drop.data != nullptr) {
            try {
                logging::write(logging::Level::Info, "JAR intake", std::string("Importing dropped file ") + event.drop.data);
                imported_ = install::prepare_resources(cache_, std::filesystem::u8path(event.drop.data));
                error_.clear();
            } catch (const std::exception &error) {
                logging::exception("JAR intake", "Importing dropped file", error);
                error_ = error.what();
                frame_ = jar_prompt_frame(error_);
            }
        }
    }

    bool closed() const { return closed_; }
    const std::optional<install::PreparedResources> &imported() const { return imported_; }
    const std::string &error() const { return error_; }
    const Frame &frame() const { return frame_; }

private:
    std::filesystem::path cache_;
    std::string error_;
    Frame frame_;
    bool closed_ = false;
    std::optional<install::PreparedResources> imported_;
};

}

#endif
