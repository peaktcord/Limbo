#include "src/limbo/sprite_atlas.hpp"

SpriteAtlasStatics &SpriteAtlas::S() { return oblivion_state().atlas; }

#include "src/limbo/game_canvas.hpp"

#include "src/limbo/string_table.hpp"


std::unordered_map<std::string, Image *> &SpriteAtlas::image_cache() {
    return currentSession().sprite_image_cache;
}

std::unordered_map<std::string, Sprite *> &SpriteAtlas::mirrored_sprite_cache() {
    return currentSession().mirrored_sprite_cache;
}

void SpriteAtlas::clinit() {
    image_cache().clear();
    mirrored_sprite_cache().clear();
    S().var_byte_a = 0;
    S().var_byte_b = 0;
}

void SpriteAtlas::clear_image_cache() { image_cache().clear(); }

int32_t SpriteAtlas::read_attributes(const SharedArray<int8_t> &byArray, int32_t n, const SharedArray<int32_t> &nArray) {
    int32_t n2 = 0;
    int32_t hi = (int32_t)(uint16_t)((byArray[n++] & 0xFF) << 8);
    int32_t lo = (int32_t)(uint16_t)((byArray[n++] & 0xFF) << 0);
    int32_t n3 = hi | lo;
    for (n2 = 0; n2 < nArray.length(); ++n2) {
        nArray[n2] = 0;
    }
    if ((n3 & 0x200) > 0) {
        nArray[0] = (int32_t)(uint16_t)(byArray[n++] & 0xFF);
    }
    if ((n3 & 0x100) > 0) {
        hi = (int32_t)(uint16_t)(byArray[n++] & 0xFF);
        lo = (int32_t)(uint16_t)(byArray[n++] & 0xFF);
        nArray[1] = hi << 8 | lo << 0;
    }
    if ((n3 & 0x80) > 0) {
        hi = (int32_t)(uint16_t)(byArray[n++] & 0xFF);
        lo = (int32_t)(uint16_t)(byArray[n++] & 0xFF);
        nArray[2] = hi << 8 | lo << 0;
    }
    if ((n3 & 0x40) > 0) {
        nArray[3] = (int32_t)(uint16_t)byArray[n++] & 0xFF;
    }
    if ((n3 & 0x20) > 0) {
        nArray[4] = (int32_t)(uint16_t)byArray[n++] & 0xFF;
    }
    if ((n3 & 0x10) > 0) {
        nArray[5] = byArray[n++];
    }
    if ((n3 & 8) > 0) {
        nArray[6] = byArray[n++];
    }
    if ((n3 & 4) > 0) {
        nArray[7] = byArray[n++];
    }
    if ((n3 & 2) > 0) {
        nArray[8] = byArray[n++];
    }
    if ((n3 & 1) > 0) {
        nArray[9] = byArray[n++];
    }
    return n;
}

void SpriteAtlas::apply_attributes(const SharedArray<int32_t> &attributes, SpriteFrame *frame) {
    frame->frame_id = static_cast<int8_t>(attributes[0]);
    frame->source_x = static_cast<int16_t>(attributes[1]);
    frame->source_y = static_cast<int16_t>(attributes[2]);
    frame->width = static_cast<int16_t>(attributes[3]);
    frame->height = static_cast<int16_t>(attributes[4]);
    frame->draw_offset_x = static_cast<int8_t>(attributes[5]);
    frame->draw_offset_y = static_cast<int8_t>(attributes[6]);
    frame->loop = static_cast<int8_t>(attributes[7]);
    frame->mirror_horizontal = static_cast<int8_t>(attributes[8]);
}

SpriteFrame *SpriteAtlas::load_atlas(const std::string &path) {
    int32_t file_size = GameCanvas::load_resource(path);
    SharedArray<int8_t> data(file_size);
    copySharedArray(current_string_table().resource_buffer, 0, data, 0, file_size);

    S().var_byte_a = 0;
    S().var_byte_b = 0;

    std::string path_prefix;
    uint8_t prefix_len = static_cast<uint8_t>(data[0]);
    if (prefix_len > 0) {
        path_prefix = array_text(data, 1, prefix_len);
    }
    int32_t offset = 1 + prefix_len;

    SpriteFrame *first_animation = nullptr;
    SpriteFrame *current_animation = nullptr;

    SharedArray<std::string> entry_image_paths(255);
    SharedArray<SharedArray<int32_t>> palettes_by_id(50);
    SharedArray<int32_t> base_attributes(10);
    SharedArray<int32_t> anim_attributes(10);
    SharedArray<int32_t> subframe_attributes(10);

    while (offset != file_size) {
        uint8_t entry_id = static_cast<uint8_t>(data[offset++]);
        uint8_t name_len = static_cast<uint8_t>(data[offset++]);
        std::string image_path = array_text(data, offset, name_len);
        if (!voyage::text::starts_with(image_path, "/")) {
            image_path = path_prefix + image_path;
        }

        offset = SpriteAtlas::read_attributes(data, offset + name_len, base_attributes);
        if (base_attributes[0] == 0) {
            base_attributes[0] = entry_id;
        }
        entry_image_paths[base_attributes[0]] = image_path;

        int32_t palette_count = data[offset++] & 0xFF;
        SharedArray<int32_t> palette(palette_count * 2);
        for (int32_t p = 0; p < palette_count; ++p) {
            int32_t p0 = static_cast<uint8_t>(data[offset++]);
            int32_t p1 = static_cast<uint8_t>(data[offset++]);
            int32_t p2 = static_cast<uint8_t>(data[offset++]);
            palette[p * 2] = (p0 << 16) | (p1 << 8) | p2;

            int32_t p3 = static_cast<uint8_t>(data[offset++]);
            int32_t p4 = static_cast<uint8_t>(data[offset++]);
            int32_t p5 = static_cast<uint8_t>(data[offset++]);
            palette[p * 2 + 1] = (p3 << 16) | (p4 << 8) | p5;
        }
        if (palette_count > 0) {
            palettes_by_id[base_attributes[0]] = palette;
        }

        int32_t animation_count = data[offset++] & 0xFF;
        if (image_path == "/4.png") {
            continue;
        }

        SharedArray<int32_t> entry_palette = (palette_count > 0) ? palettes_by_id[base_attributes[0]] : SharedArray<int32_t>();
        Image *image = SpriteAtlas::load_image(image_path, entry_palette);

        if (animation_count == 0) {
            SpriteFrame *new_animation = session_new<SpriteFrame>();
            if (first_animation == nullptr) {
                first_animation = new_animation;
            } else {
                current_animation->next_animation = new_animation;
            }
            current_animation = new_animation;

            SpriteAtlas::apply_attributes(base_attributes, current_animation);
            current_animation->image_path = image_path;
            current_animation->current_frame = current_animation;
            current_animation->width = image == nullptr ? 0 : static_cast<int16_t>(image->getWidth());
            current_animation->height = image == nullptr ? 0 : static_cast<int16_t>(image->getHeight());
            continue;
        }

        for (int32_t a = 0; a < animation_count; ++a) {
            offset = SpriteAtlas::read_attributes(data, offset, anim_attributes);

            SpriteFrame *new_animation = session_new<SpriteFrame>();
            if (first_animation == nullptr) {
                first_animation = new_animation;
            } else {
                current_animation->next_animation = new_animation;
            }
            current_animation = new_animation;
            current_animation->current_frame = current_animation;

            int32_t subframe_count = data[offset++];
            SpriteFrame *subframe = current_animation;
            for (int32_t f = 0; f < subframe_count; ++f) {
                offset = SpriteAtlas::read_attributes(data, offset, subframe_attributes);
                SpriteAtlas::apply_attributes(subframe_attributes, subframe);
                subframe->frame_id = static_cast<int8_t>(anim_attributes[0]);
                subframe->loop = static_cast<int8_t>(anim_attributes[7]);
                subframe->image_path = image_path;
                subframe->is_subframe = 1;
                if (f < subframe_count - 1) {
                    subframe->next_frame = session_new<SpriteFrame>();
                    subframe = subframe->next_frame;
                }
            }
        }
    }

    current_string_table().resource_buffer.setNull();
    GameCanvas::update_loading_progress(100);
    return first_animation;
}

Image *SpriteAtlas::load_image(const std::string &string, const SharedArray<int32_t> &nArray) {
    (void)nArray;
    auto cached = image_cache().find(string);
    Image *image = cached == image_cache().end() ? nullptr : cached->second;
    if (image == nullptr) {
        try {
            image = Image::createImage(string);
            image_cache()[string] = image;
        } catch (const std::exception &exception) {
            voyage::logging::exception("Resources", ("Loading sprite image " + string).c_str(), exception);
            image = nullptr;
        }
    }
    S().var_byte_b = (int8_t)(S().var_byte_b + 1);
    S().var_byte_a = (int8_t)(S().var_byte_a + 1);
    GameCanvas::update_loading_progress(S().var_byte_a * 100 / S().var_byte_b);
    return image;
}

SpriteFrame *SpriteAtlas::find_animation(SpriteFrame *atlas, int32_t frame_id) {
    for (SpriteFrame *frame = atlas; frame != nullptr; frame = frame->next_animation) {
        if (frame->frame_id == frame_id) {
            return frame;
        }
    }
    return nullptr;
}

int32_t SpriteAtlas::draw_frame(Graphics *graphics, SpriteFrame *atlas, int32_t frame_id, int32_t x, int32_t y) {
    SpriteFrame *anim = SpriteAtlas::find_animation(atlas, static_cast<int8_t>(frame_id));
    if (anim == nullptr || anim->current_frame == nullptr) {
        return 0;
    }
    std::string image_path = anim->image_path;
    auto cached = image_cache().find(image_path);
    Image *image = cached == image_cache().end() ? nullptr : cached->second;
    SpriteFrame *frame = anim->current_frame;
    if (image == nullptr) return 0;
    if (frame->is_subframe == 0) {
        graphics->drawImage(image, x + frame->draw_offset_x, y + frame->draw_offset_y, 0);
    } else {
        int32_t clip_x = x + frame->draw_offset_x;
        int32_t clip_y = y + frame->draw_offset_y;
        if (clip_x < GameCanvas::state().canvas_width && clip_y < GameCanvas::state().canvas_height) {
            int32_t clip_w = GameCanvas::state().canvas_width < frame->width ? GameCanvas::state().canvas_width : frame->width;
            int32_t clip_h = GameCanvas::state().canvas_height < frame->height ? GameCanvas::state().canvas_height : frame->height;
            graphics->setClip(clip_x, clip_y, clip_w, clip_h);
            graphics->clipRect(clip_x, clip_y, clip_w, clip_h);
            if (frame->mirror_horizontal == 1) {
                auto cachedSprite = mirrored_sprite_cache().find(image_path);
                Sprite *sprite = cachedSprite == mirrored_sprite_cache().end() ? nullptr : cachedSprite->second;
                if (sprite == nullptr) {
                    try {
                        sprite = session_new<Sprite>(image);
                        sprite->setTransform(2);
                        mirrored_sprite_cache()[image_path] = sprite;
                    } catch (const std::exception &exception) {
                        voyage::logging::exception("Rendering", ("Mirroring sprite " + image_path).c_str(), exception);
                        graphics->setClip(0, 0, GameCanvas::state().canvas_width, GameCanvas::state().canvas_height);
                        return 0;
                    }
                }
                sprite->setPosition(x + frame->width - image->getWidth() + frame->source_x + frame->draw_offset_x,
                                    y - frame->source_y + frame->draw_offset_y);
                sprite->paint(graphics);
            } else {
                graphics->drawImage(image, x - frame->source_x + frame->draw_offset_x, y - frame->source_y + frame->draw_offset_y,
                                    0);
            }
            graphics->setClip(0, 0, GameCanvas::state().canvas_width, GameCanvas::state().canvas_height);
        }
    }
    return frame->width;
}

int32_t SpriteAtlas::frame_width(SpriteFrame *atlas, int32_t frame_id) {
    SpriteFrame *anim = SpriteAtlas::find_animation(atlas, static_cast<int8_t>(frame_id));
    if (anim == nullptr) {
        return 0;
    }
    return anim->width;
}

int32_t SpriteAtlas::frame_height(SpriteFrame *atlas, int32_t frame_id) {
    SpriteFrame *anim = SpriteAtlas::find_animation(atlas, static_cast<int8_t>(frame_id));
    if (anim == nullptr) {
        return 0;
    }
    return anim->height;
}

bool SpriteAtlas::advance_animation(SpriteFrame *atlas, int32_t frame_id) {
    SpriteFrame *anim = SpriteAtlas::find_animation(atlas, frame_id);
    if (anim == nullptr) {
        return true;
    }
    SpriteFrame *prev_frame = anim->current_frame;
    anim->current_frame = anim->current_frame->next_frame;
    if (anim->current_frame == nullptr) {
        if (anim->loop == 1) {
            anim->current_frame = SpriteAtlas::find_animation(atlas, frame_id);
        } else {
            anim->current_frame = prev_frame;
            return true;
        }
    }
    return false;
}

bool SpriteAtlas::set_animation_frame(SpriteFrame *atlas, int32_t frame_id, int32_t frame_index) {
    SpriteFrame *anim = SpriteAtlas::find_animation(atlas, frame_id);
    if (anim == nullptr) {
        return true;
    }
    SpriteFrame *frame = anim;
    for (int32_t i = 0; i < frame_index; ++i) {
        if (frame->next_frame == nullptr) {
            return true;
        }
        frame = frame->next_frame;
    }
    anim->current_frame = frame;
    return false;
}

void SpriteAtlas::reset_animation(SpriteFrame *atlas, int32_t frame_id) {
    SpriteFrame *anim = SpriteAtlas::find_animation(atlas, frame_id);
    if (anim != nullptr) {
        anim->current_frame = SpriteAtlas::find_animation(atlas, frame_id);
    }
}

void SpriteAtlas::evict_image_prefix(const std::string &path_prefix) {
    for (auto it = image_cache().begin(); it != image_cache().end();) {
        if (!voyage::text::starts_with(it->first, path_prefix)) {
            ++it;
            continue;
        }
        it = image_cache().erase(it);
        mirrored_sprite_cache().clear();
    }
}
