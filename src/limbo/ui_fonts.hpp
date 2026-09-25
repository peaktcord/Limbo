#ifndef OBLIVION_UI_FONTS_HPP
#define OBLIVION_UI_FONTS_HPP

#include "src/common/ui.hpp"

class UiFonts {
public:
    Font *small = nullptr;
    Font *plain = nullptr;
    Font *bold_small = nullptr;
    Font *bold_medium = nullptr;

    void load();
};

UiFonts &ui_fonts();

#endif
