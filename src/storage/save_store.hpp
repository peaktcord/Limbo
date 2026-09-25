#ifndef VOYAGE_STORAGE_SAVE_STORE_HPP
#define VOYAGE_STORAGE_SAVE_STORE_HPP

#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "src/storage/binary_io.hpp"

namespace voyage::storage {

using SaveRecords = std::vector<Bytes>;

class SaveStore {
public:
    virtual ~SaveStore() = default;
    virtual std::optional<SaveRecords> load(const std::string &slot) = 0;
    virtual void save(const std::string &slot, const SaveRecords &records) = 0;
};

class DirectorySaveStore final : public SaveStore {
public:
    explicit DirectorySaveStore(std::filesystem::path root, std::function<void()> on_persist = {})
        : root_(std::move(root)), on_persist_(std::move(on_persist)) {}
    std::optional<SaveRecords> load(const std::string &slot) override;
    void save(const std::string &slot, const SaveRecords &records) override;
    const std::filesystem::path &root() const { return root_; }
private:
    std::filesystem::path path(const std::string &slot) const;
    std::filesystem::path root_;
    std::function<void()> on_persist_;
};

}

#endif
