#include "src/game/sha256.hpp"

#include <fstream>
#include <stdexcept>

namespace voyage::crypto {
namespace {

constexpr uint32_t kRoundConstants[64] = {
    0x428a2f98U, 0x71374491U, 0xb5c0fbcfU, 0xe9b5dba5U, 0x3956c25bU, 0x59f111f1U,
    0x923f82a4U, 0xab1c5ed5U, 0xd807aa98U, 0x12835b01U, 0x243185beU, 0x550c7dc3U,
    0x72be5d74U, 0x80deb1feU, 0x9bdc06a7U, 0xc19bf174U, 0xe49b69c1U, 0xefbe4786U,
    0x0fc19dc6U, 0x240ca1ccU, 0x2de92c6fU, 0x4a7484aaU, 0x5cb0a9dcU, 0x76f988daU,
    0x983e5152U, 0xa831c66dU, 0xb00327c8U, 0xbf597fc7U, 0xc6e00bf3U, 0xd5a79147U,
    0x06ca6351U, 0x14292967U, 0x27b70a85U, 0x2e1b2138U, 0x4d2c6dfcU, 0x53380d13U,
    0x650a7354U, 0x766a0abbU, 0x81c2c92eU, 0x92722c85U, 0xa2bfe8a1U, 0xa81a664bU,
    0xc24b8b70U, 0xc76c51a3U, 0xd192e819U, 0xd6990624U, 0xf40e3585U, 0x106aa070U,
    0x19a4c116U, 0x1e376c08U, 0x2748774cU, 0x34b0bcb5U, 0x391c0cb3U, 0x4ed8aa4aU,
    0x5b9cca4fU, 0x682e6ff3U, 0x748f82eeU, 0x78a5636fU, 0x84c87814U, 0x8cc70208U,
    0x90befffaU, 0xa4506cebU, 0xbef9a3f7U, 0xc67178f2U,
};

uint32_t rotate_right(uint32_t value, int bits) {
    return (value >> bits) | (value << (32 - bits));
}

}

Sha256Digest sha256(const uint8_t *data, size_t size) {
    if (size > (UINT64_MAX / 8U)) throw std::runtime_error("input is too large for SHA-256");
    std::vector<uint8_t> message;
    message.reserve(size + 72);
    if (size != 0) message.insert(message.end(), data, data + size);
    message.push_back(0x80);
    while ((message.size() % 64) != 56) message.push_back(0);
    uint64_t bit_length = (uint64_t)size * 8U;
    for (int shift = 56; shift >= 0; shift -= 8) message.push_back((uint8_t)(bit_length >> shift));

    uint32_t state[8] = {
        0x6a09e667U, 0xbb67ae85U, 0x3c6ef372U, 0xa54ff53aU,
        0x510e527fU, 0x9b05688cU, 0x1f83d9abU, 0x5be0cd19U,
    };
    for (size_t offset = 0; offset < message.size(); offset += 64) {
        uint32_t words[64] = {};
        for (int n = 0; n < 16; ++n) {
            size_t p = offset + (size_t)n * 4;
            words[n] = ((uint32_t)message[p] << 24) | ((uint32_t)message[p + 1] << 16) |
                       ((uint32_t)message[p + 2] << 8) | (uint32_t)message[p + 3];
        }
        for (int n = 16; n < 64; ++n) {
            uint32_t s0 = rotate_right(words[n - 15], 7) ^ rotate_right(words[n - 15], 18) ^
                          (words[n - 15] >> 3);
            uint32_t s1 = rotate_right(words[n - 2], 17) ^ rotate_right(words[n - 2], 19) ^
                          (words[n - 2] >> 10);
            words[n] = words[n - 16] + s0 + words[n - 7] + s1;
        }

        uint32_t a = state[0], b = state[1], c = state[2], d = state[3];
        uint32_t e = state[4], f = state[5], g = state[6], h = state[7];
        for (int n = 0; n < 64; ++n) {
            uint32_t sum1 = rotate_right(e, 6) ^ rotate_right(e, 11) ^ rotate_right(e, 25);
            uint32_t choice = (e & f) ^ (~e & g);
            uint32_t temp1 = h + sum1 + choice + kRoundConstants[n] + words[n];
            uint32_t sum0 = rotate_right(a, 2) ^ rotate_right(a, 13) ^ rotate_right(a, 22);
            uint32_t majority = (a & b) ^ (a & c) ^ (b & c);
            uint32_t temp2 = sum0 + majority;
            h = g;
            g = f;
            f = e;
            e = d + temp1;
            d = c;
            c = b;
            b = a;
            a = temp1 + temp2;
        }
        state[0] += a;
        state[1] += b;
        state[2] += c;
        state[3] += d;
        state[4] += e;
        state[5] += f;
        state[6] += g;
        state[7] += h;
    }

    Sha256Digest digest{};
    for (int n = 0; n < 8; ++n) {
        digest[(size_t)n * 4] = (uint8_t)(state[n] >> 24);
        digest[(size_t)n * 4 + 1] = (uint8_t)(state[n] >> 16);
        digest[(size_t)n * 4 + 2] = (uint8_t)(state[n] >> 8);
        digest[(size_t)n * 4 + 3] = (uint8_t)state[n];
    }
    return digest;
}

Sha256Digest sha256_file(const std::filesystem::path &path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("cannot open file for SHA-256: " + path.string());
    input.seekg(0, std::ios::end);
    std::streamoff length = input.tellg();
    if (length < 0) throw std::runtime_error("cannot measure file for SHA-256: " + path.string());
    input.seekg(0, std::ios::beg);
    std::vector<uint8_t> bytes((size_t)length);
    if (!bytes.empty() && !input.read((char *)bytes.data(), length)) {
        throw std::runtime_error("cannot read file for SHA-256: " + path.string());
    }
    return sha256(bytes);
}

std::string sha256_hex(const Sha256Digest &digest) {
    static constexpr char kHex[] = "0123456789abcdef";
    std::string result;
    result.resize(64);
    for (size_t n = 0; n < digest.size(); ++n) {
        result[n * 2] = kHex[digest[n] >> 4];
        result[n * 2 + 1] = kHex[digest[n] & 15];
    }
    return result;
}

}
