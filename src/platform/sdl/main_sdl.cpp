#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <optional>
#include <string>

#include "src/game/game_install.hpp"
#include "src/game/game_runtime.hpp"
#include "src/game/port_settings.hpp"
#include "src/game/sha256.hpp"
#include "src/diagnostics/log.hpp"
#include "src/diagnostics/version.hpp"
#include "src/platform/crash_trace.hpp"
#include "src/platform/adapter_policy.hpp"
#include "src/platform/held_input.hpp"
#include "src/platform/sdl/jar_intake_screen.hpp"
#include "src/platform/sdl/sdl_presenter.hpp"

namespace {

using voyage::Input;
using voyage::Viewport;
namespace fs = std::filesystem;
namespace logging = voyage::logging;

struct CommandLine {
    std::optional<fs::path> jar;
    std::optional<fs::path> resources;
    std::optional<fs::path> cache;
    std::optional<fs::path> saves;
    int scale = 3;
    bool wide = false;
    bool install_only = false;
    bool portable = false;
    bool help = false;
};

void printUsage(const char *program) {
    std::printf(
        "usage: %s [game.jar|resource-dir] [save-dir] [options]\n\n"
        "  --jar PATH        import this supported original JAR\n"
        "  --resources DIR   use an extracted directory without importing (developer)\n"
        "  --cache DIR       override the persistent extracted cache directory\n"
        "  --saves DIR       override the save directory (also contains limbo.log)\n"
        "  --install-only    validate/import resources, then exit\n"
        "  --portable        keep resources and saves in data/ beside the executable\n"
        "  --scale N         integer window scale (default 3)\n"
        "  --wide            start with the wide viewport\n"
        "\nWith no resource argument, the game validates oblivion/ in the user data directory.\n"
        "If it is missing, drop the original JAR onto the game's prompt screen.\n"
        "The second positional argument is the RMS save directory. With none, saves\n"
        "go to Limbo/saves/rms in the user data directory. --portable or\n"
        "portable.txt beside the executable selects its data/ directory instead.\n\n"
        "  move          WASD or the arrow keys (hold to keep walking)\n"
        "  act / confirm Space, E, F, Enter or left mouse\n"
        "  (confirm also skips the opening story crawl)\n"
        "  character     Tab, C or I\n"
        "  menu / back   Escape, Backspace, P or right mouse\n"
        "  quick health  1 or H   quick magicka  2, M or R\n"
        "  weapon/spell  Q, X or middle mouse\n"
        "  view          F11 toggles 16:9 and the phone aspect\n"
        "  window        Alt+Enter toggles fullscreen\n"
        "  (top-row 0 and 3-9 plus the numeric keypad send the original\n"
        "   phone digits used by the in-game Controls screen)\n",
        program);
}

CommandLine parseCommandLine(int argc, char **argv) {
    CommandLine line;
    int positional = 0;
    for (int n = 1; n < argc; ++n) {
        if (std::strcmp(argv[n], "--scale") == 0 && n + 1 < argc) {
            line.scale = std::atoi(argv[++n]);
            if (line.scale < 1) line.scale = 1;
        } else if (std::strcmp(argv[n], "--wide") == 0) {
            line.wide = true;
        } else if (std::strcmp(argv[n], "--jar") == 0 && n + 1 < argc) {
            line.jar = fs::u8path(argv[++n]);
        } else if (std::strcmp(argv[n], "--resources") == 0 && n + 1 < argc) {
            line.resources = fs::u8path(argv[++n]);
        } else if (std::strcmp(argv[n], "--cache") == 0 && n + 1 < argc) {
            line.cache = fs::u8path(argv[++n]);
        } else if (std::strcmp(argv[n], "--saves") == 0 && n + 1 < argc) {
            line.saves = fs::u8path(argv[++n]);
        } else if (std::strcmp(argv[n], "--install-only") == 0) {
            line.install_only = true;
        } else if (std::strcmp(argv[n], "--portable") == 0) {
            line.portable = true;
        } else if (std::strcmp(argv[n], "--help") == 0) {
            line.help = true;
        } else if (positional == 0) {
            fs::path input = fs::u8path(argv[n]);
            std::string extension = input.extension().string();
            for (char &c : extension) if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
            if (extension == ".jar") {
                line.jar = input;
            } else {
                line.resources = input;
            }
            ++positional;
        } else if (positional == 1) {
            line.saves = fs::u8path(argv[n]);
            ++positional;
        }
    }
    return line;
}

struct ResourceChoice {
    fs::path dir;
    int exit_code = 0;
};

ResourceChoice runIntakeScreen(const fs::path &cache, int scale,
                              voyage::platform::SdlPresenter &presenter, const std::string &error = {}) {
    if (!presenter.create("The Elder Scrolls Travels: Oblivion", voyage::kPhoneWidth, scale, true)) {
        logging::write(logging::Level::Error, "Display", SDL_GetError());
        return {{}, 1};
    }
    voyage::platform::JarIntakeScreen screen(cache, error);
    logging::write(logging::Level::Info, "JAR intake", "Waiting for a JAR; cache=" + cache.string());
    presenter.present(&screen.frame());
    while (!screen.closed() && !screen.imported()) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat &&
                (event.key.key == SDLK_RETURN || event.key.key == SDLK_KP_ENTER) &&
                (event.key.mod & SDL_KMOD_ALT) != 0) {
                presenter.toggleFullscreen();
            } else {
                screen.handleEvent(event);
            }
            if (screen.closed() || screen.imported()) break;
        }
        if (screen.closed() || screen.imported()) break;
        presenter.present(&screen.frame());
        SDL_Delay(16);
    }
    if (screen.closed()) return {{}, 0};
    logging::write(logging::Level::Info, "JAR intake", "Imported " + screen.imported()->canonical_jar_name +
                   "; SHA-256=" + screen.imported()->archive_sha256 + "; cache=" + cache.string());
    return {screen.imported()->resource_dir, 0};
}

ResourceChoice locateResources(const CommandLine &line, const fs::path &executable,
                               voyage::platform::SdlPresenter &presenter) {
    if (line.resources) return {*line.resources, 0};

    std::optional<fs::path> jarPath = line.jar;
    fs::path primary = line.cache ? *line.cache : voyage::install::resolve_cache_directory(executable, line.portable);
    fs::path legacy = line.cache ? fs::path() : voyage::install::colocated_cache_directory(executable);
    voyage::platform::StartupFacts startup;
    startup.has_explicit_jar = jarPath.has_value();
    startup.has_cache_override = line.cache.has_value();
    startup.legacy_cache_available = !legacy.empty() && legacy != primary;
    voyage::install::CacheValidation primary_validation;
    voyage::install::CacheValidation legacy_validation;
    if (!jarPath) {
        primary_validation = voyage::install::validate_cache(primary);
        if (startup.legacy_cache_available) legacy_validation = voyage::install::validate_cache(legacy);
        startup.primary_cache_valid = primary_validation.valid;
        startup.legacy_cache_valid = legacy_validation.valid;
        if (!primary_validation.valid && fs::exists(primary)) {
            logging::write(logging::Level::Warning, "JAR intake", "Cache rejected: " + primary.string() +
                           "; " + primary_validation.reason);
        }
        if (startup.legacy_cache_available && !legacy_validation.valid && fs::exists(legacy)) {
            logging::write(logging::Level::Warning, "JAR intake", "Legacy cache rejected: " + legacy.string() +
                           "; " + legacy_validation.reason);
        }
    }
    switch (voyage::platform::select_startup_source(startup)) {
        case voyage::platform::StartupSource::PrimaryCache:
            logging::write(logging::Level::Info, "JAR intake", "Using validated cache " + primary.string() +
                           "; SHA-256=" + primary_validation.archive_sha256);
            return {primary, 0};
        case voyage::platform::StartupSource::LegacyCache:
            logging::write(logging::Level::Info, "JAR intake", "Using validated legacy cache " + legacy.string() +
                           "; SHA-256=" + legacy_validation.archive_sha256);
            return {legacy, 0};
        case voyage::platform::StartupSource::IntakeScreen:
            if (line.install_only) {
                logging::write(logging::Level::Error, "JAR intake", "No valid cache; supply --jar PATH to import Oblivion");
                return {{}, 1};
            }
            return runIntakeScreen(primary, line.scale, presenter);
        case voyage::platform::StartupSource::ExplicitJar:
        case voyage::platform::StartupSource::ResourceOverride:
            break;
    }

    try {
        voyage::install::PreparedResources prepared =
            voyage::install::prepare_resources(primary, jarPath);
        logging::write(logging::Level::Info, "JAR intake", "Imported " + prepared.canonical_jar_name +
                       "; SHA-256=" + prepared.archive_sha256 + "; cache=" + prepared.resource_dir.string());
        return {prepared.resource_dir, 0};
    } catch (const std::exception &primary_error) {
        logging::exception("JAR intake", "Importing command-line archive", primary_error);
        if (line.install_only) return {{}, 1};
        return runIntakeScreen(primary, line.scale, presenter, primary_error.what());
    }
}

void SDLCALL sdlLog(void *, int category, SDL_LogPriority priority, const char *message) {
    if (priority < SDL_LOG_PRIORITY_WARN) return;
    char context[64]{};
    std::snprintf(context, sizeof context, "SDL category %d", category);
    logging::write(priority >= SDL_LOG_PRIORITY_ERROR ? logging::Level::Error : logging::Level::Warning,
                   context, message);
}

}

int runDesktop(const CommandLine &line, const fs::path &executable, const fs::path &saveDir) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        logging::write(logging::Level::Error, "Startup", SDL_GetError());
        return 1;
    }
    struct SdlLifetime { ~SdlLifetime() { SDL_Quit(); } } sdl_lifetime;
    voyage::platform::SdlPresenter presenter;

    ResourceChoice resources = locateResources(line, executable, presenter);
    if (resources.dir.empty()) {
        return resources.exit_code;
    }
    if (line.install_only) {
        logging::write(logging::Level::Info, "JAR intake", "Validated resources at " + resources.dir.string());
        return 0;
    }

    const fs::path settingsDir = voyage::install::resolve_settings_directory(executable, line.portable);
    voyage::PortOptions options;
    voyage::loadPortOptions(settingsDir.string(), &options);
    if (line.wide) {
        options.widescreen = true;
    }

    voyage::RuntimeConfig config;
    config.resource_dir = resources.dir.string();
    if (!voyage::install::directory_is_writable(saveDir)) {
        logging::write(logging::Level::Error, "Save", "Directory is not writable: " + saveDir.string());
    }
    config.save_dir = saveDir.string();
    config.settings_dir = settingsDir.string();
    config.port_options = options;
    bool wide = options.widescreen;
    config.viewport = wide ? Viewport::Wide : Viewport::Phone;
    config.centered_camera = options.centered_camera;
    config.modern_movement = options.modern_movement;
    config.screen_aligned_movement = options.screen_aligned_movement;
    config.modernized_dialogue_input = true;
    config.top_level_port_options = true;
    voyage::GameRuntime game(config);

    if (presenter.created()) {
        presenter.setLogicalWidth(game.width());
        if (options.fullscreen) presenter.setFullscreen(true);
    } else if (!presenter.create("The Elder Scrolls Travels: Oblivion", game.width(), line.scale, true)) {
        logging::write(logging::Level::Error, "Display", SDL_GetError());
        return 1;
    } else if (options.fullscreen) {
        presenter.setFullscreen(true);
    }

    game.set_port_options_listener([&presenter, &wide, settingsDir](const voyage::PortOptions &opts) {
        voyage::savePortOptions(settingsDir.string(), opts);
        wide = opts.widescreen;
        presenter.setLogicalWidth(opts.widescreen ? voyage::kWideWidth : voyage::kPhoneWidth);
        presenter.setFullscreen(opts.fullscreen);
    });

    try {
        game.start();
    } catch (const std::exception &error) {
        logging::exception("Startup", "Starting the game", error);
        logging::write(logging::Level::Error, "Game context", logging::last_context());
        return 1;
    }

    voyage::platform::FrameClock frameClock(SDL_GetTicks());
    voyage::platform::HeldInput heldInput;
    bool running = true;
    while (running && game.running()) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            } else {
                voyage::platform::dispatchShellEvent(event, game, heldInput, presenter, wide);
            }
        }

        if (std::optional<Input> input = heldInput.repeat(game.state(), game.modern_movement())) {
            game.press(*input);
        }

        game.tick(frameClock.advance(SDL_GetTicks()));
        voyage::Frame frame = game.copy_frame();
        presenter.present(&frame);
        SDL_Delay(16);
    }

    game.stop();
    return 0;
}

int main(int argc, char **argv) {
    const CommandLine line = parseCommandLine(argc, argv);
    if (line.help) {
        printUsage(argv[0]);
        return 0;
    }

    const char *base = SDL_GetBasePath();
    fs::path executable = base ? fs::u8path(base) / fs::u8path(argv[0]).filename() : fs::u8path(argv[0]);
    if (executable.extension().empty()) executable += ".exe";
    const fs::path saveDir = line.saves ? *line.saves :
        voyage::install::resolve_data_directory(executable, line.portable) / "saves" / "rms";
    const fs::path logPath = saveDir / "limbo.log";
    try {
        logging::open_file(logPath);
    } catch (const std::exception &error) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Limbo could not start",
                                 error.what(), nullptr);
        return 1;
    }
    struct LogLifetime {
        ~LogLifetime() {
            logging::write(logging::Level::Info, "Application", "Session ended");
            logging::close_file();
        }
    } log_lifetime;
    voyage::crash_trace::install();
    struct CrashLifetime { ~CrashLifetime() { voyage::crash_trace::uninstall(); } } crash_lifetime;
    SDL_SetLogOutputFunction(sdlLog, nullptr);
    logging::write(logging::Level::Info, "Application", "Limbo " + voyage::version::describe() +
                   "; Oblivion mobile 1.0.10; session started");
    logging::write(logging::Level::Info, "Application", "Executable=" + executable.string() +
                   "; SDL=" + std::to_string(SDL_GetVersion()) + "; pointer_bits=" + std::to_string(sizeof(void *) * 8));
    logging::write(logging::Level::Info, "Storage", "Saves=" + saveDir.string() + "; log=" + logPath.string());
#ifdef VOYAGE_THROW_TRACE
    voyage::crash_trace::install_throw_trace();
    logging::write(logging::Level::Info, "Crash trace", "Throw-site stack tracing enabled");
#endif
    try {
        logging::write(logging::Level::Info, "Application", "Executable SHA-256=" +
                       voyage::crypto::sha256_hex(voyage::crypto::sha256_file(executable)));
    } catch (const std::exception &error) {
        logging::exception("Application", "Identifying executable", error);
    }
    try {
        if (!line.saves) voyage::install::resolve_save_directory(executable, line.portable);
        return runDesktop(line, executable, saveDir);
    } catch (const std::exception &error) {
        logging::exception("Application", "Unhandled desktop failure", error);
        logging::write(logging::Level::Error, "Game context", logging::last_context());
    } catch (...) {
        logging::write(logging::Level::Fatal, "Application", "Unhandled exception of unknown type");
        logging::write(logging::Level::Fatal, "Game context", logging::last_context());
    }
    const std::string message = "Limbo could not continue. Details are in " + logPath.string();
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Limbo", message.c_str(), nullptr);
    return 1;
}
