#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <emscripten/emscripten.h>

#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>

#include "src/game/game_install.hpp"
#include "src/diagnostics/log.hpp"
#include "src/diagnostics/version.hpp"
#include "src/game/game_runtime.hpp"
#include "src/game/jar_prompt.hpp"
#include "src/platform/adapter_policy.hpp"
#include "src/platform/held_input.hpp"
#include "src/platform/sdl/sdl_presenter.hpp"

namespace {

constexpr const char *kCacheDirectory = "/persistent/oblivion";
constexpr const char *kIncomingJar = "/incoming/oblivion.jar";
constexpr const char *kSaveDirectory = "/persistent/saves/rms";

using voyage::Input;
using voyage::Viewport;
namespace logging = voyage::logging;

extern "C" EMSCRIPTEN_KEEPALIVE void oblivion_log_error(const char *message) {
    logging::write(logging::Level::Error, "Browser", message);
}

EM_JS(void, browserInstallLogging, (char *buffer, int capacity), {
    Module['oblivionLogError'] = function (message) {
        stringToUTF8(String(message), buffer, capacity);
        _oblivion_log_error(buffer);
    };
    (Module['oblivionLogMessages'] || []).forEach(Module['oblivionLogError']);
    Module['oblivionLogMessages'] = [];
});

void SDLCALL sdlLog(void *, int category, SDL_LogPriority priority, const char *message) {
    if (priority < SDL_LOG_PRIORITY_WARN) return;
    char component[64]{};
    std::snprintf(component, sizeof component, "SDL category %d", category);
    logging::write(priority >= SDL_LOG_PRIORITY_ERROR ? logging::Level::Error : logging::Level::Warning,
                   component, message);
}

EM_JS(void, browserStatus, (const char *message, int needsJar), {
    if (Module['oblivionSetStatus']) {
        Module['oblivionSetStatus'](UTF8ToString(message), !!needsJar);
    }
});

EM_JS(void, browserFlushSaves, (), {
    if (Module['oblivionFlushPersistent']) {
        Module['oblivionFlushPersistent']();
    }
});

voyage::JarPromptText browserPromptText() {
    return voyage::JarPromptText{
        "Select Oblivion's .jar\nto begin.",
        "Select Oblivion's .jar\nto try again.",
        "Nothing is uploaded",
    };
}

struct WebApp {
    voyage::platform::SdlPresenter presenter;
    std::unique_ptr<voyage::GameRuntime> game;
    voyage::platform::HeldInput heldInput;
    voyage::platform::FrameClock frameClock;
    bool wide = false;
    bool reportedError = false;

    std::optional<voyage::Frame> prompt;
};

WebApp *gApp = nullptr;

void startGame(WebApp &app, const std::string &identity) {
    voyage::RuntimeConfig config;
    config.resource_dir = kCacheDirectory;
    config.save_dir = kSaveDirectory;
    config.on_save_persisted = browserFlushSaves;
    config.viewport = app.wide ? Viewport::Wide : Viewport::Phone;
    config.centered_camera = app.wide;
    config.modern_movement = app.wide;
    config.screen_aligned_movement = app.wide;
    config.modernized_dialogue_input = true;
    config.top_level_port_options = true;

    std::unique_ptr<voyage::GameRuntime> game =
        std::make_unique<voyage::GameRuntime>(config);
    WebApp *owner = &app;
    game->set_port_options_listener([owner](const voyage::PortOptions &options) {
        owner->wide = options.widescreen;
        owner->presenter.setLogicalWidth(options.widescreen ? voyage::kWideWidth
                                                            : voyage::kPhoneWidth);
    });
    game->start();
    app.presenter.setLogicalWidth(game->width());
    app.game = std::move(game);
    app.prompt.reset();
    app.frameClock.reset(SDL_GetTicks());
    app.reportedError = false;
    browserStatus(("Running " + identity + ".").c_str(), 0);
}

void present(WebApp &app) {
    if (app.game) {
        voyage::Frame frame = app.game->copy_frame();
        app.presenter.present(&frame);
    } else {
        app.presenter.present(app.prompt ? &*app.prompt : nullptr);
    }
}

void showPrompt(WebApp &app, const std::string &error) {
    app.prompt = voyage::jar_prompt_frame(error, browserPromptText());
    app.presenter.setLogicalWidth(app.prompt->logical_width);
}

}

extern "C" EMSCRIPTEN_KEEPALIVE int oblivion_import_selected_jar() {
    if (gApp == nullptr || gApp->game) return 0;
    try {
        voyage::install::PreparedResources prepared =
            voyage::install::prepare_resources(kCacheDirectory,
                                               std::filesystem::path(kIncomingJar));
        logging::write(logging::Level::Info, "JAR intake", "Imported " + prepared.canonical_jar_name +
                       "; SHA-256=" + prepared.archive_sha256);
        startGame(*gApp, prepared.canonical_jar_name);
        return 1;
    } catch (const std::exception &error) {
        std::string message = std::string("Import failed: ") + error.what();
        logging::exception("JAR intake", "Importing selected archive", error);
        showPrompt(*gApp, error.what());
        present(*gApp);
        browserStatus(message.c_str(), 1);
        return 0;
    } catch (...) {
        logging::write(logging::Level::Error, "JAR intake", "Unknown exception importing selected archive");
        showPrompt(*gApp, "Unknown error. Details are in the session log.");
        present(*gApp);
        browserStatus("The JAR could not be imported. Details are in the session log.", 1);
        return 0;
    }
}

SDL_AppResult SDL_AppInit(void **appstate, int, char **) {
    try {
        logging::open_file(std::filesystem::path(kSaveDirectory) / "limbo.log");
        logging::set_persist_hook(browserFlushSaves);
        logging::write(logging::Level::Info, "Application", "Limbo " + voyage::version::describe() +
                       " browser session started; Oblivion mobile 1.0.10");
        static char browser_log_buffer[8192];
        browserInstallLogging(browser_log_buffer, sizeof browser_log_buffer);
        SDL_SetLogOutputFunction(sdlLog, nullptr);
        if (!SDL_Init(SDL_INIT_VIDEO)) {
            logging::write(logging::Level::Error, "Startup", SDL_GetError());
            browserStatus("The display could not start. Details are in the session log.", 0);
            return SDL_APP_FAILURE;
        }

        std::unique_ptr<WebApp> app = std::make_unique<WebApp>();
        if (!app->presenter.create("The Elder Scrolls Travels: Oblivion", voyage::kPhoneWidth, 3, true)) {
            logging::write(logging::Level::Error, "Display", SDL_GetError());
            browserStatus("The display could not start. Details are in the session log.", 0);
            return SDL_APP_FAILURE;
        }

        gApp = app.get();
        *appstate = app.release();

        voyage::install::CacheValidation cached =
            voyage::install::validate_cache(kCacheDirectory);
        if (cached.valid) {
            logging::write(logging::Level::Info, "JAR intake", "Using validated cache; SHA-256=" + cached.archive_sha256);
            try {
                startGame(*gApp, cached.canonical_jar_name);
            } catch (const std::exception &error) {
                std::string message = std::string("Cached resources could not boot: ") +
                                      error.what() + ". Select the original JAR to repair them.";
                logging::exception("Startup", "Starting cached game", error);
                logging::write(logging::Level::Error, "Game context", logging::last_context());
                showPrompt(*gApp, error.what());
                browserStatus(message.c_str(), 1);
            }
        } else {
            if (std::filesystem::exists(kCacheDirectory)) {
                logging::write(logging::Level::Warning, "JAR intake", "Cache rejected: " + cached.reason);
            }
            showPrompt(*gApp, {});
            browserStatus("Select your original Oblivion mobile JAR. It is verified and extracted locally; nothing is uploaded.", 1);
        }

        present(*gApp);
        return SDL_APP_CONTINUE;
    } catch (const std::exception &error) {
        logging::exception("Startup", "Starting browser application", error);
        browserStatus(error.what(), 0);
        return SDL_APP_FAILURE;
    } catch (...) {
        logging::write(logging::Level::Fatal, "Startup", "Unknown exception starting browser application");
        browserStatus("The game could not start. Details are in the session log.", 0);
        return SDL_APP_FAILURE;
    }
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    WebApp &app = *(WebApp *)appstate;
    if (event->type == SDL_EVENT_QUIT) return SDL_APP_SUCCESS;
    if (!app.game) return SDL_APP_CONTINUE;
    try {
        voyage::platform::dispatchShellEvent(*event, *app.game, app.heldInput, app.presenter, app.wide);
    } catch (const std::exception &error) {
        logging::exception("Input", "Handling browser event", error);
        logging::write(logging::Level::Error, "Game context", logging::last_context());
        browserStatus("The game could not continue. Details are in the session log.", 0);
        return SDL_APP_FAILURE;
    } catch (...) {
        logging::write(logging::Level::Fatal, "Input", "Unknown exception handling browser event");
        logging::write(logging::Level::Fatal, "Game context", logging::last_context());
        browserStatus("The game could not continue. Details are in the session log.", 0);
        return SDL_APP_FAILURE;
    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
    WebApp &app = *(WebApp *)appstate;
    if (!app.game) {
        present(app);
        return SDL_APP_CONTINUE;
    }

    try {
        if (std::optional<Input> input = app.heldInput.repeat(app.game->state(), app.game->modern_movement())) {
            app.game->press(*input);
        }
        app.game->tick(app.frameClock.advance(SDL_GetTicks()));
        present(app);
        if (!app.game->running()) {
            browserStatus("Game closed. Reload the page to play again.", 0);
            return SDL_APP_SUCCESS;
        }

        if (app.game->has_error() && !app.reportedError) {
            app.reportedError = true;
            std::string message = "Game error: " + app.game->error_message();
            browserStatus(message.c_str(), 0);
        }
    } catch (const std::exception &error) {
        logging::exception("Game loop", "Running browser frame", error);
        logging::write(logging::Level::Error, "Game context", logging::last_context());
        browserStatus((std::string("Browser frame failed: ") + error.what()).c_str(), 0);
        return SDL_APP_FAILURE;
    } catch (...) {
        logging::write(logging::Level::Fatal, "Game loop", "Unknown exception running browser frame");
        logging::write(logging::Level::Fatal, "Game context", logging::last_context());
        browserStatus("The game could not continue. Details are in the session log.", 0);
        return SDL_APP_FAILURE;
    }
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult) {
    std::unique_ptr<WebApp> app((WebApp *)appstate);
    gApp = nullptr;
    if (app) {
        if (app->game) app->game->stop();
        app->game.reset();
        app->presenter.destroy();
    }
    logging::write(logging::Level::Info, "Application", "Session ended");
    logging::close_file();
}
