#include "src/game/save_codec.hpp"

#include <cassert>
#include <stdexcept>

int main() {
    using namespace voyage;
    const storage::Bytes fixture{
        55,57,51, 1,1, 6,'/','a','.','s','c','r',
        0,2, 0x12,0x34,0x56, 0,7,
        0,11, 0,12, 0,13, 0,14, 0,15, 0,16,
        0xff,0xff, 0x01,0x23, 0x04,0x56, 1, 0xab,0xcd,
        6,'/','p','.','s','p','r', 2, 0x81,3, 2,4};
    const auto game = save::decode(fixture);
    assert(game.key_bindings[1] == 57 && game.intro_seen);
    assert(game.level_path == "/a.scr" && game.player);
    const auto &p = *game.player;
    assert(p.kind == 0 && p.character_class == 2 && p.experience == 0x123456 && p.level == 7);
    assert(p.strength == 11 && p.intelligence == 12 && p.agility == 13 && p.move_speed == 14);
    assert(p.endurance == 15 && p.willpower == 16 && p.equipped_weapon_id == -1);
    assert(p.engage_radius == 0x123 && p.disengage_radius == 0x456 && p.faction_id == 1 && p.gold == 0xabcd);
    assert(p.sprite_path == "/p.spr" && p.inventory.size() == 2);
    assert(p.inventory[0].kind == 1 && p.inventory[0].record_id == 3 && p.inventory[0].equipped);
    assert(p.inventory[1].kind == 2 && p.inventory[1].record_id == 4 && !p.inventory[1].equipped);
    assert(save::encode(game) == fixture);

    const storage::Bytes settings{55,57,51,0,0};
    assert(!save::decode(settings).player);
    assert(save::encode(save::decode(settings)) == settings);
    for (size_t size = 0; size < fixture.size(); ++size) {
        storage::Bytes truncated(fixture.begin(), fixture.begin() + size);
        bool rejected = false;
        try { (void)save::decode(truncated); } catch (const std::runtime_error &) { rejected = true; }
        assert(rejected);
    }
    auto bad = fixture;
    bad.push_back(0);
    bool rejected = false;
    try { (void)save::decode(bad); } catch (const std::runtime_error &) { rejected = true; }
    assert(rejected);
    auto boundary = game;
    boundary.player->inventory.assign(200, {2, 3, false});
    assert(save::decode(save::encode(boundary)).player->inventory.size() == 200);
    return 0;
}
