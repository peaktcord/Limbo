#ifndef OBLIVION_MENU_SCREEN_HPP
#define OBLIVION_MENU_SCREEN_HPP

#include "src/common/ui.hpp"
#include "src/limbo/menu_node.hpp"
#include "src/limbo/oblivion_state.hpp"
#include "src/limbo/sprite_frame.hpp"

class GameCanvas;

class MenuScreen {
public:
    static MenuScreenStatics &S();
    SpriteFrame *atlas = nullptr;
    std::optional<std::string> title;
    SharedArray<MenuNode *> tab_roots;
    GameCanvas *game = nullptr;
    Image *background = nullptr;
    SharedArray<int8_t> tab_ids;
    int8_t selected_index = 0;
    int8_t last_visible_row = 0;
    int8_t first_visible_row = 0;
    int8_t tab_index = 0;
    int8_t label_scroll_chars = 0;
    int8_t label_scroll_dir = 1;
    int8_t i = 0;
    int8_t label_scroll_paused = 0;
    int8_t is_open = 0;
    int16_t scroll_offset = 0;
    int16_t scroll_timer_ms = 0;

    MenuScreen(const std::string &string, GameCanvas *game);

    void render(Graphics *graphics);
    void handle_input(uint16_t input);
    bool a(MenuNode *c2, MenuNode *c3);
    void a(const SharedArray<int8_t> &byArray, const SharedArray<MenuNode *> &cArray, const std::optional<std::string> &string, Image *image,
           Graphics *graphics);
    void b(Graphics *graphics);
    bool navigate_back();
    void a(int64_t l);
    void a(int32_t n);

    static void clinit();
};

#endif
