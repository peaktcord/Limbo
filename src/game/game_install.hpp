#ifndef VOYAGE_GAME_INSTALL_HPP
#define VOYAGE_GAME_INSTALL_HPP

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace voyage::install {

struct CacheValidation {
    bool valid = false;
    std::string reason;
    std::string canonical_jar_name;
    std::string archive_sha256;
};

struct PreparedResources {
    std::filesystem::path resource_dir;
    bool imported = false;
    std::string canonical_jar_name;
    std::string archive_sha256;
};

CacheValidation validate_cache(const std::filesystem::path &cache_dir);

PreparedResources prepare_resources(const std::filesystem::path &cache_dir,
                                    const std::optional<std::filesystem::path> &jar_path);

PreparedResources import_resources(const std::filesystem::path &cache_dir,
                                   const std::vector<uint8_t> &jar_bytes);

std::filesystem::path colocated_cache_directory(const std::filesystem::path &executable_path);

std::filesystem::path user_cache_directory();

std::filesystem::path colocated_save_directory(const std::filesystem::path &executable_path);

std::filesystem::path user_save_directory();

bool directory_is_writable(const std::filesystem::path &dir);

std::filesystem::path resolve_data_directory(const std::filesystem::path &executable_path,
                                             bool portable = false);
std::filesystem::path resolve_cache_directory(const std::filesystem::path &executable_path,
                                              bool portable = false);
std::filesystem::path resolve_save_directory(const std::filesystem::path &executable_path,
                                             bool portable = false);
std::filesystem::path resolve_settings_directory(const std::filesystem::path &executable_path,
                                                 bool portable = false);

}

#endif
