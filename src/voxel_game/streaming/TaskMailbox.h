#pragma once

#include "voxel_game/streaming/CpuJob.h"
#include "voxel_game/streaming/PriorityKey.h"

#include <algorithm>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <utility>
#include <vector>

namespace tt::voxel_game::streaming
{

// Fixed-capacity priority mailbox for pending jobs.
// - try_submit never blocks: it takes the lock with try_lock and returns
//   Busy when a worker's pop critical section holds it, Full at capacity,
//   Closed after close/discard. The value is untouched unless Accepted.
// - pop_wait blocks worker threads until a task is available or the mailbox
//   is closed and drained.
// - The heap storage is reserved to full capacity at construction, so no
//   submission reallocates.
// - Pop order follows the strict total order sequenced_before (priority
//   key, then submission sequence), so scheduling is deterministic
//   regardless of heap internals.
// - discard_all moves the queued jobs out under the lock and destroys them
//   after releasing it: job callables/snapshots may run arbitrary
//   destructors, and those may re-enter this mailbox without deadlocking.
class TaskMailbox
{
public:
    using PopHook = std::function<void()>;
    struct Task
    {
        SequencedKey key{};
        VoxelJob job{};
    };

    explicit TaskMailbox(std::size_t capacity, PopHook pop_hook = {})
        : m_capacity(capacity ? capacity : 1), m_pop_hook(std::move(pop_hook))
    {
        m_heap.reserve(m_capacity);
    }

    MailboxStatus try_submit(SequencedKey key, VoxelJob&& job)
    {
        std::unique_lock<std::mutex> lock(m_mutex, std::try_to_lock);
        if (!lock.owns_lock()) return MailboxStatus::Busy;
        if (m_closed) return MailboxStatus::Closed;
        if (m_heap.size() >= m_capacity) return MailboxStatus::Full;
        m_heap.push_back(Task{key, std::move(job)});
        std::push_heap(m_heap.begin(), m_heap.end(), TaskOrder{});
        m_pop_cv.notify_one();
        return MailboxStatus::Accepted;
    }

    bool pop_wait(Task& out)
    {
        // First evacuate the caller's previous task. Its callable/snapshot
        // may own the final reference and re-enter this mailbox on
        // destruction, so it must not be destroyed while m_mutex is held.
        Task previous = std::move(out);
        bool popped = false;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_pop_cv.wait(lock, [this] { return m_closed || !m_heap.empty(); });
            if (!m_heap.empty()) {
                if (m_pop_hook) m_pop_hook(); // deterministic test seam
                std::pop_heap(m_heap.begin(), m_heap.end(), TaskOrder{});
                out = std::move(m_heap.back());
                m_heap.pop_back();
                popped = true;
            }
        }
        // `previous` is intentionally destroyed after releasing m_mutex.
        return popped;
    }

    // Cancel path: close and discard every queued task; returns how many
    // were dropped. Destructors of the discarded jobs (which may re-enter
    // this mailbox) run after the lock is released.
    std::size_t discard_all()
    {
        std::vector<Task> discarded;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_closed = true;
            m_heap.swap(discarded);
            m_pop_cv.notify_all();
        }
        return discarded.size(); // `discarded` destroyed after the lock
    }

    void close()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_closed = true;
        m_pop_cv.notify_all();
    }

    // Diagnostic; briefly takes the lock, so it is not part of the
    // non-blocking owner API.
    std::size_t size() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_heap.size();
    }

private:
    // Max-heap ordering: with comp(a, b) == "a is scheduled after b", the
    // heap front is the most preferred task.
    struct TaskOrder
    {
        bool operator()(const Task& a, const Task& b) const
        {
            return sequenced_before(b.key, a.key);
        }
    };

    mutable std::mutex m_mutex;
    std::condition_variable m_pop_cv;
    std::vector<Task> m_heap;
    const std::size_t m_capacity;
    PopHook m_pop_hook;
    bool m_closed = false;
};

} // namespace tt::voxel_game::streaming
