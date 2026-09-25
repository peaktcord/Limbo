#include "src/game/state_inspector.hpp"

#include "src/limbo/actor.hpp"
#include "src/limbo/game_canvas.hpp"
#include "src/limbo/game_data.hpp"

namespace voyage {

namespace {

std::string script_blocked_on(const GameData *script) {
    if (script == nullptr) return "no script";
    if (script->call_depth <= 0) return "script stack empty";
    if (script->waiting_for_key) return "paused, waiting for a key";
    if (script->game_canvas != nullptr && script->game_canvas->dialogue_open) {
        return "dialogue box open -- press the action key to advance";
    }
    if (script->game_canvas != nullptr && script->game_canvas->is_merchant_screen()) return "merchant menu";
    if (script->script_timer_target_ms >= 0) {
        return "timer " + std::to_string((long long)script->script_timer_ms) + "/" +
               std::to_string((long long)script->script_timer_target_ms) + " ms";
    }
    if (!script->waiting_for_actors.isNull()) {
        for (int32_t n = 0; n < script->waiting_for_actors.length(); ++n) {
            int32_t slot = script->waiting_for_actors[n];
            Actor *actor = GameCanvas::state().actors[slot];
            if (actor == nullptr || actor->move_target[0] == -1) continue;
            return "waiting for actor " + std::to_string((long long)slot) + " to reach (" +
                   std::to_string((long long)actor->move_target[0]) + "," +
                   std::to_string((long long)actor->move_target[1]) + "), it is at (" +
                   std::to_string((long long)actor->world_position[0]) + "," +
                   std::to_string((long long)actor->world_position[1]) +
                   ") dead=" + std::to_string((long long)actor->is_dead);
        }
        return "actor wait, all satisfied";
    }
    if (!script->scripted_walk_target.isNull()) {
        return "scripted walk, phase " + std::to_string((long long)script->interpreter_state) + " actor " +
               std::to_string((long long)script->scripted_walk_actor);
    }
    return "running";
}

}

RuntimeSnapshot inspect_canvas(const GameCanvas &canvas, int32_t state) {
    RuntimeSnapshot snapshot;
    snapshot.state = state;
    const GameCanvasStatics &statics = GameCanvas::state();

    if (statics.level_script_path.has_value()) {
        snapshot.level_id = *statics.level_script_path;
    }
    snapshot.camera_offset_x = canvas.camera_offset[0];
    snapshot.camera_offset_y = canvas.camera_offset[1];
    snapshot.camera_target_actor = canvas.camera_actor_slot;
    Actor *player = canvas.player;
    for (int32_t slot = 0; slot < statics.actors.length(); ++slot) {
        Actor *actor = statics.actors[slot];
        if (actor == nullptr) continue;
        RuntimeActorSnapshot actor_snapshot;
        actor_snapshot.slot = slot;
        actor_snapshot.alive = actor->is_dead == 0;
        actor_snapshot.hostile_to_player = player != nullptr && actor->faction_id != player->faction_id;
        actor_snapshot.attacking = actor->anim_state == 4;
        actor_snapshot.world_x = actor->world_position[0];
        actor_snapshot.world_y = actor->world_position[1];
        actor_snapshot.health = actor->health;
        actor_snapshot.max_health = actor->max_health;
        if (actor->combat_target != nullptr) {
            for (int32_t target_slot = 0; target_slot < statics.actors.length(); ++target_slot) {
                if (statics.actors[target_slot] == actor->combat_target) {
                    actor_snapshot.target_actor = target_slot;
                    break;
                }
            }
        }
        snapshot.actors.push_back(actor_snapshot);
    }
    if (player == nullptr) return snapshot;

    snapshot.player_present = true;
    snapshot.player_world_x = player->world_position[0];
    snapshot.player_world_y = player->world_position[1];
    snapshot.player_screen_x = player->screen_position[0];
    snapshot.player_screen_y = player->screen_position[1];
    return snapshot;
}

std::string state_name(int32_t state) {
    switch (state) {
        case 0: return "play";
        case 1: return "merchant menu";
        case 2: return "character menu";
        case 3: return "main menu";
        case 4: case 9: case 10: case 17: case 21: case 23: return "text screen";
        case 5: return "controls";
        case 6: case 7: return "loading";
        case 8: return "logo / intro";
        case 11: return "died";
        case 12: return "quitting";
        case 13: return "saved";
        case 15: return "please wait";
        case 18: return "help page";
        case 22: return "paused";
        case 24: return "port options";
        default: return "";
    }
}

std::string script_status(const GameCanvas &) { return script_blocked_on(GameCanvas::state().game_data); }

}
