#include "src/game/input_mapper.hpp"

#include "src/limbo/key_bindings.hpp"

namespace voyage {

int32_t midp_key_code(Input input, int32_t game_state, const KeyBindings &bindings) {
    switch (input) {
        case Input::Up: return -1;
        case Input::Down: return -2;
        case Input::Left: return -3;
        case Input::Right: return -4;
        case Input::UpLeft: return -11;
        case Input::UpRight: return -12;
        case Input::DownLeft: return -13;
        case Input::DownRight: return -14;
        case Input::Action: {
            switch (game_state) {
                case 11:
                case 14:
                case 16:
                case 19:
                case 22:
                    return 22;
                case 4:
                case 9:
                case 10:
                case 13:
                case 17:
                case 18:
                case 20:
                case 21:
                case 23:
                    return 21;
                default:
                    return -5;
            }
        }
        case Input::Menu: return 21;
        case Input::Character: return 22;
        case Input::QuickHealth:
        case Input::QuickMagicka:
        case Input::ToggleWeapon: {
            int32_t slot = input == Input::QuickHealth ? 0 : (input == Input::QuickMagicka ? 1 : 2);
            if (bindings.codes.isNull() || slot >= bindings.codes.length()) return 0;
            return bindings.codes[slot];
        }
        case Input::Digit0: return 48;
        case Input::Digit1: return 49;
        case Input::Digit2: return 50;
        case Input::Digit3: return 51;
        case Input::Digit4: return 52;
        case Input::Digit5: return 53;
        case Input::Digit6: return 54;
        case Input::Digit7: return 55;
        case Input::Digit8: return 56;
        case Input::Digit9: return 57;
    }
    return 0;
}

}
