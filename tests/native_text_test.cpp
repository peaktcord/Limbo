#include "src/text/text.hpp"
#include "src/diagnostics/log.hpp"

#include <cassert>
#include <climits>
#include <optional>
#include <vector>

namespace {
std::vector<std::string> throw_sites;
void record_throw(const char *message) { throw_sites.emplace_back(message); }

template <typename Exception>
void rejects_integer(const std::string &input, const std::string &reason) {
    const auto before = throw_sites.size();
    bool rejected = false;
    try {
        (void)voyage::text::parse_int32(input);
    } catch (const Exception &error) {
        rejected = true;
        assert(std::string(error.what()).find(reason) == 0);
        assert(throw_sites.size() == before + 1);
        assert(throw_sites.back() == error.what());
    }
    assert(rejected);
}
}

template <typename F>
void rejects_range(F operation) {
    bool rejected = false;
    try { operation(); } catch (const std::out_of_range &) { rejected = true; }
    assert(rejected);
}

int main() {
    using namespace voyage::text;
    assert(parse_int32("-2147483648") == INT32_MIN);
    assert(parse_int32("2147483647") == INT32_MAX);
    assert(parse_int32("0") == 0);
    assert(parse_int32("-0") == 0);
    assert(parse_int32("+123") == 123);
    assert(parse_int32("000123") == 123);
    assert(parse_int32(" \t\n\r\f\v+42") == 42);
    voyage::logging::set_throw_trace_hook(record_throw);
    for (const std::string input : {"", " ", "+", "-", "12x", "12 ", "0x10", "--1"}) {
        rejects_integer<std::invalid_argument>(input, "Invalid decimal integer:");
    }
    rejects_integer<std::invalid_argument>(std::string("12\0x", 4), "Invalid decimal integer:");
    rejects_integer<std::invalid_argument>(std::string("\xff", 1), "Invalid decimal integer:");
    for (const std::string input : {"2147483648", "-2147483649", "999999999999999999999999999",
                                   "-999999999999999999999999999"}) {
        rejects_integer<std::out_of_range>(input, "Integer outside signed 32-bit range:");
    }
    voyage::logging::set_throw_trace_hook(nullptr);
    assert(length("") == 0);
    assert(index_of("banana", 'a', -3) == 1);
    assert(index_of("banana", "ana", 2) == 3);
    assert(index_of("banana", 'a', 6) == -1);
    assert(index_of("banana", 'a', INT_MAX) == -1);
    assert(index_of("banana", "missing") == -1);
    assert(index_of("banana", "", 6) == 6);
    assert(index_of("banana", "", 7) == -1);
    assert(index_of("", 'a') == -1);
    assert(last_index_of("banana", 'a') == 5);
    assert(last_index_of("banana", 'a', 4) == 3);
    assert(last_index_of("banana", 'a', INT_MAX) == 5);
    assert(last_index_of("banana", 'a', -1) == -1);
    assert(last_index_of("banana", 'z') == -1);
    assert(last_index_of("", 'a') == -1);
    assert(slice("banana", 1, 4) == "ana");
    assert(slice("banana", 3) == "ana");
    assert(slice("banana", 6).empty());
    assert(slice("banana", 2, 2).empty());
    rejects_range([] { slice("abc", -1); });
    rejects_range([] { slice("abc", 2, 1); });
    rejects_range([] { slice("abc", 0, 4); });
    rejects_range([] { byte_at("abc", -1); });
    rejects_range([] { byte_at("abc", 3); });
    assert(starts_with("abc", ""));
    assert(starts_with("abc", "ab"));
    assert(!starts_with("ab", "abc"));
    assert(!starts_with("Abc", "ab"));

    std::string bytes;
    for (int i = 0; i < 256; ++i) bytes += static_cast<char>(i);
    const auto upper = upper_ascii(bytes);
    assert(length(upper) == 256);
    for (int i = 0; i < 256; ++i) {
        assert(byte_at(bytes, i) == i);
        assert(index_of(bytes, i) == i);
        assert(last_index_of(bytes, i) == i);
        assert(byte_at(upper, i) == (i >= 'a' && i <= 'z' ? i - 'a' + 'A' : i));
    }
    assert(slice(bytes, 127, 130) == std::string("\x7f\x80\x81", 3));
    assert(slice(bytes, 0, 2) == std::string("\0\1", 2));

    auto copied = slice(bytes, 128, 131);
    bytes.clear();
    assert(copied == std::string("\x80\x81\x82", 3));
    auto temporary = slice(std::string("temporary"), 0, 4);
    assert(temporary == "temp");
    std::optional<std::string> absent;
    std::optional<std::string> empty = "";
    assert(absent != empty);
    assert(empty.has_value() && empty->empty());
    return 0;
}
