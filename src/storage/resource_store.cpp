#include "src/storage/resource_store.hpp"

#include "src/diagnostics/log.hpp"

#include <fstream>
#include <limits>
#include <stdexcept>
#include <utility>

namespace voyage::storage {
namespace {
std::string relative_name(const std::string &name) {
    const std::string path = !name.empty() && name.front() == '/' ? name.substr(1) : name;
    const std::filesystem::path file(path);
    if (path.empty() || path.find('\0') != std::string::npos ||
        path.find('\\') != std::string::npos || path.find(':') != std::string::npos || file.has_root_path()) {
        throw std::invalid_argument("Invalid resource path: " + name);
    }
    for (const auto &part : file) {
        if (part == ".." || part == ".") throw std::invalid_argument("Invalid resource path: " + name);
    }
    return path;
}
}

DirectoryResourceStore::DirectoryResourceStore(std::filesystem::path root) : root_(std::move(root)) {}

std::optional<std::vector<uint8_t>> DirectoryResourceStore::read(const std::string &path) const {
    const auto file = root_ / relative_name(path);
    std::error_code error;
    const auto status = std::filesystem::status(file, error);
    if (error == std::errc::no_such_file_or_directory) return std::nullopt;
    if (error) throw std::filesystem::filesystem_error("Cannot inspect resource", file, error);
    if (!std::filesystem::exists(status)) return std::nullopt;
    if (!std::filesystem::is_regular_file(status)) throw std::runtime_error("Resource is not a file: " + file.string());

    std::ifstream input(file, std::ios::binary | std::ios::ate);
    if (!input) throw std::runtime_error("Cannot open resource: " + file.string());
    const std::streamoff size = input.tellg();
    if (size < 0) throw std::runtime_error("Cannot measure resource: " + file.string());
    std::vector<uint8_t> bytes;
    if (static_cast<uintmax_t>(size) > bytes.max_size() || size > std::numeric_limits<std::streamsize>::max()) {
        throw std::length_error("Resource is too large: " + file.string());
    }
    input.seekg(0);
    if (!input) throw std::runtime_error("Cannot seek resource: " + file.string());
    bytes.resize(static_cast<size_t>(size));
    if (!bytes.empty()) input.read(reinterpret_cast<char *>(bytes.data()), static_cast<std::streamsize>(size));
    if (!input) throw std::runtime_error("Cannot read complete resource: " + file.string());
    return bytes;
}

std::optional<std::vector<uint8_t>> read_resource(const ResourceStore *store, const std::string &name) {
    const auto path = relative_name(name);
    return store == nullptr ? std::nullopt : store->read(path);
}

std::vector<uint8_t> require_resource(const ResourceStore *store, const std::string &name) {
    auto bytes = read_resource(store, name);
    if (!bytes) {
        const std::string message = "Resource not found: " + name;
        logging::trace_throw(message.c_str());
        throw std::logic_error(message);
    }
    return std::move(*bytes);
}

}
