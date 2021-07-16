#include "modules/script/TransHelper.h"

#include <guard/check.h>

#include <vessel.h>

namespace tt
{

std::vector<sm::vec2> list_to_vec2_array(int index)
{
	std::vector<sm::vec2> ret;
	if (ves_type(index) != VES_TYPE_LIST) {
		return ret;
	}

	const int num = ves_len(index);
    ret.reserve(num / 2);
    for (int i = 0; i < num;)
    {
        ves_geti(index, i++);
        float x = (float)ves_tonumber(-1);
        ves_pop(1);
        ves_geti(index, i++);
        float y = (float)ves_tonumber(-1);
        ves_pop(1);
        ret.emplace_back(x, y);
    }

	return ret;
}

uint32_t list_to_abgr(int index)
{
    if (ves_type(index) != VES_TYPE_LIST) {
        return 0;
    }

    int r, g, b, a;
    const int num = ves_len(index);
    GD_ASSERT(num == 3 || num == 4, "error num");
    ves_geti(index, 0);
    r = (int)ves_tonumber(-1);
    ves_pop(1);
    ves_geti(index, 1);
    g = (int)ves_tonumber(-1);
    ves_pop(1);
    ves_geti(index, 2);
    b = (int)ves_tonumber(-1);
    ves_pop(1);
    if (num == 4) {
        ves_geti(index, 3);
        a = (int)ves_tonumber(-1);
        ves_pop(1);
    } else {
        a = 255;
    }

    const uint32_t col = a << 24 | b << 16 | g << 8 | r;
    return col;
}

uint32_t list_to_rgba(int index)
{
    uint32_t abgr = list_to_abgr(index);
    uint8_t a = (abgr >> 24) & 0xff;
    uint8_t b = (abgr >> 16) & 0xff;
    uint8_t g = (abgr >> 8) & 0xff;
    uint8_t r = abgr & 0xff;
    const uint32_t col = r << 24 | g << 16 | b << 8 | a;
    return col;
}

sm::vec2 list_to_vec2(int index)
{
    GD_ASSERT(ves_type(index) == VES_TYPE_LIST, "error type");

    sm::vec2 ret;

    const int num = ves_len(index);
    GD_ASSERT(num == 2, "error num");

    for (int i = 0; i < 2; ++i) {
        ves_geti(index, i);
        ret.xy[i] = (float)ves_tonumber(-1);
        ves_pop(1);
    }

    return ret;
}

sm::vec3 list_to_vec3(int index)
{
    GD_ASSERT(ves_type(index) == VES_TYPE_LIST, "error type");

    sm::vec3 ret;

    const int num = ves_len(index);
    GD_ASSERT(num == 3, "error num");

    for (int i = 0; i < 3; ++i) {
        ves_geti(index, i);
        ret.xyz[i] = (float)ves_tonumber(-1);
        ves_pop(1);
    }

    return ret;
}

sm::vec4 list_to_vec4(int index)
{
    GD_ASSERT(ves_type(index) == VES_TYPE_LIST, "error type");

    sm::vec4 ret;

    const int num = ves_len(index);
    GD_ASSERT(num == 4, "error num");

    for (int i = 0; i < 4; ++i) {
        ves_geti(index, i);
        ret.xyzw[i] = (float)ves_tonumber(-1);
        ves_pop(1);
    }

    return ret;
}

}