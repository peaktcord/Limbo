#ifndef OBLIVION_STRING_TABLE_HPP
#define OBLIVION_STRING_TABLE_HPP

#include "src/common/session.hpp"

class StringTable {
public:
    SharedArray<uint16_t> language0_chars;
    SharedArray<int16_t> language0_offsets;
    SharedArray<uint16_t> language1_chars;
    SharedArray<int16_t> language1_offsets;

    SharedArray<int8_t> resource_buffer;

    bool language0_loaded = false;

    void load_language(int32_t language_id);
    std::string get_text(int32_t string_id);
    int32_t find_text_id(const std::string &text);
    int32_t load_resource(const std::string &path);
};

StringTable &current_string_table();

#endif
