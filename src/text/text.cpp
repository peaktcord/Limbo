#include "src/text/text.hpp"

#include "src/diagnostics/log.hpp"

namespace voyage::text {

int32_t parse_int32(const std::string &text) {
    size_t used = 0;
    long long value = 0;
    try {
        value = std::stoll(text, &used, 10);
    } catch (const std::invalid_argument &) {
        const std::string message = "Invalid decimal integer: " + text;
        logging::trace_throw(message.c_str());
        throw std::invalid_argument(message);
    } catch (const std::out_of_range &) {
        const std::string message = "Integer outside signed 32-bit range: " + text;
        logging::trace_throw(message.c_str());
        throw std::out_of_range(message);
    }
    if (used != text.size()) {
        const std::string message = "Invalid decimal integer: " + text;
        logging::trace_throw(message.c_str());
        throw std::invalid_argument(message);
    }
    if (value < std::numeric_limits<int32_t>::min() || value > std::numeric_limits<int32_t>::max()) {
        const std::string message = "Integer outside signed 32-bit range: " + text;
        logging::trace_throw(message.c_str());
        throw std::out_of_range(message);
    }
    return static_cast<int32_t>(value);
}

}
