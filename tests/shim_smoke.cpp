#include "src/common/session.hpp"
#include "src/storage/resource_store.hpp"
#include "src/storage/save_store.hpp"
#include "src/common/render/render.hpp"
#include <cassert>
#include <cstdlib>
#include <filesystem>

struct CountedObject {
    static int live;
    CountedObject() { ++live; }
    ~CountedObject() { --live; }
};
int CountedObject::live = 0;

int main() {
    SharedArray<int8_t> a(3);
    SharedArray<int8_t> alias = a;
    assert(alias.sameRef(a));
    GameRandom r;
    (void)r.nextInt();
    currentSession().set_canvas_size(240, 320);
    assert(currentSession().canvas_width == 240);
    render::Surface surf(8, 8);
    render::SoftGraphics g(&surf);
    g.setColor(0xFF0000);
    g.fillRect(0, 0, 4, 4);
    assert(surf.row(0)[0] == 0xFFFF0000u);

    SessionContext first;
    SessionContext second;
    voyage::storage::DirectoryResourceStore first_resources("first-resources");
    voyage::storage::DirectorySaveStore first_saves("first-saves");
    {
        SessionScope scope(first);
        first.resources = &first_resources;
        first.saves = &first_saves;
        first.set_canvas_size(111, 222);
        currentSession().clock.pin(40);
        session_new<CountedObject>();
        assert(CountedObject::live == 1);
    }
    {
        SessionScope scope(second);
        assert(currentSession().resources == nullptr);
        assert(currentSession().saves == nullptr);
        assert(!voyage::storage::read_resource(currentSession().resources, "/anything"));
        assert(currentSession().canvas_width == 176);
        assert(!currentSession().clock.pinned);
        session_new<CountedObject>();
        assert(CountedObject::live == 2);
    }
    second.reset();
    assert(CountedObject::live == 1);
    assert(second.resources == nullptr);
    {
        SessionScope scope(first);
        assert(currentSession().resources == &first_resources);
        assert(currentSession().saves == &first_saves);
        assert(currentSession().canvas_width == 111);
        assert(currentSession().clock.now() == 40);
    }
    const char *test_tmp = std::getenv("TEST_TMPDIR");
    assert(test_tmp != nullptr);
    voyage::storage::DirectorySaveStore saves((std::filesystem::path(test_tmp) / "rms").string());
    currentSession().saves = &saves;
    assert(!saves.load("nope"));
    saves.save("ESO", {{9, 0}});
    assert(saves.load("ESO")->front()[0] == 9);
    currentSession().saves = nullptr;
    first.objects.clear();
    assert(CountedObject::live == 0);
    return 0;
}
