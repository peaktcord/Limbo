#include "src/storage/resource_store.hpp"
#include "src/common/ui.hpp"
#include "src/storage/save_store.hpp"
#include "src/common/render/render.hpp"
#include "src/limbo/game_canvas.hpp"
#include "src/limbo/ui_fonts.hpp"
#include "src/limbo/statics.hpp"

#include <cassert>
#include <algorithm>
#include <cstdlib>
#include <filesystem>

namespace {

struct MenuGraphics : Graphics {
    int rectangles = 0;
    std::vector<std::string> labels;
    void drawRect(int32_t, int32_t, int32_t, int32_t) override { ++rectangles; }
    void drawString(const std::string &text, int32_t, int32_t, int32_t) override { labels.push_back(text); }
};

void advance(GameCanvas *canvas, int32_t elapsed_ms) {
    while (elapsed_ms > 0) {
        int32_t step = std::min(elapsed_ms, 16);
        currentSession().clock.advance(step);
        canvas->tick(step);
        elapsed_ms -= step;
    }
}

void tap(GameCanvas *canvas, int32_t key_code) {
    canvas->keyPressed(key_code);
    advance(canvas, 120);
    canvas->keyReleased(key_code);
}

}

int main(int argc, char **argv) {
    assert(argc == 2);
    SessionContext session;
    SessionScope scope(session);
    voyage::storage::DirectoryResourceStore resources(argv[1]);
    session.resources = &resources;
    const char *test_tmp = std::getenv("TEST_TMPDIR");
    assert(test_tmp != nullptr);
    voyage::storage::DirectorySaveStore saves((std::filesystem::path(test_tmp) / "rms").string());
    session.saves = &saves;
    session.clock.pin(0);
    session.set_canvas_size(391, 195);
    render::Surface screen(391, 220);
    render::SoftRenderer renderer(&screen);
    currentSession().paint = &renderer;
    oblivion_init_statics();

    GameCanvas *canvas = session_new<GameCanvas>("/startup.scr", "/oh_menu.cml", "1.0.10");
    Display::getDisplay()->setCurrent(canvas);
    assert(canvas != nullptr);
    canvas->modernized_dialogue_input = true;
    canvas->start_game_loop();
    canvas->tick(0);

    bool found_controls = false;
    for (int32_t step = 0; step < 120 && !found_controls; ++step) {
        advance(canvas, 1000);
        tap(canvas, step % 2 == 0 ? -5 : -2);
        if (!canvas->dialogue_lines.empty()) {
            std::string controls = GameCanvas::get_text(550);
            int32_t separator = voyage::text::index_of(controls, ' ');
            std::string prefix = voyage::text::slice(controls, 0, separator);
            found_controls = voyage::text::starts_with(canvas->dialogue_lines[0], prefix);
        }
    }

    assert(found_controls);
    assert(GameCanvas::state().canvas_width == 391);
    assert(GameCanvas::interface_width() == 176);
    assert(GameCanvas::interface_left() == 107);
    assert(canvas->dialogue_wrap_width == 144);
    assert(canvas->dialogue_box_height == 44);
    assert(canvas->dialogue_lines.size() == 5);
    const int32_t expected_widths[] = {131, 123, 131, 135, 31};
    for (size_t line = 0; line < canvas->dialogue_lines.size(); ++line) {
        int32_t width = ui_fonts().bold_small->stringWidth(
            canvas->dialogue_lines[line]);
        assert(width == expected_widths[line]);
        assert(width < canvas->dialogue_wrap_width);
    }
    assert(!canvas->dialogue_at_end);
    int32_t scroll = canvas->dialogue_scroll_y;
    canvas->handle_dialogue_input(4);
    assert(canvas->dialogue_scroll_y == scroll + 4);

    canvas->keyPressed(-5);
    canvas->open_dialogue();
    canvas->handle_dialogue_input(7);
    assert(canvas->dialogue_open);
    canvas->keyReleased(-5);
    canvas->handle_dialogue_input(7);
    assert(!canvas->dialogue_open);

    canvas->modernized_dialogue_input = false;
    canvas->set_confirm_input(false);
    canvas->open_dialogue();
    canvas->handle_dialogue_input(7);
    assert(canvas->dialogue_open);
    advance(canvas, 1000);
    canvas->handle_dialogue_input(7);
    assert(!canvas->dialogue_open);

    MenuNode root("Root", std::nullopt, false);
    MenuNode child("Item", std::nullopt, false);
    root.children.push_back(&child);
    auto *menu = canvas->menu_screen;
    menu->tab_roots = SharedArray<MenuNode *>{&root};
    menu->tab_ids.setNull();
    menu->tab_index = 0;
    menu->selected_index = 0;
    menu->scroll_offset = 0;
    menu->title = std::nullopt;
    MenuGraphics absent_hint;
    menu->render(&absent_hint);
    child.selection_hint = "";
    MenuGraphics empty_hint;
    menu->render(&empty_hint);
    assert(empty_hint.rectangles == absent_hint.rectangles + 1);
    assert(std::find(absent_hint.labels.begin(), absent_hint.labels.end(), "") == absent_hint.labels.end());
    assert(std::find(empty_hint.labels.begin(), empty_hint.labels.end(), "") != empty_hint.labels.end());

    canvas->stop_game_loop();
    return 0;
}
