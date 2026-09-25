#include "src/platform/sdl/jar_intake_screen.hpp"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>

#include "src/common/render/render.hpp"

namespace {

void check_frame(const voyage::Frame &frame) {
    assert(frame.logical_width == voyage::kPhoneWidth);
    assert(frame.height == voyage::kScreenHeight);
    assert(frame.surface_width == voyage::kSurfaceWidth);
    assert(frame.pixels.size() == (size_t)(frame.surface_width * frame.height));
    size_t ink = 0;
    for (int y = 0; y < frame.height; ++y) {
        for (int x = 0; x < frame.surface_width; ++x) {
            const uint32_t pixel = frame.pixels[(size_t)(y * frame.surface_width + x)];
            assert(pixel == 0xff000000U || pixel == 0xffffffffU);
            if (pixel == 0xffffffffU) {
                assert(x >= 8 && x < voyage::kPhoneWidth - 8);
                ++ink;
            }
        }
    }
    assert(ink > 200);
}

void capture(const voyage::Frame &frame, const char *name) {
    const char *output = std::getenv("TEST_UNDECLARED_OUTPUTS_DIR");
    if (output == nullptr) return;
    render::Surface surface(frame.logical_width, frame.height);
    for (int y = 0; y < frame.height; ++y) {
        std::copy_n(frame.pixels.begin() + y * frame.surface_width, frame.logical_width, surface.row(y));
    }
    assert(render::writePng(surface, (std::filesystem::path(output) / name).string()));
}

}

int main() {
    namespace fs = std::filesystem;
    const char *test_tmp = std::getenv("TEST_TMPDIR");
    assert(test_tmp != nullptr);
    const fs::path root = fs::path(test_tmp) / "jar-prompt";
    fs::create_directories(root);

    voyage::platform::JarIntakeScreen screen(root / "cache");
    assert(!screen.closed() && !screen.imported() && screen.error().empty());
    const voyage::Frame invitation = screen.frame();
    check_frame(invitation);
    capture(invitation, "jar-prompt.png");
    assert(!fs::exists(root / "cache"));

    SDL_Event drop{};
    drop.type = SDL_EVENT_DROP_FILE;
    screen.handleEvent(drop);
    assert(screen.frame().pixels == invitation.pixels);
    const fs::path bad = root / "wrong.jar";
    { std::ofstream file(bad); file << "not a game"; }
    const std::string bad_path = bad.u8string();
    drop.drop.data = bad_path.c_str();
    screen.handleEvent(drop);
    assert(!screen.closed() && !screen.imported() && !screen.error().empty());
    assert(screen.frame().pixels != invitation.pixels);
    check_frame(screen.frame());
    capture(screen.frame(), "jar-prompt-error.png");

    const voyage::Frame long_error = voyage::jar_prompt_frame(std::string(1000, 'W'));
    check_frame(long_error);
    const auto stride = invitation.surface_width;
    assert(std::equal(invitation.pixels.begin(), invitation.pixels.begin() + 30 * stride,
                      long_error.pixels.begin()));
    assert(std::equal(invitation.pixels.end() - 18 * stride, invitation.pixels.end(),
                      long_error.pixels.end() - 18 * stride));

    SDL_Event escape{};
    escape.type = SDL_EVENT_KEY_DOWN;
    escape.key.key = SDLK_ESCAPE;
    escape.key.repeat = true;
    screen.handleEvent(escape);
    assert(!screen.closed());
    escape.key.repeat = false;
    screen.handleEvent(escape);
    assert(screen.closed() && !screen.imported());
    screen.handleEvent(drop);
    assert(!screen.imported());

    for (Uint32 event_type : {SDL_EVENT_QUIT, SDL_EVENT_WINDOW_CLOSE_REQUESTED}) {
        voyage::platform::JarIntakeScreen closing(root / "cache");
        SDL_Event close{};
        close.type = event_type;
        closing.handleEvent(close);
        assert(closing.closed() && !closing.imported());
    }
    return 0;
}
