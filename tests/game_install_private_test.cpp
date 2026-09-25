#include "src/game/game_install.hpp"
#include "src/game/game_runtime.hpp"
#include "src/platform/sdl/jar_intake_screen.hpp"

#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <iterator>

int main(int argc, char **argv) {
    assert(argc > 1);
    const char *test_tmp = std::getenv("TEST_TMPDIR");
    assert(test_tmp != nullptr);
    std::filesystem::path first_cache;

    for (int n = 1; n < argc; ++n) {
        std::filesystem::path cache = std::filesystem::path(test_tmp) / ("oblivion-" + std::to_string(n));
        voyage::platform::JarIntakeScreen screen(cache);
        const voyage::Frame invitation = screen.frame();
        SDL_Event drop{};
        drop.type = SDL_EVENT_DROP_FILE;
        drop.drop.data = "missing.jar";
        screen.handleEvent(drop);
        assert(!screen.imported() && !screen.closed());
        assert(!screen.error().empty() && screen.frame().pixels != invitation.pixels);
        drop.drop.data = argv[n];
        screen.handleEvent(drop);
        assert(screen.imported() && !screen.closed() && screen.error().empty());
        voyage::install::PreparedResources imported = *screen.imported();
        assert(imported.imported);
        assert(imported.resource_dir == cache);
        assert(voyage::install::validate_cache(cache).valid);

        voyage::install::PreparedResources reused = voyage::install::prepare_resources(cache, std::nullopt);
        assert(!reused.imported);
        assert(reused.archive_sha256 == imported.archive_sha256);
        const auto manifest_path = cache / ".voyage-install";
        std::ifstream manifest_input(manifest_path, std::ios::binary);
        std::string manifest((std::istreambuf_iterator<char>(manifest_input)), {});
        manifest_input.close();
        manifest.replace(0, std::string("format 2\ngame oblivion\n").size(), "format 1\n");
        { std::ofstream legacy(manifest_path, std::ios::binary); legacy << manifest; }
        assert(voyage::install::validate_cache(cache).valid);

        std::ifstream archive_input(argv[n], std::ios::binary);
        std::vector<uint8_t> archive((std::istreambuf_iterator<char>(archive_input)), {});
        size_t eocd = archive.size() - 22;
        while (!(archive[eocd] == 0x50 && archive[eocd + 1] == 0x4b &&
                 archive[eocd + 2] == 5 && archive[eocd + 3] == 6 &&
                 eocd + 22 + archive[eocd + 20] + 256U * archive[eocd + 21] == archive.size())) {
            assert(eocd > 0);
            --eocd;
        }
        archive.resize(eocd + 22);
        archive[eocd + 20] = 1;
        archive[eocd + 21] = 0;
        archive.push_back('x');
        const auto repacked = voyage::install::import_resources(cache, archive);
        assert(repacked.archive_sha256 != imported.archive_sha256);
        assert(voyage::install::validate_cache(cache).valid);
        if (n == 1) first_cache = cache;
    }

    voyage::RuntimeConfig config;
    config.resource_dir = first_cache.string();
    config.save_dir = (std::filesystem::path(test_tmp) / "saves").string();
    config.random_seed = 0x4f626c6976696f6eLL;
    voyage::GameRuntime game(config);
    game.start();
    for (int n = 0; n < 125; ++n) game.tick(16);
    assert(!game.has_error());
    assert(!game.copy_frame().pixels.empty());
    game.stop();

    std::ofstream corrupt(first_cache / "startup.scr", std::ios::binary | std::ios::app);
    assert(corrupt);
    corrupt.put('\0');
    corrupt.close();
    assert(!voyage::install::validate_cache(first_cache).valid);

    voyage::install::prepare_resources(first_cache, std::filesystem::path(argv[1]));
    assert(voyage::install::validate_cache(first_cache).valid);
    return 0;
}
