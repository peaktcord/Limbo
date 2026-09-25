#include "src/game/game_runtime.hpp"
#include "src/storage/save_store.hpp"
#include "src/limbo/game_canvas.hpp"
#include "src/limbo/menu_screen.hpp"
#include "src/limbo/key_bindings.hpp"
#include "src/limbo/string_table.hpp"

#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>

namespace {
std::string read(const std::filesystem::path &path) {
    std::ifstream file(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(file), {}};
}

class FailedSaves final : public voyage::storage::SaveStore {
public:
    std::optional<voyage::storage::SaveRecords> load(const std::string &) override { throw std::runtime_error("controlled read failure"); }
    void save(const std::string &, const voyage::storage::SaveRecords &) override { throw std::runtime_error("controlled write failure"); }
};
class FailedSpriteResources final : public voyage::storage::ResourceStore {
public:
    std::optional<std::vector<uint8_t>> read(const std::string &path) const override {
        if (path == "controlled.cml") {
            return std::vector<uint8_t>{
                0, 1, 12, '/', 'm', 'i', 's', 's', 'i', 'n', 'g', '.', 'p', 'n', 'g', 0, 0, 0, 0};
        }
        throw std::runtime_error("controlled sprite read failure");
    }
};
}

int main(int argc, char **argv) {
    assert(argc == 2);
    const char *temp = std::getenv("TEST_TMPDIR");
    assert(temp != nullptr);
    const auto root = std::filesystem::path(temp) / "runtime-logging";
    const auto log = root / "limbo.log";
    voyage::logging::open_file(log);
    voyage::RuntimeConfig config;
    config.resource_dir = argv[1];
    config.save_dir = root.string();
    config.random_seed = 42;
    {
        voyage::GameRuntime game(config);
        game.start();
        assert(!game.has_error());
        SessionScope scope(game.session());
        GameCanvas *canvas = GameCanvas::state().instance;

        assert(!canvas->has_saved_player());
        assert(read(log).find("[ERROR]") == std::string::npos);

        FailedSaves failed;
        auto *original = game.session().saves;
        game.session().saves = &failed;
        assert(!canvas->save_game());
        assert(canvas->persistence_error == "Save failed");
        assert(!canvas->has_saved_player());
        assert(!canvas->load_game(false));
        assert(canvas->persistence_error == "Load failed");
        game.session().saves = original;
        std::string report = read(log);
        assert(report.find("Writing ESO: controlled write failure") != std::string::npos);
        assert(report.find("Checking ESO for a saved player: controlled read failure") != std::string::npos);
        assert(report.find("Restoring ESO: controlled read failure") != std::string::npos);

        const auto previous_key = key_bindings_state().codes[0];
        const bool previous_intro = canvas->var_boolean_o;
        const auto previous_state = game.state();
        original->save("ESO", {{1,2,3,1,1}});
        assert(!canvas->load_game(true));
        assert(!canvas->has_saved_player());
        assert(key_bindings_state().codes[0] == previous_key);
        assert(canvas->var_boolean_o == previous_intro);
        assert(game.state() == previous_state);
        assert(read(log).find("Save data truncated") != std::string::npos);

        FailedSpriteResources failed_images;
        auto *original_resources = game.session().resources;
        game.session().resources = &failed_images;
        auto *frame = SpriteAtlas::load_atlas("/controlled.cml");
        game.session().resources = original_resources;
        assert(frame != nullptr && frame->width == 0 && frame->height == 0);
        assert(SpriteAtlas::draw_frame(canvas->scene_graphics, frame, 1, 0, 0) == 0);
        assert(read(log).find("Loading sprite image /missing.png: controlled sprite read failure") != std::string::npos);

        auto *script = GameCanvas::state().game_data;
        script->call_depth = 1;
        script->frame_pc[0] = 0;
        script->script_bytecode = SharedArray<int8_t>{8, 3, '/', 'x'};
        script->execute_script_step(0);
        assert(read(log).find("Loading the scripted level: Invalid resource text byte range") != std::string::npos);

        GameCanvas::state().game_state = 0;
        GameCanvas::state().level_script_path = std::string("/controlled.scr");
        canvas->menu_screen->is_open = 0;
        script->call_depth = 1;
        script->frame_pc[0] = 12345;
        script->frame_script_id[0] = 73;
        script->script_bytecode = SharedArray<int8_t>(1);
        game.press(voyage::Input::Left);
        game.tick(50);
        assert(game.has_error());
        report = read(log);
        assert(report.find("[ERROR] Game loop:") != std::string::npos);
        assert(report.find("phase=script interpreter") != std::string::npos);
        assert(report.find("level=/controlled.scr") != std::string::npos);
        assert(report.find("script_frames=[73@12345]") != std::string::npos);
        assert(report.find("opcode_pc=12345") != std::string::npos);
        assert(report.find(" input=" + std::to_string((int)voyage::Input::Left)) != std::string::npos);
        for (int i = 0; i < 100; ++i) game.tick(50);
        assert(read(log) == report);
        assert(report.find("moveInWorld") == std::string::npos);
    }

    config.save_dir = (root / "merchant").string();
    for (int label : {36, 37}) {
        for (const std::string price : {"invalid", "2147483648", "missing-delimiters"}) {
            voyage::GameRuntime game(config);
            game.start();
            assert(!game.has_error());
            SessionScope scope(game.session());
            auto *canvas = GameCanvas::state().instance;
            current_string_table().load_language(0);
            assert(!GameCanvas::get_text(36).empty() && GameCanvas::get_text(36) != GameCanvas::get_text(37));
            canvas->player = session_new<Actor>();
            canvas->player->inventory_entries = SharedArray<int32_t>{0x1234, 0x5678, 0};
            auto *parent = session_new<MenuNode>(GameCanvas::get_text(label), std::nullopt, false);
            const std::string row = price == "missing-delimiters" ? price : "Potion : " + price + " gold";
            auto *item = session_new<MenuNode>(row, std::nullopt, false);
            item->parent = parent;
            parent->children.push_back(item);
            auto *menu = canvas->menu_screen;
            menu->tab_roots = SharedArray<MenuNode *>{parent};
            menu->tab_index = menu->selected_index = 0;
            menu->is_open = 1;
            GameCanvas::state().game_state = 1;
            GameCanvas::state().player_gold = 123;
            canvas->paused = false;
            const auto *player = canvas->player;
            game.press(voyage::Input::Action);
            game.tick(50);
            assert(game.running() && game.has_error());
            assert(GameCanvas::state().player_gold == 123);
            assert(canvas->player == player && parent->children == std::vector<MenuNode *>{item});
            const std::string operation = label == 36 ? "purchase" : "sale";
            const auto report = read(log);
            assert(report.find("Parsing " + operation + " price in row: " + row) != std::string::npos);
            const std::string expected_error = price == "invalid" ? "Invalid decimal integer: invalid" :
                price == "2147483648" ? "Integer outside signed 32-bit range: 2147483648" : "Invalid resource text range";
            assert(game.error_message() == "Game loop failure | " + expected_error);
            assert(player->inventory_entries.length() == 3);
            assert(player->inventory_entries[0] == 0x1234 && player->inventory_entries[1] == 0x5678 && player->inventory_entries[2] == 0);
            game.tick(50);
            assert(read(log) == report);
        }
    }
    voyage::logging::close_file();
}
