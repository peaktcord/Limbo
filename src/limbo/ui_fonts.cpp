#include "src/limbo/ui_fonts.hpp"

void UiFonts::load() {
    small = Font::getFont(0, 0, 8);
    plain = Font::getFont(0, 0, 0);
    bold_small = Font::getFont(0, 1, 8);
    bold_medium = Font::getFont(0, 1, 16);
}

UiFonts &ui_fonts() {
    SessionContext &session = currentSession();
    if (session.ui_fonts == nullptr) {
        UiFonts *fonts = session_new<UiFonts>();
        fonts->load();
        session.ui_fonts = fonts;
    }
    return *session.ui_fonts;
}
