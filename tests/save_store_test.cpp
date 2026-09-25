#include "src/storage/save_store.hpp"

#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iterator>

namespace {
using namespace voyage::storage;
template <typename F> bool fails(F action) {
    try { action(); } catch (const std::exception &) { return true; }
    return false;
}
Bytes read(const std::filesystem::path &path) {
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), {}};
}
void write(const std::filesystem::path &path, const Bytes &bytes) {
    std::ofstream output(path, std::ios::binary);
    output.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
}
}

int main() {
    namespace fs = std::filesystem;
    const char *temp = std::getenv("TEST_TMPDIR");
    assert(temp != nullptr);
    const auto root = fs::path(temp) / "native-save-store";
    int flushes = 0;
    DirectorySaveStore store(root, [&] { ++flushes; });
    assert(!store.load("missing"));
    assert(flushes == 0);
    store.save("ESO", {{1, 2, 3}, {4, 5}});
    const Bytes expected{2,0,0,0, 3,0,0,0, 1,2,3, 2,0,0,0, 4,5};
    assert(read(root / "ESO.rs") == expected);
    assert(*store.load("ESO") == (SaveRecords{{1,2,3}, {4,5}}));
    assert(flushes == 1);
    store.save("ESO", {{9}});
    assert(*store.load("ESO") == (SaveRecords{{9}}));
    assert(flushes == 2 && !fs::exists(root / "ESO.rs.tmp"));

    fs::create_directory(root / "ESO.rs.tmp");
    assert(fails([&] { store.save("ESO", {{7,8}}); }));
    assert(*store.load("ESO") == (SaveRecords{{9}}));
    assert(flushes == 2 && fs::is_directory(root / "ESO.rs.tmp"));
    fs::remove(root / "ESO.rs.tmp");
    fs::create_directory(root / "blocked.rs");
    assert(fails([&] { store.save("blocked", {{7}}); }));
    assert(fs::is_directory(root / "blocked.rs") && !fs::exists(root / "blocked.rs.tmp"));
    assert(flushes == 2);

    for (const Bytes bad : {Bytes{}, Bytes{1}, Bytes{1,0,0,0},
         Bytes{1,0,0,0, 3,0,0,0, 1,2}, Bytes{255,255,255,255},
         Bytes{1,0,0,0, 255,255,255,255}, Bytes{0,0,0,0,1}}) {
        write(root / "broken.rs", bad);
        assert(fails([&] { (void)store.load("broken"); }));
    }
    store.save("broken", {{3,2,1}});
    assert(*store.load("broken") == (SaveRecords{{3,2,1}}));
    store.save("empty", {});
    assert(store.load("empty") && store.load("empty")->empty());
    assert(fails([&] { store.save("../escape", {}); }));
    assert(fails([&] { store.load("C:\\escape"); }));
    return 0;
}
