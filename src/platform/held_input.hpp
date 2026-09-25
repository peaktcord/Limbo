#ifndef VOYAGE_PLATFORM_HELD_INPUT_HPP
#define VOYAGE_PLATFORM_HELD_INPUT_HPP

#include <cstdint>
#include <optional>

#include "src/game/game_runtime.hpp"

namespace voyage::platform {

class HeldInput {
public:
    static constexpr uint8_t MASK_UP    = 1 << 0;
    static constexpr uint8_t MASK_DOWN  = 1 << 1;
    static constexpr uint8_t MASK_LEFT  = 1 << 2;
    static constexpr uint8_t MASK_RIGHT = 1 << 3;

    void press(Input input) {
        if (GameRuntime::is_movement(input)) legacy_held_ = input;
        switch (input) {
            case Input::Up: held_mask_ |= MASK_UP; break;
            case Input::Down: held_mask_ |= MASK_DOWN; break;
            case Input::Left: held_mask_ |= MASK_LEFT; break;
            case Input::Right: held_mask_ |= MASK_RIGHT; break;
            default: break;
        }
    }

    void release(Input input) {
        if (legacy_held_ == input) legacy_held_.reset();
        switch (input) {
            case Input::Up: held_mask_ &= ~MASK_UP; break;
            case Input::Down: held_mask_ &= ~MASK_DOWN; break;
            case Input::Left: held_mask_ &= ~MASK_LEFT; break;
            case Input::Right: held_mask_ &= ~MASK_RIGHT; break;
            default: break;
        }
    }

    std::optional<Input> repeat(int32_t game_state, bool modern_movement = false) const {
        if (game_state != 0) return std::nullopt;

        if (!modern_movement) {
            return legacy_held_;
        }

        if (held_mask_ == 0) return std::nullopt;

        const bool up = (held_mask_ & MASK_UP) != 0;
        const bool down = (held_mask_ & MASK_DOWN) != 0;
        const bool left = (held_mask_ & MASK_LEFT) != 0;
        const bool right = (held_mask_ & MASK_RIGHT) != 0;

        const int32_t vert = (up ? -1 : 0) + (down ? 1 : 0);
        const int32_t horiz = (left ? -1 : 0) + (right ? 1 : 0);

        if (vert < 0 && horiz < 0) return Input::UpLeft;
        if (vert < 0 && horiz > 0) return Input::UpRight;
        if (vert > 0 && horiz < 0) return Input::DownLeft;
        if (vert > 0 && horiz > 0) return Input::DownRight;
        if (vert < 0) return Input::Up;
        if (vert > 0) return Input::Down;
        if (horiz < 0) return Input::Left;
        if (horiz > 0) return Input::Right;
        return std::nullopt;
    }

private:
    std::optional<Input> legacy_held_;
    uint8_t held_mask_ = 0;
};

}

#endif
