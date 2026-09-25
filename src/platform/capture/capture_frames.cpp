#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "src/common/render/render.hpp"
#include "src/game/game_runtime.hpp"

namespace {

void advance(voyage::GameRuntime &game, int elapsedMs) {
    while (elapsedMs > 0) {
        int step = std::min(elapsedMs, 16);
        game.tick((uint32_t)step);
        elapsedMs -= step;
    }
}

}

int main(int argc, char **argv) {
    std::string resourceDir = "artifacts/extracted/v1010";
    std::string outDir = ".";
    int frames = 12;
    int intervalMs = 1000;
    bool driveKeys = false;
    bool wide = false;
    int positional = 0;
    for (int n = 1; n < argc; ++n) {
        if (std::strcmp(argv[n], "--frames") == 0 && n + 1 < argc) {
            frames = std::atoi(argv[++n]);
        } else if (std::strcmp(argv[n], "--interval") == 0 && n + 1 < argc) {
            intervalMs = std::atoi(argv[++n]);
        } else if (std::strcmp(argv[n], "--keys") == 0) {
            driveKeys = true;
        } else if (std::strcmp(argv[n], "--wide") == 0) {
            wide = true;
        } else if (std::strcmp(argv[n], "--help") == 0) {
            std::printf(
                "usage: %s [resource-dir] [out-dir] [--frames N] [--interval MS]"
                " [--keys] [--wide]\n\n"
                "  --keys   alternate FIRE and DOWN each tick, which walks the\n"
                "           intro, the menus and the opening cutscene\n"
                "  --wide   start on the 16:9 canvas instead of the phone one\n",
                argv[0]);
            return 0;
        } else if (positional == 0) {
            resourceDir = argv[n];
            ++positional;
        } else if (positional == 1) {
            outDir = argv[n];
            ++positional;
        }
    }

    voyage::RuntimeConfig config;
    config.resource_dir = resourceDir;
    config.viewport = wide ? voyage::Viewport::Wide : voyage::Viewport::Phone;
    config.centered_camera = wide;
    voyage::GameRuntime game(config);
    try {
        game.start();
    } catch (const std::exception &error) {
        std::fprintf(stderr, "game startup failed: %s\n", error.what());
        return 1;
    }
    std::printf("canvas %dx%d  (the game derived this from getWidth/getHeight+25)\n",
                (int)game.width(), (int)game.height());

    bool reportedError = false;
    for (int frame = 0; frame < frames && game.running(); ++frame) {
        advance(game, intervalMs);
        if (driveKeys) {
            voyage::Input input = (frame % 2 == 0) ? voyage::Input::Action : voyage::Input::Down;
            game.press(input);
            advance(game, 120);
            game.release(input);
        }
        voyage::Frame snapshot = game.copy_frame();
        render::Surface out(snapshot.logical_width, snapshot.height);
        for (int32_t y = 0; y < out.height; ++y) {
            for (int32_t x = 0; x < out.width; ++x) {
                out.row(y)[x] = snapshot.pixels[(size_t)y * (size_t)snapshot.surface_width + (size_t)x];
            }
        }
        char path[1024];
        std::snprintf(path, sizeof path, "%s/frame%02d.png", outDir.c_str(), frame);
        bool written = render::writePng(out, path);
        std::printf("frame %2d  state %2d %-15s %-34s %s\n", frame, (int)game.state(),
                    game.state_name().c_str(), game.script_status().c_str(),
                    written ? path : "PNG WRITE FAILED");
        if (game.has_error() && !reportedError) {
            reportedError = true;
            std::printf("  !! the game's error screen is up: %s\n", game.error_message().c_str());
        }
        std::fflush(stdout);
    }

    game.stop();
    return reportedError ? 2 : 0;
}
