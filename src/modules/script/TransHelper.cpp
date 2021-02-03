#include "modules/script/TransHelper.h"

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
        ves_geti(index, i++);
        float y = (float)ves_tonumber(-1);
        ret.emplace_back(x, y);
    }
    ves_pop(num);

	return ret;
}

uint32_t list_to_color(int index)
{
    if (ves_type(index) != VES_TYPE_LIST) {
        return 0;
    }

    int r, g, b, a;
    const int num = ves_len(index);
    assert(num == 3 || num == 4);
    ves_geti(index, 0);
    r = (int)ves_tonumber(-1);
    ves_geti(index, 1);
    g = (int)ves_tonumber(-1);
    ves_geti(index, 2);
    b = (int)ves_tonumber(-1);
    if (num == 4) {
        ves_geti(index, 3);
        a = (int)ves_tonumber(-1);
    } else {
        a = 255;
    }
    ves_pop(num);

    const uint32_t col = a << 24 | b << 16 | g << 8 | r;
    return col;
}

}