#ifndef VOYAGE_PLATFORM_ADAPTER_POLICY_HPP
#define VOYAGE_PLATFORM_ADAPTER_POLICY_HPP

#include <algorithm>
#include <cstdint>
#include <limits>

namespace voyage::platform {

enum class PresentationMode {
    IntegerScale,
    Letterbox,
};

struct Presentation {
    int32_t logical_width;
    int32_t logical_height;
    int32_t window_width;
    int32_t window_height;
    PresentationMode mode;
    bool resize_window;
};

inline Presentation presentation(int32_t logical_width, int32_t logical_height,
                                 int scale, bool fullscreen) {
    const int safe_scale = std::max(scale, 1);
    return {
        logical_width,
        logical_height,
        logical_width * safe_scale,
        logical_height * safe_scale,
        fullscreen ? PresentationMode::Letterbox : PresentationMode::IntegerScale,
        !fullscreen,
    };
}

enum class StartupSource {
    ResourceOverride,
    ExplicitJar,
    PrimaryCache,
    LegacyCache,
    IntakeScreen,
};

struct StartupFacts {
    bool has_resource_override = false;
    bool has_explicit_jar = false;
    bool has_cache_override = false;
    bool primary_cache_valid = false;
    bool legacy_cache_available = false;
    bool legacy_cache_valid = false;
};

inline StartupSource select_startup_source(const StartupFacts &facts) {
    if (facts.has_resource_override) return StartupSource::ResourceOverride;
    if (facts.has_explicit_jar) return StartupSource::ExplicitJar;
    if (facts.primary_cache_valid) return StartupSource::PrimaryCache;
    if (!facts.has_cache_override && facts.legacy_cache_available && facts.legacy_cache_valid) {
        return StartupSource::LegacyCache;
    }
    return StartupSource::IntakeScreen;
}

class FrameClock {
public:
    FrameClock() = default;
    explicit FrameClock(uint64_t now) : previous_(now) {}

    void reset(uint64_t now) { previous_ = now; }

    uint32_t advance(uint64_t now) {
        const uint64_t elapsed = now >= previous_ ? now - previous_ : 0;
        previous_ = now;
        return static_cast<uint32_t>(
            std::min<uint64_t>(elapsed, std::numeric_limits<uint32_t>::max()));
    }

private:
    uint64_t previous_ = 0;
};

}

#endif
