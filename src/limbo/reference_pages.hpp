#ifndef OBLIVION_REFERENCE_PAGES_HPP
#define OBLIVION_REFERENCE_PAGES_HPP

#include <array>
#include <cstdint>
#include <string_view>

#include "src/common/session.hpp"

inline constexpr std::array<const char *, 8> kClassNames = {
    "Monk",
    "Nightblade",
    "Barbarian",
    "Archer",
    "Knight",
    "Spellsword",
    "Sorcerer",
    "Battlemage",
};

inline constexpr std::array<int32_t, 8> kClassTextIds = {
    9,
    10,
    11,
    12,
    13,
    14,
    15,
    16,
};

class ReferencePages {
public:
    SharedArray<SharedArray<std::optional<std::string>>> game_overview;
    SharedArray<SharedArray<std::optional<std::string>>> classes_overview;
    SharedArray<SharedArray<std::optional<std::string>>> weapons_overview;
    SharedArray<SharedArray<std::optional<std::string>>> armour_overview;
    SharedArray<SharedArray<std::optional<std::string>>> spells_overview;
    SharedArray<SharedArray<std::optional<std::string>>> items_overview;

    SharedArray<SharedArray<std::optional<std::string>>> build_game_overview();
    SharedArray<SharedArray<std::optional<std::string>>> build_classes_overview();
    SharedArray<SharedArray<std::optional<std::string>>> build_items_overview();
    SharedArray<SharedArray<std::optional<std::string>>> build_spells_overview();
    SharedArray<SharedArray<std::optional<std::string>>> build_armour_overview();
    SharedArray<SharedArray<std::optional<std::string>>> build_weapons_overview();

    void clear();
};

ReferencePages &reference_pages_cache();

#endif
