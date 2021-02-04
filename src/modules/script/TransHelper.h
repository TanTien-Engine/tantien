#pragma once

#include <SM_Vector.h>

#include <vector>

namespace tt
{

std::vector<sm::vec2> list_to_vec2_array(int index);

uint32_t list_to_abgr(int index);
uint32_t list_to_rgba(int index);

}