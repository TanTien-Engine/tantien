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

template<typename T>
void return_list(const std::vector<T>& vals)
{
    ves_pop(ves_argnum());

    const int num = static_cast<int>(vals.size());
    ves_newlist(num);
    for (int i = 0; i < num; ++i)
    {
        ves_pushnumber(static_cast<double>(vals[i]));
        ves_seti(-2, i);
        ves_pop(1);
    }
}

template<typename T>
void return_list2(const std::vector<std::vector<T>>& vals)
{
    ves_pop(ves_argnum());

    const int num0 = static_cast<int>(vals.size());
    ves_newlist(num0);
    for (int i = 0; i < num0; ++i)
    {
        const int num1 = static_cast<int>(vals[i].size());
        ves_newlist(num1);

        for (int j = 0; j < num1; ++j)
        {
            ves_pushnumber(static_cast<double>(vals[i][j]));
            ves_seti(-2, j);
            ves_pop(1);
        }

        ves_seti(-2, i);
        ves_pop(1);
    }
}

}