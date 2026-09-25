#include "src/limbo/reference_pages.hpp"

#include "src/limbo/game_canvas.hpp"
#include "src/limbo/game_data.hpp"

ReferencePages &reference_pages_cache() {
    SessionContext &session = currentSession();
    if (session.reference_pages == nullptr) {
        session.reference_pages = session_new<ReferencePages>();
    }
    return *session.reference_pages;
}

void ReferencePages::clear() {
    game_overview.setNull();
    classes_overview.setNull();
    weapons_overview.setNull();
    armour_overview.setNull();
    spells_overview.setNull();
    items_overview.setNull();
}

SharedArray<SharedArray<std::optional<std::string>>> ReferencePages::build_game_overview() {
    if (!this->game_overview.isNull()) {
        return this->game_overview;
    }
    this->game_overview = makeSharedArray2D<std::optional<std::string>>(1, 1);
    this->game_overview[0][0] = GameCanvas::get_text(574);
    return this->game_overview;
}

SharedArray<SharedArray<std::optional<std::string>>> ReferencePages::build_classes_overview() {
    if (!this->classes_overview.isNull()) {
        return this->classes_overview;
    }

    GameData &game_data = *GameCanvas::state().game_data;
    const int32_t class_count = game_data.creature_table.length() - 1;
    this->classes_overview = SharedArray<SharedArray<std::optional<std::string>>>(class_count);

    for (int32_t class_idx = 1; class_idx < game_data.creature_table.length(); ++class_idx) {
        const int32_t page_idx = class_idx - 1;
        const auto &creature = game_data.creature_table[class_idx];

        SharedArray<std::optional<std::string>> page(30);
        int32_t line = 0;

        const std::string class_name =
            (page_idx >= 0 && page_idx < static_cast<int32_t>(kClassNames.size()))
                ? kClassNames[static_cast<size_t>(page_idx)]
                : game_data.get_string(creature[1]);

        page[line++] = GameCanvas::get_text(481) + class_name;
        page[line++] = GameCanvas::get_text(415) + ": " + std::to_string(creature[7] * 3);
        page[line++] = GameCanvas::get_text(416) + ": " + std::to_string(creature[8] * 3);
        page[line++] = GameCanvas::get_text(417) + ": " + std::to_string(creature[9] * 3);
        page[line++] = GameCanvas::get_text(418) + ": " + std::to_string(creature[10] * 3);
        page[line++] = GameCanvas::get_text(419) + ": " + std::to_string(creature[11] * 3);
        page[line++] = GameCanvas::get_text(420) + ": " + std::to_string(creature[12] * 3);
        page[line++] = GameCanvas::get_text(305) +
                       game_data.get_string(game_data.weapon_table[creature[4]][1]);
        page[line++] = GameCanvas::get_text(499) +
                       game_data.get_string(game_data.armour_table[creature[5]][1]);
        page[line++] = GameCanvas::get_text(538);

        const auto &skills = game_data.class_skills_table[class_idx];
        for (int32_t skill_idx = 0; skill_idx < 15 && skills[skill_idx] != -1; ++skill_idx) {
            page[line++] = "   " + GameData::effect_name(skills[skill_idx]).value_or("null");
        }

        page[line++] = GameCanvas::get_text(539);

        const auto &spells = game_data.class_spells_table[class_idx];
        if (spells.length() == 0 || spells[0] == -1) {
            page[line++] = "   " + GameCanvas::get_text(572);
        } else {
            for (int32_t spell_idx = 0; spell_idx < spells.length() && spells[spell_idx] != -1; ++spell_idx) {
                page[line++] = "   " + game_data.get_string(game_data.spell_table[spells[spell_idx]][1]);
            }
        }

        this->classes_overview[page_idx] = page;
    }

    return this->classes_overview;
}

namespace {

std::string class_display_name(int32_t class_idx, const GameData &game_data) {
    const int32_t index = class_idx - 1;
    if (index >= 0 && index < static_cast<int32_t>(kClassNames.size())) {
        return kClassNames[static_cast<size_t>(index)];
    }
    if (class_idx >= 0 && class_idx < game_data.creature_table.length()) {
        return const_cast<GameData &>(game_data).get_string(game_data.creature_table[class_idx][1]);
    }
    return "";
}

}

SharedArray<SharedArray<std::optional<std::string>>> ReferencePages::build_items_overview() {
    if (!this->items_overview.isNull()) {
        return this->items_overview;
    }

    GameData &game_data = *GameCanvas::state().game_data;
    const int32_t item_count = game_data.potion_table.length() - 1;
    this->items_overview = SharedArray<SharedArray<std::optional<std::string>>>(item_count);

    for (int32_t item_idx = 1; item_idx < game_data.potion_table.length(); ++item_idx) {
        const int32_t page_idx = item_idx - 1;
        const auto &potion = game_data.potion_table[item_idx];

        SharedArray<std::optional<std::string>> page(10);
        int32_t line = 0;

        page[line++] = GameCanvas::get_text(481) + game_data.get_string(potion[1]);
        page[line++] = GameCanvas::get_text(484) + std::to_string(potion[13]);
        page[line++] = GameCanvas::get_text(485) + std::to_string(potion[13] >> 2);

        if (potion[2] > 0) {
            page[line++] = GameCanvas::get_text(496) + std::to_string(potion[2]);
        }
        if (potion[3] > 0) {
            page[line++] = GameCanvas::get_text(497) + std::to_string(potion[3]);
        }
        if (potion[6] > 0) {
            page[line++] = GameCanvas::get_text(498) + std::to_string(potion[6]);
        }
        if (potion[7] > 0) {
            page[line++] = GameCanvas::get_text(499) + std::to_string(potion[7]);
        }
        if (potion[8] > 0) {
            page[line++] = GameCanvas::get_text(500) + std::to_string(potion[8]);
        }
        if (potion[10] > 0) {
            page[line++] = GameCanvas::get_text(501) + std::to_string(potion[10]);
        }
        if (potion[5] > 0) {
            page[line++] = GameCanvas::get_text(502) +
                           std::to_string(potion[5] / 1000) +
                           GameCanvas::get_text(521);
        }

        this->items_overview[page_idx] = page;
    }

    return this->items_overview;
}

SharedArray<SharedArray<std::optional<std::string>>> ReferencePages::build_spells_overview() {
    if (!this->spells_overview.isNull()) {
        return this->spells_overview;
    }

    GameData &game_data = *GameCanvas::state().game_data;
    const int32_t spell_count = game_data.spell_table.length() - 1;
    this->spells_overview = SharedArray<SharedArray<std::optional<std::string>>>(spell_count);

    for (int32_t spell_idx = 1; spell_idx < game_data.spell_table.length(); ++spell_idx) {
        const int32_t page_idx = spell_idx - 1;
        const auto &spell = game_data.spell_table[spell_idx];

        std::string school = "null";
        switch (spell[2]) {
            case 0:
            case 1: school = GameCanvas::get_text(437); break;
            case 2: school = GameCanvas::get_text(505); break;
            case 3: school = GameCanvas::get_text(439); break;
            case 4: school = GameCanvas::get_text(504); break;
            case 5: school = GameCanvas::get_text(433); break;
            case 6: school = GameCanvas::get_text(503); break;
        }

        std::string target = "null";
        switch (spell[7]) {
            case 0: target = GameCanvas::get_text(506); break;
            case 1: target = GameCanvas::get_text(507); break;
            case 2: target = GameCanvas::get_text(508); break;
            case 3: target = GameCanvas::get_text(509); break;
            case 4: target = GameCanvas::get_text(510); break;
        }

        this->spells_overview[page_idx] = SharedArray<std::optional<std::string>>{
            GameCanvas::get_text(481) + game_data.get_string(spell[1]),
            GameCanvas::get_text(511) + school,
            GameCanvas::get_text(512),
            GameCanvas::get_text(513) + std::to_string(spell[8]),
            GameCanvas::get_text(514) + std::to_string(spell[9]),
            GameCanvas::get_text(515) + std::to_string(spell[10]),
            GameCanvas::get_text(516),
            GameCanvas::get_text(513) + std::to_string(wrappingAbs(spell[3])),
            GameCanvas::get_text(514) + std::to_string(wrappingAbs(spell[4])),
            GameCanvas::get_text(515) + std::to_string(wrappingAbs(spell[5])),
            GameCanvas::get_text(517),
            GameCanvas::get_text(513) + std::to_string(spell[11]),
            GameCanvas::get_text(514) + std::to_string(spell[12]),
            GameCanvas::get_text(515) + std::to_string(spell[13]),
            GameCanvas::get_text(518) + std::to_string(spell[6] / 1000) + GameCanvas::get_text(521),
            GameCanvas::get_text(519) + target,
            GameCanvas::get_text(520) + std::to_string(spell[14]),
        };
    }

    return this->spells_overview;
}

SharedArray<SharedArray<std::optional<std::string>>> ReferencePages::build_armour_overview() {
    if (!this->armour_overview.isNull()) {
        return this->armour_overview;
    }

    GameData &game_data = *GameCanvas::state().game_data;
    const int32_t armour_count = game_data.armour_table.length() - 1;
    this->armour_overview = SharedArray<SharedArray<std::optional<std::string>>>(armour_count);

    for (int32_t armour_idx = 1; armour_idx < game_data.armour_table.length(); ++armour_idx) {
        const int32_t page_idx = armour_idx - 1;
        const auto &armour = game_data.armour_table[armour_idx];

        std::string slot_name = "null";
        switch (armour[3]) {
            case 0: slot_name = GameCanvas::get_text(28); break;
            case 1: slot_name = GameCanvas::get_text(29); break;
            case 2: slot_name = GameCanvas::get_text(30); break;
            case 3: slot_name = GameCanvas::get_text(31); break;
            case 4: slot_name = GameCanvas::get_text(32); break;
            case 5: slot_name = GameCanvas::get_text(33); break;
            case 6: slot_name = GameCanvas::get_text(34); break;
            case 7: slot_name = GameCanvas::get_text(35); break;
        }

        std::string weight_name = "null";
        int32_t required_skill = -1;
        switch (armour[2]) {
            case 0:
                weight_name = GameCanvas::get_text(489);
                required_skill = 1;
                break;
            case 1:
                weight_name = GameCanvas::get_text(488);
                required_skill = 3;
                break;
            case 2:
                weight_name = GameCanvas::get_text(487);
                required_skill = 4;
                break;
        }

        SharedArray<std::optional<std::string>> page(15);
        int32_t line = 0;

        page[line++] = GameCanvas::get_text(481) + game_data.get_string(armour[1]);
        page[line++] = GameCanvas::get_text(482) + weight_name;
        page[line++] = GameCanvas::get_text(490) + slot_name;
        page[line++] = GameCanvas::get_text(483) + std::to_string(armour[4]);
        page[line++] = GameCanvas::get_text(484) + std::to_string(armour[9]);
        page[line++] = GameCanvas::get_text(485) + std::to_string(armour[9] >> 2);
        page[line++] = GameCanvas::get_text(486);

        for (int32_t class_idx = 1; class_idx < game_data.creature_table.length(); ++class_idx) {
            const auto &creature = game_data.creature_table[class_idx];
            if (required_skill >= 0 && game_data.class_has_skill(creature[0], required_skill)) {
                page[line++] = "   " + class_display_name(class_idx, game_data);
            }
        }

        this->armour_overview[page_idx] = page;
    }

    return this->armour_overview;
}

SharedArray<SharedArray<std::optional<std::string>>> ReferencePages::build_weapons_overview() {
    if (!this->weapons_overview.isNull()) {
        return this->weapons_overview;
    }

    GameData &game_data = *GameCanvas::state().game_data;
    const int32_t weapon_count = game_data.weapon_table.length() - 1;
    this->weapons_overview = SharedArray<SharedArray<std::optional<std::string>>>(weapon_count);

    for (int32_t weapon_idx = 1; weapon_idx < game_data.weapon_table.length(); ++weapon_idx) {
        const int32_t page_idx = weapon_idx - 1;
        const auto &weapon = game_data.weapon_table[weapon_idx];

        std::string type_name = "null";
        int32_t required_skill = -1;
        switch (weapon[2]) {
            case 0:
                type_name = GameCanvas::get_text(476);
                required_skill = 14;
                break;
            case 1:
                type_name = GameCanvas::get_text(477);
                required_skill = 5;
                break;
            case 2:
                type_name = GameCanvas::get_text(479);
                required_skill = 6;
                break;
            case 3:
                type_name = GameCanvas::get_text(480);
                required_skill = 7;
                break;
            case 4:
                type_name = GameCanvas::get_text(478);
                required_skill = 8;
                break;
        }

        SharedArray<std::optional<std::string>> page(14);
        int32_t line = 0;

        page[line++] = GameCanvas::get_text(481) + game_data.get_string(weapon[1]);
        page[line++] = GameCanvas::get_text(482) + type_name;
        page[line++] = GameCanvas::get_text(483) + std::to_string(weapon[3]);
        page[line++] = GameCanvas::get_text(484) + std::to_string(weapon[7]);
        page[line++] = GameCanvas::get_text(485) + std::to_string(weapon[7] >> 2);
        page[line++] = GameCanvas::get_text(486);

        for (int32_t class_idx = 1; class_idx < game_data.creature_table.length(); ++class_idx) {
            const auto &creature = game_data.creature_table[class_idx];
            if (required_skill >= 0 && game_data.class_has_skill(creature[0], required_skill)) {
                page[line++] = "   " + class_display_name(class_idx, game_data);
            }
        }

        this->weapons_overview[page_idx] = page;
    }

    return this->weapons_overview;
}
