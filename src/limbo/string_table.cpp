#include "src/limbo/string_table.hpp"

#include "src/storage/resource_store.hpp"
#include "src/limbo/text_resources.hpp"
#include "src/limbo/crash_report.hpp"

StringTable &current_string_table() {
    SessionContext &session = currentSession();
    if (session.string_table == nullptr) {
        session.string_table = session_new<StringTable>();
    }
    return *session.string_table;
}

void StringTable::load_language(int32_t n) {
    if ((n == 65535 || n == 0) && language0_loaded) {
        return;
    }
    int16_t s = 0;
    int32_t n2 = 0;
    SharedArray<uint16_t> cArray;
    SharedArray<int16_t> sArray;
    if (n == 65535) {
        n = 0;
    }
    language1_chars.setNull();
    language1_offsets.setNull();
    TextResources *text_resources = session_new<TextResources>();
    cArray = text_resources->load_language(n);
    if (n == 0) {
        language0_chars = cArray;
        language0_offsets = SharedArray<int16_t>(TextResources::max_string_id(n) + 1);
        sArray = language0_offsets;
    } else {
        language1_chars = cArray;
        language1_offsets = SharedArray<int16_t>(TextResources::max_string_id(n) + 1);
        sArray = language1_offsets;
    }
    for (n2 = 1; n2 < sArray.length(); ++n2) {
        sArray[n2] = -1;
    }
    int32_t n3 = 0;
    int32_t n4 = 0;
    std::string string2;
    for (n2 = 1; n2 <= TextResources::record_count(n); ++n2) {
        while (cArray[n4] != '|') {
            ++n4;
        }
        if (n3 != 0) {
            n3 += 2;
        }
        string2 = array_text(cArray, n3, n4 - n3);
        int32_t n5 = voyage::text::index_of(string2, 32);
        s = (int16_t)voyage::text::parse_int32(voyage::text::slice(string2, 0, n5));
        sArray[s] = (int16_t)(n3 + n5 + 1);
        n3 = n4++ + 1;
    }
    if (n == 0) {
        language0_offsets = sArray;
        language0_loaded = true;
    } else {
        language1_offsets = sArray;
    }
}

std::string StringTable::get_text(int32_t n) {
    int32_t n2 = 0;
    if (!language0_offsets.isNull() && n >= 0 && n < language0_offsets.length() && language0_offsets[n] != -1) {
        for (int32_t k = language0_offsets[n]; k < language0_chars.length() && language0_chars[k] != '|'; ++k) {
            ++n2;
        }
        return array_text(language0_chars, (int32_t)language0_offsets[n], n2);
    }
    if (!language1_offsets.isNull() && n >= 0 && n < language1_offsets.length() && language1_offsets[n] != -1) {
        for (int32_t k = language1_offsets[n]; k < language1_chars.length() && language1_chars[k] != '|'; ++k) {
            ++n2;
        }
        return array_text(language1_chars, (int32_t)language1_offsets[n], n2);
    }
    return std::string("");
}

int32_t StringTable::find_text_id(const std::string &string) {
    int32_t n = 0;
    int32_t n2 = 0;
    int32_t n3 = 0;
    int32_t n4 = 0;
    for (n2 = 1; n2 < language0_offsets.length(); ++n2) {
        if (language0_offsets[n2] == -1) continue;
        n = 0;
        for (int32_t k = language0_offsets[n2]; k < language0_chars.length() && language0_chars[k] != '|'; ++k) {
            ++n;
        }
        if (voyage::text::length(string) != n) continue;
        n3 = language0_offsets[n2];
        n4 = 0;
        while (n3 < language0_offsets[n2] + n && voyage::text::byte_at(string, n4) == language0_chars[n3]) {
            ++n3;
            ++n4;
        }
        if (n4 != n) continue;
        return n2;
    }
    return -1;
}

int32_t StringTable::load_resource(const std::string &path) {
    crash_report().resource = path;
    if (resource_buffer.isNull()) resource_buffer = SharedArray<int8_t>(6144);
    try {
        const auto bytes = voyage::storage::require_resource(currentSession().resources, path);
        const auto count = static_cast<int32_t>(std::min(bytes.size(), static_cast<size_t>(resource_buffer.length())));
        for (int32_t i = 0; i < count; ++i) resource_buffer[i] = static_cast<int8_t>(bytes[static_cast<size_t>(i)]);
        return count;
    } catch (const std::exception &error) {
        voyage::logging::exception("Resources", ("Reading resource " + path).c_str(), error);
        return 0;
    }
}
