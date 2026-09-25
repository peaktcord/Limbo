#ifndef VOYAGE_STORAGE_BINARY_IO_HPP
#define VOYAGE_STORAGE_BINARY_IO_HPP

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace voyage::storage {

using Bytes = std::vector<uint8_t>;

class BinaryReader {
public:
    explicit BinaryReader(const Bytes &bytes) : bytes_(bytes) {}
    size_t remaining() const { return bytes_.size() - position_; }
    uint8_t u8() { require(1); return bytes_[position_++]; }
    uint32_t big_endian(size_t width) {
        if (width == 0 || width > 4) throw std::invalid_argument("Invalid integer width");
        require(width);
        uint32_t value = 0;
        for (size_t i = 0; i < width; ++i) value = (value << 8) | u8();
        return value;
    }
    uint32_t u32le() {
        require(4);
        uint32_t value = 0;
        for (unsigned shift = 0; shift < 32; shift += 8) value |= uint32_t(u8()) << shift;
        return value;
    }
    Bytes bytes(size_t count) {
        require(count);
        Bytes out(bytes_.begin() + position_, bytes_.begin() + position_ + count);
        position_ += count;
        return out;
    }
    std::string text(size_t count) {
        require(count);
        std::string out(bytes_.begin() + position_, bytes_.begin() + position_ + count);
        position_ += count;
        return out;
    }
private:
    void require(size_t count) const {
        if (count > remaining()) throw std::runtime_error("Save data truncated at byte " +
            std::to_string(position_) + "; need " + std::to_string(count) + ", have " + std::to_string(remaining()));
    }
    const Bytes &bytes_;
    size_t position_ = 0;
};

class BinaryWriter {
public:
    void u8(int32_t value) { bytes_.push_back(static_cast<uint8_t>(value)); }
    void big_endian(uint32_t value, size_t width) {
        if (width == 0 || width > 4) throw std::invalid_argument("Invalid integer width");
        for (size_t i = width; i > 0; --i) u8(static_cast<uint8_t>(value >> (8 * (i - 1))));
    }
    void u32le(uint32_t value) {
        for (unsigned shift = 0; shift < 32; shift += 8) u8(static_cast<uint8_t>(value >> shift));
    }
    void bytes(const Bytes &value) { bytes_.insert(bytes_.end(), value.begin(), value.end()); }
    void text(const std::string &value) { bytes_.insert(bytes_.end(), value.begin(), value.end()); }
    const Bytes &data() const { return bytes_; }
private:
    Bytes bytes_;
};

}

#endif
