#pragma once

#include "modules/script/Proxy.h"

#include <SM_Vector.h>
#include <SM_Matrix.h>
#include <vessel.h>

#include <vector>
#include <memory>

namespace tt
{

std::vector<sm::vec2> list_to_vec2_array(int index);
std::vector<sm::vec3> list_to_vec3_array(int index);
std::vector<int> list_to_int_array(int index);
std::vector<float> list_to_float_array(int index);

uint32_t list_to_abgr(int index);
uint32_t list_to_rgba(int index);

sm::vec2 list_to_vec2(int index);
sm::vec3 list_to_vec3(int index);
sm::vec4 list_to_vec4(int index);

template<typename T>
void list_to_foreigns(int index, std::vector<std::shared_ptr<T>>& foreigns)
{
    const int num = ves_len(index);
    for (int i = 0; i < num; ++i) {
        ves_geti(index, i);
        foreigns.push_back(((tt::Proxy<T>*)ves_toforeign(-1))->obj);
        ves_pop(1);
    }
}

template<typename T>
void list_to_foreigns(int index, std::vector<T>& foreigns)
{
    const int num = ves_len(index);
    for (int i = 0; i < num; ++i) {
        ves_geti(index, i);
        foreigns.push_back(*(T*)ves_toforeign(-1));
        ves_pop(1);
    }
}

}