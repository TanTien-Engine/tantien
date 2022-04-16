#pragma once

namespace tt
{

template <typename T>
std::vector<T> list_to_array(int index)
{
    std::vector<T> ret;
    if (ves_type(index) != VES_TYPE_LIST) {
        return ret;
    }

    const int num = ves_len(index);
    ret.reserve(num);
    for (int i = 0; i < num; ++i)
    {
        ves_geti(index, i);
        ret.push_back((T)ves_tonumber(-1));
        ves_pop(1);        
    }

    return ret;
}

template <typename T>
std::vector<std::vector<T>> list_to_array2(int index)
{
    std::vector<std::vector<T>> ret;
    if (ves_type(index) != VES_TYPE_LIST) {
        return ret;
    }

    const int num = ves_len(index);
    ret.reserve(num);
    for (int i = 0; i < num; ++i)
    {
        ves_geti(-1, i);

        std::vector<T> list;

        const int num2 = ves_len(-1);
        list.reserve(num2);
        for (int j = 0; j < num2; ++j)
        {
            ves_geti(-1, j);
            list.push_back((T)ves_tonumber(-1));
            ves_pop(1);
        }

        ret.push_back(list);

        ves_pop(1);
    }

    return ret;
}

}