#include "src/limbo/key_bindings.hpp"

#include "src/limbo/game_canvas.hpp"

KeyBindings &key_bindings_state() {
    SessionContext &session = currentSession();
    if (session.key_bindings == nullptr) {
        session.key_bindings = session_new<KeyBindings>();
    }
    return *session.key_bindings;
}

void KeyBindings::reset() {
    codes[0] = 55;
    codes[1] = 57;
    codes[2] = 51;
    for (int32_t n = 0; n < pending.length(); ++n) {
        pending[n] = 0;
    }
    for (int32_t n = 0; n < names.length(); ++n) {
        names[n] = std::nullopt;
    }
}

void KeyBindings::load_names() {
    names[1] = GameCanvas::get_text(281);
    names[6] = GameCanvas::get_text(282);
    names[2] = GameCanvas::get_text(283);
    names[5] = GameCanvas::get_text(284);
    names[8] = GameCanvas::get_text(285);
    names[48] = "# 0";
    names[49] = "# 1";
    names[50] = "# 2";
    names[51] = "# 3";
    names[52] = "# 4";
    names[53] = "# 5";
    names[54] = "# 6";
    names[55] = "# 7";
    names[56] = "# 8";
    names[57] = "# 9";
    names[35] = GameCanvas::get_text(286);
    names[42] = GameCanvas::get_text(287);
}

std::string KeyBindings::substitute(const std::string &string) {
    std::string text = string;
    int32_t n6 = voyage::text::index_of(text, "ACTION_KEY");
    if (n6 != -1) {
        text = voyage::text::slice(text, 0, n6) + names[8].value_or("null") + voyage::text::slice(text, n6 + voyage::text::length("ACTION_KEY"));
    }
    if ((n6 = voyage::text::index_of(text, "TOGGLE_WEAPON_KEY")) != -1) {
        text = voyage::text::slice(text, 0, n6) + names[codes[2]].value_or("null") +
               voyage::text::slice(text, n6 + voyage::text::length("TOGGLE_WEAPON_KEY"));
    }
    if ((n6 = voyage::text::index_of(text, "QUICK_HEALTH_KEY")) != -1) {
        text = voyage::text::slice(text, 0, n6) + names[codes[0]].value_or("null") +
               voyage::text::slice(text, n6 + voyage::text::length("QUICK_HEALTH_KEY"));
    }
    if ((n6 = voyage::text::index_of(text, "QUICK_MAGIKA_KEY")) != -1) {
        text = voyage::text::slice(text, 0, n6) + names[codes[1]].value_or("null") +
               voyage::text::slice(text, n6 + voyage::text::length("QUICK_MAGIKA_KEY"));
    }
    return text;
}
