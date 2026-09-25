#ifndef OBLIVION_SPRITE_FRAME_HPP
#define OBLIVION_SPRITE_FRAME_HPP

#include "src/common/session.hpp"

class SpriteFrame {
public:
    std::string image_path;
    int8_t frame_id = 0;
    int8_t draw_offset_x = 0;
    int8_t draw_offset_y = 0;
    int8_t loop = 0;
    int8_t mirror_horizontal = 0;
    int8_t is_subframe = 0;
    int16_t source_x = 0;
    int16_t source_y = 0;
    int16_t width = 0;
    int16_t height = 0;
    SpriteFrame *next_frame = nullptr;
    SpriteFrame *current_frame = nullptr;
    SpriteFrame *next_animation = nullptr;
};

#endif
