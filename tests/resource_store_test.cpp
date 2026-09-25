#include "src/storage/resource_store.hpp"
#include "src/diagnostics/log.hpp"

#include <cassert>
#include <cstdlib>
#include <fstream>
#include <stdexcept>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace {
using Bytes = std::vector<uint8_t>;
using namespace voyage::storage;

template <typename Exception, typename F>
void rejects(F operation, const std::string &context) {
    bool rejected = false;
    try { operation(); }
    catch (const Exception &error) {
        rejected = true;
        assert(std::string(error.what()).find(context) != std::string::npos);
    }
    assert(rejected);
}

void write(const std::filesystem::path &path, const Bytes &bytes) {
    std::ofstream output(path, std::ios::binary);
    if (!bytes.empty()) output.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    output.close();
    assert(output);
}

class MemoryResources final : public ResourceStore {
public:
    mutable std::string requested;
    std::optional<Bytes> read(const std::string &path) const override {
        requested = path;
        if (path == "nested/bytes") return Bytes{0, 128, 255};
        return std::nullopt;
    }
};

std::string throw_site;
void record_throw(const char *message) { throw_site = message; }
}

int main() {
    namespace fs = std::filesystem;
    const char *temp = std::getenv("TEST_TMPDIR");
    assert(temp != nullptr);
    const auto root = fs::path(temp) / "native-resources";
    const auto directory = root / "nested";
    fs::create_directories(directory);
    DirectoryResourceStore store(root);
    assert(!store.read("absent.bin"));
    assert(!read_resource(&store, "/absent.bin"));
    assert(!read_resource(nullptr, "/absent.bin"));
    write(root / "empty", {});
    assert(store.read("empty") && store.read("empty")->empty());
    assert(require_resource(&store, "/empty").empty());

    Bytes expected;
    for (size_t i = 0; i < 70000; ++i) expected.push_back(static_cast<uint8_t>(i));
    write(directory / "all-bytes.bin", expected);
    auto result = read_resource(&store, "/nested/all-bytes.bin");
    assert(result && *result == expected);
    assert(store.read("nested/all-bytes.bin") == result);
    write(directory / "all-bytes.bin", {7});
    assert(*result == expected);
    assert(require_resource(&store, "/nested/all-bytes.bin") == Bytes{7});

    rejects<std::runtime_error>([&] { store.read("nested"); }, "nested");
#ifdef _WIN32
    HANDLE locked = CreateFileW((directory / "all-bytes.bin").c_str(), GENERIC_READ,
                                0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    assert(locked != INVALID_HANDLE_VALUE);
    rejects<std::runtime_error>([&] { store.read("nested/all-bytes.bin"); }, "all-bytes.bin");
    CloseHandle(locked);
    assert(require_resource(&store, "/nested/all-bytes.bin") == Bytes{7});
#endif
    fs::remove(directory / "all-bytes.bin");
    assert(!read_resource(&store, "/nested/all-bytes.bin"));
    assert(*result == expected);

    MemoryResources memory;
    auto copy = require_resource(&memory, "/nested/bytes");
    assert(memory.requested == "nested/bytes");
    assert((copy == Bytes{0, 128, 255}));
    copy[0] = 42;
    assert((require_resource(&memory, "nested/bytes") == Bytes{0, 128, 255}));
    const auto last_request = memory.requested;
    for (const std::string name : {"", "/", "//escape", "../escape", "nested/../escape", "./file",
                                   "C:/escape", "C:escape", "nested\\file"}) {
        rejects<std::invalid_argument>([&] { read_resource(&memory, name); }, "Invalid resource path");
        assert(memory.requested == last_request);
    }
    rejects<std::invalid_argument>([&] { read_resource(&memory, std::string("file\0extra", 10)); }, "Invalid resource path");
    rejects<std::invalid_argument>([&] { store.read("../escape"); }, "Invalid resource path");

    voyage::logging::set_throw_trace_hook(record_throw);
    rejects<std::logic_error>([&] { require_resource(&store, "/missing.txt"); }, "Resource not found: /missing.txt");
    assert(throw_site == "Resource not found: /missing.txt");
    rejects<std::logic_error>([&] { require_resource(nullptr, "/unbound.txt"); }, "Resource not found: /unbound.txt");
    assert(throw_site == "Resource not found: /unbound.txt");
    voyage::logging::set_throw_trace_hook(nullptr);
}
