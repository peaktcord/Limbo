#include <SDL3/SDL.h>

#include <cassert>
#include <cstdint>
#include <limits>

#include "src/game/game_runtime.hpp"
#include "src/platform/adapter_policy.hpp"
#include "src/platform/held_input.hpp"
#include "src/platform/sdl/key_map.hpp"

int main() {
    using voyage::Input;
    using namespace voyage::platform;

    const struct {
        SDL_Keycode key;
        Input input;
    } mappings[] = {
        {SDLK_UP, Input::Up},          {SDLK_W, Input::Up},
        {SDLK_DOWN, Input::Down},      {SDLK_S, Input::Down},
        {SDLK_LEFT, Input::Left},      {SDLK_A, Input::Left},
        {SDLK_RIGHT, Input::Right},    {SDLK_D, Input::Right},
        {SDLK_SPACE, Input::Action},   {SDLK_RETURN, Input::Action},
        {SDLK_KP_ENTER, Input::Action},{SDLK_E, Input::Action},
        {SDLK_F, Input::Action},       {SDLK_ESCAPE, Input::Menu},
        {SDLK_BACKSPACE, Input::Menu}, {SDLK_P, Input::Menu},
        {SDLK_TAB, Input::Character},  {SDLK_C, Input::Character},
        {SDLK_I, Input::Character},    {SDLK_1, Input::QuickHealth},
        {SDLK_H, Input::QuickHealth},  {SDLK_2, Input::QuickMagicka},
        {SDLK_M, Input::QuickMagicka}, {SDLK_R, Input::QuickMagicka},
        {SDLK_Q, Input::ToggleWeapon}, {SDLK_X, Input::ToggleWeapon},
        {SDLK_0, Input::Digit0},       {SDLK_3, Input::Digit3},
        {SDLK_4, Input::Digit4},       {SDLK_5, Input::Digit5},
        {SDLK_6, Input::Digit6},       {SDLK_7, Input::Digit7},
        {SDLK_8, Input::Digit8},       {SDLK_9, Input::Digit9},
        {SDLK_KP_0, Input::Digit0},    {SDLK_KP_1, Input::Digit1},
        {SDLK_KP_2, Input::Digit2},    {SDLK_KP_3, Input::Digit3},
        {SDLK_KP_4, Input::Digit4},    {SDLK_KP_5, Input::Digit5},
        {SDLK_KP_6, Input::Digit6},    {SDLK_KP_7, Input::Digit7},
        {SDLK_KP_8, Input::Digit8},    {SDLK_KP_9, Input::Digit9},
    };
    for (const auto &mapping : mappings) assert(map_sdl_key(mapping.key) == mapping.input);
    assert(!map_sdl_key(SDLK_F1));
    assert(map_sdl_mouse_button(SDL_BUTTON_LEFT) == Input::Action);
    assert(map_sdl_mouse_button(SDL_BUTTON_RIGHT) == Input::Menu);
    assert(map_sdl_mouse_button(SDL_BUTTON_MIDDLE) == Input::ToggleWeapon);
    assert(map_sdl_mouse_button(SDL_BUTTON_X1) == Input::QuickHealth);
    assert(map_sdl_mouse_button(SDL_BUTTON_X2) == Input::QuickMagicka);
    assert(!map_sdl_mouse_button(0));

    HeldInput held;
    assert(!held.repeat(0));
    held.press(Input::Action);
    assert(!held.repeat(0));
    held.press(Input::Left);
    assert(held.repeat(0) == Input::Left);
    assert(!held.repeat(3));
    held.release(Input::Right);
    assert(held.repeat(0) == Input::Left);
    held.press(Input::Up);
    assert(held.repeat(0) == Input::Up);
    held.release(Input::Up);
    assert(!held.repeat(0));

    HeldInput modern_held;
    modern_held.press(Input::Up);
    assert(modern_held.repeat(0, true) == Input::Up);
    modern_held.press(Input::Left);
    assert(modern_held.repeat(0, true) == Input::UpLeft);
    modern_held.release(Input::Up);
    assert(modern_held.repeat(0, true) == Input::Left);
    modern_held.press(Input::Right);
    assert(!modern_held.repeat(0, true));
    modern_held.press(Input::Down);
    assert(modern_held.repeat(0, true) == Input::Down);
    modern_held.release(Input::Left);
    assert(modern_held.repeat(0, true) == Input::DownRight);
    modern_held.release(Input::Right);
    modern_held.release(Input::Down);
    assert(!modern_held.repeat(0, true));

    Presentation phone = presentation(voyage::kPhoneWidth, voyage::kScreenHeight, 3, false);
    assert(phone.logical_width == voyage::kPhoneWidth);
    assert(phone.window_width == voyage::kPhoneWidth * 3);
    assert(phone.window_height == voyage::kScreenHeight * 3);
    assert(phone.mode == PresentationMode::IntegerScale && phone.resize_window);
    Presentation wide = presentation(voyage::kWideWidth, voyage::kScreenHeight, 0, false);
    assert(wide.window_width == voyage::kWideWidth);
    Presentation full = presentation(voyage::kWideWidth, voyage::kScreenHeight, 3, true);
    assert(full.mode == PresentationMode::Letterbox && !full.resize_window);

    StartupFacts facts;
    facts.has_resource_override = true;
    facts.has_explicit_jar = true;
    assert(select_startup_source(facts) == StartupSource::ResourceOverride);
    facts.has_resource_override = false;
    assert(select_startup_source(facts) == StartupSource::ExplicitJar);
    facts.has_explicit_jar = false;
    facts.primary_cache_valid = true;
    assert(select_startup_source(facts) == StartupSource::PrimaryCache);
    facts.primary_cache_valid = false;
    facts.legacy_cache_available = true;
    facts.legacy_cache_valid = true;
    assert(select_startup_source(facts) == StartupSource::LegacyCache);
    facts.primary_cache_valid = true;
    assert(select_startup_source(facts) == StartupSource::PrimaryCache);
    facts.primary_cache_valid = false;
    facts.has_cache_override = true;
    assert(select_startup_source(facts) == StartupSource::IntakeScreen);

    FrameClock clock(100);
    assert(clock.advance(116) == 16);
    assert(clock.advance(116) == 0);
    assert(clock.advance(90) == 0);
    clock.reset(0);
    assert(clock.advance(static_cast<uint64_t>(std::numeric_limits<uint32_t>::max()) + 10) ==
           std::numeric_limits<uint32_t>::max());
    return 0;
}
