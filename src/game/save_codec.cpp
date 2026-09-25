#include "src/game/save_codec.hpp"

#include <stdexcept>

namespace voyage::save {
namespace {
void write_path(storage::BinaryWriter &writer, const std::string &path) {
    if (path.empty() || path.size() > 255) throw std::runtime_error("Invalid save resource path length");
    writer.u8(static_cast<int32_t>(path.size()));
    writer.text(path);
}
std::string read_path(storage::BinaryReader &reader) {
    const size_t size = reader.u8();
    if (size == 0) throw std::runtime_error("Empty resource path in save");
    return reader.text(size);
}
bool read_flag(storage::BinaryReader &reader) {
    const uint8_t value = reader.u8();
    if (value > 1) throw std::runtime_error("Invalid flag in save");
    return value == 1;
}
}

storage::Bytes encode(const Game &game) {
    storage::BinaryWriter out;
    for (int8_t key : game.key_bindings) out.u8(key);
    out.u8(game.intro_seen ? 1 : 0);
    out.u8(game.player ? 1 : 0);
    if (game.player) {
        write_path(out, game.level_path);
        const Player &p = *game.player;
        out.u8(p.kind);
        out.u8(p.character_class);
        out.big_endian(p.experience, 3);
        out.big_endian(static_cast<uint32_t>(p.level), 2);
        for (int16_t value : {p.strength, p.intelligence, p.agility, p.move_speed, p.endurance, p.willpower}) {
            out.big_endian(static_cast<uint32_t>(value), 2);
        }
        out.big_endian(static_cast<uint32_t>(p.equipped_weapon_id), 2);
        out.big_endian(static_cast<uint32_t>(p.engage_radius), 2);
        out.big_endian(static_cast<uint32_t>(p.disengage_radius), 2);
        out.u8(p.faction_id);
        out.big_endian(p.gold, 2);
        write_path(out, p.sprite_path);
        if (p.inventory.size() > 254) throw std::runtime_error("Too many inventory entries in save");
        out.u8(static_cast<int32_t>(p.inventory.size()));
        for (const Item &item : p.inventory) {
            if (item.kind > 2) throw std::runtime_error("Invalid inventory kind in save");
            out.u8(item.kind | (item.equipped ? 0x80 : 0));
            out.u8(item.record_id);
        }
    }
    return out.data();
}

Game decode(const storage::Bytes &bytes) {
    storage::BinaryReader in(bytes);
    Game game;
    for (int8_t &key : game.key_bindings) key = static_cast<int8_t>(in.u8());
    game.intro_seen = read_flag(in);
    if (read_flag(in)) {
        game.level_path = read_path(in);
        Player p;
        p.kind = static_cast<int8_t>(in.u8());
        p.character_class = static_cast<int8_t>(in.u8());
        p.experience = in.big_endian(3);
        p.level = static_cast<int8_t>(in.big_endian(2));
        for (int16_t *value : {&p.strength, &p.intelligence, &p.agility, &p.move_speed, &p.endurance, &p.willpower}) {
            *value = static_cast<int16_t>(in.big_endian(2));
        }
        p.equipped_weapon_id = static_cast<int8_t>(in.big_endian(2));
        p.engage_radius = static_cast<int16_t>(in.big_endian(2));
        p.disengage_radius = static_cast<int16_t>(in.big_endian(2));
        p.faction_id = static_cast<int8_t>(in.u8());
        p.gold = static_cast<uint16_t>(in.big_endian(2));
        p.sprite_path = read_path(in);
        const uint8_t count = in.u8();
        if (count > 254) throw std::runtime_error("Too many inventory entries in save");
        for (uint32_t i = 0; i < count; ++i) {
            const uint8_t flags = in.u8();
            const uint8_t kind = flags & 0x7f;
            if (kind > 2) throw std::runtime_error("Invalid inventory kind in save");
            p.inventory.push_back(Item{kind, in.u8(), (flags & 0x80) != 0});
        }
        game.player = std::move(p);
    }
    if (in.remaining() != 0) throw std::runtime_error("Unexpected trailing game-save data");
    return game;
}

}
