#pragma once

#include <cstdint>

namespace tt
{
	
struct ImageData
{
    uint8_t* pixels = nullptr;
    int width = 0, height = 0;
    int format = 0;
    // How `pixels` was allocated, so it is released with the matching deallocator.
    // gimg_import() returns malloc()'d memory (stb_image under the hood); every
    // other path uses new[]. Freeing a malloc block with delete[] is UB, so the
    // owner records the allocator here. true = free(), false = delete[].
    bool pixels_malloc = false;
};

}