#include "src/limbo/actor_system.hpp"
#include "src/limbo/game_canvas.hpp"
#include "src/limbo/key_bindings.hpp"
#include "src/limbo/ui_fonts.hpp"
#include "src/limbo/game_data.hpp"
#include "src/limbo/menu_screen.hpp"
#include "src/limbo/reference_pages.hpp"
#include "src/limbo/statics.hpp"
#include "src/limbo/text_resources.hpp"

#include <cassert>

int main() {
    oblivion_init_statics();

    assert(ActorSystem::S().level_xp_thresholds.length() == 26);
    assert(ActorSystem::S().level_xp_thresholds[1] == 0);
    assert(ActorSystem::S().level_xp_thresholds[2] == 100);
    assert(ActorSystem::S().level_xp_thresholds[25] == 25400);
    assert(ActorSystem::S().level_xp_awards[25] == 334);
    assert(ActorSystem::S().anim_frame_offsets.length() == 8);
    assert(ActorSystem::S().anim_frame_offsets[7] == 25);

    assert(key_bindings_state().codes[0] == 55);
    assert(key_bindings_state().codes[2] == 51);
    assert(GameCanvas::state().game_state == (int8_t)-1);
    assert(!GameCanvas::has_pending_key());
    assert(GameCanvas::state().pending_key_code == GameCanvas::NO_KEY_CODE);
    assert(GameCanvas::state().actors.length() == 25);
    assert(ui_fonts().bold_medium != nullptr);
    assert(GameCanvas::state().random != nullptr);

    assert(GameData::S().class_stat_growth_table.length() == 3);
    assert(GameData::S().class_stat_growth_table[2][6] == 3);

    assert(MenuScreen::S().row_height == (int8_t)ui_fonts().small->getHeight());

    assert(TextResources::max_string_id(0) == 574);
    assert(TextResources::max_string_id(12) == 547);
    assert(TextResources::record_count(0) == 305);

    assert(kClassNames.size() == 8);
    assert(std::string(kClassNames[0]) == "Monk");
    assert(std::string(kClassNames[1]) == "Nightblade");
    assert(std::string(kClassNames[2]) == "Barbarian");
    assert(std::string(kClassNames[3]) == "Archer");
    assert(std::string(kClassNames[4]) == "Knight");
    assert(std::string(kClassNames[5]) == "Spellsword");
    assert(std::string(kClassNames[6]) == "Sorcerer");
    assert(std::string(kClassNames[7]) == "Battlemage");
    assert(kClassTextIds.size() == 8);
    assert(kClassTextIds[0] == 9);
    assert(kClassTextIds[1] == 10);
    assert(kClassTextIds[2] == 11);
    assert(kClassTextIds[3] == 12);
    assert(kClassTextIds[4] == 13);
    assert(kClassTextIds[5] == 14);
    assert(kClassTextIds[6] == 15);
    assert(kClassTextIds[7] == 16);

    SharedArray<int32_t> origin{0, 0};
    SharedArray<int32_t> east{1024, 0};
    assert(ActorSystem::distance(origin, east) == 1007);
    SharedArray<int32_t> diagonal{1024, 1024};
    assert(ActorSystem::distance(origin, diagonal) == 1408);

    oblivion_init_statics();
    assert(ActorSystem::S().level_xp_thresholds[25] == 25400);

    SessionContext session;
    SessionScope scope(session);
    oblivion_init_statics();
    Actor *actor = session_new<Actor>();
    actor->move_speed = 1000;
    actor->world_position[0] = 500;
    actor->world_position[1] = 500;
    actor->collision_enabled = 0;

    ActorSystem::move_diagonal(actor, 1, 1, 60);
    assert(actor->world_position[0] == 543);
    assert(actor->world_position[1] == 543);
    assert(actor->facing == 3);

    ActorSystem::move_diagonal(actor, -1, -1, 60);
    assert(actor->world_position[0] == 500);
    assert(actor->world_position[1] == 500);
    assert(actor->facing == 4);

    GameCanvas::state().level_rows = 10;
    GameCanvas::state().level_cols = 10;
    GameCanvas::state().tile_walkable = SharedArray<int8_t>(100);
    actor->collision_enabled = 1;
    ActorSystem::set_position(actor, 10, 500);

    ActorSystem::move_diagonal(actor, -1, 1, 60);
    assert(actor->world_position[0] == 10);
    assert(actor->world_position[1] == 543);
    assert(actor->facing == 1);

    ActorSystem::set_position(actor, 500, 10);
    ActorSystem::move_diagonal(actor, 1, -1, 60);
    assert(actor->world_position[0] == 543);
    assert(actor->world_position[1] == 10);
    assert(actor->facing == 3);

    actor->collision_enabled = 0;

    ActorSystem::set_position(actor, 500, 500);
    ActorSystem::handle_input(actor, ActorSystem::ACTION_DOWN, 60, true);
    assert(actor->world_position[0] == 543);
    assert(actor->world_position[1] == 543);

    ActorSystem::handle_input(actor, ActorSystem::ACTION_UP, 60, true);
    assert(actor->world_position[0] == 500);
    assert(actor->world_position[1] == 500);

    ActorSystem::handle_input(actor, ActorSystem::ACTION_LEFT, 60, true);
    assert(actor->world_position[0] == 457);
    assert(actor->world_position[1] == 543);

    ActorSystem::handle_input(actor, ActorSystem::ACTION_RIGHT, 60, true);
    assert(actor->world_position[0] == 500);
    assert(actor->world_position[1] == 500);

    ActorSystem::set_position(actor, 500, 500);
    ActorSystem::handle_input(actor, ActorSystem::ACTION_UP_LEFT, 60, true);
    assert(actor->world_position[0] == 438);
    assert(actor->world_position[1] == 500);
    assert(actor->facing == ActorSystem::FACING_UP_LEFT);

    ActorSystem::set_position(actor, 500, 500);
    ActorSystem::handle_input(actor, ActorSystem::ACTION_DOWN_LEFT, 60, true);
    assert(actor->world_position[0] == 500);
    assert(actor->world_position[1] == 562);
    assert(actor->facing == ActorSystem::FACING_DOWN_LEFT);

    return 0;
}
