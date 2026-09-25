#include "src/limbo/actor_system.hpp"
#include "src/limbo/game_canvas.hpp"
#include "src/limbo/game_data.hpp"
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
    int32_t health;
    int32_t max_health;
    int32_t magicka;
    int32_t max_magicka;
    int32_t health_potion;
    int32_t magicka_potion;
    int32_t learned_spell;
    int32_t active_spell;
    int32_t held_icon;
    int32_t effect_icon;
    int32_t inventory_count;
    uint64_t frame_hash;
};

int32_t record_id(const SharedArray<int32_t> &record) { return record.isNull() ? -1 : record[0]; }

int32_t inventory_count(const Actor *player) {
    int32_t count = 0;
    while (count < player->inventory_entries.length() && player->inventory_entries[count] != 0) ++count;
    return count;
}

Checkpoint capture(const char *name, voyage::GameRuntime &game) {
    Actor *player = GameCanvas::state().actors[0];
    return Checkpoint{name,
                      game.simulation_time_ms(),
                      game.state(),
                      player->health,
                      player->max_health,
                      player->magicka,
                      player->max_magicka,
                      record_id(player->quick_health_potion_record),
                      record_id(player->quick_magicka_potion_record),
                      record_id(player->stowed_hand_record),
                      record_id(player->active_hand_record),
                      player->hand_icon_frame,
                      player->offhand_icon_frame,
                      inventory_count(player),
                      voyage::test::hash_logical_frame(game.copy_frame())};
}

std::string encode(const Checkpoint &checkpoint) {
    std::ostringstream line;
    line << checkpoint.name << '|' << checkpoint.time_ms << '|' << checkpoint.state << '|'
         << checkpoint.health << '|' << checkpoint.max_health << '|' << checkpoint.magicka << '|'
         << checkpoint.max_magicka << '|' << checkpoint.health_potion << '|'
         << checkpoint.magicka_potion << '|' << checkpoint.learned_spell << '|'
         << checkpoint.active_spell << '|' << checkpoint.held_icon << '|' << checkpoint.effect_icon << '|'
         << checkpoint.inventory_count << '|' << checkpoint.frame_hash;
    return line.str();
}

SharedArray<int32_t> find_potion(int effect_column) {
    for (int32_t index = 0; index < GameCanvas::state().game_data->potion_table.length(); ++index) {
        const SharedArray<int32_t> &record = GameCanvas::state().game_data->potion_table[index];
        if (!record.isNull() && record[0] != 0 && record[5] == 0 && record[effect_column] > 0) return record;
    }
    return SharedArray<int32_t>();
}

SharedArray<int32_t> find_spell(const Actor *player) {
    for (int32_t index = 0; index < GameCanvas::state().game_data->spell_table.length(); ++index) {
        const SharedArray<int32_t> &record = GameCanvas::state().game_data->spell_table[index];
        if (record.isNull() || record[0] == 0) continue;
        const int32_t cost = player->level >= record[10]
                                 ? record[13]
                                 : (player->level >= record[9] ? record[12] : record[11]);
        if (cost > 0 && cost < player->max_magicka) return record;
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
    config.save_dir += "/items-spell-scenario";

    voyage::GameRuntime game(config);
    game.start();
    SessionScope scope(game.session());
    voyage::test::drive_opening(game);

    Actor *player = GameCanvas::state().actors[0];
    if (!voyage::test::require(player != nullptr, "items/spell scenario",
                               "player is absent at gameplay start")) {
        return 1;
    }
    const SharedArray<int32_t> health_potion = find_potion(2);
    const SharedArray<int32_t> magicka_potion = find_potion(3);
    const SharedArray<int32_t> spell = find_spell(player);

    bool valid = true;
    valid &= voyage::test::require(!health_potion.isNull(), "items/spell scenario", "no instant health potion was loaded");
    valid &= voyage::test::require(!magicka_potion.isNull(), "items/spell scenario", "no instant magicka potion was loaded");
    valid &= voyage::test::require(!spell.isNull(), "items/spell scenario", "no affordable spell was loaded");
    if (!valid) return 1;

    ActorSystem::add_inventory_item(player, 2, health_potion, false);
    ActorSystem::add_inventory_item(player, 2, magicka_potion, false);
    player->health = (int16_t)(player->max_health - health_potion[2]);
    player->magicka = 0;

    std::vector<Checkpoint> observed;
    observed.push_back(capture("items-ready", game));
    voyage::test::tap(game, voyage::Input::QuickHealth);
    observed.push_back(capture("health-used", game));
    voyage::test::tap(game, voyage::Input::QuickMagicka);
    observed.push_back(capture("magicka-used", game));

    player->stowed_hand_record = spell;
    player->active_hand_record.setNull();
    ActorSystem::update_hand_icon(player);
    observed.push_back(capture("spell-ready", game));
    voyage::test::tap(game, voyage::Input::ToggleWeapon);
    observed.push_back(capture("spell-equipped", game));
    const int32_t magicka_before_cast = player->magicka;
    voyage::test::tap(game, voyage::Input::Action);
    voyage::test::advance(game, 500);
    observed.push_back(capture("spell-cast", game));

    valid &= voyage::test::require(!game.has_error(), "items/spell scenario", "the game entered its error screen");
    valid &= voyage::test::require(observed[0].health < observed[0].max_health &&
                                       observed[0].magicka < observed[0].max_magicka,
                                   "items/spell scenario", "staged resources did not lower both player meters");
    valid &= voyage::test::require(observed[1].health == observed[1].max_health &&
                                       observed[1].inventory_count == observed[0].inventory_count - 1,
                                   "items/spell scenario", "quick health did not heal and consume its potion");
    valid &= voyage::test::require(observed[2].magicka == observed[2].max_magicka &&
                                       observed[2].inventory_count == observed[1].inventory_count - 1,
                                   "items/spell scenario", "quick magicka did not restore and consume its potion");
    valid &= voyage::test::require(observed[3].active_spell == -1 && observed[4].active_spell == record_id(spell),
                                   "items/spell scenario", "weapon/spell toggle did not equip the learned spell");
    valid &= voyage::test::require(observed[5].magicka < magicka_before_cast,
                                   "items/spell scenario", "Action did not spend magicka on the equipped spell");

    std::vector<std::string> actual;
    for (const Checkpoint &checkpoint : observed) actual.push_back(encode(checkpoint));
    if (emit) {
        if (valid && !voyage::test::write_trace(
                         argv[3],
                         "# name|time_ms|state|health|max_health|magicka|max_magicka|health_potion|"
                         "magicka_potion|learned_spell|active_spell|held_icon|effect_icon|inventory_count|"
                         "logical_frame_fnv1a64",
                         actual)) {
            std::fprintf(stderr, "items/spell scenario: cannot write %s\n", argv[3]);
            valid = false;
        }
    } else {
        valid &= voyage::test::compare_trace(argv[2], actual, "items/spell scenario");
    }

    game.stop();
    return valid ? 0 : 1;
}
