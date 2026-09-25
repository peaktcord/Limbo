#ifndef OBLIVION_TEXT_RESOURCES_HPP
#define OBLIVION_TEXT_RESOURCES_HPP

#include "src/common/session.hpp"
#include "src/limbo/oblivion_state.hpp"

class TextResources {
public:
    static TextResourcesStatics &S();

    static int32_t max_string_id(int32_t language_id);
    static int32_t record_count(int32_t language_id);
    SharedArray<uint16_t> load_language(int32_t language_id);
    std::string load_copyright_text();
    std::string load_intro_segment(int8_t segment);

    static void clinit();
};

#endif
