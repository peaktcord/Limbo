#ifndef VOYAGE_TEXT_TEXT_HPP
#define VOYAGE_TEXT_TEXT_HPP

#include <algorithm>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>

namespace voyage::text {

int32_t parse_int32(const std::string &text);

inline uint16_t byte_at(std::string_view text, int32_t index) {
    return static_cast<unsigned char>(text.at(static_cast<size_t>(index)));
}

inline int32_t length(std::string_view text) {
    if (text.size() > static_cast<size_t>(std::numeric_limits<int32_t>::max())) {
        throw std::length_error("Resource text exceeds the signed index range");
    }
    return static_cast<int32_t>(text.size());
}

inline int32_t index_of(std::string_view text, std::string_view needle, int32_t from = 0) {
    const auto pos = text.find(needle, static_cast<size_t>(std::max(from, 0)));
    return pos == std::string_view::npos ? -1 : length(text.substr(0, pos));
}

inline int32_t index_of(std::string_view text, int32_t byte, int32_t from = 0) {
    const char needle = static_cast<char>(byte);
    return index_of(text, std::string_view(&needle, 1), from);
}

inline int32_t last_index_of(std::string_view text, int32_t byte, int32_t from) {
    if (from < 0) return -1;
    const auto pos = text.rfind(static_cast<char>(byte), static_cast<size_t>(from));
    return pos == std::string_view::npos ? -1 : length(text.substr(0, pos));
}

inline int32_t last_index_of(std::string_view text, int32_t byte) {
    return last_index_of(text, byte, length(text) - 1);
}

inline std::string slice(std::string_view text, int32_t begin, int32_t end) {
    if (begin < 0 || end < begin || static_cast<size_t>(end) > text.size()) {
        throw std::out_of_range("Invalid resource text range");
    }
    return std::string(text.substr(static_cast<size_t>(begin), static_cast<size_t>(end - begin)));
}

inline std::string slice(std::string_view text, int32_t begin) {
    return slice(text, begin, length(text));
}

inline bool starts_with(std::string_view text, std::string_view prefix) {
    return text.substr(0, prefix.size()) == prefix;
}

inline std::string upper_ascii(std::string text) {
    for (char &byte : text) {
        if (byte >= 'a' && byte <= 'z') byte = static_cast<char>(byte - 'a' + 'A');
    }
    return text;
}

}

#endif
