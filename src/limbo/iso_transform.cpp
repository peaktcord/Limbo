#include "src/limbo/iso_transform.hpp"

void IsoTransform::world_to_screen(const SharedArray<int32_t> &nArray, const SharedArray<int32_t> &nArray2) {
    nArray2[0] = (nArray[0] - nArray[1]) >> 3;
    nArray2[1] = (nArray[0] + nArray[1]) >> 4;
}

void IsoTransform::screen_to_world(const SharedArray<int32_t> &nArray, const SharedArray<int32_t> &nArray2) {
    nArray[0] = shiftLeft32(nArray2[0], 2) + shiftLeft32(nArray2[1], 3);
    nArray[1] = shiftLeft32(nArray2[1], 3) - shiftLeft32(nArray2[0], 2);
}

void IsoTransform::screen_to_tile(const SharedArray<int32_t> &nArray, const SharedArray<int32_t> &nArray2) {
    SharedArray<int32_t> world(2);
    IsoTransform::screen_to_world(world, nArray);
    nArray2[0] = world[0] >> 7;
    nArray2[1] = world[1] >> 7;
}
