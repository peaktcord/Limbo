#include "src/limbo/text_resources.hpp"

TextResourcesStatics &TextResources::S() { return oblivion_state().text; }

#include "src/storage/resource_store.hpp"


void TextResources::clinit() { S().default_language_loaded = false; }

int32_t TextResources::max_string_id(int32_t language_id) {
    switch (language_id) {
        case 0: return 574;
        case 1: return 550;
        case 2: return 399;
        case 3: return 345;
        case 4: return 362;
        case 5: return 565;
        case 6: return 494;
        case 7: return 546;
        case 8: return 495;
        case 9: return 495;
        case 10: return 495;
        case 11: return 409;
        case 12: return 547;
    }
    return 0;
}

int32_t TextResources::record_count(int32_t language_id) {
    switch (language_id) {
        case 0: return 305;
        case 1: return 34;
        case 2: return 46;
        case 3: return 9;
        case 4: return 16;
        case 5: return 6;
        case 6: return 42;
        case 7: return 20;
        case 8: return 16;
        case 9: return 30;
        case 10: return 9;
        case 11: return 7;
        case 12: return 6;
    }
    return 0;
}

SharedArray<uint16_t> TextResources::load_language(int32_t language_id) {
    const std::string path = "/lang_" + std::to_string(language_id) + ".txt";
    try {
        const auto bytes = voyage::storage::require_resource(currentSession().resources, path);
        if (bytes.size() >= static_cast<size_t>(std::numeric_limits<int32_t>::max())) {
            throw std::length_error("Language resource is too large: " + path);
        }
        const auto length = static_cast<int32_t>(bytes.size());
        SharedArray<uint16_t> characters(length + 1);
        for (int32_t i = 0; i < length; ++i) characters[i] = bytes[static_cast<size_t>(i)];
        S().language_data = characters;
        if (language_id == 0) S().default_language_loaded = true;
    } catch (const std::runtime_error &error) {
        voyage::logging::exception("Resources", ("Loading language file " + path).c_str(), error);
    }
    return S().language_data;
}

std::string TextResources::load_copyright_text() {
    try {
        const auto bytes = voyage::storage::require_resource(currentSession().resources, "/copywrite.txt");
        return {bytes.begin(), bytes.end()};
    } catch (const std::runtime_error &error) {
        voyage::logging::exception("Resources", "Loading /copywrite.txt", error);
        return {};
    }
}

std::string TextResources::load_intro_segment(int8_t segment) {
    try {
        const auto bytes = voyage::storage::require_resource(currentSession().resources, "/start.txt");
        if (segment < 0) return {};
        std::string text;
        int current_segment = 0;
        for (uint8_t byte : bytes) {
            if (current_segment == segment) {
                if (byte == '|') break;
                text += static_cast<char>(byte);
            } else if (byte == '|') {
                ++current_segment;
            }
        }
        return text;
    } catch (const std::runtime_error &error) {
        voyage::logging::exception("Resources", ("Loading /start.txt segment " + std::to_string(segment)).c_str(), error);
        return {};
    }
}
