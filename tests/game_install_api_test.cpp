#include "src/game/game_install.hpp"
#include "src/game/sha256.hpp"

#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <utility>

namespace {
using Bytes = std::vector<uint8_t>;
using Files = std::vector<std::pair<std::string, std::string>>;

void u16(Bytes &bytes, uint16_t value) {
    bytes.push_back((uint8_t)value);
    bytes.push_back((uint8_t)(value >> 8));
}
void u32(Bytes &bytes, uint32_t value) {
    u16(bytes, (uint16_t)value);
    u16(bytes, (uint16_t)(value >> 16));
}
uint32_t crc32(const std::string &text) {
    uint32_t crc = 0xffffffffU;
    for (unsigned char c : text) {
        crc ^= c;
        for (int n = 0; n < 8; ++n) crc = (crc >> 1) ^ ((crc & 1) ? 0xedb88320U : 0);
    }
    return ~crc;
}

Bytes jar(const Files &files) {
    Bytes archive, directory;
    for (const auto &file : files) {
        const auto offset = (uint32_t)archive.size();
        const auto size = (uint32_t)file.second.size();
        const auto name_size = (uint16_t)file.first.size();
        u32(archive, 0x04034b50); u16(archive, 20); u16(archive, 0);
        u16(archive, 0); u16(archive, 0); u16(archive, 0);
        u32(archive, crc32(file.second)); u32(archive, size); u32(archive, size);
        u16(archive, name_size); u16(archive, 0);
        archive.insert(archive.end(), file.first.begin(), file.first.end());
        archive.insert(archive.end(), file.second.begin(), file.second.end());
        u32(directory, 0x02014b50); u16(directory, 20); u16(directory, 20);
        u16(directory, 0); u16(directory, 0); u16(directory, 0); u16(directory, 0);
        u32(directory, crc32(file.second)); u32(directory, size); u32(directory, size);
        u16(directory, name_size); u16(directory, 0); u16(directory, 0);
        u16(directory, 0); u16(directory, 0); u32(directory, 0); u32(directory, offset);
        directory.insert(directory.end(), file.first.begin(), file.first.end());
    }
    const auto central_offset = (uint32_t)archive.size();
    archive.insert(archive.end(), directory.begin(), directory.end());
    u32(archive, 0x06054b50); u16(archive, 0); u16(archive, 0);
    u16(archive, (uint16_t)files.size()); u16(archive, (uint16_t)files.size());
    u32(archive, (uint32_t)directory.size()); u32(archive, central_offset); u16(archive, 0);
    return archive;
}

void set_data_root(const std::filesystem::path &path) {
#ifdef _WIN32
    assert(_putenv_s("LOCALAPPDATA", path.string().c_str()) == 0);
#else
    assert(setenv("XDG_DATA_HOME", path.string().c_str(), 1) == 0);
#endif
}
}

int main() {
    namespace fs = std::filesystem;
    const char *test_tmp = std::getenv("TEST_TMPDIR");
    assert(test_tmp != nullptr);
    const fs::path root = fs::path(test_tmp) / "install-api";
    fs::create_directories(root);
    set_data_root(root / "userdata");
    const uint8_t abc[] = {'a', 'b', 'c'};
    assert(voyage::crypto::sha256_hex(voyage::crypto::sha256(abc, sizeof abc)) ==
           "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    assert(voyage::crypto::sha256_hex(voyage::crypto::sha256(nullptr, 0)) ==
           "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");

    std::filesystem::path cache = voyage::install::colocated_cache_directory("bin/game.exe");
    assert(cache.filename() == "oblivion");
    assert(!voyage::install::validate_cache(cache).valid);

    bool rejected = false;
    try {
        voyage::install::prepare_resources(cache, std::nullopt);
    } catch (const std::exception &) {
        rejected = true;
    }
    assert(rejected);

    const fs::path exe = root / "bin" / "game.exe";
    const std::filesystem::path colocated = voyage::install::colocated_save_directory(exe);
    assert(colocated == exe.parent_path() / "saves" / "rms");
    assert(colocated.is_absolute());

    assert(voyage::install::colocated_save_directory(exe) == colocated);

    const std::filesystem::path user = voyage::install::user_save_directory();
    assert(user.empty() || user.is_absolute());
    assert(user != colocated);

    const std::filesystem::path resolved = voyage::install::resolve_save_directory(exe);
    assert(resolved == user);
    assert(voyage::install::directory_is_writable(resolved));
    assert(voyage::install::resolve_cache_directory(exe) == voyage::install::user_cache_directory());
    const fs::path portable = exe.parent_path() / "data";
    assert(voyage::install::resolve_save_directory(exe, true) == portable / "saves" / "rms");
    assert(voyage::install::resolve_cache_directory(exe, true) == portable / "oblivion");
    fs::create_directories(exe.parent_path());
    { std::ofstream marker(exe.parent_path() / "portable.txt"); }
    assert(voyage::install::resolve_data_directory(exe) == portable);
    fs::remove(exe.parent_path() / "portable.txt");

    fs::create_directories(colocated);
    { std::ofstream legacy(colocated / "ESO.rs"); legacy << "old"; }
    assert(voyage::install::resolve_save_directory(exe) == resolved);
    assert(fs::file_size(resolved / "ESO.rs") == 3);
    { std::ofstream current(resolved / "ESO.rs"); current << "newer"; }
    assert(voyage::install::resolve_save_directory(exe) == resolved);
    assert(fs::file_size(resolved / "ESO.rs") == 5);
    assert(fs::file_size(colocated / "ESO.rs") == 3);

    assert(!std::filesystem::exists(resolved / ".voyage-write-probe"));

    Files resources{{"startup.scr", "script"}, {"startup.cml", "atlas"},
                    {"oh_pc.cml", "player"}, {"lang_0.txt", "text"},
                    {"l01_1.scr", "level"}, {"l01_1.jtm", "map"},
                    {"META-INF/MANIFEST.MF", "MIDlet-Name: Elder Scrolls\r\n"}};
    cache = root / "cache";
    const auto imported = voyage::install::import_resources(cache, jar(resources));
    assert(imported.imported && voyage::install::validate_cache(cache).valid);
    assert(voyage::install::prepare_resources(cache, std::nullopt).archive_sha256 == imported.archive_sha256);

    auto refused = [&](const Bytes &bytes) {
        bool failed = false;
        try { voyage::install::import_resources(cache, bytes); }
        catch (const std::exception &) { failed = true; }
        assert(failed);
        assert(voyage::install::validate_cache(cache).valid);
        assert(voyage::install::prepare_resources(cache, std::nullopt).archive_sha256 == imported.archive_sha256);
    };
    refused({1, 2, 3});
    Files invalid = resources;
    invalid.erase(invalid.begin());
    refused(jar(invalid));
    invalid = resources;
    invalid.back().second = "MIDlet-Name: Dawnstar\r\n";
    refused(jar(invalid));
    invalid.back() = {"meta-inf/manifest.mf", "MIDlet-Name: Stormhold\r\n"};
    refused(jar(invalid));
    for (const char *name : {"../escape", "STARTUP.SCR", ".VOYAGE-INSTALL", "aux.txt", "dir./file"}) {
        invalid = resources;
        invalid.emplace_back(name, "bad");
        refused(jar(invalid));
    }
    Bytes broken = jar(resources);
    broken[30 + resources[0].first.size()] ^= 1;
    refused(broken);
    broken = jar(resources);
    broken[30] = 'x';
    refused(broken);
    assert(!fs::exists(root / "escape"));

    resources.pop_back();
    resources.emplace_back("repack.txt", "different packaging");
    const fs::path renamed = root / "stormhold-definitely.JAR";
    const Bytes repacked = jar(resources);
    { std::ofstream file(renamed, std::ios::binary);
      file.write((const char *)repacked.data(), (std::streamsize)repacked.size()); }
    const auto changed = voyage::install::prepare_resources(cache, renamed);
    assert(changed.archive_sha256 != imported.archive_sha256);
    assert(voyage::install::validate_cache(cache).valid);
    { std::ofstream tamper(cache / "startup.scr", std::ios::app); tamper << "corrupt"; }
    assert(!voyage::install::validate_cache(cache).valid);
    voyage::install::prepare_resources(cache, renamed);
    assert(voyage::install::validate_cache(cache).valid);
    return 0;
}
