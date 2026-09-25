#include "src/storage/resource_store.hpp"
#include "src/common/ui.hpp"
#include "src/common/render/render.hpp"
#include "src/limbo/game_canvas.hpp"
#include "src/limbo/game_data.hpp"
#include "src/limbo/statics.hpp"

#include <cassert>

int main(int argc, char **argv) {
    assert(argc == 2);
    voyage::storage::DirectoryResourceStore resources(argv[1]);
    currentSession().resources = &resources;
    currentSession().set_canvas_size(176, 195);
    render::Surface screen(391, 220);
    render::SoftRenderer renderer(&screen);
    currentSession().paint = &renderer;
    oblivion_init_statics();

    GameCanvas canvas("/startup.scr", "/oh_menu.cml", "1.0.10");

    GameData *script = GameCanvas::state().game_data;
    script->script_bytecode = SharedArray<int8_t>(5);
    script->script_bytecode[0] = 25;
    script->script_bytecode[1] = 0x10;
    script->script_bytecode[2] = 0x18;
    script->script_bytecode[3] = 0x16;
    script->script_bytecode[4] = (int8_t)0x9D;
    script->call_depth = 1;
    script->frame_pc[0] = 0;
    script->script_timer_target_ms = -1;
    script->waiting_for_actors.setNull();
    script->scripted_walk_target.setNull();
    script->waiting_for_key = false;
    canvas.dialogue_open = false;

    script->execute_script_step(0);

    assert(script->frame_pc[0] == 5);
    assert(canvas.camera_actor_slot == (int8_t)-1);
    assert(canvas.camera_offset[0] == 297);
    assert(canvas.camera_offset[1] == -509);

    script->string_pool[254] = "/oh_pc.cml";
    script->script_table[24] = SharedArray<int32_t>(21);
    script->script_table[24][1] = 254;
    script->script_table[24][2] = 1;
    script->script_bytecode = SharedArray<int8_t>{15, 0, 0, 24, 24, 0, 0, 0, 0};
    script->frame_pc[0] = 0;
    script->execute_script_step(0);
    assert(script->frame_pc[0] == 9);
    assert(GameCanvas::state().actors[24] != nullptr);
    assert(!GameCanvas::state().actors[24]->name);

    script->script_bytecode = SharedArray<int8_t>{15, 0, 2, 'N', static_cast<int8_t>(0xe9), 24, 24, 0, 0, 0, 0};
    script->frame_pc[0] = 0;
    script->execute_script_step(0);
    assert(script->frame_pc[0] == 11);
    assert(GameCanvas::state().actors[24]->name == std::string("N\xe9", 2));
    return 0;
}
