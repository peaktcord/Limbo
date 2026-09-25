#ifndef COMMON_SESSION_HPP
#define COMMON_SESSION_HPP

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "src/common/runtime.hpp"
#include "src/diagnostics/log.hpp"
#include "src/text/text.hpp"

class Displayable;
class Graphics;
class Image;
class PaintBackend;
namespace voyage::storage { class SaveStore; class ResourceStore; }
class Sprite;
class CrashReport;
class KeyBindings;
class ReferencePages;
class StringTable;
class UiFonts;
struct OblivionState;

class SessionArena {
    struct Entry { void *object; void (*destroy)(void *); };
    std::vector<Entry> objects_;
public:
    SessionArena() = default;
    SessionArena(const SessionArena &) = delete;
    SessionArena &operator=(const SessionArena &) = delete;
    ~SessionArena() { clear(); }
    template <typename T, typename... Args>
    T *make(Args &&...args) {
        T *object = new T(std::forward<Args>(args)...);
        objects_.push_back({object, [](void *p) { delete static_cast<T *>(p); }});
        return object;
    }
    void clear() {
        for (auto it = objects_.rbegin(); it != objects_.rend(); ++it) it->destroy(it->object);
        objects_.clear();
    }
    size_t size() const { return objects_.size(); }
};

struct Clock {
    bool pinned = false;
    int64_t millis = 0;

    void pin(int64_t at = 0) {
        pinned = true;
        millis = at;
    }
    void advance(int64_t elapsed) {
        if (pinned) millis += elapsed;
    }
    void release() { pinned = false; }
    int64_t now() const {
        if (pinned) return millis;
        return (int64_t)std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::system_clock::now().time_since_epoch())
            .count();
    }
};

struct SessionContext {
    voyage::storage::ResourceStore *resources = nullptr;
    voyage::storage::SaveStore *saves = nullptr;
    Clock clock;
    int32_t canvas_width = 176;
    int32_t canvas_height = 208;
    PaintBackend *paint = nullptr;
    Displayable *current_display = nullptr;
    std::vector<void *> font_cache;
    std::unordered_map<std::string, Image *> sprite_image_cache;
    std::unordered_map<std::string, Sprite *> mirrored_sprite_cache;
    GameRandom *random = nullptr;
    StringTable *string_table = nullptr;
    UiFonts *ui_fonts = nullptr;
    CrashReport *crash_report = nullptr;
    ReferencePages *reference_pages = nullptr;
    KeyBindings *key_bindings = nullptr;
    OblivionState *game = nullptr;
    SessionArena objects;

    void set_canvas_size(int32_t w, int32_t h) {
        canvas_width = w;
        canvas_height = h;
    }

    void reset() {
        current_display = nullptr;
        paint = nullptr;
        font_cache.clear();
        sprite_image_cache.clear();
        mirrored_sprite_cache.clear();
        random = nullptr;
        string_table = nullptr;
        ui_fonts = nullptr;
        crash_report = nullptr;
        reference_pages = nullptr;
        key_bindings = nullptr;
        game = nullptr;
        clock = Clock();
        canvas_width = 176;
        canvas_height = 208;
        resources = nullptr;
        saves = nullptr;
        objects.clear();
    }
};

inline SessionContext *&activeSessionSlot() {
    static thread_local SessionContext *active = nullptr;
    return active;
}
inline SessionContext &currentSession() {
    static SessionContext fallback;
    SessionContext *active = activeSessionSlot();
    return active == nullptr ? fallback : *active;
}
class SessionScope {
    SessionContext *previous_;
public:
    explicit SessionScope(SessionContext &session) : previous_(activeSessionSlot()) {
        activeSessionSlot() = &session;
    }
    ~SessionScope() { activeSessionSlot() = previous_; }
    SessionScope(const SessionScope &) = delete;
    SessionScope &operator=(const SessionScope &) = delete;
};
template <typename T, typename... Args>
T *session_new(Args &&...args) {
    return currentSession().objects.make<T>(std::forward<Args>(args)...);
}

#endif
