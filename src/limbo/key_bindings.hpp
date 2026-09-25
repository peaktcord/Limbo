#ifndef OBLIVION_KEY_BINDINGS_HPP
#define OBLIVION_KEY_BINDINGS_HPP

#include <vector>

#include "src/common/session.hpp"

class KeyBindings {
public:
    SharedArray<int8_t> codes = SharedArray<int8_t>{55, 57, 51};

    SharedArray<int8_t> pending = SharedArray<int8_t>(3);

    SharedArray<std::optional<std::string>> names = SharedArray<std::optional<std::string>>(100);

    void reset();
    void load_names();

    std::string substitute(const std::string &text);
};

KeyBindings &key_bindings_state();

#endif
