#include "src/limbo/actor_system.hpp"
#include "src/limbo/game_canvas.hpp"
#include "src/limbo/game_data.hpp"
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
    int32_t state;
    int32_t page;
    int32_t tab;
    int32_t selected;
    int32_t child_count;
    std::string root_label;
    std::string selected_label;
    int32_t health;
    int32_t max_health;
    int32_t quick_health;
    int32_t inventory_count;
    uint64_t frame_hash;
};

int32_t inventory_count(const Actor *player) {
    int32_t count = 0;
    while (count < player->inventory_entries.length() && player->inventory_entries[count] != 0) ++count;
    return count;
}

Checkpoint capture(const char *name, voyage::GameRuntime &game) {
    Checkpoint checkpoint{name, game.simulation_time_ms(), game.state(), GameCanvas::state().menu_page};
    Actor *player = GameCanvas::state().actors[0];
    if (player != nullptr) {
        checkpoint.health = player->health;
        checkpoint.max_health = player->max_health;
        checkpoint.quick_health = player->quick_health_potion_record.isNull() ? -1 : player->quick_health_potion_record[0];
        checkpoint.inventory_count = inventory_count(player);
    }

    if (checkpoint.state == 2 && GameCanvas::state().instance != nullptr && GameCanvas::state().instance->menu_screen != nullptr) {
        MenuScreen *menu = GameCanvas::state().instance->menu_screen;
        checkpoint.tab = menu->tab_index;
        checkpoint.selected = menu->selected_index;
        if (!menu->tab_roots.isNull() && checkpoint.tab >= 0 && checkpoint.tab < menu->tab_roots.length()) {
            MenuNode *root = menu->tab_roots[checkpoint.tab];
            if (root != nullptr) {
                checkpoint.root_label = root->label;
                checkpoint.child_count = (int32_t)root->children.size();
                if (checkpoint.selected >= 0 && checkpoint.selected < checkpoint.child_count) {
                    checkpoint.selected_label = root->children[(size_t)checkpoint.selected]->label;
                }
            }
        }
    } else if (checkpoint.state == 3 && checkpoint.page >= 0 &&
               checkpoint.page < GameCanvas::state().menu_pages.length()) {
        const SharedArray<std::string> &rows = GameCanvas::state().menu_pages[checkpoint.page];
        checkpoint.selected = GameCanvas::state().menu_selection[checkpoint.page];
        checkpoint.child_count = rows.length();
        if (checkpoint.selected >= 0 && checkpoint.selected < rows.length()) {
            checkpoint.selected_label = rows[checkpoint.selected];
        }
    }

    checkpoint.frame_hash = voyage::test::hash_logical_frame(game.copy_frame());
    return checkpoint;
}

std::string encode(const Checkpoint &checkpoint) {
    std::ostringstream line;
    line << checkpoint.name << '|' << checkpoint.time_ms << '|' << checkpoint.state << '|'
         << checkpoint.page << '|' << checkpoint.tab << '|' << checkpoint.selected << '|'
         << checkpoint.child_count << '|' << checkpoint.root_label << '|' << checkpoint.selected_label << '|'
         << checkpoint.health << '|' << checkpoint.max_health << '|' << checkpoint.quick_health << '|'
         << checkpoint.inventory_count << '|' << checkpoint.frame_hash;
    return line.str();
}

SharedArray<int32_t> find_health_potion() {
    for (int32_t index = 0; index < GameCanvas::state().game_data->potion_table.length(); ++index) {
        const SharedArray<int32_t> &record = GameCanvas::state().game_data->potion_table[index];
        if (!record.isNull() && record[0] != 0 && record[5] == 0 && record[2] > 0) return record;
    }
    return SharedArray<int32_t>();
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
    config.save_dir += "/menu-actions-scenario";

    voyage::GameRuntime game(config);
    game.start();
    SessionScope scope(game.session());
    voyage::test::drive_opening(game);

    Actor *player = GameCanvas::state().actors[0];
    const SharedArray<int32_t> potion = find_health_potion();
    bool valid = true;
    valid &= voyage::test::require(player != nullptr, "menu actions scenario", "player is absent at gameplay start");
    valid &= voyage::test::require(!potion.isNull(), "menu actions scenario", "no instant health potion was loaded");
    if (!valid) return 1;

    ActorSystem::add_inventory_item(player, 2, potion, false);
    player->quick_health_potion_record.setNull();
    player->health = (int16_t)(player->max_health - potion[2]);
    const int32_t inventory_before = inventory_count(player);

    std::vector<Checkpoint> observed;
    voyage::test::tap(game, voyage::Input::Character);
    voyage::test::tap(game, voyage::Input::Right);
    voyage::test::tap(game, voyage::Input::Right);

    MenuScreen *menu = GameCanvas::state().instance->menu_screen;
    const std::string potion_label = GameCanvas::state().game_data->get_string(potion[1]);
    int32_t potion_index = -1;
    if (menu->tab_index == 2 && !menu->tab_roots.isNull() && menu->tab_roots[2] != nullptr) {
        const std::vector<MenuNode *> &items = menu->tab_roots[2]->children;
        for (int32_t index = 0; index < (int32_t)items.size(); ++index) {
            if (items[(size_t)index]->label == potion_label) {
                potion_index = index;
                break;
            }
        }
    }
    valid &= voyage::test::require(potion_index >= 0, "menu actions scenario",
                                   "staged potion did not appear in the Potions tab");
    if (!valid) return 1;
    for (int32_t index = 0; index < potion_index; ++index) voyage::test::tap(game, voyage::Input::Down);
    observed.push_back(capture("potion-selected", game));
    voyage::test::tap(game, voyage::Input::Action);
    observed.push_back(capture("potion-assigned", game));
    voyage::test::tap(game, voyage::Input::Menu);
    observed.push_back(capture("inventory-closed", game));

    voyage::test::tap(game, voyage::Input::Menu);
    observed.push_back(capture("pause-open", game));
    voyage::test::tap(game, voyage::Input::Right);
    observed.push_back(capture("pause-next", game));
    voyage::test::tap(game, voyage::Input::Left);
    observed.push_back(capture("pause-continue-selected", game));
    voyage::test::tap(game, voyage::Input::Action);
    observed.push_back(capture("pause-closed", game));

    valid &= voyage::test::require(!game.has_error(), "menu actions scenario", "the game entered its error screen");
    valid &= voyage::test::require(observed[0].state == 2 && observed[0].tab == 2 &&
                                       observed[0].selected_label == potion_label,
                                   "menu actions scenario", "the health potion was not selected in the Potions tab");
    valid &= voyage::test::require(observed[1].health == observed[0].health &&
                                       observed[1].quick_health == potion[0] &&
                                       observed[1].inventory_count == inventory_before,
                                   "menu actions scenario", "menu Action did not assign the quick-health potion");
    valid &= voyage::test::require(observed[2].state == 0, "menu actions scenario",
                                   "Menu did not close the inventory screen");
    valid &= voyage::test::require(observed[3].state == 3 && observed[3].selected == 0,
                                   "menu actions scenario", "Menu did not open the pause menu at Continue");
    valid &= voyage::test::require(observed[4].state == 3 && observed[4].selected == 1 &&
                                       observed[4].selected_label != observed[3].selected_label,
                                   "menu actions scenario", "Right did not advance the pause selection");
    valid &= voyage::test::require(observed[5].state == 3 && observed[5].selected == 0 &&
                                       observed[5].selected_label == observed[3].selected_label,
                                   "menu actions scenario", "Left did not return to Continue");
    valid &= voyage::test::require(observed[6].state == 0, "menu actions scenario",
                                   "Action on Continue did not resume gameplay");

    std::vector<std::string> actual;
    for (const Checkpoint &checkpoint : observed) actual.push_back(encode(checkpoint));
    if (emit) {
        if (valid && !voyage::test::write_trace(
                         argv[3],
                         "# name|time_ms|state|page|tab|selected|child_count|root_label|selected_label|health|"
                         "max_health|quick_health|inventory_count|logical_frame_fnv1a64",
                         actual)) {
            std::fprintf(stderr, "menu actions scenario: cannot write %s\n", argv[3]);
            valid = false;
        }
    } else {
        valid &= voyage::test::compare_trace(argv[2], actual, "menu actions scenario");
    }

    game.stop();
    return valid ? 0 : 1;
}
