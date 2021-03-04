#pragma once

#include <memory>

namespace tt
{

template<typename T>
struct Proxy
{
    ~Proxy() {}

    std::shared_ptr<T> obj = nullptr;
};

}