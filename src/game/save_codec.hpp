#ifndef VOYAGE_GAME_SAVE_CODEC_HPP
#define VOYAGE_GAME_SAVE_CODEC_HPP

#include <array>
#include <optional>
#include <string>
#include <vector>
#include "src/storage/binary_io.hpp"

namespace voyage::save {

inline constexpr const char *kSlot = "ESO";

struct Item {
    uint8_t kind = 0;
    uint8_t record_id = 0;
    bool equipped = false;
};

struct Player {
    int8_t kind = 0;
    int8_t character_class = 0;
    uint32_t experience = 0;
    int8_t level = 0;
    int16_t strength = 0;
    int16_t intelligence = 0;
    int16_t agility = 0;
    int16_t move_speed = 0;
    int16_t endurance = 0;
    int16_t willpower = 0;
    int8_t equipped_weapon_id = 0;
    int16_t engage_radius = 0;
    int16_t disengage_radius = 0;
    int8_t faction_id = 0;
    uint16_t gold = 0;
    std::string sprite_path;
    std::vector<Item> inventory;
};

struct Game {
    std::array<int8_t, 3> key_bindings{55, 57, 51};
    bool intro_seen = false;
    std::string level_path;
    std::optional<Player> player;
};

storage::Bytes encode(const Game &game);
Game decode(const storage::Bytes &bytes);

}

#endif
