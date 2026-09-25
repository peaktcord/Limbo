#include "src/common/session.hpp"
#include "src/common/ui.hpp"

#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

namespace {

class CountingBackend final : public PaintBackend {
public:
    Graphics screen;
    int begins = 0;
    int ends = 0;

    Graphics *screenGraphics() override { return &screen; }
    Graphics *imageGraphics(Image *) override { return session_new<Graphics>(); }
    void beginPaint() override { ++begins; }
    void endPaint() override { ++ends; }
};

class TestCanvas final : public Canvas {
public:
    int paints = 0;
    int shows = 0;
    int hides = 0;
    bool throw_from_paint = false;
    Graphics *last_graphics = nullptr;

    void paint(Graphics *graphics) override {
        ++paints;
        last_graphics = graphics;
        if (throw_from_paint) throw std::runtime_error("paint failed");
    }
    void showNotify() override { ++shows; }
    void hideNotify() override { ++hides; }
};

template <typename Exception, typename Function>
bool throws(Function function) {
    try {
        function();
    } catch (const Exception &) {
        return true;
    }
    return false;
}

bool unwound = false;
std::vector<std::string> throw_sites;
void record_throw(const char *message) {
    assert(!unwound);
    throw_sites.emplace_back(message);
}
struct UnwindGuard {
    ~UnwindGuard() { unwound = true; }
};

}

int main() {
    SessionContext session;
    SessionScope scope(session);

    SharedArray<int8_t> null_array;
    assert(throws<std::out_of_range>([&]() { (void)null_array[0]; }));
    assert(throws<std::out_of_range>([&]() { (void)null_array.length(); }));
    assert(throws<std::length_error>([]() { SharedArray<int8_t> invalid(-1); }));
    SharedArray<int8_t> bytes(4);
    assert(bytes[0] == 0 && bytes[3] == 0);
    assert(throws<std::out_of_range>([&]() { (void)bytes[-1]; }));
    assert(throws<std::out_of_range>([&]() { (void)bytes[4]; }));
    bytes[0] = (int8_t)0x80;
    assert(bytes[0] == -128);
    SharedArray<int8_t> alias = bytes;
    alias[1] = 12;
    assert(bytes[1] == 12 && alias.sameRef(bytes));

    SharedArray<int8_t> overlap{1, 2, 3, 4};
    copySharedArray(overlap, 0, overlap, 1, 3);
    assert(overlap[0] == 1 && overlap[1] == 1 && overlap[2] == 2 && overlap[3] == 3);

    assert(shiftLeft32(1, 32) == 1);
    assert(shiftLeft32(1, -1) == (int32_t)0x80000000u);
    assert(unsignedShiftRight32(-2, 1) == 0x7FFFFFFF);
    assert(unsignedShiftRight64(-2LL, 1) == 0x7FFFFFFFFFFFFFFFLL);
    assert((-2 >> 1) == -1);
    assert(wrappingAbs(-5) == 5);
    assert(wrappingAbs((int32_t)0x80000000u) == (int32_t)0x80000000u);
    assert(min32(3, 7) == 3);
    assert(max32(3, 7) == 7);

    GameRandom random(0);
    assert(random.nextInt() == -1155484576);
    assert(random.nextInt() == -723955400);
    assert(random.nextInt() == 1033096058);

    voyage::logging::set_throw_trace_hook(record_throw);
    try {
        UnwindGuard guard;
        bytes[17] = 99;
        assert(false);
    } catch (const std::out_of_range &error) {
        assert(unwound);
        assert(std::string(error.what()) == "Array index 17 out of bounds for length 4");
    }
    assert(throw_sites == std::vector<std::string>{"Array index 17 out of bounds for length 4"});
    assert(bytes[0] == -128 && bytes[1] == 12 && bytes[2] == 0 && bytes[3] == 0);
    unwound = false;

    CountingBackend backend;
    session.paint = &backend;
    TestCanvas first_canvas;
    TestCanvas second_canvas;
    first_canvas.repaint();
    first_canvas.serviceRepaints();
    assert(first_canvas.paints == 0 && backend.begins == 0 && backend.ends == 0);

    Display::getDisplay()->setCurrent(&first_canvas);
    assert(first_canvas.shows == 1 && first_canvas.hides == 0);
    first_canvas.serviceRepaints();
    assert(first_canvas.paints == 1 && first_canvas.last_graphics == &backend.screen);
    assert(backend.begins == 1 && backend.ends == 1);
    first_canvas.serviceRepaints();
    assert(first_canvas.paints == 1);

    first_canvas.repaint();
    first_canvas.repaint();
    first_canvas.serviceRepaints();
    assert(first_canvas.paints == 2 && backend.begins == 2 && backend.ends == 2);
    Display::getDisplay()->setCurrent(&first_canvas);
    assert(first_canvas.shows == 1 && first_canvas.hides == 0);
    Display::getDisplay()->setCurrent(&second_canvas);
    assert(first_canvas.hides == 1 && second_canvas.shows == 1);

    second_canvas.throw_from_paint = true;
    second_canvas.repaint();
    assert(throws<std::runtime_error>([&]() { second_canvas.serviceRepaints(); }));
    assert(second_canvas.paints == 1 && backend.begins == 3 && backend.ends == 3);
    second_canvas.serviceRepaints();
    assert(second_canvas.paints == 1);
    Display::getDisplay()->setCurrent(nullptr);
    assert(second_canvas.hides == 1);
    session.paint = nullptr;

    assert(throws<std::runtime_error>([]() { Image::createImage("/missing.png"); }));
    assert(throw_sites.back() == "Image resource not found: /missing.png");
    voyage::logging::set_throw_trace_hook(nullptr);

    return 0;
}
