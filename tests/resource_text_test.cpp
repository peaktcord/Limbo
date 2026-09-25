#include "src/storage/resource_store.hpp"
#include "src/limbo/game_canvas.hpp"
#include "src/limbo/crash_report.hpp"
#include "src/limbo/game_data.hpp"
#include "src/limbo/key_bindings.hpp"
#include "src/limbo/statics.hpp"
#include "src/limbo/string_table.hpp"
#include "src/limbo/text_resources.hpp"

#include <cassert>
#include <climits>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>

namespace {
template <typename F>
void rejects_range(F operation) {
    bool rejected = false;
    try { operation(); } catch (const std::out_of_range &) { rejected = true; }
    assert(rejected);
}

class TextFiles : public voyage::storage::ResourceStore {
public:
    std::map<std::string, std::vector<uint8_t>> files{
        {"copywrite.txt", {'@', '2', '0', '0', '6', 0, 128, 255}},
        {"start.txt", {'A', 128, '|', '|', 'B', 255}},
    };
    mutable size_t reads = 0;
    std::optional<std::vector<uint8_t>> read(const std::string &path) const override {
        ++reads;
        const auto entry = files.find(path);
        return entry == files.end() ? std::nullopt : std::optional<std::vector<uint8_t>>(entry->second);
    }
};

class ClipGraphics : public Graphics {
public:
    bool has_test_clip() const { return clipX_ == 3 && clipY_ == 4 && clipW_ == 50 && clipH_ == 60; }
};

class FailedResources : public voyage::storage::ResourceStore {
public:
    bool invalid_data = false;
    std::optional<std::vector<uint8_t>> read(const std::string &) const override {
        if (invalid_data) throw std::out_of_range("controlled decoder failure");
        throw std::runtime_error("controlled resource read failure");
    }
};

std::string read_log(const std::filesystem::path &path) {
    std::ifstream file(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(file), {}};
}
}

int main() {
    SessionContext session;
    SessionScope scope(session);
    oblivion_init_statics();
    SharedArray<int8_t> bytes(256);
    SharedArray<uint16_t> codes(256);
    std::string expected;
    for (int i = 0; i < 256; ++i) {
        bytes[i] = static_cast<int8_t>(i);
        codes[i] = static_cast<uint16_t>(i);
        expected += static_cast<char>(i);
    }
    assert(array_text(bytes, 0, 256) == expected);
    assert(array_text(codes, 0, 256) == expected);
    assert(array_text(bytes, 127, 3) == std::string("\x7f\x80\x81", 3));
    assert(array_text(bytes, 256, 0).empty());
    rejects_range([&] { array_text(bytes, -1, 1); });
    rejects_range([&] { array_text(bytes, 0, -1); });
    rejects_range([&] { array_text(bytes, 255, 2); });
    rejects_range([&] { array_text(bytes, INT_MAX, 1); });
    rejects_range([&] { array_text(bytes, 1, INT_MAX); });
    rejects_range([] { array_text(SharedArray<int8_t>(), 0, 0); });

    GameData data(nullptr);
    data.script_bytecode = bytes;
    data.call_depth = 1;
    data.frame_pc[0] = 127;
    auto operand = data.read_script_text(3);
    assert(operand == std::string("\x7f\x80\x81", 3));
    assert(data.frame_pc[0] == 130);
    rejects_range([&] { data.read_script_text(127); });
    assert(data.frame_pc[0] == 130);
    bytes[128] = 'x';
    assert(operand == std::string("\x7f\x80\x81", 3));

    auto &table = current_string_table();
    table.language0_chars = SharedArray<uint16_t>{'A', 0x80, 0xff, '|', '|'};
    table.language0_offsets = SharedArray<int16_t>{-1, 0, 4, -1};
    table.language1_chars = SharedArray<uint16_t>{'B', 0xe9, '|'};
    table.language1_offsets = SharedArray<int16_t>{-1, 0, -1, 0};
    const std::string primary("A\x80\xff", 3);
    assert(table.get_text(1) == primary);
    assert(table.get_text(2).empty());
    assert(table.get_text(3) == std::string("B\xe9", 2));
    assert(table.get_text(-1).empty() && table.get_text(99).empty());
    assert(table.find_text_id(primary) == 1);
    assert(table.find_text_id("") == 2);
    assert(table.find_text_id("A") == -1);
    assert(table.find_text_id(primary + "x") == -1);
    data.string_pool[0] = primary;
    data.string_pool_count = 1;
    assert(data.get_string(0) == primary);
    assert(data.get_string(0xf001) == primary);
    assert(data.resolve_string_id(primary) == 0);

    TextFiles resources;
    session.resources = &resources;
    TextResources text;
    assert(text.load_copyright_text() == std::string("@2006\0\x80\xff", 8));
    assert(text.load_intro_segment(0) == std::string("A\x80", 2));
    assert(text.load_intro_segment(1).empty());
    assert(text.load_intro_segment(2) == std::string("B\xff", 2));
    assert(text.load_intro_segment(3).empty());
    assert(text.load_intro_segment(-1).empty());
    (void)crash_report();
    const auto arena_size = session.objects.size();
    for (int i = 0; i < 20; ++i) {
        assert(text.load_copyright_text() == std::string("@2006\0\x80\xff", 8));
        assert(text.load_intro_segment(2) == std::string("B\xff", 2));
    }
    assert(session.objects.size() == arena_size);

    auto &language = resources.files["lang_0.txt"];
    for (int i = 0; i < 256; ++i) language.push_back(static_cast<uint8_t>(i));
    const auto reads = resources.reads;
    const auto characters = text.load_language(0);
    assert(resources.reads == reads + 1);
    assert(characters.length() == 257 && characters[256] == 0);
    for (int i = 0; i < 256; ++i) assert(characters[i] == i);
    assert(TextResources::S().default_language_loaded);
    assert(session.objects.size() == arena_size);

    resources.files["large.scr"] = std::vector<uint8_t>(6500, 255);
    assert(table.load_resource("/large.scr") == 6144);
    assert(table.resource_buffer.length() == 6144);
    for (int i = 0; i < 6144; ++i) assert(table.resource_buffer[i] == -1);
    resources.files["short.scr"] = {0, 128, 255};
    assert(table.load_resource("/short.scr") == 3);
    assert(table.resource_buffer[0] == 0 && table.resource_buffer[1] == -128 && table.resource_buffer[2] == -1);
    assert(session.objects.size() == arena_size);

    const std::vector<uint8_t> png_header{137, 'P', 'N', 'G', 13, 10, 26, 10,
        0, 0, 0, 13, 'I', 'H', 'D', 'R', 0, 0, 0, 2, 0, 0, 0, 3};
    resources.files["image.png"] = png_header;
    Image *image = Image::createImage("/image.png");
    resources.files.erase("image.png");
    assert(image->encodedBytes() == png_header);
    assert(image->getWidth() == 2 && image->getHeight() == 3);
    assert(session.objects.size() == arena_size + 1);

    resources.files["copywrite.txt"].clear();
    resources.files["start.txt"].clear();
    resources.files["lang_1.txt"] = {};
    assert(text.load_copyright_text().empty());
    assert(text.load_intro_segment(0).empty());
    const auto empty_language = text.load_language(1);
    assert(empty_language.length() == 1 && empty_language[0] == 0);
    assert(table.load_resource("/start.txt") == 0);
    assert(characters.length() == 257 && characters[255] == 255);

    const char *temp = std::getenv("TEST_TMPDIR");
    assert(temp != nullptr);
    const auto log = std::filesystem::path(temp) / "resource-exceptions.log";
    voyage::logging::open_file(log);
    FailedResources failed;
    session.resources = &failed;
    assert(text.load_intro_segment(2).empty());
    assert(text.load_copyright_text().empty());
    TextResources::S().default_language_loaded = false;
    assert(text.load_language(0).sameRef(empty_language));
    assert(!TextResources::S().default_language_loaded);
    assert(table.load_resource("/controlled.scr") == 0);
    auto report = read_log(log);
    assert(report.find("Loading /copywrite.txt: controlled resource read failure") != std::string::npos);
    assert(report.find("Loading language file /lang_0.txt: controlled resource read failure") != std::string::npos);
    assert(table.resource_buffer[0] == 0 && table.resource_buffer[1] == -128 && table.resource_buffer[2] == -1);
    assert(report.find("Loading /start.txt segment 2: controlled resource read failure") != std::string::npos);
    assert(report.find("Reading resource /controlled.scr: controlled resource read failure") != std::string::npos);
    failed.invalid_data = true;
    rejects_range([&] { text.load_intro_segment(2); });
    assert(table.load_resource("/controlled.scr") == 0);
    assert(read_log(log).find("Reading resource /controlled.scr: controlled decoder failure") != std::string::npos);
    session.resources = nullptr;
    bool missing_rejected = false;
    try { text.load_intro_segment(2); }
    catch (const std::logic_error &error) {
        missing_rejected = true;
        assert(std::string(error.what()) == "Resource not found: /start.txt");
    }
    assert(missing_rejected);
    missing_rejected = false;
    try { text.load_language(0); }
    catch (const std::logic_error &error) {
        missing_rejected = true;
        assert(std::string(error.what()) == "Resource not found: /lang_0.txt");
    }
    assert(missing_rejected);
    missing_rejected = false;
    try { text.load_copyright_text(); }
    catch (const std::logic_error &error) {
        missing_rejected = true;
        assert(std::string(error.what()) == "Resource not found: /copywrite.txt");
    }
    assert(missing_rejected);
    assert(table.load_resource("/missing.scr") == 0);
    assert(read_log(log).find("Reading resource /missing.scr: Resource not found: /missing.scr") != std::string::npos);

    SpriteFrame frame;
    frame.image_path = "/missing.png";
    frame.frame_id = 1;
    frame.width = frame.height = 16;
    frame.current_frame = &frame;
    ClipGraphics graphics;
    graphics.setClip(3, 4, 50, 60);
    assert(SpriteAtlas::draw_frame(&graphics, &frame, 1, 0, 0) == 0);
    frame.is_subframe = frame.mirror_horizontal = 1;
    assert(SpriteAtlas::draw_frame(&graphics, &frame, 1, 0, 0) == 0);
    assert(graphics.has_test_clip());
    voyage::logging::close_file();
    session.resources = &resources;

    GameCanvas::set_status_message(std::nullopt, 0, 0, 0);
    assert(!GameCanvas::state().status_text);
    GameCanvas::set_status_message("", 1, 0, 0);
    assert(GameCanvas::state().status_text && GameCanvas::state().status_text->empty());
    std::vector<std::string> lines;
    GameCanvas::set_speaker_name(std::nullopt);
    GameCanvas::format_wrapped_text("", lines, 100);
    assert(lines.empty());
    GameCanvas::format_wrapped_text("Hi", lines, 100);
    assert(lines == std::vector<std::string>{"Hi"});
    GameCanvas::set_speaker_name("");
    GameCanvas::format_wrapped_text("Hi", lines, 100);
    assert(lines == std::vector<std::string>{": Hi"});
    auto &keys = key_bindings_state();
    keys.names[8] = "ACTION";
    keys.names[keys.codes[0]] = "7";
    keys.names[keys.codes[1]] = "";
    keys.names[keys.codes[2]] = std::nullopt;
    assert(keys.substitute("ACTION_KEY QUICK_HEALTH_KEY QUICK_MAGIKA_KEY TOGGLE_WEAPON_KEY") == "ACTION 7  null");
    assert(keys.substitute("No placeholders") == "No placeholders");
    return 0;
}
