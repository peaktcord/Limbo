#include "src/game/game_runtime.hpp"
#include "tests/support/gameplay_scenario.hpp"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>

namespace {

struct Checkpoint {
    std::string name;
    uint64_t time_ms;
    voyage::RuntimeSnapshot state;
    int32_t logical_width;
    int32_t surface_width;
    int32_t frame_height;
    uint64_t frame_hash;
};

Checkpoint capture(const char *name, voyage::GameRuntime &game) {
    const voyage::Frame frame = game.copy_frame();
    return Checkpoint{name,
                      game.simulation_time_ms(),
                      game.inspect_state(),
                      frame.logical_width,
                      frame.surface_width,
                      frame.height,
                      voyage::test::hash_logical_frame(frame)};
}

bool has_fixed_width_interface(const voyage::Frame &frame) {
    const int32_t left = (frame.logical_width - voyage::kPhoneWidth) / 2;
    const int32_t right = left + voyage::kPhoneWidth;
    bool has_interface_pixel = false;
    for (int32_t y = 0; y < frame.height; ++y) {
        for (int32_t x = 0; x < frame.logical_width; ++x) {
            const uint32_t pixel = frame.pixels[(size_t)y * (size_t)frame.surface_width + (size_t)x];
            if (x < left || x >= right) {
                if (pixel != 0xFF000000u) return false;
            } else if (pixel != 0xFF000000u) {
                has_interface_pixel = true;
            }
        }
    }
    return has_interface_pixel;
}

std::string encode(const Checkpoint &checkpoint) {
    const voyage::RuntimeSnapshot &state = checkpoint.state;
    std::ostringstream line;
    line << checkpoint.name << '|' << checkpoint.time_ms << '|' << state.state << '|'
         << checkpoint.logical_width << '|' << checkpoint.surface_width << '|' << checkpoint.frame_height << '|'
         << state.player_world_x << '|' << state.player_world_y << '|' << state.player_screen_x << '|'
         << state.player_screen_y << '|' << state.camera_offset_x << '|' << state.camera_offset_y << '|'
         << state.camera_target_actor << '|' << checkpoint.frame_hash;
    return line.str();
}

}

int main(int argc, char **argv) {
    const bool emit = argc == 4 && std::string(argv[2]) == "--emit";
    if (argc != 3 && !emit) {
        std::fprintf(stderr, "usage: %s RESOURCE_DIR EXPECTED_TRACE\n"
                             "       %s RESOURCE_DIR --emit OUTPUT_TRACE\n",
                     argv[0], argv[0]);
        return 2;
    }

    voyage::RuntimeConfig config;
    config.resource_dir = argv[1];
    config.random_seed = 0x4f626c6976696f6eLL;
    config.viewport = voyage::Viewport::Wide;
    config.centered_camera = true;
    if (const char *test_tmp = std::getenv("TEST_TMPDIR")) config.save_dir = test_tmp;
    config.save_dir += "/wide-viewport-scenario";

    voyage::GameRuntime game(config);
    game.start();
    voyage::test::drive_opening(game);

    std::vector<Checkpoint> observed;
    observed.push_back(capture("wide-gameplay", game));
    for (int step = 0; step < 20; ++step) {
        voyage::test::tap(game, voyage::Input::Up);
        voyage::test::advance(game, 200);
    }
    observed.push_back(capture("wide-moved", game));
    game.set_viewport(voyage::Viewport::Phone);
    game.set_centered_camera(false);
    voyage::test::advance(game, 100);
    observed.push_back(capture("phone-resized", game));
    game.set_viewport(voyage::Viewport::Wide);
    game.set_centered_camera(true);
    voyage::test::advance(game, 100);
    observed.push_back(capture("wide-restored", game));
    voyage::test::tap(game, voyage::Input::Character);
    voyage::test::advance(game, 100);
    const voyage::Frame menu_frame = game.copy_frame();
    observed.push_back(capture("wide-menu", game));

    bool valid = true;
    valid &= voyage::test::require(!game.has_error(), "wide viewport scenario", "the game entered its error screen");
    valid &= voyage::test::require(observed[0].state.state == 0 && observed[0].logical_width == voyage::kWideWidth &&
                                       observed[0].surface_width == voyage::kSurfaceWidth,
                                   "wide viewport scenario", "wide runtime did not reach free play at wide width");
    valid &= voyage::test::require(observed[1].state.player_world_y < observed[0].state.player_world_y,
                                   "wide viewport scenario", "player movement failed in the wide viewport");
    valid &= voyage::test::require(
        observed[0].state.player_screen_x + observed[0].state.camera_offset_x == voyage::kWideWidth / 2 &&
            observed[1].state.player_screen_x + observed[1].state.camera_offset_x == voyage::kWideWidth / 2 &&
            observed[1].state.camera_offset_y != observed[0].state.camera_offset_y,
        "wide viewport scenario", "wide camera did not continuously center the moving player");
    valid &= voyage::test::require(observed[2].logical_width == voyage::kPhoneWidth &&
                                       observed[2].state.player_world_x == observed[1].state.player_world_x &&
                                       observed[2].state.player_world_y == observed[1].state.player_world_y,
                                   "wide viewport scenario", "phone resize changed world state or reported the wrong width");
    valid &= voyage::test::require(observed[3].logical_width == voyage::kWideWidth &&
                                       observed[3].state.player_world_x == observed[2].state.player_world_x &&
                                       observed[3].state.player_world_y == observed[2].state.player_world_y &&
                                       observed[3].state.camera_target_actor == 0 && game.centered_camera(),
                                   "wide viewport scenario", "restoring wide mode changed world state or camera ownership");
    valid &= voyage::test::require(observed[4].state.state == 2 && has_fixed_width_interface(menu_frame),
                                   "wide viewport scenario",
                                   "wide menu did not retain a centered phone-width interface");

    std::vector<std::string> actual;
    for (const Checkpoint &checkpoint : observed) actual.push_back(encode(checkpoint));
    if (emit) {
        if (valid && !voyage::test::write_trace(
                         argv[3],
                         "# name|time_ms|state|logical_width|surface_width|frame_height|player_world_x|"
                         "player_world_y|player_screen_x|player_screen_y|camera_offset_x|camera_offset_y|"
                         "camera_target_actor|logical_frame_fnv1a64",
                         actual)) {
            std::fprintf(stderr, "wide viewport scenario: cannot write %s\n", argv[3]);
            valid = false;
        }
    } else {
        valid &= voyage::test::compare_trace(argv[2], actual, "wide viewport scenario");
    }

    game.stop();
    return valid ? 0 : 1;
}
