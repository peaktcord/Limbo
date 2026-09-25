#include "src/storage/save_store.hpp"
#include "src/diagnostics/log.hpp"

#include <fstream>
#include <stdexcept>

namespace voyage::storage {
namespace {
constexpr size_t kMaxSaveBytes = 16 * 1024 * 1024;
}

std::filesystem::path DirectorySaveStore::path(const std::string &slot) const {
    if (slot.empty() || slot == "." || slot == ".." || slot.find_first_of("/\\:") != std::string::npos ||
        slot.find('\0') != std::string::npos) throw std::invalid_argument("Invalid save slot name");
    return root_ / (slot + ".rs");
}

std::optional<SaveRecords> DirectorySaveStore::load(const std::string &slot) {
    const auto file = path(slot);
    if (!std::filesystem::exists(file)) return std::nullopt;
    const auto size = std::filesystem::file_size(file);
    if (size > kMaxSaveBytes) throw std::runtime_error("Save exceeds size limit: " + file.string());
    std::ifstream input(file, std::ios::binary);
    if (!input) throw std::runtime_error("Cannot open save: " + file.string());
    Bytes data(static_cast<size_t>(size));
    if (!data.empty()) input.read(reinterpret_cast<char *>(data.data()), static_cast<std::streamsize>(data.size()));
    if (!input) throw std::runtime_error("Cannot read complete save: " + file.string());
    try {
        BinaryReader reader(data);
        const uint32_t count = reader.u32le();
        if (count > reader.remaining() / 4) throw std::runtime_error("Invalid record count " + std::to_string(count));
        SaveRecords records;
        records.reserve(count);
        for (uint32_t i = 0; i < count; ++i) {
            const uint32_t length = reader.u32le();
            records.push_back(reader.bytes(length));
        }
        if (reader.remaining() != 0) throw std::runtime_error("Unexpected trailing record data");
        return records;
    } catch (const std::exception &error) {
        throw std::runtime_error("Reading " + file.string() + ": " + error.what());
    }
}

void DirectorySaveStore::save(const std::string &slot, const SaveRecords &records) {
    const auto file = path(slot);
    auto temporary = file;
    temporary += ".tmp";
    size_t size = 4;
    for (const auto &record : records) {
        if (record.size() > kMaxSaveBytes - 4 || size > kMaxSaveBytes - 4 - record.size()) {
            throw std::runtime_error("Save exceeds size limit: " + file.string());
        }
        size += 4 + record.size();
    }
    BinaryWriter writer;
    writer.u32le(static_cast<uint32_t>(records.size()));
    for (const auto &record : records) {
        writer.u32le(static_cast<uint32_t>(record.size()));
        writer.bytes(record);
    }
    std::filesystem::create_directories(root_);
    bool opened = false;
    try {
        std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
        if (!output) throw std::runtime_error("Cannot open temporary save: " + temporary.string());
        opened = true;
        const auto &data = writer.data();
        output.write(reinterpret_cast<const char *>(data.data()), static_cast<std::streamsize>(data.size()));
        output.close();
        if (!output) throw std::runtime_error("Cannot finish writing save: " + file.string());
        std::filesystem::rename(temporary, file);
    } catch (...) {
        if (opened) {
            std::error_code ignored;
            std::filesystem::remove(temporary, ignored);
        }
        throw;
    }
    logging::write(logging::Level::Info, "Save", "Wrote " + file.string() + "; records=" + std::to_string(records.size()));
    if (on_persist_) on_persist_();
}

}
