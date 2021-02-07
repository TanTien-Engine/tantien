#pragma once

#include <cstdint>

namespace tt
{
	
struct ImageData
{
    uint8_t* pixels = nullptr;
    int width = 0, height = 0;
    int format = 0;
};

}