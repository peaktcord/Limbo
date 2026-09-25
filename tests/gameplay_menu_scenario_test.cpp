#include "src/limbo/game_canvas.hpp"
#include "src/limbo/menu_node.hpp"
#include "src/limbo/menu_screen.hpp"
#include "src/game/game_runtime.hpp"
#include "tests/support/gameplay_scenario.hpp"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>

namespace {

struct Checkpoint {
    std::string name;
    uint64_t time_ms;
    voyage::RuntimeSnapshot state;
    int32_t tab_index = -1;
    int32_t selected_index = -1;
    int32_t child_count = 0;
    std::string root_label;
    std::string selected_label;
    uint64_t frame_hash = 0;
};

Checkpoint capture(const char *name, voyage::GameRuntime &game) {
    Checkpoint checkpoint;
    checkpoint.name = name;
    checkpoint.time_ms = game.simulation_time_ms();
    checkpoint.state = game.inspect_state();
    checkpoint.frame_hash = voyage::test::hash_logical_frame(game.copy_frame());

    if (GameCanvas::state().instance == nullptr || GameCanvas::state().instance->menu_screen == nullptr) return checkpoint;
    MenuScreen *menu = GameCanvas::state().instance->menu_screen;
    checkpoint.tab_index = menu->tab_index;
    checkpoint.selected_index = menu->selected_index;
    if (menu->tab_roots.isNull() || checkpoint.tab_index < 0 ||
        checkpoint.tab_index >= menu->tab_roots.length()) {
        return checkpoint;
    }
    MenuNode *root = menu->tab_roots[checkpoint.tab_index];
    if (root == nullptr) return checkpoint;
    checkpoint.root_label = root->label;
    checkpoint.child_count = (int32_t)root->children.size();
    if (checkpoint.selected_index >= 0 && checkpoint.selected_index < checkpoint.child_count) {
        checkpoint.selected_label = root->children[(size_t)checkpoint.selected_index]->label;
    }
    return checkpoint;
}

std::string encode(const Checkpoint &checkpoint) {
    std::ostringstream line;
    line << checkpoint.name << '|' << checkpoint.time_ms << '|' << checkpoint.state.state << '|'
         << checkpoint.tab_index << '|' << checkpoint.selected_index << '|' << checkpoint.child_count << '|'
         << checkpoint.root_label << '|' << checkpoint.selected_label << '|' << checkpoint.frame_hash;
    return line.str();
}

}

int main(int argc, char **argv) {
    const bool emit = argc == 4 && std::string(argv[2]) == "--emit";
    if (argc != 3 && !emit) {
        std::fprintf(stderr, "usage: %s RESOURCE_DIR EXPECTED_TRACE\n"
                             "       %s RESOURCE_DIR --emit OUTPUT_TRACE\n",
                     argv[0], argv[0]);
        return 2;
    }

    voyage::RuntimeConfig config;
    config.resource_dir = argv[1];
    config.random_seed = 0x4f626c6976696f6eLL;
    if (const char *test_tmp = std::getenv("TEST_TMPDIR")) config.save_dir = test_tmp;
    config.save_dir += "/menu-scenario";

    voyage::GameRuntime game(config);
    game.start();
    SessionScope scope(game.session());
    voyage::test::drive_opening(game);

    std::vector<Checkpoint> observed;
    voyage::test::tap(game, voyage::Input::Character);
    observed.push_back(capture("character-menu", game));
    voyage::test::tap(game, voyage::Input::Down);
    observed.push_back(capture("selection-down", game));
    voyage::test::tap(game, voyage::Input::Right);
    observed.push_back(capture("next-tab", game));
    voyage::test::tap(game, voyage::Input::Menu);
    observed.push_back(capture("gameplay-return", game));

    bool valid = true;
    valid &= voyage::test::require(!game.has_error(), "menu scenario", "the game entered its error screen");
    valid &= voyage::test::require(observed[0].state.state == 2, "menu scenario",
                                   "Character did not open the character menu");
    valid &= voyage::test::require(observed[0].child_count > 0, "menu scenario",
                                   "the initial character-menu tab is empty");
    valid &= voyage::test::require(observed[1].selected_index != observed[0].selected_index, "menu scenario",
                                   "Down did not change the selected menu item");
    valid &= voyage::test::require(observed[2].tab_index != observed[1].tab_index, "menu scenario",
                                   "Right did not change the active menu tab");
    valid &= voyage::test::require(observed[3].state.state == 0, "menu scenario",
                                   "Menu did not return to gameplay");

    std::vector<std::string> actual;
    for (const Checkpoint &checkpoint : observed) actual.push_back(encode(checkpoint));
    if (emit) {
        if (valid && !voyage::test::write_trace(
                         argv[3],
                         "# name|time_ms|state|tab_index|selected_index|child_count|root_label|selected_label|"
                         "logical_frame_fnv1a64",
                         actual)) {
            std::fprintf(stderr, "menu scenario: cannot write %s\n", argv[3]);
            valid = false;
        }
    } else {
        valid &= voyage::test::compare_trace(argv[2], actual, "menu scenario");
    }

    game.stop();
    return valid ? 0 : 1;
}
