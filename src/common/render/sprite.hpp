#ifndef COMMON_RENDER_SPRITE_HPP
#define COMMON_RENDER_SPRITE_HPP

#include <cstdint>
#include "src/common/ui.hpp"

class Sprite {
    Image *image_;
    int32_t transform_ = 0;
    int32_t x_ = 0, y_ = 0;
public:
    enum : int32_t { TRANS_NONE = 0, TRANS_MIRROR = 2 };
    explicit Sprite(Image *image) : image_(image) {}
    virtual ~Sprite() = default;
    void setTransform(int32_t transform) { transform_ = transform; }
    void setPosition(int32_t x, int32_t y) { x_ = x; y_ = y; }
    void paint(Graphics *graphics) {
        graphics->drawImage(image_, x_, y_, 0, transform_);
    }
};

namespace render {
using Sprite = ::Sprite;
}

#endif
