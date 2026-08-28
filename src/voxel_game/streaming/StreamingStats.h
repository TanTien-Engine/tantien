#pragma once

#include <atomic>
#include <cstdint>

namespace tt::voxel_game::streaming
{

// Approximate point-in-time plain copy for reporting. Each field is loaded
// independently with relaxed ordering, so a snapshot is not a transactional
// cut across counters modified by different threads.
struct StreamingStatsSnapshot
{
    std::uint64_t submitted = 0;         // jobs accepted into the task queue
    std::uint64_t completed = 0;         // successful jobs whose result reached the result queue
    std::uint64_t failed = 0;            // jobs that threw; the error result is still delivered
    std::uint64_t queue_full = 0;        // aggregate: any submit/push that observed a full queue
    std::uint64_t task_queue_full = 0;   // detail: task submits rejected because the task queue was full
    std::uint64_t result_queue_full = 0; // detail: result pushes that observed a full result queue
    std::uint64_t dropped_stale = 0;     // results the consumer rejected via validate_token
    std::uint64_t accepted = 0;          // results the consumer kept and handed to upload
    std::uint64_t cancelled = 0;         // tasks discarded by stop() (mailbox discard or worker-side pre-invoke cancel)
    std::uint64_t dropped_closed = 0;    // finished results dropped because the pipeline was closing
    std::uint64_t invalid = 0;           // submits rejected for null snapshot / empty callable / token mismatch
    std::uint64_t internal_failure = 0;  // worker-loop failures outside the job function (e.g. bad_alloc)
    std::int64_t pending = 0;            // accepted jobs without a consumed or dropped outcome yet
};

// All counters are atomics: the owner thread (submit/pop/notes) and workers
// (complete/fail/drop) touch them concurrently; stats() may be read from any
// thread. pending is incremented BEFORE a task becomes visible to workers
// and decremented exactly once per accepted task (consumed result, cancel,
// drop, or internal failure), so any sample satisfies pending >= 0.
struct StreamingStats
{
    std::atomic<std::uint64_t> submitted{0};
    std::atomic<std::uint64_t> completed{0};
    std::atomic<std::uint64_t> failed{0};
    std::atomic<std::uint64_t> queue_full{0};
    std::atomic<std::uint64_t> task_queue_full{0};
    std::atomic<std::uint64_t> result_queue_full{0};
    std::atomic<std::uint64_t> dropped_stale{0};
    std::atomic<std::uint64_t> accepted{0};
    std::atomic<std::uint64_t> cancelled{0};
    std::atomic<std::uint64_t> dropped_closed{0};
    std::atomic<std::uint64_t> invalid{0};
    std::atomic<std::uint64_t> internal_failure{0};
    std::atomic<std::int64_t> pending{0};

    // Consumer-side records for popped results, after validate_token.
    void note_result_accepted()
    {
        accepted.fetch_add(1, std::memory_order_relaxed);
    }

    void note_result_stale()
    {
        dropped_stale.fetch_add(1, std::memory_order_relaxed);
    }

    // Deliberately a per-field approximate sample; see snapshot type comment.
    StreamingStatsSnapshot snapshot() const
    {
        StreamingStatsSnapshot out;
        out.submitted = submitted.load(std::memory_order_relaxed);
        out.completed = completed.load(std::memory_order_relaxed);
        out.failed = failed.load(std::memory_order_relaxed);
        out.queue_full = queue_full.load(std::memory_order_relaxed);
        out.task_queue_full = task_queue_full.load(std::memory_order_relaxed);
        out.result_queue_full = result_queue_full.load(std::memory_order_relaxed);
        out.dropped_stale = dropped_stale.load(std::memory_order_relaxed);
        out.accepted = accepted.load(std::memory_order_relaxed);
        out.cancelled = cancelled.load(std::memory_order_relaxed);
        out.dropped_closed = dropped_closed.load(std::memory_order_relaxed);
        out.invalid = invalid.load(std::memory_order_relaxed);
        out.internal_failure = internal_failure.load(std::memory_order_relaxed);
        out.pending = pending.load(std::memory_order_relaxed);
        return out;
    }
};

} // namespace tt::voxel_game::streaming
