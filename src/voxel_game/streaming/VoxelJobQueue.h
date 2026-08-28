#pragma once

#include "voxel_game/streaming/CpuJob.h"
#include "voxel_game/streaming/PriorityKey.h"
#include "voxel_game/streaming/ResultMailbox.h"
#include "voxel_game/streaming/SectionSnapshot.h"
#include "voxel_game/streaming/StreamingStats.h"
#include "voxel_game/streaming/TaskMailbox.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

namespace tt::voxel_game::streaming
{

enum class SubmitStatus : std::uint8_t
{
    Accepted,
    Full,             // task queue at capacity: retry later or drop by policy
    Busy,             // mailbox lock momentarily held by a worker: retry immediately
    Closed,           // stopping/stopped: no new work
    WrongThread,      // only the constructing (owner) thread may submit
    Invalid,          // null snapshot, empty callable, or snapshot/token mismatch
    SequenceExhausted // sequence space (UINT64_MAX) consumed: never wraps
};

enum class PopStatus : std::uint8_t
{
    Ok,
    Empty,      // running and nothing queued
    Busy,       // mailbox lock momentarily held by a worker: retry immediately
    Closed,     // stopped and fully drained
    WrongThread // only the owner thread may pop
};

enum class StopStatus : std::uint8_t
{
    Joined,      // barrier completed: all workers joined, state Stopped
    Reentrant,   // an owner-thread destructor called stop() inside stop()
    WrongThread  // only the owner thread may stop
};

enum class Lifecycle : std::uint8_t
{
    Running,
    Stopping, // result publication closed; tasks cancelled; workers draining
    Stopped   // stop() (or the destructor) joined every worker
};

enum class WorkerFaultPoint : std::uint8_t
{
    BeforeTaskConstruction,
    AfterPop,
    BeforeExecute,
    LoopTail
};

// Small async CPU job queue for voxel section work.
//
// Owner-thread contract:
// - try_submit / try_pop_result / stop may only be called from the thread
//   that constructed the queue; violators get WrongThread and no state
//   changes. stats().snapshot() may be read from any thread. The
//   destructor runs the same shutdown path without the thread check (it has
//   no error channel) and always joins.
// - try_submit and try_pop_result are strictly non-blocking: both mailboxes
//   are taken with try_lock, so a worker inside its short critical section
//   yields Busy instead of waiting. stop() deliberately blocks: it is the
//   synchronous lifecycle barrier.
//
// Stop barrier (Running -> Stopping -> Stopped, idempotent):
// 1. Enter Stopping, then close result publication FIRST. Everything
//    published before this point stays drainable; anything after is
//    dropped (dropped_closed).
// 2. Discard unclaimed tasks (cancelled). Workers re-check the lifecycle
//    before invoking a job function and cancel locally for tasks they had
//    already popped (also cancelled).
// 3. Join all workers before returning. In-flight job functions run to
//    completion (they only touch snapshots); their results are dropped by
//    rule 1. After stop() returns it is safe to destroy captured state,
//    the World, and anything else the callables referenced.
//
// Pending accounting: pending is incremented BEFORE the task becomes
// visible to workers and decremented exactly once per accepted task —
// consumed result (owner pop), cancel (mailbox discard or worker-side
// pre-invoke), drop (closed publication), or internal failure — so every
// sample satisfies pending >= 0.
class VoxelJobQueue
{
public:
    static constexpr unsigned kMaxWorkerCount = 32;

    using WorkerLoop = std::function<void(unsigned)>;
    // Test seam: constructs the thread for worker `id`. Throwing from the
    // factory triggers the creation-failure rollback (already created
    // workers are closed, joined, and the exception propagates).
    using ThreadFactory = std::function<std::thread(unsigned, WorkerLoop)>;
    using FaultInjector = std::function<void(WorkerFaultPoint)>;
    using ResultCutoffHook = std::function<void()>;

    struct Config
    {
        std::size_t task_capacity = 64;
        std::size_t result_capacity = 64;
        // -1 (default) selects default_worker_count(); 0 creates NO workers
        // (pure buffering, useful for deterministic tests); 1..kMaxWorkerCount
        // creates exactly that many.
        int worker_count = -1;
        std::uint64_t initial_sequence = 0; // test/replay seam; sequence space ends at UINT64_MAX
        ThreadFactory thread_factory;       // empty -> std::thread
        // Deterministic test seams. Production leaves these empty.
        TaskMailbox::PopHook task_pop_hook;
        FaultInjector fault_injector;
        ResultCutoffHook result_cutoff_hook;
    };

    static unsigned default_worker_count()
    {
        const unsigned cores = std::thread::hardware_concurrency();
        if (cores <= 1) return 1;
        const unsigned usable = cores - 1;
        return usable < 4 ? usable : 4; // min(4, max(1, cores - 1))
    }

    explicit VoxelJobQueue(Config config = {})
        : m_task_capacity(config.task_capacity),
          m_result_capacity(config.result_capacity),
          m_tasks(m_task_capacity, config.task_pop_hook),
          m_results(m_result_capacity),
          m_next_sequence(config.initial_sequence),
          m_thread_factory(config.thread_factory ? config.thread_factory : ThreadFactory{}),
          m_fault_injector(config.fault_injector ? config.fault_injector : FaultInjector{}),
          m_result_cutoff_hook(config.result_cutoff_hook ? config.result_cutoff_hook : ResultCutoffHook{}),
          m_owner(std::this_thread::get_id())
    {
        if (config.task_capacity == 0 || config.result_capacity == 0) {
            throw std::invalid_argument("streaming mailbox capacity must be non-zero");
        }
        if (config.worker_count < -1 || config.worker_count > static_cast<int>(kMaxWorkerCount)) {
            throw std::invalid_argument("streaming worker_count must be -1, 0, or 1..32");
        }
        unsigned workers = 0;
        if (config.worker_count < 0) {
            workers = default_worker_count();
        } else {
            workers = static_cast<unsigned>(config.worker_count);
        }
        m_workers.reserve(workers);
        for (unsigned id = 0; id < workers; ++id) {
            try {
                if (m_thread_factory) {
                    m_workers.push_back(
                        m_thread_factory(id, [this](unsigned worker_id) { worker_loop(worker_id); }));
                } else {
                    m_workers.push_back(std::thread([this, id] { worker_loop(id); }));
                }
            } catch (...) {
                // Thread creation failed: enter Stopping so the workers that
                // were created see closed mailboxes, join them, rethrow.
                shutdown();
                join_workers();
                throw;
            }
        }
    }

    ~VoxelJobQueue()
    {
        shutdown();
        join_workers();
    }

    VoxelJobQueue(const VoxelJobQueue&) = delete;
    VoxelJobQueue& operator=(const VoxelJobQueue&) = delete;

    // Owner-thread submit; strictly non-blocking (see class comment). The
    // snapshot must be non-null, the callable non-empty, and the snapshot's
    // token must equal `token`, otherwise Invalid.
    SubmitStatus try_submit(const ChunkToken& token, SectionSnapshotPtr snapshot,
                            CpuSectionFn function, bool in_frustum,
                            SectionCoord camera_section)
    {
        if (std::this_thread::get_id() != m_owner) return SubmitStatus::WrongThread;
        if (m_shutdown_started.load(std::memory_order_acquire) ||
            m_lifecycle.load(std::memory_order_acquire) != Lifecycle::Running) {
            return SubmitStatus::Closed;
        }
        if (!snapshot || !function || !(snapshot->token() == token)) {
            bump(m_stats.invalid);
            return SubmitStatus::Invalid;
        }

        // The submission sequence never wraps: the last value is reserved
        // as the exhausted marker and submits stop there.
        const std::uint64_t sequence = m_next_sequence.load(std::memory_order_relaxed);
        if (sequence == UINT64_MAX) return SubmitStatus::SequenceExhausted;
        SequencedKey key;
        key.key = make_priority_key(token.coord, in_frustum, camera_section);
        key.sequence = sequence;
        VoxelJob job{token, std::move(snapshot), std::move(function)};

        // Increment pending BEFORE the task becomes visible to workers so
        // no worker-side settlement can race ahead of it; rolled back when
        // the mailbox does not accept the task.
        m_stats.pending.fetch_add(1, std::memory_order_relaxed);

        const MailboxStatus status = m_tasks.try_submit(key, std::move(job));
        if (status == MailboxStatus::Accepted) {
            m_next_sequence.store(sequence + 1, std::memory_order_relaxed);
            bump(m_stats.submitted);
            return SubmitStatus::Accepted;
        }
        m_stats.pending.fetch_sub(1, std::memory_order_relaxed); // rollback

        if (status == MailboxStatus::Full) {
            bump(m_stats.task_queue_full);
            bump(m_stats.queue_full);
            return SubmitStatus::Full;
        }
        if (status == MailboxStatus::Busy) return SubmitStatus::Busy;
        return SubmitStatus::Closed;
    }

    // Owner-thread result consumption; strictly non-blocking. A successful
    // pop resolves one pending job; validate the token against authoritative
    // state (validate_token / plan_upload), then note_result_accepted or
    // note_result_stale.
    PopStatus try_pop_result(MeshResult& out)
    {
        if (std::this_thread::get_id() != m_owner) return PopStatus::WrongThread;
        switch (m_results.try_pop(out)) {
        case PopResult::Ok:
            m_stats.pending.fetch_sub(1, std::memory_order_relaxed);
            return PopStatus::Ok;
        case PopResult::Busy:
            return PopStatus::Busy;
        case PopResult::Closed:
            return PopStatus::Closed;
        case PopResult::Empty:
            break;
        }
        return PopStatus::Empty;
    }

    // Synchronous lifecycle barrier; see class comment. Blocks until every
    // worker has joined. WrongThread performs no state change.
    StopStatus stop()
    {
        if (std::this_thread::get_id() != m_owner) return StopStatus::WrongThread;
        bool expected = false;
        if (!m_stop_in_progress.compare_exchange_strong(expected, true,
                                                        std::memory_order_acq_rel)) {
            return StopStatus::Reentrant;
        }
        struct StopScope
        {
            std::atomic<bool>& active;
            ~StopScope() { active.store(false, std::memory_order_release); }
        } scope{m_stop_in_progress};
        shutdown();
        join_workers();
        m_lifecycle.store(Lifecycle::Stopped, std::memory_order_release);
        return StopStatus::Joined;
    }

    // Any thread. During the stop barrier the state is Stopping; after
    // stop() returns it is Stopped.
    Lifecycle state() const { return m_lifecycle.load(std::memory_order_acquire); }

    // Any thread (atomics only); the mutable reference itself is for the
    // owner thread.
    const StreamingStats& stats() const { return m_stats; }
    void note_result_accepted() { m_stats.note_result_accepted(); }
    void note_result_stale() { m_stats.note_result_stale(); }

    std::size_t task_capacity() const { return m_task_capacity; }
    std::size_t result_capacity() const { return m_result_capacity; }
    std::uint64_t next_sequence_for_testing() const noexcept
    {
        return m_next_sequence.load(std::memory_order_relaxed);
    }

private:
    static void bump(std::atomic<std::uint64_t>& counter)
    {
        counter.fetch_add(1, std::memory_order_relaxed);
    }

    // Running -> Stopping transition plus publication close and task
    // cancel; idempotent. Callers join workers separately.
    void shutdown()
    {
        bool expected = false;
        if (!m_shutdown_started.compare_exchange_strong(expected, true,
                                                        std::memory_order_acq_rel)) {
            return; // already stopping or stopped
        }
        m_results.close(); // publication cutoff: later results are dropped
        if (m_result_cutoff_hook) {
            try {
                m_result_cutoff_hook();
            } catch (...) {
                // A seam must not leave shutdown half-open. Its exception is
                // observable as infrastructure failure, but cancellation and
                // the lifecycle transition still complete.
                bump(m_stats.internal_failure);
            }
        }
        // Publishing is closed before Stopping becomes observable. Thus a
        // caller that observes Stopping may rely on the cutoff having passed.
        m_lifecycle.store(Lifecycle::Stopping, std::memory_order_release);
        const std::size_t discarded = m_tasks.discard_all();
        m_stats.cancelled.fetch_add(discarded, std::memory_order_relaxed);
        m_stats.pending.fetch_sub(discarded, std::memory_order_relaxed);
    }

    void join_workers()
    {
        for (std::thread& worker : m_workers) {
            if (worker.joinable()) worker.join();
        }
    }

    void drop_closed_result()
    {
        bump(m_stats.dropped_closed);
        m_stats.pending.fetch_sub(1, std::memory_order_relaxed);
    }

    static void set_error_text(MeshResult& result, const char* text) noexcept
    {
        // Best effort: even if copying the message throws (bad_alloc), the
        // worker swallows it and delivers ok == false with an empty message
        // — never std::terminate.
        try {
            result.error = text;
        } catch (...) {
            try {
                result.error.clear();
            } catch (...) {
            }
        }
    }

    void worker_loop(unsigned id)
    {
        for (;;) {
            bool claimed = false;
            try {
                inject_fault(WorkerFaultPoint::BeforeTaskConstruction);
                TaskMailbox::Task task;
                if (!m_tasks.pop_wait(task)) break; // closed and drained
                claimed = true;
                inject_fault(WorkerFaultPoint::AfterPop);

                // Re-check the lifecycle before invoking the function: tasks
                // claimed during the stop window are cancelled, not executed.
                if (m_shutdown_started.load(std::memory_order_acquire) ||
                    m_lifecycle.load(std::memory_order_acquire) != Lifecycle::Running) {
                    bump(m_stats.cancelled);
                    m_stats.pending.fetch_sub(1, std::memory_order_relaxed);
                    claimed = false;
                    continue;
                }
                inject_fault(WorkerFaultPoint::BeforeExecute);
                execute_and_publish(task, id);
                claimed = false; // execute_and_publish settled exactly once
                inject_fault(WorkerFaultPoint::LoopTail);
            } catch (...) {
                // Construction, pop, execution, and loop-tail seams are all
                // inside this boundary. A claimed task is settled once; an
                // unclaimed infrastructure error cannot affect pending.
                bump(m_stats.internal_failure);
                if (claimed) m_stats.pending.fetch_sub(1, std::memory_order_relaxed);
            }
        }
    }

    void inject_fault(WorkerFaultPoint point)
    {
        if (m_fault_injector) m_fault_injector(point);
    }

    void execute_and_publish(const TaskMailbox::Task& task, unsigned id)
    {
        MeshResult result;
        result.token = task.job.token;
        result.worker_id = id;

        bool ok = false;
        try {
            if (!task.job.snapshot) {
                throw std::invalid_argument("streaming job has no snapshot");
            }
            if (!task.job.function) {
                throw std::invalid_argument("streaming job has no function");
            }
            result.mesh = task.job.function(*task.job.snapshot);
            ok = true;
        } catch (const std::exception& error) {
            set_error_text(result, error.what());
        } catch (...) {
            set_error_text(result, "unknown exception in cpu voxel job");
        }
        result.ok = ok;

        const MailboxStatus pushed = m_results.try_push(std::move(result));
        if (pushed == MailboxStatus::Accepted) {
            if (ok) bump(m_stats.completed); else bump(m_stats.failed);
            return;
        }
        if (pushed == MailboxStatus::Full) {
            // Full result queue: count the event, then wait for space.
            // This can stall this worker but never the owner thread; if the
            // mailbox closes while waiting, the result is dropped.
            bump(m_stats.result_queue_full);
            bump(m_stats.queue_full);
            if (m_results.push_wait(std::move(result))) {
                if (ok) bump(m_stats.completed); else bump(m_stats.failed);
            } else {
                drop_closed_result();
            }
            return;
        }
        drop_closed_result(); // Closed: publication cutoff already passed
    }

    const std::size_t m_task_capacity;
    const std::size_t m_result_capacity;
    TaskMailbox m_tasks;
    ResultMailbox m_results;
    StreamingStats m_stats;
    std::atomic<std::uint64_t> m_next_sequence;
    ThreadFactory m_thread_factory;
    FaultInjector m_fault_injector;
    ResultCutoffHook m_result_cutoff_hook;
    std::vector<std::thread> m_workers;
    std::thread::id m_owner;
    std::atomic<Lifecycle> m_lifecycle{Lifecycle::Running};
    std::atomic<bool> m_shutdown_started{false};
    std::atomic<bool> m_stop_in_progress{false};
};

} // namespace tt::voxel_game::streaming
