#ifndef VOYAGE_SHA256_HPP
#define VOYAGE_SHA256_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace voyage::crypto {

using Sha256Digest = std::array<uint8_t, 32>;

Sha256Digest sha256(const uint8_t *data, size_t size);
inline Sha256Digest sha256(const std::vector<uint8_t> &data) {
    return sha256(data.data(), data.size());
}
Sha256Digest sha256_file(const std::filesystem::path &path);
std::string sha256_hex(const Sha256Digest &digest);

}

#endif
