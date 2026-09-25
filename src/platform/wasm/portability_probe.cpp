#include <SDL3/SDL.h>

#include <cstdint>
#include <stdexcept>
#include <fstream>

#include "src/game/game_install.hpp"
#include "src/game/game_runtime.hpp"
#include "src/game/sha256.hpp"
#include "src/game/save_codec.hpp"
#include "src/storage/save_store.hpp"
#include "src/storage/resource_store.hpp"
#include "src/text/text.hpp"

int main() {
    if (SDL_GetVersion() != SDL_VERSIONNUM(3, 4, 14)) return 1;

    const uint8_t abc[] = {'a', 'b', 'c'};
    if (voyage::crypto::sha256_hex(voyage::crypto::sha256(abc, sizeof abc)) !=
        "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad") {
        return 2;
    }

    if (voyage::kPhoneWidth != 176 || voyage::kWideWidth != 391 ||
        voyage::kScreenHeight != 220) {
        return 3;
    }
    if (!voyage::GameRuntime::is_movement(voyage::Input::Up) ||
        voyage::GameRuntime::is_movement(voyage::Input::Action)) {
        return 4;
    }

    bool caught_library_exception = false;
    try {
        (void)voyage::crypto::sha256_file("/__limbo_wasm_probe_missing__");
    } catch (const std::runtime_error &) {
        caught_library_exception = true;
    }
    if (!caught_library_exception) return 5;

    auto importer = static_cast<voyage::install::PreparedResources (*)(
        const std::filesystem::path &, const std::vector<uint8_t> &)>(
        &voyage::install::import_resources);
    (void)importer;

    voyage::storage::DirectorySaveStore saves("/probe-saves");
    voyage::save::Game settings;
    settings.intro_seen = true;
    settings.key_bindings = {49, 50, 51};
    saves.save(voyage::save::kSlot, {voyage::save::encode(settings)});
    const auto records = saves.load(voyage::save::kSlot);
    if (!records || records->size() != 1) return 6;
    const auto restored = voyage::save::decode(records->front());
    if (!restored.intro_seen || restored.player || restored.key_bindings[0] != 49) return 7;

    const std::string resource_text("A\0\x80\xffz", 5);
    if (voyage::text::length(resource_text) != 5 ||
        voyage::text::byte_at(resource_text, 2) != 128 ||
        voyage::text::index_of(resource_text, 255) != 3 ||
        voyage::text::index_of(resource_text, 'x') != -1 ||
        voyage::text::slice(resource_text, 1, 4) != std::string("\0\x80\xff", 3) ||
        voyage::text::upper_ascii(resource_text) != std::string("A\0\x80\xffZ", 5)) return 8;
    voyage::RuntimeConfig config;
    config.resource_dir = "/__oblivion_probe_missing_resources__";
    config.save_dir = "/probe-saves";
    voyage::GameRuntime runtime(config);
    runtime.pause();
    runtime.resume();
    bool startup_failed = false;
    try { runtime.start(); } catch (const std::exception &) { startup_failed = true; }
    if (!startup_failed || runtime.running() || runtime.paused() || runtime.state() != -1) return 9;
    runtime.stop();
    if (voyage::text::parse_int32("-2147483648") != INT32_MIN ||
        voyage::text::parse_int32("2147483647") != INT32_MAX) return 10;
    bool invalid_number = false;
    try { (void)voyage::text::parse_int32("12x"); }
    catch (const std::invalid_argument &) { invalid_number = true; }
    if (!invalid_number) return 11;
    bool overflow = false;
    try { (void)voyage::text::parse_int32("2147483648"); }
    catch (const std::out_of_range &) { overflow = true; }
    if (!overflow) return 12;
    std::filesystem::create_directories("/probe-resources");
    const std::vector<uint8_t> raw{0, 13, 10, 26, 128, 255};
    {
        std::ofstream output("/probe-resources/bytes", std::ios::binary);
        output.write(reinterpret_cast<const char *>(raw.data()), static_cast<std::streamsize>(raw.size()));
    }
    voyage::storage::DirectoryResourceStore resources("/probe-resources");
    if (voyage::storage::require_resource(&resources, "/bytes") != raw) return 13;
    if (voyage::storage::read_resource(&resources, "/missing")) return 14;
    bool missing_resource = false;
    try { (void)voyage::storage::require_resource(&resources, "/missing"); }
    catch (const std::logic_error &) { missing_resource = true; }
    if (!missing_resource) return 15;
    return 0;
}
