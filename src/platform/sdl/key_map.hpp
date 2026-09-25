#ifndef VOYAGE_PLATFORM_SDL_KEY_MAP_HPP
#define VOYAGE_PLATFORM_SDL_KEY_MAP_HPP

#include <SDL3/SDL.h>

#include <optional>

#include "src/game/game_runtime.hpp"

namespace voyage::platform {

inline std::optional<Input> map_sdl_key(SDL_Keycode key) {
    switch (key) {
        case SDLK_UP:
        case SDLK_W: return Input::Up;
        case SDLK_DOWN:
        case SDLK_S: return Input::Down;
        case SDLK_LEFT:
        case SDLK_A: return Input::Left;
        case SDLK_RIGHT:
        case SDLK_D: return Input::Right;
        case SDLK_SPACE:
        case SDLK_RETURN:
        case SDLK_KP_ENTER:
        case SDLK_E:
        case SDLK_F: return Input::Action;
        case SDLK_ESCAPE:
        case SDLK_BACKSPACE:
        case SDLK_P: return Input::Menu;
        case SDLK_TAB:
        case SDLK_C:
        case SDLK_I: return Input::Character;
        case SDLK_1:
        case SDLK_H: return Input::QuickHealth;
        case SDLK_2:
        case SDLK_M:
        case SDLK_R: return Input::QuickMagicka;
        case SDLK_Q:
        case SDLK_X: return Input::ToggleWeapon;
        case SDLK_0: return Input::Digit0;
        case SDLK_3: return Input::Digit3;
        case SDLK_4: return Input::Digit4;
        case SDLK_5: return Input::Digit5;
        case SDLK_6: return Input::Digit6;
        case SDLK_7: return Input::Digit7;
        case SDLK_8: return Input::Digit8;
        case SDLK_9: return Input::Digit9;
        case SDLK_KP_0: return Input::Digit0;
        case SDLK_KP_1: return Input::Digit1;
        case SDLK_KP_2: return Input::Digit2;
        case SDLK_KP_3: return Input::Digit3;
        case SDLK_KP_4: return Input::Digit4;
        case SDLK_KP_5: return Input::Digit5;
        case SDLK_KP_6: return Input::Digit6;
        case SDLK_KP_7: return Input::Digit7;
        case SDLK_KP_8: return Input::Digit8;
        case SDLK_KP_9: return Input::Digit9;
        default: return std::nullopt;
    }
}

inline std::optional<Input> map_sdl_mouse_button(Uint8 button) {
    switch (button) {
        case SDL_BUTTON_LEFT: return Input::Action;
        case SDL_BUTTON_RIGHT: return Input::Menu;
        case SDL_BUTTON_MIDDLE: return Input::ToggleWeapon;
        case SDL_BUTTON_X1: return Input::QuickHealth;
        case SDL_BUTTON_X2: return Input::QuickMagicka;
        default: return std::nullopt;
    }
}

}

#endif
