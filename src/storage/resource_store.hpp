#ifndef VOYAGE_STORAGE_RESOURCE_STORE_HPP
#define VOYAGE_STORAGE_RESOURCE_STORE_HPP

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace voyage::storage {

class ResourceStore {
public:
    virtual ~ResourceStore() = default;
    virtual std::optional<std::vector<uint8_t>> read(const std::string &path) const = 0;
};

class DirectoryResourceStore final : public ResourceStore {
public:
    explicit DirectoryResourceStore(std::filesystem::path root);
    std::optional<std::vector<uint8_t>> read(const std::string &path) const override;
private:
    std::filesystem::path root_;
};

std::optional<std::vector<uint8_t>> read_resource(const ResourceStore *store, const std::string &name);

std::vector<uint8_t> require_resource(const ResourceStore *store, const std::string &name);

}

#endif
