#pragma once

#include <cstddef>
#include <type_traits>
#include <vector>

namespace tt::voxel_game
{

// Storage is allocated only by the constructor. All queue operations below are
// noexcept and never grow storage, which makes fixed-tick result publication
// safe after a user adapter has already observed a command.
template <typename Value>
class FixedRing
{
public:
    static_assert(std::is_nothrow_default_constructible<Value>::value,
                  "FixedRing values must default construct without throwing");
    static_assert(std::is_nothrow_copy_assignable<Value>::value,
                  "FixedRing values must copy assign without throwing");

    explicit FixedRing(std::size_t capacity)
        : m_storage(capacity)
    {
    }

    std::size_t size() const noexcept
    {
        return m_size;
    }

    bool empty() const noexcept
    {
        return m_size == 0;
    }

    bool full() const noexcept
    {
        return m_size == m_storage.size();
    }

    bool try_push_back(const Value& value) noexcept
    {
        Value* slot = reserve_back();
        if (slot == nullptr)
        {
            return false;
        }

        *slot = value;
        commit_reserved_back();
        return true;
    }

    const Value& front() const noexcept
    {
        return m_storage[m_head];
    }

    void pop_front() noexcept
    {
        m_head = (m_head + 1) % m_storage.size();
        --m_size;
    }

    Value* reserve_back() noexcept
    {
        if (full())
        {
            return nullptr;
        }

        return &m_storage[(m_head + m_size) % m_storage.size()];
    }

    void commit_reserved_back() noexcept
    {
        ++m_size;
    }

    void clear() noexcept
    {
        m_head = 0;
        m_size = 0;
    }

private:
    std::vector<Value> m_storage;
    std::size_t m_head = 0;
    std::size_t m_size = 0;
};

} // namespace tt::voxel_game
