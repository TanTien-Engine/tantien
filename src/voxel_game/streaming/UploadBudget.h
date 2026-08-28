#pragma once

#include "voxel_game/streaming/CpuJob.h"
#include "voxel_game/streaming/ChunkToken.h"
#include "voxel_game/streaming/StreamingStats.h"

#include <cstddef>
#include <cstdint>
#include <deque>
#include <utility>

namespace tt::voxel_game::streaming
{

// Pure data describing how many result meshes the main thread may accept and
// upload per frame. The streaming layer performs no GPU work; the integrator
// calls plan_upload for each popped result and feeds the counters.
struct UploadBudget
{
    std::uint32_t max_uploads_per_frame = 2;
    std::size_t max_bytes_per_frame = 1 << 20; // 1 MiB
};

enum class UploadReject : std::uint8_t
{
    None,
    StaleToken,          // token validation failed: drop and resubmit from current state
    FailedResult,        // worker job threw: log error, apply caller retry policy
    MeshTooLarge,        // mesh alone exceeds max_bytes_per_frame: retrying never helps
    FrameBudgetExhausted,// valid mesh, but this frame's upload/byte budget is spent: retry next frame
    SizeOverflow         // byte size computation saturated: treat as hostile/corrupt
};

struct UploadDecision
{
    bool accept = false;
    UploadReject reject = UploadReject::None;
};

inline UploadDecision plan_upload(const UploadBudget& budget,
                                  const ChunkToken& authoritative,
                                  const MeshResult& result,
                                  std::uint32_t uploads_this_frame,
                                  std::size_t bytes_this_frame) noexcept;

// Owner-side, caller-visible retention for results that cannot fit this
// frame. The front is deliberately retained on FrameBudgetExhausted; callers
// do not need to remember an already-popped mesh themselves.
enum class UploadDisposition : std::uint8_t
{
    Empty,
    Accepted,
    StaleToken,
    FailedResult,
    MeshTooLarge,
    FrameBudgetExhausted,
    SizeOverflow
};

class UploadFrameQueue
{
public:
    UploadFrameQueue(UploadBudget budget, StreamingStats& stats)
        : m_budget(budget), m_stats(stats) {}

    void begin_frame() noexcept
    {
        m_uploads_this_frame = 0;
        m_bytes_this_frame = 0;
    }

    void enqueue(MeshResult&& result)
    {
        m_pending.push_back(std::move(result));
    }

    UploadDisposition try_take(const ChunkToken& authoritative, MeshResult& out)
    {
        if (m_pending.empty()) return UploadDisposition::Empty;
        MeshResult& candidate = m_pending.front();
        const UploadDecision decision = plan_upload(
            m_budget, authoritative, candidate, m_uploads_this_frame, m_bytes_this_frame);
        if (decision.accept) {
            const MeshSizeBytes size = checked_mesh_bytes(candidate.mesh);
            out = std::move(candidate);
            m_pending.pop_front();
            ++m_uploads_this_frame;
            m_bytes_this_frame += size.bytes; // validated non-overflow above
            m_stats.accepted.fetch_add(1, std::memory_order_relaxed);
            m_front_deferred = false;
            return UploadDisposition::Accepted;
        }

        switch (decision.reject) {
        case UploadReject::FrameBudgetExhausted:
            if (!m_front_deferred) {
                m_stats.deferred.fetch_add(1, std::memory_order_relaxed);
                m_front_deferred = true;
            }
            return UploadDisposition::FrameBudgetExhausted; // keep ownership
        case UploadReject::StaleToken:
            m_pending.pop_front();
            m_stats.dropped_stale.fetch_add(1, std::memory_order_relaxed);
            m_front_deferred = false;
            return UploadDisposition::StaleToken;
        case UploadReject::FailedResult:
            m_pending.pop_front();
            m_stats.dropped_failed.fetch_add(1, std::memory_order_relaxed);
            m_front_deferred = false;
            return UploadDisposition::FailedResult;
        case UploadReject::MeshTooLarge:
            m_pending.pop_front();
            m_stats.dropped_too_large.fetch_add(1, std::memory_order_relaxed);
            m_front_deferred = false;
            return UploadDisposition::MeshTooLarge;
        case UploadReject::SizeOverflow:
            m_pending.pop_front();
            m_stats.dropped_overflow.fetch_add(1, std::memory_order_relaxed);
            m_front_deferred = false;
            return UploadDisposition::SizeOverflow;
        case UploadReject::None:
            break;
        }
        return UploadDisposition::Empty;
    }

    std::size_t deferred_count() const noexcept { return m_pending.size(); }
    std::uint32_t uploads_this_frame() const noexcept { return m_uploads_this_frame; }
    std::size_t bytes_this_frame() const noexcept { return m_bytes_this_frame; }

private:
    UploadBudget m_budget;
    StreamingStats& m_stats;
    std::deque<MeshResult> m_pending;
    std::uint32_t m_uploads_this_frame = 0;
    std::size_t m_bytes_this_frame = 0;
    bool m_front_deferred = false;
};

// Pure decision core, callable with synthetic sizes so every reject reason
// (including SizeOverflow) is testable without hostile real meshes.
inline UploadDecision plan_upload_decision(const UploadBudget& budget,
                                           bool token_current, bool result_ok,
                                           MeshSizeBytes size,
                                           std::uint32_t uploads_this_frame,
                                           std::size_t bytes_this_frame) noexcept
{
    UploadDecision decision;
    if (!token_current) {
        decision.reject = UploadReject::StaleToken;
        return decision;
    }
    if (!result_ok) {
        decision.reject = UploadReject::FailedResult;
        return decision;
    }
    if (size.overflow) {
        decision.reject = UploadReject::SizeOverflow;
        return decision;
    }
    if (size.bytes > budget.max_bytes_per_frame) {
        decision.reject = UploadReject::MeshTooLarge;
        return decision;
    }
    if (uploads_this_frame >= budget.max_uploads_per_frame) {
        decision.reject = UploadReject::FrameBudgetExhausted;
        return decision;
    }
    if (bytes_this_frame > budget.max_bytes_per_frame ||
        budget.max_bytes_per_frame - bytes_this_frame < size.bytes) {
        decision.reject = UploadReject::FrameBudgetExhausted;
        return decision;
    }
    decision.accept = true;
    return decision;
}

// Pure decision for one popped result. `authoritative` is the token the game
// currently holds for result.token.coord; `uploads_this_frame` and
// `bytes_this_frame` count results already accepted this frame.
inline UploadDecision plan_upload(const UploadBudget& budget,
                                  const ChunkToken& authoritative,
                                  const MeshResult& result,
                                  std::uint32_t uploads_this_frame,
                                  std::size_t bytes_this_frame) noexcept
{
    return plan_upload_decision(
        budget, token_is_current(result.token, authoritative), result.ok,
        checked_mesh_bytes(result.mesh), uploads_this_frame, bytes_this_frame);
}

} // namespace tt::voxel_game::streaming
