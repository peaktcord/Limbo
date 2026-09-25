#include "src/limbo/menu_screen.hpp"

#include "src/limbo/game_canvas.hpp"
#include "src/limbo/sprite_atlas.hpp"
#include "src/limbo/ui_fonts.hpp"

MenuScreenStatics &MenuScreen::S() { return oblivion_state().menu; }

void MenuScreen::clinit() {
    S().row_height = (int8_t)ui_fonts().small->getHeight();
    S().needs_rebuild = false;
}

MenuScreen::MenuScreen(const std::string &string, GameCanvas *game) {
    this->game = game;
    this->atlas = SpriteAtlas::load_atlas(string);
}

void MenuScreen::render(Graphics *graphics) {
    bool bl = false;
    bool bl2 = false;
    std::vector<MenuNode *> &items = this->tab_roots[this->tab_index]->children;
    MenuNode *c2 = nullptr;
    int32_t n = 0;
    int32_t n2 = 0;
    int32_t n3 = this->scroll_offset;
    int32_t n4 = 0;
    graphics->drawImage(this->background, 0, 0, 0);
    if (!this->tab_ids.isNull()) {
        SpriteAtlas::draw_frame(graphics, this->atlas, this->tab_ids[0],
               (GameCanvas::interface_width() >> 1) - (SpriteAtlas::frame_width(this->atlas, this->tab_ids[0]) >> 1),
               GameCanvas::state().canvas_height - ui_fonts().small->getHeight() - 4 -
                   SpriteAtlas::frame_height(this->atlas, this->tab_ids[0]));
        SpriteAtlas::draw_frame(graphics, this->atlas, this->tab_ids[this->tab_index + 1],
               (GameCanvas::interface_width() >> 1) - (SpriteAtlas::frame_width(this->atlas, this->tab_ids[this->tab_index + 1]) >> 1),
               GameCanvas::state().canvas_height - ui_fonts().small->getHeight() - 4 -
                   SpriteAtlas::frame_height(this->atlas, this->tab_ids[this->tab_index + 1]));
    }
    graphics->setFont(ui_fonts().small);
    graphics->setColor(0);
    graphics->drawString(this->tab_roots[this->tab_index]->label,
                         (GameCanvas::interface_width() >> 1) -
                             (ui_fonts().small->stringWidth(
                                      this->tab_roots[this->tab_index]->label) >>
                                  1),
                         12, 0);
    if (this->title.has_value()) {
        graphics->drawString(
            *this->title,
            (GameCanvas::interface_width() >> 1) -
                (ui_fonts().small->stringWidth(*this->title) >> 1),
            GameCanvas::state().canvas_height - SpriteAtlas::frame_height(this->atlas, 5) - shiftLeft32(S().row_height, 1), 0);
    }
    n3 += 12 + shiftLeft32(S().row_height, 1);
    this->first_visible_row = (int8_t)-1;
    for (n2 = 0; n2 < (int32_t)items.size(); ++n2) {
        c2 = items[(size_t)n2];
        if (n3 >= 12 + shiftLeft32(S().row_height, 1)) {
            if (this->first_visible_row == -1) {
                this->first_visible_row = (int8_t)n2;
            }
            if (n2 == this->selected_index) {
                graphics->setColor(16448974);
                graphics->fillRect(15, n3, GameCanvas::interface_width() - 30, (int32_t)S().row_height);
                if (c2->selection_hint.has_value()) {
                    graphics->setFont(ui_fonts().small);
                    graphics->setColor(0);
                    graphics->drawRect(20,
                                       GameCanvas::state().canvas_height - ui_fonts().small->getHeight() - 4 -
                                           SpriteAtlas::frame_height(this->atlas, 5) - shiftLeft32(S().row_height, 1) - 6,
                                       GameCanvas::interface_width() - 40, S().row_height + 4);
                    graphics->drawString(*c2->selection_hint, 23,
                                         GameCanvas::state().canvas_height - ui_fonts().small->getHeight() - 4 -
                                             SpriteAtlas::frame_height(this->atlas, 5) - shiftLeft32(S().row_height, 1) - 3,
                                         0);
                }
                graphics->setColor(c2->is_enabled ? 10318649 : 0xFF0000);
            } else {
                graphics->setColor(c2->is_enabled ? 0 : 0xFF0000);
            }
            if (c2->is_selected) {
                SpriteAtlas::draw_frame(graphics, this->atlas, 14, 15, n3);
                graphics->setFont(ui_fonts().bold_small);
                n = 15;
            } else {
                graphics->setFont(ui_fonts().small);
                n = 0;
            }
            if (c2->children.empty()) {
                std::string object = c2->label;
                std::string string = object;
                bool bl3 = false;
                if (n2 == this->selected_index) {
                    object = voyage::text::slice(object, this->label_scroll_chars);
                    string = object;
                }
                while (SpriteAtlas::frame_width(this->atlas, 12) + n + 15 > GameCanvas::interface_width() - graphics->getFont()->stringWidth(string)) {
                    bl3 = true;
                    object = voyage::text::slice(object, 0, voyage::text::length(object) - 1);
                    string = object + "...";
                }
                if (n2 == this->selected_index) {
                    if (bl3) {
                        if (this->label_scroll_dir == -1 && this->label_scroll_chars == 0) {
                            this->label_scroll_dir = 1;
                            this->label_scroll_paused = 1;
                        }
                        this->a(1);
                    } else if (this->i == 1 && this->label_scroll_dir == 1) {
                        this->label_scroll_dir = (int8_t)-1;
                        this->label_scroll_paused = 1;
                    }
                }
                graphics->drawString(string, 15 + n, n3, 0);
            } else {
                graphics->drawString(std::string("<") + c2->label + ">", 15 + n, n3, 0);
            }
        } else {
            bl = true;
            bl2 = true;
        }
        if ((n3 += S().row_height) + shiftLeft32(S().row_height, 1) <
            GameCanvas::state().canvas_height - ui_fonts().small->getHeight() - 4 - SpriteAtlas::frame_height(this->atlas, 5) -
                shiftLeft32(S().row_height, 1))
            continue;
        bl = true;
        bl2 = true;
        break;
    }
    if (!this->tab_roots.isNull() && this->tab_index < this->tab_roots.length() &&
        this->tab_roots[this->tab_index] != nullptr && !this->tab_roots[this->tab_index]->detail_lines.isNull()) {
        SharedArray<std::optional<std::string>> object = this->tab_roots[this->tab_index]->detail_lines;
        n3 = this->scroll_offset + S().row_height * 3;
        for (n4 = 0; n4 < object.length(); n4 += 2) {
            if (n3 >= 12 + shiftLeft32(S().row_height, 1)) {
                if (object[n4].has_value()) {
                    graphics->setFont(ui_fonts().bold_small);
                    graphics->setColor(0);
                    graphics->drawString(*object[n4], 10, n3, 0);
                }
                if (object[n4 + 1].has_value()) {
                    graphics->setFont(ui_fonts().small);
                    graphics->setColor(0xFF0000);
                    graphics->drawString(*object[n4 + 1],
                                         15 + ui_fonts().bold_small->stringWidth(object[n4].value_or("")), n3, 0);
                }
            } else {
                bl = true;
            }
            if ((n3 += S().row_height) + S().row_height <
                GameCanvas::state().canvas_height - ui_fonts().small->getHeight() - 4 -
                    SpriteAtlas::frame_height(this->atlas, 5))
                continue;
            if (n4 >= object.length() - 2) break;
            bl2 = true;
            break;
        }
    }
    this->last_visible_row = (int8_t)n2;
    graphics->setFont(ui_fonts().small);
    graphics->setColor(0xFF0000);
    graphics->drawString(voyage::text::upper_ascii(GameCanvas::get_text(449)), 2,
                         GameCanvas::state().canvas_height - ui_fonts().small->getHeight() - 2, 0);
    if (bl) {
        SpriteAtlas::draw_frame(graphics, this->atlas, 54, GameCanvas::interface_width() - SpriteAtlas::frame_width(this->atlas, 54) - 10, 35);
    }
    if (bl2) {
        SpriteAtlas::draw_frame(graphics, this->atlas, 53, GameCanvas::interface_width() - SpriteAtlas::frame_width(this->atlas, 53) - 10,
               GameCanvas::state().canvas_height - ui_fonts().small->getHeight() - SpriteAtlas::frame_height(this->atlas, 53) -
                   SpriteAtlas::frame_height(this->atlas, 5) - 6);
    }
    if (S().needs_rebuild) {
        graphics->setColor(0);
        graphics->fillRect(0, 0, GameCanvas::interface_width(), (int32_t)GameCanvas::state().canvas_height);
        graphics->setColor(0xFFFFFF);
        graphics->setFont(ui_fonts().bold_medium);
        graphics->drawString(GameCanvas::get_text(571),
                             (GameCanvas::interface_width() >> 1) -
                                 (ui_fonts().bold_medium->stringWidth(
                                          GameCanvas::get_text(571)) >>
                                      1),
                             (GameCanvas::state().canvas_height >> 1) -
                                 (ui_fonts().bold_medium->getHeight() >> 1),
                             0);
        graphics->drawString(voyage::text::upper_ascii(GameCanvas::get_text(22)), 2,
                             GameCanvas::state().canvas_height - ui_fonts().bold_medium->getHeight() - 2, 0);
        graphics->drawString(voyage::text::upper_ascii(GameCanvas::get_text(426)),
                             GameCanvas::interface_width() -
                                 ui_fonts().bold_medium->stringWidth(GameCanvas::get_text(426)) - 2,
                             GameCanvas::state().canvas_height - ui_fonts().bold_medium->getHeight() - 2, 0);
    }
}

void MenuScreen::handle_input(uint16_t input) {
    int32_t n = 0;
    int32_t n2 = 0;
    if (input == 4) {
        this->selected_index = (int8_t)(this->selected_index + 1);
        if (this->selected_index >= (int32_t)this->tab_roots[this->tab_index]->children.size()) {
            this->selected_index = 0;
        }
        if (!this->tab_roots.isNull() && this->tab_index < this->tab_roots.length() &&
            this->tab_roots[this->tab_index] != nullptr &&
            !this->tab_roots[this->tab_index]->detail_lines.isNull()) {
            this->scroll_offset = (int16_t)(this->scroll_offset - S().row_height);
            n = (this->tab_roots[this->tab_index]->detail_lines.length() >> 1) * S().row_height;
            if (n + this->scroll_offset + shiftLeft32(S().row_height, 2) <
                GameCanvas::state().canvas_height - ui_fonts().small->getHeight() - 4 -
                    SpriteAtlas::frame_height(this->atlas, 5)) {
                this->scroll_offset = (int16_t)(this->scroll_offset + S().row_height);
            }
        }
    } else if (input == 3) {
        this->selected_index = (int8_t)(this->selected_index - 1);
        if (this->selected_index < 0) {
            this->selected_index = (int8_t)((int32_t)this->tab_roots[this->tab_index]->children.size() - 1);
        }
        if (!this->tab_roots.isNull() && this->tab_index < this->tab_roots.length() &&
            this->tab_roots[this->tab_index] != nullptr &&
            !this->tab_roots[this->tab_index]->detail_lines.isNull()) {
            this->scroll_offset = (int16_t)(this->scroll_offset + S().row_height);
            if (this->scroll_offset > 0) {
                this->scroll_offset = 0;
            }
        }
    } else if (input == 5) {
        this->navigate_back();
        this->tab_index = (int8_t)(this->tab_index - 1);
        if (this->tab_index < 0) {
            this->tab_index = (int8_t)(this->tab_ids.length() - 2);
        }
        this->selected_index = 0;
        this->last_visible_row = 0;
        this->first_visible_row = 0;
        this->scroll_offset = 0;
    } else if (input == 6) {
        this->navigate_back();
        this->tab_index = (int8_t)(this->tab_index + 1);
        if (this->tab_index == this->tab_ids.length() - 1) {
            this->tab_index = 0;
        }
        this->selected_index = 0;
        this->last_visible_row = 0;
        this->first_visible_row = 0;
        this->scroll_offset = 0;
    } else if (input == 7 && this->selected_index < (int32_t)this->tab_roots[this->tab_index]->children.size() && this->selected_index >= 0) {
        MenuNode *c3 = this->tab_roots[this->tab_index]->children[(size_t)this->selected_index];
        if (!((this->tab_roots[this->tab_index]->label == GameCanvas::get_text(36)) ||
              (this->tab_roots[this->tab_index]->label == GameCanvas::get_text(37)) ||
              c3->is_enabled)) {
            return;
        }
        MenuNode *c4 = nullptr;
        if (c3->children.empty()) {
            for (n2 = 0; n2 < (int32_t)this->tab_roots[this->tab_index]->children.size(); ++n2) {
                c4 = this->tab_roots[this->tab_index]->children[(size_t)n2];
                if ((c3->parent->label == GameCanvas::get_text(27))) {
                    if (!this->a(c4, c3)) continue;
                    c4->is_selected = false;
                    continue;
                }
                c4->is_selected = false;
            }
            c3->is_selected = true;
        } else {
            this->tab_roots[this->tab_index] = c3;
            this->selected_index = 0;
        }
        if (this->game != nullptr) {
            this->game->handle_menu_selection(c3);
        }
    }
    if (this->selected_index > this->last_visible_row) {
        this->scroll_offset = (int16_t)(-S().row_height * (this->selected_index - (this->last_visible_row - this->first_visible_row)));
    } else if (this->selected_index < this->first_visible_row) {
        this->scroll_offset = (int16_t)(-S().row_height * this->selected_index);
    }
    this->a(0);
}

bool MenuScreen::a(MenuNode *c2, MenuNode *c3) {
    if (((c2->label == GameCanvas::get_text(149)) ||
         (c2->label == GameCanvas::get_text(151))) &&
        ((c3->label == GameCanvas::get_text(149)) ||
         (c3->label == GameCanvas::get_text(151)))) {
        return true;
    }
    return !(!(c2->label == GameCanvas::get_text(150)) &&
                 !(c2->label == GameCanvas::get_text(152)) ||
             !(c3->label == GameCanvas::get_text(150)) &&
                 !(c3->label == GameCanvas::get_text(152)));
}

void MenuScreen::a(const SharedArray<int8_t> &byArray, const SharedArray<MenuNode *> &cArray, const std::optional<std::string> &string, Image *image,
          Graphics *graphics) {
    this->tab_ids = byArray;
    this->tab_roots = cArray;
    this->is_open = 1;
    this->tab_index = 0;
    this->title = string;
    this->selected_index = 0;
    this->last_visible_row = 0;
    this->first_visible_row = 0;
    this->scroll_offset = 0;
    const int32_t ui_width = GameCanvas::interface_width();
    if (image != nullptr && image->getWidth() > ui_width) {
        this->background = Image::createImage(ui_width, (int32_t)GameCanvas::state().canvas_height);
        graphics = this->background->getGraphics();
    } else {
        this->background = image;
    }
    this->label_scroll_chars = 0;
    this->i = 0;
    this->b(graphics);
}

void MenuScreen::b(Graphics *graphics) {
    int32_t n = 0;
    int32_t n2 = 0;
    int32_t n3 = 0;
    int32_t n4 = 0;
    int32_t n5 = 0;
    int32_t n6 = 0;
    int32_t n7 = 0;
    int32_t n8 = 0;
    int32_t n9 = 0;
    int32_t n10 = 0;
    graphics->setColor(0);
    graphics->fillRect(0, 0, GameCanvas::interface_width(), (int32_t)GameCanvas::state().canvas_height);
    n = SpriteAtlas::frame_width(this->atlas, 13);
    n2 = SpriteAtlas::frame_height(this->atlas, 13);
    n3 = SpriteAtlas::frame_height(this->atlas, 11);
    n4 = SpriteAtlas::frame_height(this->atlas, 12);
    n5 = SpriteAtlas::frame_width(this->atlas, 12);
    n6 = SpriteAtlas::frame_width(this->atlas, 8);
    n7 = SpriteAtlas::frame_height(this->atlas, 5);
    n8 = SpriteAtlas::frame_width(this->atlas, 5);
    for (n9 = 0; n9 < GameCanvas::interface_width(); n9 += n) {
        for (n10 = 0; n10 < GameCanvas::state().canvas_height - ui_fonts().small->getHeight() - 4 - n2;
             n10 += n2) {
            SpriteAtlas::draw_frame(graphics, this->atlas, 13, n9, n10);
        }
    }
    for (n10 = 0; n10 < GameCanvas::state().canvas_height - ui_fonts().small->getHeight() - 4 - n3; n10 += n3) {
        SpriteAtlas::draw_frame(graphics, this->atlas, 11, 0, n10);
    }
    for (n10 = 0; n10 < GameCanvas::state().canvas_height - ui_fonts().small->getHeight() - 4 - n4; n10 += n4) {
        SpriteAtlas::draw_frame(graphics, this->atlas, 12, GameCanvas::interface_width() - n5, n10);
    }
    for (n9 = 0; n9 < GameCanvas::interface_width(); n9 += n6) {
        SpriteAtlas::draw_frame(graphics, this->atlas, 8, n9, 0);
    }
    for (n9 = 0; n9 < GameCanvas::interface_width(); n9 += n8) {
        SpriteAtlas::draw_frame(graphics, this->atlas, 5, n9,
               GameCanvas::state().canvas_height - ui_fonts().small->getHeight() - 4 - n7);
    }
    SpriteAtlas::draw_frame(graphics, this->atlas, 9, 0, 0);
    SpriteAtlas::draw_frame(graphics, this->atlas, 10, GameCanvas::interface_width() - SpriteAtlas::frame_width(this->atlas, 10), 0);
    SpriteAtlas::draw_frame(graphics, this->atlas, 6, 0,
           GameCanvas::state().canvas_height - ui_fonts().small->getHeight() - 4 - SpriteAtlas::frame_height(this->atlas, 6));
    SpriteAtlas::draw_frame(graphics, this->atlas, 7, GameCanvas::interface_width() - SpriteAtlas::frame_width(this->atlas, 7),
           GameCanvas::state().canvas_height - ui_fonts().small->getHeight() - 4 - SpriteAtlas::frame_height(this->atlas, 7));
}

bool MenuScreen::navigate_back() {
    if (this->tab_roots.isNull() || this->tab_roots[this->tab_index] == nullptr) {
        return false;
    }
    if (this->tab_roots[this->tab_index]->parent != nullptr) {
        this->tab_roots[this->tab_index] = this->tab_roots[this->tab_index]->parent;
        this->selected_index = 0;
        this->last_visible_row = 0;
        this->first_visible_row = 0;
        this->scroll_offset = 0;
        return true;
    }
    return false;
}

void MenuScreen::a(int64_t l) {
    if (this->i == 1) {
        this->scroll_timer_ms = (int16_t)((int64_t)this->scroll_timer_ms + l);
        if (this->label_scroll_paused == 1) {
            if (this->scroll_timer_ms >= 1000) {
                this->scroll_timer_ms = 0;
                this->label_scroll_paused = 0;
                return;
            }
        } else if (this->scroll_timer_ms >= 500) {
            this->label_scroll_chars = (int8_t)(this->label_scroll_chars + this->label_scroll_dir);
            this->scroll_timer_ms = 0;
        }
    }
}

void MenuScreen::a(int32_t n) {
    if (n == this->i) {
        return;
    }
    this->scroll_timer_ms = 0;
    this->label_scroll_chars = 0;
    this->label_scroll_dir = 1;
    this->label_scroll_paused = 0;
    this->i = (int8_t)n;
}
