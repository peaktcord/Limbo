#include "src/game/game_install.hpp"

#include <zlib.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <cstdint>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <vector>

#include "src/game/sha256.hpp"
#include "supported_game_jars.hpp"

namespace voyage::install {
namespace {

namespace fs = std::filesystem;
using generated::SupportedJar;

constexpr uint64_t kMaximumArchiveBytes = 32ULL * 1024 * 1024;
constexpr uint64_t kMaximumEntryBytes = 64ULL * 1024 * 1024;
constexpr uint64_t kMaximumExtractedBytes = 256ULL * 1024 * 1024;
constexpr uint32_t kMaximumEntries = 4096;
constexpr const char *kInstallManifest = ".voyage-install";

struct ZipEntry {
    std::string name;
    uint16_t method = 0;
    uint32_t crc = 0;
    uint32_t compressed_size = 0;
    uint32_t uncompressed_size = 0;
    uint32_t local_offset = 0;
};

uint16_t read_u16(const std::vector<uint8_t> &bytes, size_t offset) {
    if (offset > bytes.size() || bytes.size() - offset < 2) throw std::runtime_error("truncated ZIP field");
    return (uint16_t)((uint16_t)bytes[offset] | ((uint16_t)bytes[offset + 1] << 8));
}

uint32_t read_u32(const std::vector<uint8_t> &bytes, size_t offset) {
    if (offset > bytes.size() || bytes.size() - offset < 4) throw std::runtime_error("truncated ZIP field");
    return (uint32_t)bytes[offset] | ((uint32_t)bytes[offset + 1] << 8) |
           ((uint32_t)bytes[offset + 2] << 16) | ((uint32_t)bytes[offset + 3] << 24);
}

std::vector<uint8_t> read_file(const fs::path &path, uint64_t maximum_size) {
    std::error_code ec;
    uint64_t size = fs::file_size(path, ec);
    if (ec) throw std::runtime_error("cannot measure file: " + path.string());
    if (size > maximum_size) throw std::runtime_error("file exceeds the safety limit: " + path.string());
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("cannot open file: " + path.string());
    std::vector<uint8_t> bytes((size_t)size);
    if (!bytes.empty() && !input.read((char *)bytes.data(), (std::streamsize)bytes.size())) {
        throw std::runtime_error("cannot read file: " + path.string());
    }
    return bytes;
}

const SupportedJar *find_supported(const std::string &digest, uint64_t size) {
    for (size_t n = 0; n < generated::kSupportedJarCount; ++n) {
        const SupportedJar &jar = generated::kSupportedJars[n];
        if (digest == jar.archive_sha256 && size == jar.archive_size) return &jar;
    }
    return nullptr;
}

const SupportedJar *find_supported(const std::string &digest) {
    for (size_t n = 0; n < generated::kSupportedJarCount; ++n) {
        if (digest == generated::kSupportedJars[n].archive_sha256) return &generated::kSupportedJars[n];
    }
    return nullptr;
}

std::string lower(std::string value) {
    for (char &c : value) {
        if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
    }
    return value;
}

void require_oblivion_tree(const fs::path &root) {
    for (const char *name : {"startup.scr", "startup.cml", "oh_pc.cml", "lang_0.txt",
                             "l01_1.scr", "l01_1.jtm"}) {
        if (!fs::is_regular_file(root / name)) {
            throw std::runtime_error(std::string("not a usable Oblivion JAR: missing ") + name);
        }
    }
    for (const auto &directory : fs::directory_iterator(root)) {
        if (lower(directory.path().filename().string()) != "meta-inf" || !directory.is_directory()) continue;
        for (const auto &file : fs::directory_iterator(directory.path())) {
            if (lower(file.path().filename().string()) != "manifest.mf" || !file.is_regular_file()) continue;
            const auto bytes = read_file(file.path(), kMaximumEntryBytes);
            const std::string manifest = lower(std::string(bytes.begin(), bytes.end()));
            if (manifest.find("dawnstar") != std::string::npos || manifest.find("stormhold") != std::string::npos) {
                throw std::runtime_error("JAR resources look like Oblivion but its manifest names another game");
            }
        }
    }
}

bool safe_entry_name(const std::string &name, bool directory) {
    if (name.empty() || name.size() > 512 || name[0] == '/' || name[0] == '\\' ||
        name.find('\\') != std::string::npos || name.find(':') != std::string::npos ||
        name.find_first_of("<>\"|?*") != std::string::npos ||
        std::any_of(name.begin(), name.end(), [](unsigned char c) { return c < 32; })) {
        return false;
    }
    size_t start = 0;
    while (start < name.size()) {
        size_t end = name.find('/', start);
        if (end == std::string::npos) end = name.size();
        std::string component = name.substr(start, end - start);
        if (component.empty()) return directory && end + 1 == name.size();
        if (component.back() == '.' || component.back() == ' ') return false;
        const std::string stem = lower(component.substr(0, component.find('.')));
        if (stem == "con" || stem == "prn" || stem == "aux" || stem == "nul" ||
            (stem.size() == 4 && (stem.compare(0, 3, "com") == 0 || stem.compare(0, 3, "lpt") == 0) &&
             stem[3] >= '1' && stem[3] <= '9')) return false;
        start = end + 1;
    }
    return true;
}

std::vector<ZipEntry> parse_zip(const std::vector<uint8_t> &bytes) {
    if (bytes.size() < 22) throw std::runtime_error("JAR is too short to be a ZIP archive");
    size_t search_start = bytes.size() > 65557 ? bytes.size() - 65557 : 0;
    size_t eocd = SIZE_MAX;
    for (size_t offset = bytes.size() - 22;; --offset) {
        if (read_u32(bytes, offset) == 0x06054b50U) {
            uint16_t comment_length = read_u16(bytes, offset + 20);
            if (offset + 22 + comment_length == bytes.size()) {
                eocd = offset;
                break;
            }
        }
        if (offset == search_start) break;
    }
    if (eocd == SIZE_MAX) throw std::runtime_error("JAR has no valid ZIP end record");
    if (read_u16(bytes, eocd + 4) != 0 || read_u16(bytes, eocd + 6) != 0) {
        throw std::runtime_error("multi-disk ZIP archives are not supported");
    }
    uint16_t disk_entries = read_u16(bytes, eocd + 8);
    uint16_t entry_count = read_u16(bytes, eocd + 10);
    uint32_t central_size = read_u32(bytes, eocd + 12);
    uint32_t central_offset = read_u32(bytes, eocd + 16);
    if (disk_entries == 0xffffU || entry_count == 0xffffU || central_size == 0xffffffffU ||
        central_offset == 0xffffffffU) {
        throw std::runtime_error("ZIP64 JARs are not supported");
    }
    if (disk_entries != entry_count || entry_count > kMaximumEntries) {
        throw std::runtime_error("invalid or excessive ZIP entry count");
    }
    if ((uint64_t)central_offset + central_size > eocd) {
        throw std::runtime_error("ZIP central directory is out of bounds");
    }

    std::vector<ZipEntry> entries;
    std::set<std::string> names;
    uint64_t total_size = 0;
    size_t cursor = central_offset;
    for (uint32_t n = 0; n < entry_count; ++n) {
        if (read_u32(bytes, cursor) != 0x02014b50U) throw std::runtime_error("invalid ZIP directory entry");
        uint16_t version_made_by = read_u16(bytes, cursor + 4);
        uint16_t flags = read_u16(bytes, cursor + 8);
        uint16_t method = read_u16(bytes, cursor + 10);
        uint32_t crc = read_u32(bytes, cursor + 16);
        uint32_t compressed_size = read_u32(bytes, cursor + 20);
        uint32_t uncompressed_size = read_u32(bytes, cursor + 24);
        uint16_t name_length = read_u16(bytes, cursor + 28);
        uint16_t extra_length = read_u16(bytes, cursor + 30);
        uint16_t comment_length = read_u16(bytes, cursor + 32);
        uint16_t disk = read_u16(bytes, cursor + 34);
        uint32_t external_attributes = read_u32(bytes, cursor + 38);
        uint32_t local_offset = read_u32(bytes, cursor + 42);
        uint64_t record_size = 46ULL + name_length + extra_length + comment_length;
        if (cursor > bytes.size() || record_size > bytes.size() - cursor ||
            cursor + record_size > (uint64_t)central_offset + central_size) {
            throw std::runtime_error("truncated ZIP directory entry");
        }
        std::string name((const char *)&bytes[cursor + 46], name_length);
        bool directory = !name.empty() && name.back() == '/';
        if (!safe_entry_name(name, directory)) throw std::runtime_error("unsafe ZIP entry path: " + name);
        if (lower(name.substr(0, name.find('/'))) == kInstallManifest) {
            throw std::runtime_error("JAR contains the reserved cache manifest name");
        }
        if ((flags & 1U) != 0) throw std::runtime_error("encrypted ZIP entries are not supported");
        if (method != 0 && method != 8) throw std::runtime_error("unsupported ZIP compression method");
        if (disk != 0 || compressed_size == 0xffffffffU || uncompressed_size == 0xffffffffU ||
            local_offset == 0xffffffffU) {
            throw std::runtime_error("ZIP64 or multi-disk entries are not supported");
        }
        uint16_t host = (uint16_t)(version_made_by >> 8);
        uint32_t unix_mode = external_attributes >> 16;
        if (host == 3 && (unix_mode & 0170000U) == 0120000U) {
            throw std::runtime_error("symbolic links are not allowed in a game JAR");
        }
        if (!directory) {
            if (!names.insert(lower(name)).second) throw std::runtime_error("duplicate ZIP entry: " + name);
            if (uncompressed_size > kMaximumEntryBytes || total_size > kMaximumExtractedBytes - uncompressed_size) {
                throw std::runtime_error("JAR expands beyond the safety limit");
            }
            total_size += uncompressed_size;
            entries.push_back(ZipEntry{name, method, crc, compressed_size, uncompressed_size, local_offset});
        }
        cursor += (size_t)record_size;
    }
    if (cursor != (uint64_t)central_offset + central_size) {
        throw std::runtime_error("ZIP central directory length does not match its entries");
    }
    return entries;
}

std::vector<uint8_t> inflate_entry(const std::vector<uint8_t> &archive, const ZipEntry &entry,
                                   size_t central_offset) {
    size_t local = entry.local_offset;
    if (read_u32(archive, local) != 0x04034b50U) throw std::runtime_error("invalid ZIP local header");
    uint16_t local_flags = read_u16(archive, local + 6);
    uint16_t local_method = read_u16(archive, local + 8);
    uint16_t name_length = read_u16(archive, local + 26);
    uint16_t extra_length = read_u16(archive, local + 28);
    uint64_t data_offset = (uint64_t)local + 30 + name_length + extra_length;
    if ((local_flags & 1U) != 0 || local_method != entry.method || data_offset > central_offset ||
        entry.compressed_size > central_offset - data_offset) {
        throw std::runtime_error("invalid ZIP local entry bounds");
    }
    if (std::string((const char *)archive.data() + local + 30, name_length) != entry.name) {
        throw std::runtime_error("ZIP local entry name disagrees with its directory");
    }
    const uint8_t *compressed = archive.data() + (size_t)data_offset;
    std::vector<uint8_t> output(entry.uncompressed_size);
    if (entry.method == 0) {
        if (entry.compressed_size != entry.uncompressed_size) throw std::runtime_error("invalid stored ZIP entry");
        if (!output.empty()) std::copy(compressed, compressed + entry.compressed_size, output.begin());
    } else {
        z_stream stream{};
        stream.next_in = const_cast<Bytef *>(compressed);
        stream.avail_in = (uInt)entry.compressed_size;
        uint8_t dummy = 0;
        stream.next_out = output.empty() ? &dummy : output.data();
        stream.avail_out = output.empty() ? 1U : (uInt)output.size();
        if (inflateInit2(&stream, -MAX_WBITS) != Z_OK) throw std::runtime_error("cannot initialize deflate");
        int result = inflate(&stream, Z_FINISH);
        inflateEnd(&stream);
        if (result != Z_STREAM_END || stream.total_out != entry.uncompressed_size ||
            stream.total_in != entry.compressed_size) {
            throw std::runtime_error("invalid deflate stream in ZIP entry: " + entry.name);
        }
    }
    uLong actual_crc = crc32(0L, Z_NULL, 0);
    actual_crc = crc32(actual_crc, output.empty() ? Z_NULL : output.data(), (uInt)output.size());
    if ((uint32_t)actual_crc != entry.crc) throw std::runtime_error("CRC mismatch in ZIP entry: " + entry.name);
    return output;
}

std::map<std::string, std::string> read_install_manifest(const fs::path &path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("cache completion manifest is missing");
    std::map<std::string, std::string> fields;
    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        size_t separator = line.find(' ');
        if (separator == std::string::npos || separator == 0 || separator + 1 >= line.size()) {
            throw std::runtime_error("cache completion manifest is malformed");
        }
        if (!fields.emplace(line.substr(0, separator), line.substr(separator + 1)).second) {
            throw std::runtime_error("cache completion manifest has duplicate fields");
        }
    }
    return fields;
}

std::string required_field(const std::map<std::string, std::string> &fields, const char *name) {
    auto found = fields.find(name);
    if (found == fields.end()) throw std::runtime_error(std::string("cache manifest lacks ") + name);
    return found->second;
}

uint64_t parse_decimal(const std::string &value, const char *name) {
    if (value.empty()) throw std::runtime_error(std::string("cache manifest has an empty ") + name);
    uint64_t result = 0;
    for (char character : value) {
        if (character < '0' || character > '9') {
            throw std::runtime_error(std::string("cache manifest has an invalid ") + name);
        }
        uint32_t digit = (uint32_t)(character - '0');
        if (result > (UINT64_MAX - digit) / 10U) {
            throw std::runtime_error(std::string("cache manifest overflows ") + name);
        }
        result = result * 10U + digit;
    }
    return result;
}

struct TreeIdentity {
    std::string digest;
    uint32_t entries = 0;
};

TreeIdentity content_tree_identity(const fs::path &root) {
    std::vector<std::pair<std::string, fs::path>> files;
    uint64_t total_size = 0;
    std::error_code ec;
    fs::recursive_directory_iterator iterator(root, fs::directory_options::none, ec), end;
    if (ec) throw std::runtime_error("cannot enumerate resource cache");
    for (; iterator != end; iterator.increment(ec)) {
        if (ec) throw std::runtime_error("cannot enumerate resource cache");
        fs::file_status status = iterator->symlink_status(ec);
        if (ec) throw std::runtime_error("cannot inspect resource cache entry");
        if (fs::is_symlink(status)) throw std::runtime_error("resource cache contains a symbolic link");
        if (fs::is_directory(status)) continue;
        if (!fs::is_regular_file(status)) throw std::runtime_error("resource cache contains a non-file entry");
        fs::path relative = fs::relative(iterator->path(), root, ec);
        if (ec) throw std::runtime_error("cannot relativize resource cache entry");
        std::string name = relative.generic_string();
        if (name == kInstallManifest) continue;
        uint64_t size = fs::file_size(iterator->path(), ec);
        if (ec || size > kMaximumEntryBytes || total_size > kMaximumExtractedBytes - size) {
            throw std::runtime_error("resource cache exceeds the safety limit");
        }
        total_size += size;
        files.emplace_back(name, iterator->path());
    }
    std::sort(files.begin(), files.end(), [](const auto &left, const auto &right) { return left.first < right.first; });
    if (files.size() > kMaximumEntries) throw std::runtime_error("resource cache has too many files");
    std::vector<uint8_t> tree;
    tree.reserve(files.size() * 64);
    for (const auto &file : files) {
        uint64_t size = fs::file_size(file.second);
        crypto::Sha256Digest digest = crypto::sha256_file(file.second);
        tree.insert(tree.end(), file.first.begin(), file.first.end());
        tree.push_back(0);
        for (int shift = 56; shift >= 0; shift -= 8) tree.push_back((uint8_t)(size >> shift));
        tree.insert(tree.end(), digest.begin(), digest.end());
    }
    return TreeIdentity{crypto::sha256_hex(crypto::sha256(tree)), (uint32_t)files.size()};
}

void write_install_manifest(const fs::path &root, const std::string &name,
                            const std::string &archive_hash, uint64_t archive_size,
                            const TreeIdentity &tree) {
    std::ofstream output(root / kInstallManifest, std::ios::binary | std::ios::trunc);
    if (!output) throw std::runtime_error("cannot write cache completion manifest");
    output << "format 2\n"
           << "game oblivion\n"
           << "canonical_name " << name << "\n"
           << "archive_sha256 " << archive_hash << "\n"
           << "archive_size " << archive_size << "\n"
           << "content_tree_sha256 " << tree.digest << "\n"
           << "entry_count " << tree.entries << "\n";
    output.close();
    if (!output) throw std::runtime_error("cannot finish cache completion manifest");
}

fs::path unique_sibling(const fs::path &cache, const char *kind) {
    static std::atomic<uint64_t> sequence{0};
    uint64_t stamp = (uint64_t)std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return cache.parent_path() /
           ("." + cache.filename().string() + "." + kind + "-" + std::to_string(stamp) + "-" +
            std::to_string(sequence.fetch_add(1)));
}

void require_safe_cache_target(const fs::path &cache) {
    if (cache.empty() || cache.filename().empty() || cache.parent_path().empty() || cache == cache.root_path()) {
        throw std::runtime_error("resource cache must be a named directory below a parent directory");
    }
}

}

CacheValidation validate_cache(const fs::path &cache_dir) {
    CacheValidation result;
    try {
        std::error_code ec;
        if (!fs::is_directory(cache_dir, ec) || ec) throw std::runtime_error("resource cache directory is missing");
        std::map<std::string, std::string> fields = read_install_manifest(cache_dir / kInstallManifest);
        const std::string format = required_field(fields, "format");
        if (format != "1" && format != "2") throw std::runtime_error("unsupported cache manifest format");
        std::string archive_hash = required_field(fields, "archive_sha256");
        if (format == "1") {
            const SupportedJar *jar = find_supported(archive_hash);
            if (jar == nullptr) throw std::runtime_error("cache was not imported from a supported JAR");
            if (required_field(fields, "canonical_name") != jar->canonical_name ||
                parse_decimal(required_field(fields, "archive_size"), "archive_size") != jar->archive_size ||
                required_field(fields, "content_tree_sha256") != jar->content_tree_sha256 ||
                parse_decimal(required_field(fields, "entry_count"), "entry_count") != jar->entry_count) {
                throw std::runtime_error("cache manifest does not match its supported JAR");
            }
        } else {
            if (required_field(fields, "game") != "oblivion" || archive_hash.size() != 64 ||
                archive_hash.find_first_not_of("0123456789abcdef") != std::string::npos ||
                parse_decimal(required_field(fields, "archive_size"), "archive_size") > kMaximumArchiveBytes) {
                throw std::runtime_error("invalid Oblivion cache manifest");
            }
        }
        TreeIdentity tree = content_tree_identity(cache_dir);
        if (tree.entries != parse_decimal(required_field(fields, "entry_count"), "entry_count") ||
            tree.digest != required_field(fields, "content_tree_sha256")) {
            throw std::runtime_error("resource cache contents failed validation");
        }
        require_oblivion_tree(cache_dir);
        result.valid = true;
        result.reason = "valid";
        result.canonical_jar_name = required_field(fields, "canonical_name");
        result.archive_sha256 = archive_hash;
    } catch (const std::exception &error) {
        result.reason = error.what();
    }
    return result;
}

PreparedResources import_resources(const fs::path &cache_dir, const std::vector<uint8_t> &archive) {
    require_safe_cache_target(cache_dir);
    if (archive.size() > kMaximumArchiveBytes) throw std::runtime_error("JAR exceeds the safety limit");
    std::string archive_hash = crypto::sha256_hex(crypto::sha256(archive));
    const SupportedJar *supported = find_supported(archive_hash, archive.size());
    const std::string name = supported ? supported->canonical_name : "Oblivion mobile";
    std::vector<ZipEntry> entries = parse_zip(archive);

    size_t eocd = archive.size() - 22;
    while (read_u32(archive, eocd) != 0x06054b50U || eocd + 22 + read_u16(archive, eocd + 20) != archive.size()) {
        --eocd;
    }
    size_t central_offset = read_u32(archive, eocd + 16);
    fs::path temporary = unique_sibling(cache_dir, "import");
    fs::path backup = unique_sibling(cache_dir, "backup");
    std::error_code ec;
    fs::create_directories(cache_dir.parent_path(), ec);
    if (ec) throw std::runtime_error("cannot create resource cache parent: " + ec.message());
    if (!fs::create_directory(temporary, ec) || ec) throw std::runtime_error("cannot create import directory: " + ec.message());

    bool cache_moved = false;
    try {
        for (const ZipEntry &entry : entries) {
            std::vector<uint8_t> payload = inflate_entry(archive, entry, central_offset);
            fs::path destination = temporary / fs::path(entry.name);
            fs::create_directories(destination.parent_path(), ec);
            if (ec) throw std::runtime_error("cannot create resource directory: " + ec.message());
            std::ofstream output(destination, std::ios::binary | std::ios::trunc);
            if (!output) throw std::runtime_error("cannot create extracted resource: " + entry.name);
            if (!payload.empty()) output.write((const char *)payload.data(), (std::streamsize)payload.size());
            output.close();
            if (!output) throw std::runtime_error("cannot write extracted resource: " + entry.name);
        }
        require_oblivion_tree(temporary);
        write_install_manifest(temporary, name, archive_hash, archive.size(), content_tree_identity(temporary));
        CacheValidation staged = validate_cache(temporary);
        if (!staged.valid) throw std::runtime_error("new resource cache failed validation: " + staged.reason);

        bool cache_exists = fs::exists(cache_dir, ec);
        if (ec) throw std::runtime_error("cannot inspect existing resource cache: " + ec.message());
        if (cache_exists) {
            fs::rename(cache_dir, backup, ec);
            if (ec) throw std::runtime_error("cannot preserve existing resource cache: " + ec.message());
            cache_moved = true;
        }
        fs::rename(temporary, cache_dir, ec);
        if (ec) {
            if (cache_moved) {
                std::error_code restore_error;
                fs::rename(backup, cache_dir, restore_error);
            }
            throw std::runtime_error("cannot activate imported resource cache: " + ec.message());
        }
        if (cache_moved) fs::remove_all(backup, ec);
    } catch (...) {
        std::error_code cleanup_error;
        fs::remove_all(temporary, cleanup_error);
        throw;
    }
    return PreparedResources{cache_dir, true, name, archive_hash};
}

PreparedResources prepare_resources(const fs::path &cache_dir, const std::optional<fs::path> &jar_path) {
    require_safe_cache_target(cache_dir);
    if (!jar_path) {
        CacheValidation cached = validate_cache(cache_dir);
        if (!cached.valid) {
            throw std::runtime_error("no valid Oblivion resource cache at " + cache_dir.string() + ": " +
                                     cached.reason + "; select a supported original JAR to import it");
        }
        return PreparedResources{cache_dir, false, cached.canonical_jar_name, cached.archive_sha256};
    }
    return import_resources(cache_dir, read_file(*jar_path, kMaximumArchiveBytes));
}

fs::path executable_data_directory(const fs::path &executable_path) {
    std::error_code ec;
    fs::path absolute = fs::absolute(executable_path, ec);
    if (ec) absolute = executable_path;
    fs::path parent = absolute.parent_path();
    if (parent.empty()) parent = fs::current_path();
    return parent;
}

fs::path user_data_directory() {
#if defined(_WIN32)
    if (const char *local_app_data = std::getenv("LOCALAPPDATA")) {
        if (*local_app_data) return fs::path(local_app_data) / "Limbo";
    }
#else
    if (const char *xdg_data_home = std::getenv("XDG_DATA_HOME")) {
        if (*xdg_data_home) return fs::path(xdg_data_home) / "limbo";
    }
    if (const char *home = std::getenv("HOME")) {
        if (*home) return fs::path(home) / ".local" / "share" / "limbo";
    }
#endif
    return {};
}

fs::path colocated_cache_directory(const fs::path &executable_path) {
    return executable_data_directory(executable_path) / "oblivion";
}

fs::path user_cache_directory() {
    fs::path base = user_data_directory();
    if (base.empty()) return {};
    return base / "oblivion";
}

fs::path colocated_save_directory(const fs::path &executable_path) {
    return executable_data_directory(executable_path) / "saves" / "rms";
}

fs::path user_save_directory() {
    fs::path base = user_data_directory();
    if (base.empty()) return {};
    return base / "saves" / "rms";
}

bool directory_is_writable(const fs::path &dir) {
    if (dir.empty()) return false;
    std::error_code ec;
    fs::create_directories(dir, ec);
    if (!fs::is_directory(dir, ec)) return false;
    const fs::path probe = dir / ".voyage-write-probe";
    {
        std::ofstream out(probe, std::ios::binary | std::ios::trunc);
        if (!out) return false;
        out.put('x');
        if (!out) return false;
    }
    fs::remove(probe, ec);
    return true;
}

fs::path resolve_data_directory(const fs::path &executable_path, bool portable) {
    const fs::path executable_dir = executable_data_directory(executable_path);
    std::error_code ec;
    if (portable || fs::is_regular_file(executable_dir / "portable.txt", ec)) {
        return executable_dir / "data";
    }
    const fs::path user = user_data_directory();
    return user.empty() ? executable_dir / "data" : user;
}

fs::path resolve_cache_directory(const fs::path &executable_path, bool portable) {
    return resolve_data_directory(executable_path, portable) / "oblivion";
}

fs::path resolve_save_directory(const fs::path &executable_path, bool portable) {
    const fs::path destination = resolve_data_directory(executable_path, portable) / "saves" / "rms";
    const fs::path legacy = colocated_save_directory(executable_path);
    if (fs::is_directory(legacy)) {
        for (const auto &entry : fs::directory_iterator(legacy)) {
            if (entry.is_regular_file() && entry.path().extension() == ".rs") {
                const fs::path target = destination / entry.path().filename();
                if (fs::exists(target)) continue;
                fs::create_directories(destination);
                const fs::path temporary = target.string() + ".tmp";
                fs::copy_file(entry.path(), temporary, fs::copy_options::overwrite_existing);
                std::error_code ec;
                fs::rename(temporary, target, ec);
                if (ec) {
                    std::error_code cleanup;
                    fs::remove(temporary, cleanup);
                    throw fs::filesystem_error("cannot migrate legacy save", target, ec);
                }
            }
        }
    }
    return destination;
}

fs::path resolve_settings_directory(const fs::path &executable_path, bool portable) {
    return resolve_data_directory(executable_path, portable) / "settings";
}

}
