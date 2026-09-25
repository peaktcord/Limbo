#include "src/diagnostics/log.hpp"

#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <string>
#include <stdexcept>
#include <thread>

namespace {
int notifications = 0;
void persisted() { ++notifications; }
std::string read(const std::filesystem::path &path) {
    std::ifstream file(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(file), {});
}
}

int main() {
    namespace fs = std::filesystem;
    namespace log = voyage::logging;
    const char *temp = std::getenv("TEST_TMPDIR");
    assert(temp != nullptr);
    const fs::path root = fs::path(temp) / "logging";
    const fs::path path = root / "limbo.log";
    log::open_file(path);
    log::set_persist_hook(persisted);
    log::write(log::Level::Info, "Startup", "first session");
    log::exception("Save", "Writing ESO", std::runtime_error("disk full\nmore detail"));
    assert(notifications == 2);
    const std::string first = read(path);
    assert(first.find("Z [INFO] Startup: first session\n") != std::string::npos);
    assert(first.find("[ERROR] Save: Writing ESO: disk full\\nmore detail\n") != std::string::npos);
    log::close_file();
    log::write(log::Level::Error, "Test", "no sink");
    assert(read(path) == first);

    log::open_file(path);
    log::write(log::Level::Info, "Startup", "second session");
    log::close_file();
    assert(read(path.string() + ".prev") == first);
    assert(read(path).find("first session") == std::string::npos);
    log::open_file(path);
    log::write(log::Level::Info, "Startup", "third session");
    assert(read(path.string() + ".prev").find("second session") != std::string::npos);

    log::set_context("main thread");
    std::thread worker([] {
        log::set_context("SDL worker");
        assert(std::string(log::last_context()) == "SDL worker");
        log::write(log::Level::Warning, "SDL", "renderer warning");
    });
    worker.join();
    assert(std::string(log::last_context()) == "main thread");
    assert(read(path).find("[WARNING] SDL: renderer warning") != std::string::npos);
    log::set_context(std::string(4096, 'x'));
    assert(std::string(log::last_context()).size() == 2047);
    log::close_file();

    const fs::path blocked = root / "blocked.log";
    { std::ofstream file(blocked); file << "previous crash"; }
    fs::create_directory(blocked.string() + ".prev");
    bool failed = false;
    try { log::open_file(blocked); }
    catch (const std::exception &) { failed = true; }
    assert(failed && log::file_handle() == nullptr);
    assert(read(blocked) == "previous crash");
    return 0;
}
