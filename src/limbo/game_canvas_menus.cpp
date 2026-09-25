#include "src/limbo/game_canvas.hpp"

void GameCanvas::build_character_menu() {
    std::optional<std::string> object2;
    SharedArray<uint8_t> object;
    SharedArray<int32_t> nArray;
    SharedArray<int32_t> nArray2;
    int32_t n = 0;
    int32_t n2 = 0;
    uint16_t c2 = 0;
    uint16_t c3 = 0;
    int32_t n3 = 0;
    SharedArray<MenuNode *> cArray{session_new<MenuNode>(GameCanvas::get_text(25), std::nullopt, false),
                         session_new<MenuNode>(GameCanvas::get_text(26), std::nullopt, false),
                         session_new<MenuNode>(GameCanvas::get_text(27), std::nullopt, false),
                         session_new<MenuNode>(GameCanvas::get_text(394), std::nullopt, false)};
    SharedArray<MenuNode *> cArray2{session_new<MenuNode>(GameCanvas::get_text(28), std::nullopt, false),
                          session_new<MenuNode>(GameCanvas::get_text(29), std::nullopt, false),
                          session_new<MenuNode>(GameCanvas::get_text(30), std::nullopt, false),
                          session_new<MenuNode>(GameCanvas::get_text(31), std::nullopt, false),
                          session_new<MenuNode>(GameCanvas::get_text(32), std::nullopt, false),
                          session_new<MenuNode>(GameCanvas::get_text(33), std::nullopt, false),
                          session_new<MenuNode>(GameCanvas::get_text(34), std::nullopt, false),
                          session_new<MenuNode>(GameCanvas::get_text(35), std::nullopt, false)};
    for (n = 0; n < cArray2.length(); ++n) {
        MenuNode *child = cArray2[n];
        MenuNode *parent = cArray[1];
        parent->children.push_back(child);
        child->parent = parent;
    }
    object2 = std::nullopt;
    if (this->player->character_class == 4) {
        object2 = GameCanvas::get_text(12);
    } else if (this->player->character_class == 3) {
        object2 = GameCanvas::get_text(11);
    } else if (this->player->character_class == 8) {
        object2 = GameCanvas::get_text(16);
    } else if (this->player->character_class == 5) {
        object2 = GameCanvas::get_text(13);
    } else if (this->player->character_class == 1) {
        object2 = GameCanvas::get_text(9);
    } else if (this->player->character_class == 2) {
        object2 = GameCanvas::get_text(10);
    } else if (this->player->character_class == 7) {
        object2 = GameCanvas::get_text(15);
    } else if (this->player->character_class == 6) {
        object2 = GameCanvas::get_text(14);
    }
    cArray[3]->detail_lines = SharedArray<std::optional<std::string>>{
        GameCanvas::get_text(443) + ": ",
        object2,
        GameCanvas::get_text(17) + ": ",
        std::to_string((int32_t)this->player->level),
        GameCanvas::get_text(441) + ": ",
        std::to_string(this->player->experience),
        GameCanvas::get_text(442) + ": ",
        this->player->level < ActorSystem::S().level_xp_thresholds.length() - 1
            ? std::to_string((int32_t)(ActorSystem::S().level_xp_thresholds[this->player->level + 1] - this->player->experience))
            : std::string("0"),
        GameCanvas::get_text(415) + ": ",
        std::to_string((int32_t)(this->player->strength * 3)),
        GameCanvas::get_text(416) + ": ",
        std::to_string((int32_t)(this->player->intelligence * 3)),
        GameCanvas::get_text(417) + ": ",
        std::to_string((int32_t)(this->player->willpower * 3)),
        GameCanvas::get_text(418) + ": ",
        std::to_string((int32_t)(this->player->agility * 3)),
        GameCanvas::get_text(419) + ": ",
        std::to_string((int32_t)(this->player->endurance * 3)),
        GameCanvas::get_text(420) + ": ",
        std::to_string((int32_t)(this->player->personality * 3)),
        GameCanvas::get_text(431) + ": ",
        std::to_string((int32_t)(this->player->defense_rating * 3)),
        GameCanvas::get_text(432) + ": ",
        std::to_string((int32_t)(this->player->attack_rating * 3)),
        GameCanvas::get_text(563) + ": ",
        std::string("42"),
        GameCanvas::get_text(562) + ": ",
        std::string("40"),
        GameCanvas::get_text(38) + ": ",
        std::to_string(state().player_gold)};
    object = SharedArray<uint8_t>{false, false, false, false, false, false, false, false, false};
    bool bl = false;
    MenuNode *c4 = nullptr;
    bool bl2 = false;
    bool bl3 = false;
    if (this->player != nullptr) {
        while (this->player->inventory_entries[n2] != 0) {
            c2 = (uint16_t)((this->player->inventory_entries[n2] >> 8) & 0xFF);
            c3 = (uint16_t)((this->player->inventory_entries[n2] >> 0) & 0xFF);
            switch (c2) {
                case 0: {
                    MenuNode *c5;
                    nArray2 = state().game_data->get_record(4, c3);
                    if (nArray2[2] == 4) {
                        c4 = session_new<MenuNode>(GameCanvas::get_text(400) + state().game_data->get_string(nArray2[1]),
                                     GameCanvas::get_text(432) + ": " + std::to_string(nArray2[3]),
                                     ActorSystem::is_weapon_or_spell_equipped(this->player, nArray2, false) && !bl);
                        c4->is_enabled = ActorSystem::can_use_item(this->player, 0, nArray2);
                        c5 = cArray[0];
                        c5->children.push_back(c4);
                        c4->parent = c5;
                    } else {
                        c4 = session_new<MenuNode>(GameCanvas::get_text(305) + state().game_data->get_string(nArray2[1]),
                                     GameCanvas::get_text(432) + ": " + std::to_string(nArray2[3]),
                                     ActorSystem::is_weapon_or_spell_equipped(this->player, nArray2, false) && !bl);
                        if (ActorSystem::is_weapon_or_spell_equipped(this->player, nArray2, false)) {
                            state().equipped_weapon_node = c4;
                        }
                        c4->is_enabled = ActorSystem::can_use_item(this->player, 0, nArray2);
                        c5 = cArray[0];
                        c5->children.push_back(c4);
                        c4->parent = c5;
                    }
                    bl |= ActorSystem::is_weapon_or_spell_equipped(this->player, nArray2, false);
                    break;
                }
                case 1: {
                    nArray2 = state().game_data->get_record(1, c3);
                    bool n4 = ActorSystem::is_armour_equipped(this->player, nArray2[0]);
                    c4 = session_new<MenuNode>(state().game_data->get_string(nArray2[1]),
                                 GameCanvas::get_text(444) + ": " + std::to_string(nArray2[4]),
                                 n4 & (object[nArray2[3]] == false));
                    c4->is_enabled = ActorSystem::can_use_item(this->player, 1, nArray2);
                    MenuNode *c6 = cArray2[nArray2[3]];
                    c6->children.push_back(c4);
                    c4->parent = c6;
                    SharedArray<uint8_t> object3 = object;
                    int32_t n5 = nArray2[3];
                    object3[n5] = object3[n5] | n4;
                    break;
                }
                case 2: {
                    nArray2 = state().game_data->get_record(2, c3);
                    MenuNode *c7 = session_new<MenuNode>(state().game_data->get_string(nArray2[1]), std::nullopt, false);
                    if (nArray2.sameRef(this->player->quick_health_potion_record) && !bl2) {
                        c7->is_selected = true;
                        bl2 = true;
                    }
                    if (nArray2.sameRef(this->player->quick_magicka_potion_record) && !bl3) {
                        c7->is_selected = true;
                        bl3 = true;
                    }
                    MenuNode *c8 = cArray[2];
                    c8->children.push_back(c7);
                    c7->parent = c8;
                }
            }
            ++n2;
        }
        for (n3 = 0; n3 < this->player->available_spell_ids.length() && this->player->available_spell_ids[n3] != -1; ++n3) {
            nArray = state().game_data->get_record(8, this->player->available_spell_ids[n3]);
            c4 = session_new<MenuNode>(GameCanvas::get_text(304) + state().game_data->get_string(nArray[1]), std::nullopt,
                         ActorSystem::is_weapon_or_spell_equipped(this->player, nArray, true));
            MenuNode *c9 = cArray[0];
            c9->children.push_back(c4);
            c4->parent = c9;
            if (!ActorSystem::is_weapon_or_spell_equipped(this->player, nArray, true)) continue;
            state().equipped_spell_node = c4;
        }
    }
    this->menu_screen->a(SharedArray<int8_t>{4, 1, 2, 3, 18}, cArray, std::nullopt,
                     this->scene_image, this->scene_graphics);
    this->scene_dirty = true;
}

void GameCanvas::build_merchant_menu() {
    MenuNode *c2;
    MenuNode *c3;
    MenuNode *c4;
    MenuNode *c5;
    SharedArray<MenuNode *> cArray{session_new<MenuNode>(GameCanvas::get_text(36), std::nullopt, false),
                         session_new<MenuNode>(GameCanvas::get_text(37), std::nullopt, false)};
    SharedArray<int32_t> nArray = state().game_data->get_record(7, 0);
    SharedArray<int32_t> nArray2;
    int32_t n = 0;
    MenuNode *c6 = nullptr;
    while (nArray[n] != -1) {
        switch (nArray[n++]) {
            case 0: {
                nArray2 = state().game_data->get_record(4, nArray[n++]);
                c6 = session_new<MenuNode>(state().game_data->get_string(nArray2[1]) + " : " + std::to_string(nArray2[7]) + " " +
                                 GameCanvas::get_text(38),
                             GameCanvas::get_text(432) + ": " + std::to_string(nArray2[3]), false);
                c6->is_enabled = ActorSystem::can_use_item(this->player, 0, nArray2);
                MenuNode *c7 = cArray[0];
                c7->children.push_back(c6);
                c6->parent = c7;
                break;
            }
            case 1: {
                nArray2 = state().game_data->get_record(1, nArray[n++]);
                c6 = session_new<MenuNode>(state().game_data->get_string(nArray2[1]) + " : " + std::to_string(nArray2[9]) + " " +
                                 GameCanvas::get_text(38),
                             GameCanvas::get_text(444) + ": " + std::to_string(nArray2[4]), false);
                c6->is_enabled = ActorSystem::can_use_item(this->player, 1, nArray2);
                MenuNode *c8 = cArray[0];
                c8->children.push_back(c6);
                c6->parent = c8;
                break;
            }
            case 2: {
                nArray2 = state().game_data->get_record(2, nArray[n++]);
                c5 = session_new<MenuNode>(state().game_data->get_string(nArray2[1]) + " : " + std::to_string(nArray2[13]) + " " +
                                 GameCanvas::get_text(38),
                             std::nullopt, false);
                c4 = cArray[0];
                c4->children.push_back(c5);
                c5->parent = c4;
                break;
            }
            case 3: {
                nArray2 = state().game_data->get_record(3, nArray[n++]);
                c3 = session_new<MenuNode>(state().game_data->get_string(nArray2[1]) + " : " + std::to_string(nArray2[2]) + " " +
                                 GameCanvas::get_text(38),
                             std::nullopt, false);
                c2 = cArray[0];
                c2->children.push_back(c3);
                c3->parent = c2;
            }
        }
    }
    if (this->player != nullptr) {
        n = 0;
        while (this->player->inventory_entries[n] != 0) {
            uint16_t c9 = (uint16_t)((this->player->inventory_entries[n] >> 8) & 0xFF);
            uint16_t c10 = (uint16_t)((this->player->inventory_entries[n] >> 0) & 0xFF);
            switch (c9) {
                case 0: {
                    nArray2 = state().game_data->get_record(4, c10);
                    c6 = session_new<MenuNode>(state().game_data->get_string(nArray2[1]) + " : " + std::to_string(nArray2[7] >> 2) + " " +
                                     GameCanvas::get_text(38),
                                 GameCanvas::get_text(432) + ": " + std::to_string(nArray2[3]), false);
                    c6->is_enabled = ActorSystem::can_use_item(this->player, 0, nArray2);
                    c5 = cArray[1];
                    c5->children.push_back(c6);
                    c6->parent = c5;
                    break;
                }
                case 1: {
                    nArray2 = state().game_data->get_record(1, c10);
                    c6 = session_new<MenuNode>(state().game_data->get_string(nArray2[1]) + " : " + std::to_string(nArray2[9] >> 2) + " " +
                                     GameCanvas::get_text(38),
                                 GameCanvas::get_text(444) + ": " + std::to_string(nArray2[4]), false);
                    c6->is_enabled = ActorSystem::can_use_item(this->player, 1, nArray2);
                    c4 = cArray[1];
                    c4->children.push_back(c6);
                    c6->parent = c4;
                    break;
                }
                case 2: {
                    nArray2 = state().game_data->get_record(2, c10);
                    c3 = session_new<MenuNode>(state().game_data->get_string(nArray2[1]) + " : " + std::to_string(nArray2[13] >> 2) + " " +
                                     GameCanvas::get_text(38),
                                 std::nullopt, false);
                    c2 = cArray[1];
                    c2->children.push_back(c3);
                    c3->parent = c2;
                }
            }
            ++n;
        }
    }
    this->menu_screen->a(SharedArray<int8_t>{17, 15, 16}, cArray, GameCanvas::get_text(38) + " : " + std::to_string(state().player_gold),
                     this->scene_image, this->scene_graphics);
    this->scene_dirty = true;
}

void GameCanvas::handle_menu_selection(MenuNode *c2) {
    std::string string = c2->label;
    if ((c2->parent->label == GameCanvas::get_text(36))) {
        int32_t n = voyage::text::index_of(string, 58) + 2;
        int32_t n2 = voyage::text::index_of(string, 32, n);
        int32_t n3 = 0;
        try {
            n3 = voyage::text::parse_int32(voyage::text::slice(string, n, n2));
        } catch (const std::logic_error &error) {
            voyage::logging::exception("Merchant", ("Parsing purchase price in row: " + string).c_str(), error);
            throw;
        }
        if (state().player_gold >= n3) {
            state().player_gold -= n3;
            int32_t n4 = state().game_data->find_item_kind_by_name(voyage::text::slice(string, 0, n - 3));
            SharedArray<int32_t> nArray = state().game_data->find_record(voyage::text::slice(string, 0, n - 3));
            if (!nArray.isNull() && this->player != nullptr) {
                ActorSystem::add_inventory_item(this->player, n4, nArray);
                this->build_merchant_menu();
            }
        }
        c2->is_selected = false;
        this->menu_screen->title = GameCanvas::get_text(38) + " : " + std::to_string(state().player_gold);
        return;
    }
    if ((c2->parent->label == GameCanvas::get_text(37))) {
        int32_t n = voyage::text::index_of(string, 58) + 2;
        int32_t n5 = voyage::text::index_of(string, 32, n);
        int32_t n6 = 0;
        try {
            n6 = voyage::text::parse_int32(voyage::text::slice(string, n, n5));
        } catch (const std::logic_error &error) {
            voyage::logging::exception("Merchant", ("Parsing sale price in row: " + string).c_str(), error);
            throw;
        }
        if (this->player != nullptr) {
            int32_t n7 = state().game_data->find_item_kind_by_name(voyage::text::slice(string, 0, n - 3));
            SharedArray<int32_t> nArray = state().game_data->find_record(voyage::text::slice(string, 0, n - 3));
            if (!nArray.isNull()) {
                ActorSystem::remove_inventory_item(this->player, n7, nArray);
            }
            auto child_it = std::find(c2->parent->children.begin(), c2->parent->children.end(), c2);
            int32_t child_index = child_it == c2->parent->children.end()
                                      ? -1
                                      : (int32_t)(child_it - c2->parent->children.begin());
            if (child_index == (int32_t)c2->parent->children.size() - 1) {
                this->menu_screen->handle_input((uint16_t)3);
            }
            if (child_it != c2->parent->children.end()) c2->parent->children.erase(child_it);
        }
        c2->is_selected = false;
        this->menu_screen->title = GameCanvas::get_text(38) + " : " + std::to_string(state().player_gold += n6);
        return;
    }
    if ((c2->parent->label == GameCanvas::get_text(26))) {
        c2->is_selected = false;
        return;
    }
    if ((c2->parent->label == GameCanvas::get_text(25))) {
        bool bl = ActorSystem::equip_by_name(this->player, string);
        if (bl) {
            if (state().equipped_weapon_node != nullptr) {
                GameCanvas::state().equipped_weapon_node->is_selected = true;
            }
            state().equipped_spell_node = c2;
        } else {
            if (state().equipped_spell_node != nullptr) {
                GameCanvas::state().equipped_spell_node->is_selected = true;
            }
            state().equipped_weapon_node = c2;
        }
        return;
    }
    if ((c2->parent->label == GameCanvas::get_text(27))) {
        ActorSystem::use_potion(this->player, state().game_data->find_record(string));
        return;
    }
    ActorSystem::equip_armour(this->player, state().game_data->find_record(string));
}
