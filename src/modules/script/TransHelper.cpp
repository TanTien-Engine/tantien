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

std::vector<sm::vec3> list_to_vec3_array(int index)
{
    std::vector<sm::vec3> ret;
    if (ves_type(index) != VES_TYPE_LIST) {
        return ret;
    }

    const int num = ves_len(index);
    ret.reserve(num / 3);
    for (int i = 0; i < num;)
    {
        ves_geti(index, i++);
        float x = (float)ves_tonumber(-1);
        ves_pop(1);

        ves_geti(index, i++);
        float y = (float)ves_tonumber(-1);
        ves_pop(1);

        ves_geti(index, i++);
        float z = (float)ves_tonumber(-1);
        ves_pop(1);

        ret.emplace_back(x, y, z);
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

sm::vec2 map_to_vec2(int index)
{
    sm::vec2 ret;

    ves_getfield(index, "x");
    ret.x = (float)ves_tonumber(-1);
    ves_pop(1);
    ves_getfield(index, "y");
    ret.y = (float)ves_tonumber(-1);
    ves_pop(1);

    return ret;
}

sm::vec3 map_to_vec3(int index)
{
    sm::vec3 ret;

    ves_getfield(index, "x");
    ret.x = (float)ves_tonumber(-1);
    ves_pop(1);
    ves_getfield(index, "y");
    ret.y = (float)ves_tonumber(-1);
    ves_pop(1);
    ves_getfield(index, "z");
    ret.z = (float)ves_tonumber(-1);
    ves_pop(1);

    return ret;
}

sm::vec4 map_to_vec4(int index)
{
    sm::vec4 ret;

    ves_getfield(index, "x");
    ret.x = (float)ves_tonumber(-1);
    ves_pop(1);
    ves_getfield(index, "y");
    ret.y = (float)ves_tonumber(-1);
    ves_pop(1);
    ves_getfield(index, "z");
    ret.z = (float)ves_tonumber(-1);
    ves_pop(1);
    ves_getfield(index, "w");
    ret.w = (float)ves_tonumber(-1);
    ves_pop(1);

    return ret;
}

void return_list(const std::vector<std::vector<sm::vec2>>& polys)
{
    ves_pop(ves_argnum());

    ves_newlist(int(polys.size()));
    for (int i_list = 0; i_list < polys.size(); ++i_list)
    {
        ves_newlist(int(polys[i_list].size() * 2));

        for (int i = 0, n = (int)(polys[i_list].size()); i < n; ++i) {
            for (int j = 0; j < 2; ++j) {
                ves_pushnumber(polys[i_list][i].xy[j]);
                ves_seti(-2, i * 2 + j);
                ves_pop(1);
            }
        }

        ves_seti(-2, i_list);
        ves_pop(1);
    }
}

void return_list(const std::vector<sm::vec2>& pts)
{
    ves_pop(ves_argnum());

    ves_newlist(int(pts.size()) * 2);
    for (int i = 0, n = int(pts.size()); i < n; ++i)
    {
        ves_pushnumber(pts[i].x);
        ves_seti(-2, i * 2);
        ves_pop(1);

        ves_pushnumber(pts[i].y);
        ves_seti(-2, i * 2 + 1);
        ves_pop(1);
    }
}

void return_list(const std::vector<sm::vec3>& pts)
{
    ves_pop(ves_argnum());

    ves_newlist(int(pts.size()) * 3);
    for (int i = 0, n = int(pts.size()); i < n; ++i)
    {
        ves_pushnumber(pts[i].x);
        ves_seti(-2, i * 3);
        ves_pop(1);

        ves_pushnumber(pts[i].y);
        ves_seti(-2, i * 3 + 1);
        ves_pop(1);

        ves_pushnumber(pts[i].z);
        ves_seti(-2, i * 3 + 2);
        ves_pop(1);
    }
}

void return_list(const std::vector<std::string>& strs)
{
    ves_pop(ves_argnum());

    ves_newlist((int)strs.size());
    for (int i = 0; i < strs.size(); ++i)
    {
        ves_pushstring(strs[i].c_str());
        ves_seti(-2, i);
        ves_pop(1);
    }
}

}