#ifndef OBLIVION_ISO_TRANSFORM_HPP
#define OBLIVION_ISO_TRANSFORM_HPP

#include "src/common/session.hpp"

class IsoTransform {
public:
    static void world_to_screen(const SharedArray<int32_t> &world, const SharedArray<int32_t> &screen);

    static void screen_to_world(const SharedArray<int32_t> &world, const SharedArray<int32_t> &screen);

    static void screen_to_tile(const SharedArray<int32_t> &screen, const SharedArray<int32_t> &tile);
};

#endif
