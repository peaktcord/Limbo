#ifndef OBLIVION_SPRITE_ATLAS_HPP
#define OBLIVION_SPRITE_ATLAS_HPP

#include <string>
#include <unordered_map>

#include "src/common/render/sprite.hpp"
#include "src/common/ui.hpp"
#include "src/limbo/oblivion_state.hpp"
#include "src/limbo/sprite_frame.hpp"

class SpriteAtlas {
public:
    static std::unordered_map<std::string, Image *> &image_cache();
    static std::unordered_map<std::string, Sprite *> &mirrored_sprite_cache();
    static SpriteAtlasStatics &S();

    static void clear_image_cache();
    static int32_t read_attributes(const SharedArray<int8_t> &data, int32_t offset,
                                   const SharedArray<int32_t> &attributes);
    static void apply_attributes(const SharedArray<int32_t> &attributes, SpriteFrame *frame);
    static SpriteFrame *load_atlas(const std::string &path);
    static Image *load_image(const std::string &path, const SharedArray<int32_t> &palette);
    static SpriteFrame *find_animation(SpriteFrame *atlas, int32_t frame_id);
    static int32_t draw_frame(Graphics *graphics, SpriteFrame *atlas, int32_t frame_id,
                              int32_t x, int32_t y);
    static int32_t frame_width(SpriteFrame *atlas, int32_t frame_id);
    static int32_t frame_height(SpriteFrame *atlas, int32_t frame_id);
    static bool advance_animation(SpriteFrame *atlas, int32_t frame_id);
    static bool set_animation_frame(SpriteFrame *atlas, int32_t frame_id, int32_t frame_index);
    static void reset_animation(SpriteFrame *atlas, int32_t frame_id);
    static void evict_image_prefix(const std::string &path_prefix);

    static void clinit();
};

#endif
