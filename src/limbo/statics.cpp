#include "src/limbo/statics.hpp"

#include "src/limbo/actor_system.hpp"
#include "src/limbo/game_canvas.hpp"
#include "src/limbo/game_data.hpp"
#include "src/limbo/menu_screen.hpp"
#include "src/limbo/oblivion_state.hpp"
#include "src/limbo/sprite_atlas.hpp"
#include "src/limbo/text_resources.hpp"

OblivionState &oblivion_state() {
    SessionContext &session = currentSession();
    if (session.game == nullptr) session.game = session_new<OblivionState>();
    return *session.game;
}

void oblivion_init_statics() {
    OblivionState &state = oblivion_state();
    if (state.initialized) return;
    state.initialized = true;
    TextResources::clinit();
    SpriteAtlas::clinit();
    ActorSystem::clinit();
    GameData::clinit();
    GameCanvas::initialize_session_state();
    MenuScreen::clinit();
}
