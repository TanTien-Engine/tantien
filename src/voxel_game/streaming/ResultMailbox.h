#pragma once

#include "voxel_game/streaming/CpuJob.h"

#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <type_traits>
#include <utility>
#include <vector>

namespace tt::voxel_game::streaming
{

enum class PopResult : std::uint8_t
{
    Ok,
    Empty,
    Busy,
    Closed // closed and fully drained
};

// Fixed-capacity FIFO mailbox for worker results, built on a ring of slots
// preallocated at construction. Container growth can never throw during a
// worker push; moving a MeshResult into a slot is noexcept.
// - try_push (worker, preferred): takes the lock (workers may wait briefly
//   for an owner's pop critical section); returns Full at capacity and
//   Closed after close; the value is untouched unless Accepted.
// - push_wait (worker fallback): blocks while the queue is full — this may
//   stall a worker but never the owner thread; returns false once closed
//   (value dropped by the caller). close() wakes parked pushers.
// - try_pop (owner): never blocks — try_lock; Busy if a worker holds the
//   lock, FIFO delivery, still works after close so a stopping pipeline
//   can drain already-delivered results.
class ResultMailbox
{
public:
    explicit ResultMailbox(std::size_t capacity)
        : m_capacity(capacity ? capacity : 1),
          m_slots(m_capacity ? capacity : 1) // preallocated empty slots
    {
    }

    MailboxStatus try_push(MeshResult&& result)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_closed) return MailboxStatus::Closed;
        if (m_count >= m_capacity) return MailboxStatus::Full;
        slot(m_count) = std::move(result);
        ++m_count;
        return MailboxStatus::Accepted;
    }

    bool push_wait(MeshResult&& result)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_space_cv.wait(lock, [this] { return m_closed || m_count < m_capacity; });
        if (m_closed) return false;
        slot(m_count) = std::move(result);
        ++m_count;
        return true;
    }

    PopResult try_pop(MeshResult& out)
    {
        std::unique_lock<std::mutex> lock(m_mutex, std::try_to_lock);
        if (!lock.owns_lock()) return PopResult::Busy;
        if (m_count == 0) return m_closed ? PopResult::Closed : PopResult::Empty;
        out = std::move(slot(0));
        advance();
        m_space_cv.notify_all(); // parked pushers may proceed
        return PopResult::Ok;
    }

    void close()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_closed = true;
        m_space_cv.notify_all();
    }

    // Diagnostic; briefly takes the lock, so it is not part of the
    // non-blocking owner API.
    std::size_t size() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_count;
    }

private:
    static_assert(std::is_nothrow_move_constructible_v<MeshResult> &&
                      std::is_nothrow_move_assignable_v<MeshResult>,
                  "ResultMailbox moves MeshResult while holding its mutex");

    MeshResult& slot(std::size_t offset)
    {
        return m_slots[(m_head + offset) % m_capacity];
    }

    void advance()
    {
        m_head = (m_head + 1) % m_capacity;
        --m_count;
    }

    mutable std::mutex m_mutex;
    std::condition_variable m_space_cv;
    const std::size_t m_capacity;
    std::vector<MeshResult> m_slots; // ring storage, capacity entries
    std::size_t m_head = 0;
    std::size_t m_count = 0;
    bool m_closed = false;
};

} // namespace tt::voxel_game::streaming
