#include "src/limbo/game_canvas.hpp"
#include "src/limbo/key_bindings.hpp"
#include "src/limbo/crash_report.hpp"
#include "src/game/save_codec.hpp"
#include "src/storage/save_store.hpp"

namespace {
namespace save = voyage::save;

voyage::storage::SaveStore &save_store() {
    auto *store = currentSession().saves;
    if (store == nullptr) throw std::runtime_error("No save store installed for this game");
    return *store;
}

std::optional<save::Game> read_save() {
    const auto records = save_store().load(save::kSlot);
    if (!records) return std::nullopt;
    if (records->empty()) throw std::runtime_error("ESO has no game record");
    return save::decode(records->front());
}

save::Player capture_player(Actor &actor) {
    save::Player p;
    p.kind = actor.actor_kind;
    p.character_class = actor.character_class;
    p.experience = static_cast<uint32_t>(actor.experience);
    p.level = actor.level;
    p.strength = actor.strength;
    p.intelligence = actor.intelligence;
    p.agility = actor.agility;
    p.move_speed = actor.move_speed;
    p.endurance = actor.endurance;
    p.willpower = actor.willpower;
    p.equipped_weapon_id = actor.equipped_weapon_id;
    p.engage_radius = actor.engage_radius;
    p.disengage_radius = actor.disengage_radius;
    p.faction_id = actor.faction_id;
    p.gold = static_cast<uint16_t>(GameCanvas::state().player_gold);
    p.sprite_path = actor.sprite_path;
    for (int32_t i = 0; i < actor.inventory_entries.length() && actor.inventory_entries[i] != 0; ++i) {
        const uint32_t entry = static_cast<uint32_t>(actor.inventory_entries[i]);
        const uint8_t kind = static_cast<uint8_t>(entry >> 8);
        const uint8_t id = static_cast<uint8_t>(entry);
        const bool equipped = kind == 1 ? ActorSystem::is_armour_equipped(&actor, static_cast<int32_t>(id)) :
            kind == 0 ? ActorSystem::is_weapon_or_spell_equipped(&actor, static_cast<int32_t>(id), false) : false;
        p.inventory.push_back({kind, id, equipped});
    }
    return p;
}

Actor *restore_player(const save::Player &p) {
    Actor *actor = session_new<Actor>();
    actor->actor_kind = p.kind;
    actor->character_class = p.character_class;
    actor->experience = static_cast<int32_t>(p.experience);
    actor->level = p.level;
    actor->strength = p.strength;
    actor->intelligence = p.intelligence;
    actor->agility = p.agility;
    actor->move_speed = p.move_speed;
    actor->endurance = p.endurance;
    actor->willpower = p.willpower;
    actor->equipped_weapon_id = p.equipped_weapon_id;
    actor->engage_radius = p.engage_radius;
    actor->disengage_radius = p.disengage_radius;
    actor->faction_id = p.faction_id;
    GameCanvas::state().player_gold = p.gold;
    actor->sprite_path = p.sprite_path;
    actor->name = std::string("Champion");
    ActorSystem::apply_class(actor, actor->character_class, true);
    for (int32_t i = 0; i < actor->inventory_entries.length(); ++i) actor->inventory_entries[i] = 0;
    for (const auto &item : p.inventory) {
        ActorSystem::add_inventory_item(actor, item.kind, GameCanvas::state().game_data->get_item_record(item.kind, item.record_id), item.equipped);
    }
    ActorSystem::recalculate_stats(actor);
    actor->tile_position[0] = actor->tile_position[1] = 0;
    actor->sprite_atlas = SpriteAtlas::load_atlas(actor->sprite_path);
    actor->footprint_width = static_cast<int8_t>(SpriteAtlas::frame_width(actor->sprite_atlas, 1));
    actor->footprint_half_width = static_cast<int8_t>(actor->footprint_width >> 1);
    ActorSystem::recalculate_stats(actor);
    return actor;
}
}

bool GameCanvas::save_game() {
    update_game_log_context(this, "saving ESO");
    persistence_error.clear();
    try {
        save::Game game;
        for (size_t i = 0; i < game.key_bindings.size(); ++i) game.key_bindings[i] = key_bindings_state().codes[static_cast<int32_t>(i)];
        game.intro_seen = this->var_boolean_o;
        if (this->player != nullptr) {
            game.level_path = state().level_script_path.value_or("null");
            game.player = capture_player(*this->player);
        }
        save_store().save(save::kSlot, {save::encode(game)});
        return true;
    } catch (const std::exception &error) {
        voyage::logging::exception("Save", "Writing ESO", error);
        log_game_context(this);
        persistence_error = "Save failed";
        return false;
    }
}

bool GameCanvas::has_saved_player() {
    try {
        const auto game = read_save();
        return game && game->player.has_value();
    } catch (const std::exception &error) {
        voyage::logging::exception("Save", "Checking ESO for a saved player", error);
        log_game_context(this);
        return false;
    }
}

bool GameCanvas::load_game(bool enter_level) {
    update_game_log_context(this, "loading ESO");
    persistence_error.clear();
    try {
        const auto game = read_save();
        if (!game) {
            if (enter_level) persistence_error = "No saved game";
            return false;
        }
        for (size_t i = 0; i < game->key_bindings.size(); ++i) key_bindings_state().codes[static_cast<int32_t>(i)] = game->key_bindings[i];
        this->var_boolean_o = game->intro_seen;
        this->loading_percent = 0;
        GameCanvas::set_game_state(static_cast<int8_t>(6));
        this->repaint();
        this->serviceRepaints();
        if (game->player) {
            if (enter_level) this->load_level_script(game->level_path);
            else state().level_script_path = game->level_path;
            this->player = state().actors[0] = restore_player(*game->player);
        }
        voyage::logging::write(voyage::logging::Level::Info, "Save", "Restored ESO; level=" + state().level_script_path.value_or("null"));
        return true;
    } catch (const std::exception &error) {
        voyage::logging::exception("Save", "Restoring ESO", error);
        log_game_context(this);
        persistence_error = "Load failed";
        return false;
    }
}
