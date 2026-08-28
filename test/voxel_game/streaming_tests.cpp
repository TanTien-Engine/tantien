// Unit tests for voxel_game::streaming (pure CPU chunk streaming infra).
//
// Standalone main, no third-party test framework, C++17 standard library
// only. Every wait carries a steady_clock deadline: a missed handshake
// fails the test instead of hanging it, and no test sleeps. Helper threads
// only record observations into atomics; all CHECKs run on the main thread.

#include "voxel_game/streaming/ChunkToken.h"
#include "voxel_game/streaming/CpuJob.h"
#include "voxel_game/streaming/PriorityKey.h"
#include "voxel_game/streaming/ResultMailbox.h"
#include "voxel_game/streaming/SectionSnapshot.h"
#include "voxel_game/streaming/StreamingCoord.h"
#include "voxel_game/streaming/StreamingStats.h"
#include "voxel_game/streaming/TaskMailbox.h"
#include "voxel_game/streaming/UploadBudget.h"
#include "voxel_game/streaming/VoxelJobQueue.h"
#include "voxel_game/voxel/Section.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

using namespace tt::voxel_game;
using namespace std::chrono_literals;
using streaming::ChunkToken;
using streaming::Lifecycle;
using streaming::MailboxStatus;
using streaming::MeshResult;
using streaming::MeshSizeBytes;
using streaming::PopResult;
using streaming::PopStatus;
using streaming::SectionCoord;
using streaming::SectionSnapshot;
using streaming::SectionSnapshotBuilder;
using streaming::SectionSnapshotPtr;
using streaming::SequencedKey;
using streaming::StopStatus;
using streaming::SubmitStatus;
using streaming::TokenVerdict;
using streaming::UploadBudget;
using streaming::UploadReject;
using streaming::VoxelJobQueue;

using Clock = std::chrono::steady_clock;

namespace {

constexpr auto kTestTimeout = 3s;

// ---------------------------------------------------------------- harness

int g_checks = 0;
int g_failures = 0;

void check_failed(const char* file, int line, const char* expression)
{
    ++g_failures;
    std::printf("    CHECK failed at %s:%d: %s\n", file, line, expression);
}

#define CHECK(cond)                                                          \
    do {                                                                     \
        ++g_checks;                                                          \
        if (!(cond)) check_failed(__FILE__, __LINE__, #cond);                \
    } while (false)

// ---------------------------------------------------------------- waiting

// Deadline-bounded predicate wait. Never sleeps; yields until the predicate
// holds or the deadline passes. Returns the final predicate value.
template <typename Predicate>
bool wait_until(Predicate&& predicate,
                std::chrono::steady_clock::duration timeout = kTestTimeout)
{
    const Clock::time_point deadline = Clock::now() + timeout;
    while (!predicate()) {
        if (Clock::now() >= deadline) return predicate();
        std::this_thread::yield();
    }
    return true;
}

// Reusable barrier: workers wait until the test opens the gate.
struct Gate
{
    std::mutex mutex;
    std::condition_variable cv;
    bool open = false;

    bool wait(Clock::time_point deadline)
    {
        std::unique_lock<std::mutex> lock(mutex);
        return cv.wait_until(lock, deadline, [this] { return open; });
    }

    void open_up()
    {
        std::lock_guard<std::mutex> lock(mutex);
        open = true;
        cv.notify_all();
    }
};

// Ensures a gate is released on every exit path, including failed CHECKs.
struct GateReleaser
{
    Gate& gate;
    ~GateReleaser() { gate.open_up(); }
};

// Counting handshake: workers notify, tests wait for exact counts.
struct Signal
{
    std::mutex mutex;
    std::condition_variable cv;
    std::uint64_t count = 0;

    void notify()
    {
        std::lock_guard<std::mutex> lock(mutex);
        ++count;
        cv.notify_all();
    }

    std::uint64_t value()
    {
        std::lock_guard<std::mutex> lock(mutex);
        return count;
    }

    bool wait_at_least(std::uint64_t target, Clock::time_point deadline)
    {
        std::unique_lock<std::mutex> lock(mutex);
        return cv.wait_until(lock, deadline, [&] { return count >= target; });
    }
};

// ---------------------------------------------------------------- fixtures

SectionSnapshotPtr make_snapshot(const ChunkToken& token)
{
    SectionSnapshotBuilder builder(token);
    builder.set_block(0, 0, 0, BlockState::Stone);
    return std::move(builder).freeze();
}

ChunkToken make_token(int x, int y, int z, std::uint64_t generation = 1,
                      std::uint64_t block_revision = 1,
                      std::uint64_t neighborhood_revision = 1)
{
    return ChunkToken(SectionCoord{x, y, z}, streaming::WorldEpoch{1},
                      streaming::TokenRevision{generation},
                      streaming::TokenRevision{block_revision},
                      streaming::TokenRevision{neighborhood_revision});
}

streaming::VoxelJob empty_job(const ChunkToken& token)
{
    return streaming::VoxelJob{token, make_snapshot(token), {}};
}

SequencedKey sequenced(int x, int y, int z, bool in_frustum,
                       std::uint64_t distance, std::uint64_t sequence)
{
    SequencedKey key;
    key.key.in_frustum = in_frustum;
    key.key.distance_squared = distance;
    key.key.coord = SectionCoord{x, y, z};
    key.sequence = sequence;
    return key;
}

streaming::CpuSectionFn trivial_function()
{
    return [](const SectionSnapshot&) { return OpaqueMesh{}; };
}

// Owner-side submit that retries only Busy (mailbox lock momentarily held
// by a worker). Any other status returns immediately.
SubmitStatus submit_bounded(VoxelJobQueue& queue, const ChunkToken& token,
                            SectionSnapshotPtr snapshot,
                            streaming::CpuSectionFn function, bool in_frustum,
                            SectionCoord camera)
{
    SubmitStatus status = SubmitStatus::Closed;
    (void)wait_until([&] {
        status = queue.try_submit(token, snapshot, function, in_frustum, camera);
        return status != SubmitStatus::Busy;
    });
    return status;
}

// Pops exactly `expected` results in delivery order, retrying Busy, with a
// deadline.
std::vector<MeshResult> drain_results(VoxelJobQueue& queue, std::size_t expected)
{
    std::vector<MeshResult> results;
    const bool ok = wait_until([&] {
        MeshResult result;
        for (;;) {
            const PopStatus status = queue.try_pop_result(result);
            if (status == PopStatus::Busy) return false; // retry
            if (status != PopStatus::Ok) break;
            results.push_back(std::move(result));
        }
        return results.size() >= expected;
    });
    if (!ok) {
        check_failed(__FILE__, __LINE__, "drain_results timed out");
    }
    return results;
}

// ---------------------------------------------------------------- tests

void test_token_validation_pure()
{
    const ChunkToken base = make_token(1, 2, 3, 5, 7, 9);
    CHECK(streaming::validate_token(base, base) == TokenVerdict::Current);
    CHECK(streaming::token_is_current(base, base));

    ChunkToken changed = make_token(1, 2, 3, 6, 7, 9);
    CHECK(streaming::validate_token(changed, base) == TokenVerdict::StaleGeneration);

    changed = make_token(1, 2, 3, 5, 8, 9);
    CHECK(streaming::validate_token(changed, base) == TokenVerdict::StaleBlockRevision);

    changed = make_token(1, 2, 3, 5, 7, 10);
    CHECK(streaming::validate_token(changed, base) == TokenVerdict::StaleNeighborhoodRevision);

    changed = make_token(4, 2, 3, 5, 7, 9);
    CHECK(streaming::validate_token(changed, base) == TokenVerdict::StaleCoord);
    CHECK(!streaming::token_is_current(changed, base));

    streaming::TokenRevision revision{UINT64_MAX - 1};
    CHECK(revision.try_bump());
    CHECK(revision.value() == UINT64_MAX);
    CHECK(!revision.try_bump());
    CHECK(revision.value() == UINT64_MAX); // fail closed: no ABA wrap

    const ChunkToken old_world(SectionCoord{1, 2, 3}, streaming::WorldEpoch{7},
                               streaming::TokenRevision{1}, streaming::TokenRevision{1},
                               streaming::TokenRevision{1});
    const ChunkToken new_world(SectionCoord{1, 2, 3}, streaming::WorldEpoch{8},
                               streaming::TokenRevision{1}, streaming::TokenRevision{1},
                               streaming::TokenRevision{1});
    CHECK(streaming::validate_token(old_world, new_world) == TokenVerdict::StaleWorldEpoch);
    streaming::ChunkTokenAuthority authority(SectionCoord{1, 2, 3},
                                             streaming::WorldEpoch{9});
    CHECK(authority.try_bump_block_revision());
    CHECK(authority.snapshot().block_revision().value() == 1);
}

void test_fnv1a_golden()
{
    // Published FNV-1a 64 golden vectors (independently verified).
    CHECK(streaming::fnv1a64(nullptr, 0) == 14695981039346656037ull);
    const std::uint8_t a[] = {'a'};
    CHECK(streaming::fnv1a64(a, sizeof(a)) == 0xaf63dc4c8601ec8cull);
    const std::uint8_t foobar[] = {'f', 'o', 'o', 'b', 'a', 'r'};
    CHECK(streaming::fnv1a64(foobar, sizeof(foobar)) == 0x85944171f73967e8ull);

    // Layout pin: three zeroed int32 coordinates, little-endian bytes.
    const streaming::SectionCoordHash hash;
    CHECK(hash(SectionCoord{0, 0, 0}) == 0x5467b0da1d106495ull);
    CHECK(hash(SectionCoord{1, 0, 0}) == hash(SectionCoord{1, 0, 0}));
    CHECK(hash(SectionCoord{1, 0, 0}) != hash(SectionCoord{0, 0, 1}));
}

void test_snapshot_bounds_checked()
{
    static_assert(!std::is_default_constructible_v<SectionSnapshot>,
                  "workers may only receive frozen snapshots");
    static_assert(!std::is_copy_constructible_v<SectionSnapshotBuilder>,
                  "builders must not create mutable aliases");

    SectionSnapshotBuilder builder(make_token(0, 0, 0));
    builder.set_block(0, 0, 0, BlockState::Grass);
    builder.set_block(15, 15, 15, BlockState::Clay);
    builder.set_face_cell(streaming::FaceIndex::PosX, 15, 15, BlockState::Stone);
    const SectionSnapshotPtr snapshot = std::move(builder).freeze();
    CHECK(snapshot->block(15, 15, 15) == BlockState::Clay);
    CHECK(snapshot->block(0, 0, 0) == BlockState::Grass);
    CHECK(streaming::detail::block_index_unchecked(1, 2, 3) ==
          Section::index_of(BlockCoord{1, 2, 3}));
    CHECK(streaming::detail::block_index_unchecked(15, 15, 15) == 4095);

    bool threw = false;
    try {
        SectionSnapshotBuilder bad(make_token(0, 0, 0));
        bad.set_block(16, 0, 0, BlockState::Dirt);
    } catch (const std::out_of_range&) {
        threw = true;
    }
    CHECK(threw);

    threw = false;
    try {
        (void)snapshot->block(-1, 0, 0);
    } catch (const std::out_of_range&) {
        threw = true;
    }
    CHECK(threw);

    CHECK(snapshot->face_cell(streaming::FaceIndex::PosX, 15, 15) == BlockState::Stone);

    threw = false;
    try {
        SectionSnapshotBuilder bad(make_token(0, 0, 0));
        bad.set_face_cell(static_cast<streaming::FaceIndex>(6), 0, 0,
                          BlockState::Dirt);
    } catch (const std::out_of_range&) {
        threw = true;
    }
    CHECK(threw);

    threw = false;
    try {
        (void)snapshot->face_cell(streaming::FaceIndex::NegY, 16, 0);
    } catch (const std::out_of_range&) {
        threw = true;
    }
    CHECK(threw);

    threw = false;
    try {
        SectionSnapshotBuilder bad(make_token(0, 0, 0));
        bad.set_block(0, 0, 0, static_cast<BlockState>(99));
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    CHECK(threw);
}

void test_distance_extremes_no_overflow()
{
    using Limits = std::numeric_limits<std::int32_t>;
    const std::int32_t mn = Limits::min();
    const std::int32_t mx = Limits::max();

    constexpr std::uint64_t kSingleAxisExact = 18446744065119617025ull; // (2^32-1)^2

    CHECK(streaming::section_distance_squared({0, 0, 0}, {0, 0, 0}) == 0);
    CHECK(streaming::section_distance_squared({mn, 0, 0}, {mx, 0, 0}) ==
          kSingleAxisExact);
    CHECK(streaming::section_distance_squared({mn, 0, 0}, {mx, 1, 0}) ==
          kSingleAxisExact + 1);
    CHECK(streaming::section_distance_squared({mn, mn, mn}, {mx, mx, mx}) ==
          UINT64_MAX);
    CHECK(streaming::section_distance_squared({mn, mn, 0}, {mx, mx, 0}) ==
          UINT64_MAX);
    CHECK(streaming::section_distance_squared({5, -5, 0}, {5, 5, 0}) == 100);

    streaming::PriorityKey far_key{false, UINT64_MAX, SectionCoord{5, 5, 5}};
    streaming::PriorityKey near_key{false, kSingleAxisExact, SectionCoord{9, 9, 9}};
    CHECK(streaming::priority_before(near_key, far_key));
    CHECK(!streaming::priority_before(far_key, near_key));
}

void test_mesh_bytes_checked()
{
    const MeshSizeBytes exact = streaming::checked_mesh_bytes(10, 10);
    CHECK(!exact.overflow);
    CHECK(exact.bytes == 10 * streaming::kVoxelVertexSize + 10 * 4); // 520

    CHECK(streaming::checked_mesh_bytes(0, 0).bytes == 0);
    CHECK(!streaming::checked_mesh_bytes(0, 0).overflow);

    const std::size_t max = SIZE_MAX;

    const MeshSizeBytes vertex_overflow = streaming::checked_mesh_bytes(max / 2, 0);
    CHECK(vertex_overflow.overflow);
    CHECK(vertex_overflow.bytes == max);

    const MeshSizeBytes index_overflow = streaming::checked_mesh_bytes(0, max / 2);
    CHECK(index_overflow.overflow);

    const MeshSizeBytes both_huge = streaming::checked_mesh_bytes(max, max);
    CHECK(both_huge.overflow);
    CHECK(both_huge.bytes == max);

    // Individually representable, sum overflows.
    const MeshSizeBytes sum_overflow =
        streaming::checked_mesh_bytes(max / streaming::kVoxelVertexSize,
                                      max / streaming::kMeshIndexSize);
    CHECK(sum_overflow.overflow);
    CHECK(sum_overflow.bytes == max);

    // Realistic mesh: exact, no overflow.
    OpaqueMesh mesh;
    mesh.vertices.resize(10);
    mesh.indices.resize(10);
    const MeshSizeBytes real = streaming::checked_mesh_bytes(mesh);
    CHECK(!real.overflow);
    CHECK(real.bytes == 520);
}

void test_upload_budget_reasons()
{
    const UploadBudget budget{2, 1000};

    MeshResult result;
    result.ok = true;
    result.token = make_token(0, 0, 0);
    result.mesh.vertices.resize(10);
    result.mesh.indices.resize(10); // 520 bytes

    const ChunkToken authoritative = result.token;
    CHECK(streaming::plan_upload(budget, authoritative, result, 0, 0).accept);
    CHECK(streaming::plan_upload(budget, authoritative, result, 1, 480).accept);

    const auto over_bytes =
        streaming::plan_upload(budget, authoritative, result, 1, 520);
    CHECK(!over_bytes.accept);
    CHECK(over_bytes.reject == UploadReject::FrameBudgetExhausted);

    const auto over_count =
        streaming::plan_upload(budget, authoritative, result, 2, 0);
    CHECK(over_count.reject == UploadReject::FrameBudgetExhausted);

    const ChunkToken bumped = make_token(0, 0, 0, 1, 2, 1);
    const auto stale = streaming::plan_upload(budget, bumped, result, 0, 0);
    CHECK(stale.reject == UploadReject::StaleToken);

    MeshResult failed_result = result;
    failed_result.ok = false;
    failed_result.error = "boom";
    const auto failed =
        streaming::plan_upload(budget, authoritative, failed_result, 0, 0);
    CHECK(failed.reject == UploadReject::FailedResult);

    // Mesh alone above the frame byte budget.
    const MeshSizeBytes huge{9000, false};
    const auto too_large = streaming::plan_upload_decision(
        budget, true, true, huge, 0, 0);
    CHECK(too_large.reject == UploadReject::MeshTooLarge);

    // Saturating size computation: hostile counts.
    const MeshSizeBytes saturated{SIZE_MAX, true};
    const auto overflow = streaming::plan_upload_decision(
        budget, true, true, saturated, 0, 0);
    CHECK(overflow.reject == UploadReject::SizeOverflow);

    // Exact-fit boundary is an accept.
    const MeshSizeBytes fits{1000, false};
    const auto fit = streaming::plan_upload_decision(budget, true, true, fits, 0, 0);
    CHECK(fit.accept);
    const auto fit_full =
        streaming::plan_upload_decision(budget, true, true, fits, 1, 1);
    CHECK(fit_full.reject == UploadReject::FrameBudgetExhausted);
}

void test_task_mailbox_deterministic_order()
{
    streaming::TaskMailbox mailbox(16);
    const SequencedKey scrambled[] = {
        sequenced(3, 0, 0, false, 9, 0),  // C
        sequenced(2, 0, 0, true, 4, 1),   // B
        sequenced(0, 0, 0, true, 0, 2),   // E (best)
        sequenced(1, 0, 0, true, 1, 3),   // A
        sequenced(3, 0, 0, false, 9, 4),  // F (same key as C, later seq)
        sequenced(1, 0, 0, true, 1, 5),   // D (same key as A, later seq)
    };
    for (const SequencedKey& key : scrambled) {
        CHECK(mailbox.try_submit(key, empty_job(make_token(0, 0, 0))) ==
              MailboxStatus::Accepted);
    }

    mailbox.close();
    const int expected_x[] = {0, 1, 1, 2, 3, 3};
    const std::uint64_t expected_seq[] = {2, 3, 5, 1, 0, 4};
    streaming::TaskMailbox::Task task;
    std::size_t popped = 0;
    while (mailbox.pop_wait(task)) {
        CHECK(popped < 6);
        if (popped < 6) {
            CHECK(task.key.key.coord.x == expected_x[popped]);
            CHECK(task.key.sequence == expected_seq[popped]);
        }
        ++popped;
    }
    CHECK(popped == 6);
}

void test_mailbox_capacity_enforced()
{
    streaming::TaskMailbox tasks(3);
    for (int i = 0; i < 3; ++i) {
        CHECK(tasks.try_submit(sequenced(i, 0, 0, false, 0, 0),
                               empty_job(make_token(i, 0, 0))) ==
              MailboxStatus::Accepted);
    }
    CHECK(tasks.size() == 3);
    CHECK(tasks.try_submit(sequenced(9, 0, 0, false, 0, 0),
                           empty_job(make_token(9, 0, 0))) ==
          MailboxStatus::Full);
    CHECK(tasks.size() == 3);

    streaming::TaskMailbox::Task task;
    CHECK(tasks.pop_wait(task));
    CHECK(tasks.size() == 2);
    CHECK(tasks.try_submit(sequenced(9, 0, 0, false, 0, 0),
                           empty_job(make_token(9, 0, 0))) ==
          MailboxStatus::Accepted);
    tasks.close();
    CHECK(tasks.try_submit(sequenced(8, 0, 0, false, 0, 0),
                           empty_job(make_token(8, 0, 0))) ==
          MailboxStatus::Closed);

    streaming::ResultMailbox results(2);
    const auto make_result = [](const char* tag) {
        MeshResult result;
        result.error = tag;
        return result;
    };
    MeshResult first = make_result("r1");
    MeshResult second = make_result("r2");
    MeshResult third = make_result("r3");
    CHECK(results.try_push(std::move(first)) == MailboxStatus::Accepted);
    CHECK(results.try_push(std::move(second)) == MailboxStatus::Accepted);
    CHECK(results.try_push(std::move(third)) == MailboxStatus::Full);
    CHECK(third.error == "r3"); // failed push leaves the value untouched

    MeshResult popped;
    CHECK(results.try_pop(popped) == PopResult::Ok);
    CHECK(popped.error == "r1"); // FIFO
    CHECK(results.try_push(std::move(third)) == MailboxStatus::Accepted);
    CHECK(results.size() == 2);   // ring reuse, capacity respected

    results.close();
    MeshResult rejected = make_result("r4");
    CHECK(results.try_push(std::move(rejected)) == MailboxStatus::Closed);
    CHECK(results.try_pop(popped) == PopResult::Ok);
    CHECK(popped.error == "r2");
    CHECK(results.try_pop(popped) == PopResult::Ok);
    CHECK(popped.error == "r3");
    CHECK(results.try_pop(popped) == PopResult::Closed); // drained
}

void test_unload_reload_rejects_old_generation()
{
    Gate gate;
    GateReleaser releaser{gate};
    Signal started;
    VoxelJobQueue queue({16, 16, 1});

    const ChunkToken submitted = make_token(0, 0, 0, 5, 1, 1);
    CHECK(submit_bounded(
              queue, submitted, make_snapshot(submitted),
              [&](const SectionSnapshot&) {
                  started.notify();
                  if (!gate.wait(Clock::now() + kTestTimeout)) throw std::runtime_error("gate timeout");
                  return OpaqueMesh{};
              },
              true, SectionCoord{0, 0, 0}) == SubmitStatus::Accepted);

    CHECK(started.wait_at_least(1, Clock::now() + kTestTimeout));

    const ChunkToken authoritative = make_token(0, 0, 0, 6, 1, 1);
    gate.open_up();

    const auto results = drain_results(queue, 1);
    CHECK(results.size() == 1);
    if (!results.empty()) {
        CHECK(results[0].ok);
        CHECK(streaming::validate_token(results[0].token, authoritative) ==
              TokenVerdict::StaleGeneration);
        const auto decision =
            streaming::plan_upload(streaming::UploadBudget{}, authoritative,
                                   results[0], 0, 0);
        CHECK(!decision.accept);
        CHECK(decision.reject == UploadReject::StaleToken);
    }
    CHECK(queue.stats().snapshot().pending == 0);
    CHECK(queue.stop() == StopStatus::Joined);
}

void test_revision_bumps_reject_old_mesh()
{
    const ChunkToken submitted = make_token(0, 0, 0, 5, 3, 4);
    VoxelJobQueue queue({16, 16, 1});

    CHECK(submit_bounded(queue, submitted, make_snapshot(submitted),
                         trivial_function(), true, SectionCoord{0, 0, 0}) ==
          SubmitStatus::Accepted);

    const auto results = drain_results(queue, 1);
    CHECK(results.size() == 1);
    if (results.empty()) return;

    const ChunkToken block_bumped = make_token(0, 0, 0, 5, 4, 4);
    CHECK(streaming::validate_token(results[0].token, block_bumped) ==
          TokenVerdict::StaleBlockRevision);

    const ChunkToken neighbor_bumped = make_token(0, 0, 0, 5, 3, 5);
    CHECK(streaming::validate_token(results[0].token, neighbor_bumped) ==
          TokenVerdict::StaleNeighborhoodRevision);
    CHECK(streaming::plan_upload(UploadBudget{}, block_bumped, results[0], 0, 0)
              .reject == UploadReject::StaleToken);
    CHECK(streaming::plan_upload(UploadBudget{}, neighbor_bumped, results[0], 0, 0)
              .reject == UploadReject::StaleToken);
    CHECK(queue.stop() == StopStatus::Joined);
}

void test_queue_full_never_blocks_main()
{
    Gate gate;
    GateReleaser releaser{gate};
    Signal started;
    VoxelJobQueue queue({4, 16, 1});

    const ChunkToken blocker_token = make_token(0, 0, 0); // best key
    CHECK(submit_bounded(
              queue, blocker_token, make_snapshot(blocker_token),
              [&](const SectionSnapshot&) {
                  started.notify();
                  if (!gate.wait(Clock::now() + kTestTimeout)) throw std::runtime_error("gate timeout");
                  return OpaqueMesh{};
              },
              true, SectionCoord{0, 0, 0}) == SubmitStatus::Accepted);
    CHECK(started.wait_at_least(1, Clock::now() + kTestTimeout));

    for (int i = 1; i <= 4; ++i) {
        const ChunkToken token = make_token(i, 0, 0);
        CHECK(queue.try_submit(token, make_snapshot(token), trivial_function(),
                               true, SectionCoord{0, 0, 0}) ==
              SubmitStatus::Accepted);
    }
    // Task queue (capacity 4) is full: this returns false immediately — a
    // blocking submit would deadlock the test here.
    const ChunkToken overflow = make_token(99, 0, 0);
    CHECK(queue.try_submit(overflow, make_snapshot(overflow), trivial_function(),
                           true, SectionCoord{0, 0, 0}) == SubmitStatus::Full);

    const auto stats = queue.stats().snapshot();
    CHECK(stats.submitted == 5);
    CHECK(stats.task_queue_full == 1);
    CHECK(stats.queue_full == 1);
    CHECK(stats.pending == 5);

    gate.open_up();
    const auto results = drain_results(queue, 5);
    CHECK(results.size() == 5);
    CHECK(queue.stats().snapshot().completed == 5);
    CHECK(queue.stats().snapshot().pending == 0);
    CHECK(queue.stop() == StopStatus::Joined);
}

void test_priority_end_to_end_order()
{
    Gate gate;
    GateReleaser releaser{gate};
    Signal started;
    VoxelJobQueue queue({32, 32, 1});

    const ChunkToken blocker = make_token(0, 0, 0); // frustum, distance 0
    CHECK(submit_bounded(
              queue, blocker, make_snapshot(blocker),
              [&](const SectionSnapshot&) {
                  started.notify();
                  if (!gate.wait(Clock::now() + kTestTimeout)) throw std::runtime_error("gate timeout");
                  return OpaqueMesh{};
              },
              true, SectionCoord{0, 0, 0}) == SubmitStatus::Accepted);
    CHECK(started.wait_at_least(1, Clock::now() + kTestTimeout));

    const struct
    {
        int x;
        bool in_frustum;
        std::uint32_t revision;
    } submissions[] = {
        {3, false, 1},  // C: non-frustum, d9
        {-3, false, 1}, // D: non-frustum, d9, lex before C
        {5, false, 1},  // G1: non-frustum, d25
        {2, true, 1},   // B: frustum, d4
        {1, true, 1},   // A: frustum, d1
        {5, false, 2},  // G2: same key as G1, later sequence
    };
    for (const auto& entry : submissions) {
        const ChunkToken token = make_token(entry.x, 0, 0, 1, entry.revision, 1);
        CHECK(queue.try_submit(token, make_snapshot(token), trivial_function(),
                               entry.in_frustum, SectionCoord{0, 0, 0}) ==
              SubmitStatus::Accepted);
    }

    gate.open_up();
    const auto results = drain_results(queue, 7);
    CHECK(results.size() == 7);
    const struct
    {
        int x;
        std::uint32_t revision;
    } expected[] = {
        {0, 1},  // blocker: frustum, d0
        {1, 1},  // A: frustum, d1
        {2, 1},  // B: frustum, d4
        {-3, 1}, // D: non-frustum, d9, lex first
        {3, 1},  // C: non-frustum, d9
        {5, 1},  // G1: d25, earlier sequence
        {5, 2},  // G2: d25, later sequence
    };
    for (std::size_t i = 0; i < results.size() && i < 7; ++i) {
        CHECK(results[i].token.coord().x == expected[i].x);
        CHECK(results[i].token.block_revision().value() == expected[i].revision);
    }
    CHECK(queue.stop() == StopStatus::Joined);
}

void test_worker_exception_observable()
{
    VoxelJobQueue queue({8, 8, 1});

    const ChunkToken throwing = make_token(0, 0, 0, 1, 1, 1);
    CHECK(submit_bounded(
              queue, throwing, make_snapshot(throwing),
              [](const SectionSnapshot&) -> OpaqueMesh {
                  throw std::runtime_error("mesh exploded");
              },
              true, SectionCoord{0, 0, 0}) == SubmitStatus::Accepted);

    const ChunkToken alloc = make_token(0, 0, 0, 1, 2, 1);
    CHECK(submit_bounded(
              queue, alloc, make_snapshot(alloc),
              [](const SectionSnapshot&) -> OpaqueMesh { throw std::bad_alloc(); },
              true, SectionCoord{0, 0, 0}) == SubmitStatus::Accepted);

    const ChunkToken alien = make_token(0, 0, 0, 1, 3, 1);
    CHECK(submit_bounded(
              queue, alien, make_snapshot(alien),
              [](const SectionSnapshot&) -> OpaqueMesh { throw 42; },
              true, SectionCoord{0, 0, 0}) == SubmitStatus::Accepted);

    const auto results = drain_results(queue, 3);
    CHECK(results.size() == 3);
    if (results.size() == 3) {
        CHECK(!results[0].ok);
        CHECK(results[0].error == "mesh exploded");
        CHECK(!results[1].ok); // bad_alloc: message best effort
        CHECK(!results[1].error.empty());
        CHECK(!results[2].ok);
        CHECK(results[2].error == "unknown exception in cpu voxel job");
    }

    const ChunkToken healthy = make_token(0, 0, 0, 1, 4, 1);
    CHECK(submit_bounded(queue, healthy, make_snapshot(healthy),
                         trivial_function(), true, SectionCoord{0, 0, 0}) ==
          SubmitStatus::Accepted);
    const auto more = drain_results(queue, 1);
    CHECK(more.size() == 1);
    CHECK(more[0].ok);

    const auto stats = queue.stats().snapshot();
    CHECK(stats.failed == 3);
    CHECK(stats.completed == 1);
    CHECK(stats.internal_failure == 0);
    CHECK(stats.pending == 0);
    CHECK(queue.stop() == StopStatus::Joined);
}

// Opens the gate as soon as the queue enters Stopping, so the owner's
// synchronous stop() barrier can complete while its in-flight jobs park on
// the gate. Without this the join would (correctly) wait forever.
struct StopGateOpener
{
    std::thread thread;
    explicit StopGateOpener(VoxelJobQueue& queue, Gate& gate)
        : thread([&queue, &gate] {
              (void)wait_until([&] {
                  return queue.state() == Lifecycle::Stopping;
              });
              gate.open_up();
          })
    {
    }
    ~StopGateOpener() { thread.join(); }
};

void test_stop_barrier_semantics()
{
    Gate gate;
    GateReleaser releaser{gate};
    Signal started;
    VoxelJobQueue queue({8, 8, 1});
    std::atomic<int> entered_queued{0};

    // Three fast jobs complete and publish; results stay unpopped.
    for (int i = 0; i < 3; ++i) {
        const ChunkToken token = make_token(0, 0, 0, 1,
                                            static_cast<std::uint32_t>(i + 1), 1);
        CHECK(submit_bounded(queue, token, make_snapshot(token),
                             trivial_function(), true,
                             SectionCoord{0, 0, 0}) == SubmitStatus::Accepted);
    }
    CHECK(wait_until([&] { return queue.stats().snapshot().completed >= 3; }));

    // In-flight job parks on the gate (same key, next sequence).
    const ChunkToken inflight = make_token(0, 0, 0, 1, 4, 1);
    CHECK(submit_bounded(
              queue, inflight, make_snapshot(inflight),
              [&](const SectionSnapshot&) {
                  started.notify();
                  if (!gate.wait(Clock::now() + kTestTimeout)) throw std::runtime_error("gate timeout");
                  return OpaqueMesh{};
              },
              true, SectionCoord{0, 0, 0}) == SubmitStatus::Accepted);
    CHECK(started.wait_at_least(1, Clock::now() + kTestTimeout));

    // Two queued jobs whose functions must never run.
    for (int i = 0; i < 2; ++i) {
        const ChunkToken token = make_token(0, 0, 0, 1,
                                            static_cast<std::uint32_t>(5 + i), 1);
        CHECK(queue.try_submit(
                  token, make_snapshot(token),
                  [&entered_queued](const SectionSnapshot&) {
                      entered_queued.fetch_add(1);
                      return OpaqueMesh{};
                  },
                  true, SectionCoord{0, 0, 0}) == SubmitStatus::Accepted);
    }
    CHECK(queue.stats().snapshot().pending == 6);

    {
        StopGateOpener opener(queue, gate);
        CHECK(queue.stop() == StopStatus::Joined); // blocks until joined
    }
    CHECK(queue.state() == Lifecycle::Stopped);
    CHECK(queue.stop() == StopStatus::Joined); // idempotent

    // Results published before the stop point drain afterwards...
    MeshResult result;
    int popped = 0;
    while (queue.try_pop_result(result) == PopStatus::Ok) ++popped;
    CHECK(popped == 3);
    // ...and nothing published after the cutoff can enter.
    CHECK(queue.try_pop_result(result) == PopStatus::Closed);
    CHECK(queue.try_submit(make_token(7, 7, 7), make_snapshot(make_token(7, 7, 7)),
                           trivial_function(), true,
                           SectionCoord{0, 0, 0}) == SubmitStatus::Closed);

    const auto stats = queue.stats().snapshot();
    CHECK(stats.submitted == 6);
    CHECK(stats.completed == 3);
    CHECK(stats.cancelled == 2);      // queued jobs
    CHECK(stats.dropped_closed == 1); // in-flight job's result
    CHECK(stats.pending == 0);
    CHECK(stats.invalid == 0);
    CHECK(stats.internal_failure == 0);
    CHECK(entered_queued.load() == 0); // cancelled jobs never executed
}

void test_stop_wakes_full_result_queue_and_joins()
{
    Signal push_wait_entered;
    VoxelJobQueue::Config config;
    config.task_capacity = 16;
    config.result_capacity = 2;
    config.worker_count = 1;
    config.result_push_wait_hook = [&] { push_wait_entered.notify(); };
    VoxelJobQueue queue(config);
    Signal finished_functions;

    for (int i = 0; i < 5; ++i) {
        const ChunkToken token = make_token(0, 0, 0, 1,
                                            static_cast<std::uint32_t>(i + 1), 1);
        CHECK(submit_bounded(
                  queue, token, make_snapshot(token),
                  [&finished_functions](const SectionSnapshot&) {
                      finished_functions.notify();
                      return OpaqueMesh{};
                  },
                  true, SectionCoord{0, 0, 0}) == SubmitStatus::Accepted);
    }

    // Jobs 1-2 fill the result ring (capacity 2); job 3's push observes a
    // full queue and parks the worker in push_wait.
    CHECK(finished_functions.wait_at_least(3, Clock::now() + kTestTimeout));
    CHECK(push_wait_entered.wait_at_least(1, Clock::now() + kTestTimeout));

    // stop() must wake the parked pusher (close) and join synchronously; a
    // missed wakeup would hang here and hit the external test timeout.
    CHECK(queue.stop() == StopStatus::Joined);
    CHECK(queue.state() == Lifecycle::Stopped);

    MeshResult result;
    int popped = 0;
    while (queue.try_pop_result(result) == PopStatus::Ok) ++popped;
    CHECK(popped == 2); // the two published before the cutoff

    const auto stats = queue.stats().snapshot();
    CHECK(stats.submitted == 5);
    CHECK(stats.dropped_closed == 1); // job 3 finished during stop
    CHECK(stats.cancelled == 2);       // jobs 4-5 cancelled
    CHECK(stats.pending == 0);
}

void test_stop_interleavings_accounted()
{
    Gate gate;
    GateReleaser releaser{gate};
    Signal started;
    std::atomic<int> active{0};
    struct ActiveGuard
    {
        std::atomic<int>& counter;
        explicit ActiveGuard(std::atomic<int>& c) : counter(c)
        {
            counter.fetch_add(1);
        }
        ~ActiveGuard() { counter.fetch_sub(1); }
    };

    VoxelJobQueue queue({8, 8, 2});
    const auto gated_function = [&](const SectionSnapshot&) {
        ActiveGuard guard(active);
        started.notify();
        if (!gate.wait(Clock::now() + kTestTimeout)) throw std::runtime_error("gate timeout");
        return OpaqueMesh{};
    };

    // Both workers run in-flight gated jobs; three more queue behind them.
    // Submits use the Busy-retry helper: with two workers an idle worker
    // can be transiently inside its pop critical section, which yields
    // Busy by design.
    const ChunkToken first = make_token(0, 0, 0);
    const ChunkToken second = make_token(1, 0, 0);
    CHECK(submit_bounded(queue, first, make_snapshot(first), gated_function,
                         true, SectionCoord{0, 0, 0}) == SubmitStatus::Accepted);
    CHECK(submit_bounded(queue, second, make_snapshot(second), gated_function,
                         true, SectionCoord{0, 0, 0}) == SubmitStatus::Accepted);
    CHECK(started.wait_at_least(2, Clock::now() + kTestTimeout));

    std::atomic<int> entered_queued{0};
    for (int i = 0; i < 3; ++i) {
        const ChunkToken token = make_token(10 + i, 0, 0);
        CHECK(submit_bounded(
                  queue, token, make_snapshot(token),
                  [&entered_queued](const SectionSnapshot&) {
                      entered_queued.fetch_add(1);
                      return OpaqueMesh{};
                  },
                  true, SectionCoord{0, 0, 0}) == SubmitStatus::Accepted);
    }
    CHECK(queue.stats().snapshot().pending == 5);

    {
        StopGateOpener opener(queue, gate);
        CHECK(queue.stop() == StopStatus::Joined);
    }
    // stop() returned: every worker joined, so no job code is running and
    // captured state may be destroyed (the guards below are still alive
    // only to observe this fact).
    CHECK(active.load() == 0);

    const auto stats = queue.stats().snapshot();
    CHECK(stats.submitted == 5);
    CHECK(stats.cancelled == 3);
    CHECK(stats.dropped_closed == 2);
    CHECK(stats.pending == 0);
    CHECK(entered_queued.load() == 0);
}

void test_wrong_thread_contract()
{
    VoxelJobQueue queue({8, 8, 1});
    const ChunkToken token = make_token(0, 0, 0);

    std::atomic<int> wrong_submits{0};
    std::atomic<int> wrong_pops{0};
    std::atomic<int> wrong_stops{0};
    std::thread foreign([&] {
        MeshResult result;
        if (queue.try_submit(token, make_snapshot(token), trivial_function(),
                             true, SectionCoord{0, 0, 0}) ==
            SubmitStatus::WrongThread) {
            wrong_submits.fetch_add(1);
        }
        if (queue.try_pop_result(result) == PopStatus::WrongThread) {
            wrong_pops.fetch_add(1);
        }
        if (queue.stop() == StopStatus::WrongThread) {
            wrong_stops.fetch_add(1);
        }
    });
    foreign.join();

    CHECK(wrong_submits.load() == 1);
    CHECK(wrong_pops.load() == 1);
    CHECK(wrong_stops.load() == 1);
    CHECK(queue.state() == Lifecycle::Running); // wrong-thread stop: no change

    const auto stats = queue.stats().snapshot();
    CHECK(stats.submitted == 0); // rejected submit left no trace
    CHECK(stats.pending == 0);

    CHECK(queue.stop() == StopStatus::Joined);
    CHECK(queue.state() == Lifecycle::Stopped);
}

void test_thread_factory_failure_injection()
{
    Signal entered;
    std::atomic<int> created{0};

    VoxelJobQueue::Config config;
    config.task_capacity = 8;
    config.result_capacity = 8;
    config.worker_count = 3;
    config.thread_factory = [&](unsigned id, VoxelJobQueue::WorkerLoop loop)
        -> std::thread {
        if (created.fetch_add(1) + 1 == 3) {
            throw std::runtime_error("injected thread creation failure");
        }
        return std::thread([loop, id, &entered] {
            entered.notify();
            loop(id);
        });
    };

    bool threw = false;
    try {
        VoxelJobQueue queue(config);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    CHECK(threw);
    CHECK(created.load() == 3); // third creation failed

    // The two already-created workers reached their loop and were closed
    // and joined before the constructor rethrew.
    CHECK(entered.wait_at_least(2, Clock::now() + kTestTimeout));
    CHECK(wait_until([&] { return entered.value() == 2; }));

    VoxelJobQueue::Config non_joinable;
    non_joinable.task_capacity = 8;
    non_joinable.result_capacity = 8;
    non_joinable.worker_count = 1;
    non_joinable.thread_factory = [](unsigned, VoxelJobQueue::WorkerLoop) {
        return std::thread{};
    };
    threw = false;
    try {
        VoxelJobQueue queue(non_joinable);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    CHECK(threw);
}

void test_callable_dtor_reentrancy_no_deadlock()
{
    // Mailbox level: job callables whose destructors re-enter the mailbox
    // run outside the mailbox lock (discard_all swaps under the lock and
    // destroys after releasing it), so cancellation cannot deadlock.
    struct MailboxPoker
    {
        streaming::TaskMailbox* mailbox;
        std::atomic<int>* count;
        MailboxPoker(streaming::TaskMailbox* m, std::atomic<int>* c)
            : mailbox(m), count(c)
        {
        }
        ~MailboxPoker()
        {
            count->fetch_add(1);
            (void)mailbox->size(); // takes the same mutex
            (void)mailbox->try_submit(sequenced(0, 0, 0, false, 0, 0),
                                      streaming::VoxelJob{make_token(0, 0, 0),
                                                          nullptr, {}});
        }
    };

    {
        streaming::TaskMailbox mailbox(8);
        std::atomic<int> reentered{0};
        for (int i = 0; i < 3; ++i) {
            streaming::VoxelJob job;
            job.token = make_token(i, 0, 0);
            job.snapshot = make_snapshot(job.token);
            std::shared_ptr<MailboxPoker> poker =
                std::make_shared<MailboxPoker>(&mailbox, &reentered);
            job.function = [poker](const SectionSnapshot&) { return OpaqueMesh{}; };
            CHECK(mailbox.try_submit(sequenced(i, 0, 0, false, 0, 0),
                                     std::move(job)) == MailboxStatus::Accepted);
        }
        CHECK(mailbox.discard_all() == 3); // destructors re-enter: no deadlock
        CHECK(reentered.load() == 3);
    }

    {
        // Pop path: the job object is destroyed after the pop released the
        // lock, so its callable destructor may also re-enter freely.
        streaming::TaskMailbox mailbox(8);
        std::atomic<int> reentered{0};
        streaming::VoxelJob job;
        job.token = make_token(0, 0, 0);
        job.snapshot = make_snapshot(job.token);
        std::shared_ptr<MailboxPoker> poker =
            std::make_shared<MailboxPoker>(&mailbox, &reentered);
        job.function = [poker](const SectionSnapshot&) { return OpaqueMesh{}; };
        CHECK(mailbox.try_submit(sequenced(0, 0, 0, false, 0, 0), std::move(job)) ==
              MailboxStatus::Accepted);
        poker.reset(); // callable now owns the final reference

        streaming::TaskMailbox::Task task;
        CHECK(mailbox.pop_wait(task));
        streaming::VoxelJob successor;
        successor.token = make_token(1, 0, 0);
        successor.snapshot = make_snapshot(successor.token);
        successor.function = trivial_function();
        CHECK(mailbox.try_submit(sequenced(1, 0, 0, false, 0, 1),
                                 std::move(successor)) == MailboxStatus::Accepted);
        // The second pop replaces an out holding the callable's final
        // reference. Its destructor must run after the mailbox unlocks.
        CHECK(mailbox.pop_wait(task));
        CHECK(reentered.load() == 1);
        mailbox.close();
    }
}

void test_sequence_never_wraps()
{
    VoxelJobQueue::Config config;
    config.task_capacity = 8;
    config.result_capacity = 8;
    config.worker_count = 1;
    config.initial_sequence = UINT64_MAX - 2; // two usable sequences remain
    VoxelJobQueue queue(config);

    const ChunkToken first = make_token(0, 0, 0, 1, 1, 1);
    const ChunkToken second = make_token(0, 0, 0, 1, 2, 1);
    const ChunkToken third = make_token(0, 0, 0, 1, 3, 1);
    CHECK(submit_bounded(queue, first, make_snapshot(first), trivial_function(),
                         true, SectionCoord{0, 0, 0}) == SubmitStatus::Accepted);
    CHECK(submit_bounded(queue, second, make_snapshot(second),
                         trivial_function(), true,
                         SectionCoord{0, 0, 0}) == SubmitStatus::Accepted);
    CHECK(submit_bounded(queue, third, make_snapshot(third), trivial_function(),
                         true, SectionCoord{0, 0, 0}) ==
          SubmitStatus::SequenceExhausted);

    const auto results = drain_results(queue, 2);
    CHECK(results.size() == 2);
    const auto stats = queue.stats().snapshot();
    CHECK(stats.submitted == 2);
    CHECK(stats.pending == 0);
    CHECK(queue.stop() == StopStatus::Joined);
}

void test_concurrent_pending_never_negative()
{
    VoxelJobQueue queue({256, 256, 2});
    constexpr int kJobs = 200;

    std::atomic<bool> sampling{true};
    std::atomic<std::int64_t> min_pending{0};
    std::thread sampler([&] {
        while (sampling.load(std::memory_order_relaxed)) {
            const std::int64_t pending =
                queue.stats().snapshot().pending;
            std::int64_t observed = min_pending.load(std::memory_order_relaxed);
            while (pending < observed &&
                   !min_pending.compare_exchange_weak(observed, pending)) {
            }
        }
    });

    for (int i = 0; i < kJobs; ++i) {
        const ChunkToken token =
            make_token(0, 0, 0, 1, static_cast<std::uint32_t>(i + 1), 1);
        const SubmitStatus status =
            submit_bounded(queue, token, make_snapshot(token),
                           trivial_function(), true, SectionCoord{0, 0, 0});
        CHECK(status == SubmitStatus::Accepted);
    }
    const auto results = drain_results(queue, kJobs);
    CHECK(results.size() == kJobs);

    sampling.store(false);
    sampler.join();

    CHECK(min_pending.load() >= 0); // never negative under concurrent sampling
    const auto stats = queue.stats().snapshot();
    CHECK(stats.pending == 0);
    CHECK(stats.completed == kJobs);
    CHECK(queue.stop() == StopStatus::Joined);
}

void test_config_and_stats_basics()
{
    const unsigned workers = VoxelJobQueue::default_worker_count();
    CHECK(workers >= 1);
    CHECK(workers <= 4);

    {
        bool threw = false;
        try {
            VoxelJobQueue queue({0, 4, 1});
            (void)queue;
        } catch (const std::invalid_argument&) {
            threw = true;
        }
        CHECK(threw); // capacities never silently clamp

        threw = false;
        try {
            VoxelJobQueue queue({4, 4, 33});
            (void)queue;
        } catch (const std::invalid_argument&) {
            threw = true;
        }
        CHECK(threw);

        threw = false;
        try {
            VoxelJobQueue queue({4, 4, -2});
            (void)queue;
        } catch (const std::invalid_argument&) {
            threw = true;
        }
        CHECK(threw);
    }

    {
        // Zero workers: pure buffering mode, deterministic without races.
        VoxelJobQueue queue({4, 4, 0});
        const ChunkToken token = make_token(0, 0, 0);
        CHECK(queue.try_submit(token, make_snapshot(token), trivial_function(),
                               true, SectionCoord{0, 0, 0}) ==
              SubmitStatus::Accepted);
        MeshResult result;
        CHECK(queue.try_pop_result(result) == PopStatus::Empty);
        CHECK(queue.stop() == StopStatus::Joined);
        CHECK(queue.try_pop_result(result) == PopStatus::Closed);
        CHECK(queue.try_submit(token, make_snapshot(token), trivial_function(),
                               true, SectionCoord{0, 0, 0}) ==
              SubmitStatus::Closed);
        const auto stats = queue.stats().snapshot();
        CHECK(stats.cancelled == 1); // the buffered task was discarded
        CHECK(stats.pending == 0);
    }
}

void test_submit_invalid_rejections()
{
    VoxelJobQueue queue({8, 8, 1});
    const ChunkToken token = make_token(0, 0, 0);

    // Null snapshot.
    CHECK(queue.try_submit(token, nullptr, trivial_function(), true,
                           SectionCoord{0, 0, 0}) == SubmitStatus::Invalid);
    // Empty callable.
    CHECK(queue.try_submit(token, make_snapshot(token), streaming::CpuSectionFn{},
                           true, SectionCoord{0, 0, 0}) == SubmitStatus::Invalid);
    // Snapshot token does not match the submitted token.
    const ChunkToken other = make_token(5, 5, 5);
    CHECK(queue.try_submit(other, make_snapshot(token), trivial_function(), true,
                           SectionCoord{0, 0, 0}) == SubmitStatus::Invalid);

    const auto stats = queue.stats().snapshot();
    CHECK(stats.invalid == 3);
    CHECK(stats.submitted == 0);
    CHECK(stats.pending == 0);

    CHECK(queue.stop() == StopStatus::Joined);
}

void test_queue_level_dtor_reentrancy_no_deadlock()
{
    // Queue level: a callable whose captured object's destructor touches
    // queue APIs from the worker thread while the queue is still alive.
    VoxelJobQueue queue({8, 8, 1});
    Gate gate;
    GateReleaser releaser{gate};
    Signal started;

    struct QueuePoker
    {
        VoxelJobQueue* queue;
        explicit QueuePoker(VoxelJobQueue* q) : queue(q) {}
        ~QueuePoker()
        {
            // Runs on the worker thread when the job object is destroyed
            // (outside every mailbox lock).
            (void)queue->stats().snapshot();
            MeshResult result;
            (void)queue->try_pop_result(result); // WrongThread, no state change
        }
    };

    const ChunkToken poker_token = make_token(0, 0, 0, 1, 1, 1);
    std::shared_ptr<QueuePoker> poker = std::make_shared<QueuePoker>(&queue);
    CHECK(submit_bounded(
              queue, poker_token, make_snapshot(poker_token),
              [poker, &started, &gate](const SectionSnapshot&) {
                  started.notify();
                  if (!gate.wait(Clock::now() + kTestTimeout)) throw std::runtime_error("gate timeout");
                  return OpaqueMesh{};
              },
              true, SectionCoord{0, 0, 0}) == SubmitStatus::Accepted);
    CHECK(started.wait_at_least(1, Clock::now() + kTestTimeout));
    gate.open_up();

    // If the destructor deadlocked against the queue, this follow-up job
    // would never complete and the drain deadline would fail.
    const ChunkToken follow = make_token(0, 0, 0, 1, 2, 1);
    CHECK(submit_bounded(queue, follow, make_snapshot(follow),
                         trivial_function(), true,
                         SectionCoord{0, 0, 0}) == SubmitStatus::Accepted);
    const auto results = drain_results(queue, 2);
    CHECK(results.size() == 2);
    CHECK(queue.stop() == StopStatus::Joined);
    poker.reset(); // deterministic destruction point, after stop
}

void test_reentrant_owner_stop_does_not_finish_early()
{
    VoxelJobQueue queue({8, 8, 0});
    std::atomic<int> inner{static_cast<int>(StopStatus::WrongThread)};
    struct StopPoker
    {
        VoxelJobQueue* queue = nullptr;
        std::atomic<int>* inner = nullptr;
        ~StopPoker()
        {
            if (queue) inner->store(static_cast<int>(queue->stop()), std::memory_order_release);
        }
    };

    const ChunkToken token = make_token(0, 0, 0);
    std::shared_ptr<StopPoker> poker = std::make_shared<StopPoker>();
    poker->queue = &queue;
    poker->inner = &inner;
    CHECK(submit_bounded(queue, token, make_snapshot(token),
                         [poker](const SectionSnapshot&) { return OpaqueMesh{}; },
                         true, SectionCoord{}) == SubmitStatus::Accepted);
    poker.reset(); // queued callable owns the final reference
    CHECK(queue.stop() == StopStatus::Joined);
    CHECK(inner.load(std::memory_order_acquire) == static_cast<int>(StopStatus::Reentrant));
    CHECK(queue.state() == Lifecycle::Stopped);
}

void test_submit_sequence_consumed_only_on_accept_and_busy_is_deterministic()
{
    {
        VoxelJobQueue::Config config;
        config.task_capacity = 1;
        config.result_capacity = 1;
        config.worker_count = 0;
        config.initial_sequence = 9;
        VoxelJobQueue queue(config);
        const ChunkToken first = make_token(0, 0, 0);
        const ChunkToken second = make_token(1, 0, 0);
        CHECK(submit_bounded(queue, first, make_snapshot(first), trivial_function(), true,
                             SectionCoord{}) == SubmitStatus::Accepted);
        CHECK(queue.next_sequence_for_testing() == 10);
        CHECK(submit_bounded(queue, second, make_snapshot(second), trivial_function(), true,
                             SectionCoord{}) == SubmitStatus::Full);
        CHECK(queue.next_sequence_for_testing() == 10); // Full does not burn a sequence
        CHECK(queue.stop() == StopStatus::Joined);
    }

    Gate pop_locked;
    GateReleaser releaser{pop_locked};
    Signal hook_entered;
    VoxelJobQueue::Config config;
    config.task_capacity = 4;
    config.result_capacity = 4;
    config.worker_count = 1;
    config.task_pop_hook = [&] {
        hook_entered.notify();
        if (!pop_locked.wait(Clock::now() + kTestTimeout)) throw std::runtime_error("pop hook timeout");
    };
    VoxelJobQueue queue(config);
    const ChunkToken first = make_token(0, 0, 0);
    const ChunkToken second = make_token(1, 0, 0);
    CHECK(submit_bounded(queue, first, make_snapshot(first), trivial_function(), true,
                         SectionCoord{}) == SubmitStatus::Accepted);
    CHECK(hook_entered.wait_at_least(1, Clock::now() + kTestTimeout));
    CHECK(queue.try_submit(second, make_snapshot(second), trivial_function(), true,
                           SectionCoord{}) == SubmitStatus::Busy);
    CHECK(queue.next_sequence_for_testing() == 1); // Busy also does not burn a sequence
    pop_locked.open_up();
    CHECK(drain_results(queue, 1).size() == 1);
    CHECK(queue.stop() == StopStatus::Joined);
}

void test_result_cutoff_and_worker_fault_boundaries()
{
    Gate worker_gate;
    GateReleaser release_worker{worker_gate};
    Signal worker_entered;
    Signal cutoff;
    std::atomic<bool> cutoff_closed{false};
    std::atomic<bool> cutoff_still_running{false};
    VoxelJobQueue* cutoff_queue = nullptr;
    VoxelJobQueue::Config config;
    config.task_capacity = 4;
    config.result_capacity = 2;
    config.worker_count = 1;
    config.result_cutoff_hook = [&] {
        cutoff_closed.store(cutoff_queue->result_publication_closed_for_testing());
        cutoff_still_running.store(cutoff_queue->state() == Lifecycle::Running);
        cutoff.notify();
    };
    VoxelJobQueue queue(config);
    cutoff_queue = &queue;

    const ChunkToken pre = make_token(0, 0, 0);
    CHECK(submit_bounded(queue, pre, make_snapshot(pre), trivial_function(), true,
                         SectionCoord{}) == SubmitStatus::Accepted);
    CHECK(wait_until([&] { return queue.stats().snapshot().completed == 1; }));

    const ChunkToken post = make_token(1, 0, 0);
    CHECK(submit_bounded(queue, post, make_snapshot(post),
                         [&](const SectionSnapshot&) {
                             worker_entered.notify();
                             if (!worker_gate.wait(Clock::now() + kTestTimeout)) {
                                 throw std::runtime_error("worker gate timeout");
                             }
                             return OpaqueMesh{};
                         }, true, SectionCoord{}) == SubmitStatus::Accepted);
    CHECK(worker_entered.wait_at_least(1, Clock::now() + kTestTimeout));

    std::thread release_after_cutoff([&] {
        if (cutoff.wait_at_least(1, Clock::now() + kTestTimeout)) worker_gate.open_up();
    });
    CHECK(queue.stop() == StopStatus::Joined);
    release_after_cutoff.join();
    MeshResult result;
    CHECK(queue.try_pop_result(result) == PopStatus::Ok); // published before cutoff drains
    CHECK(queue.try_pop_result(result) == PopStatus::Closed);
    CHECK(queue.stats().snapshot().dropped_closed == 1);
    CHECK(cutoff_closed.load());
    CHECK(cutoff_still_running.load());

    std::atomic<bool> injected{false};
    VoxelJobQueue::Config fault_config;
    fault_config.task_capacity = 4;
    fault_config.result_capacity = 4;
    fault_config.worker_count = 1;
    fault_config.fault_injector = [&](streaming::WorkerFaultPoint point) {
        if (point == streaming::WorkerFaultPoint::BeforeExecute &&
            !injected.exchange(true)) {
            throw std::bad_alloc();
        }
    };
    VoxelJobQueue fault_queue(fault_config);
    const ChunkToken rejected = make_token(2, 0, 0);
    const ChunkToken delivered = make_token(3, 0, 0);
    CHECK(submit_bounded(fault_queue, rejected, make_snapshot(rejected), trivial_function(),
                         true, SectionCoord{}) == SubmitStatus::Accepted);
    CHECK(wait_until([&] { return fault_queue.stats().snapshot().pending == 0; }));
    CHECK(submit_bounded(fault_queue, delivered, make_snapshot(delivered), trivial_function(),
                         true, SectionCoord{}) == SubmitStatus::Accepted);
    CHECK(drain_results(fault_queue, 1).size() == 1);
    CHECK(fault_queue.stats().snapshot().internal_failure >= 1);
    CHECK(fault_queue.stats().snapshot().pending == 0);
    CHECK(fault_queue.stop() == StopStatus::Joined);
}

void test_upload_frame_queue_retains_and_accounts_once()
{
    streaming::StreamingStats stats;
    streaming::UploadFrameQueue queue({1, 520}, stats);
    MeshResult result;
    result.token = make_token(0, 0, 0);
    result.ok = true;
    result.mesh.vertices.resize(10);
    result.mesh.indices.resize(10);
    queue.enqueue(std::move(result));
    MeshResult taken;
    CHECK(queue.try_take(make_token(0, 0, 0), taken) == streaming::UploadDisposition::Accepted);
    CHECK(queue.uploads_this_frame() == 1);
    CHECK(queue.bytes_this_frame() == 520);
    CHECK(stats.snapshot().accepted == 1);

    queue.enqueue(std::move(taken));
    CHECK(queue.try_take(make_token(0, 0, 0), taken) ==
          streaming::UploadDisposition::FrameBudgetExhausted);
    CHECK(queue.deferred_count() == 1);
    CHECK(queue.uploads_this_frame() == 1);
    CHECK(stats.snapshot().deferred == 1);
    CHECK(queue.try_take(make_token(0, 0, 0), taken) ==
          streaming::UploadDisposition::FrameBudgetExhausted);
    CHECK(stats.snapshot().deferred == 1); // same retained result counts once
    queue.begin_frame();
    CHECK(queue.uploads_this_frame() == 0);
    CHECK(queue.bytes_this_frame() == 0);
    CHECK(queue.try_take(make_token(0, 0, 0), taken) == streaming::UploadDisposition::Accepted);
    CHECK(queue.deferred_count() == 0);

    MeshResult failed;
    failed.token = make_token(0, 0, 0);
    failed.ok = false;
    queue.enqueue(std::move(failed));
    CHECK(queue.try_take(make_token(0, 0, 0), taken) == streaming::UploadDisposition::FailedResult);
}

struct NamedTest
{
    const char* name;
    void (*fn)();
};

} // namespace

int main()
{
    const NamedTest tests[] = {
        {"token_validation_pure", test_token_validation_pure},
        {"fnv1a_golden", test_fnv1a_golden},
        {"snapshot_bounds_checked", test_snapshot_bounds_checked},
        {"distance_extremes_no_overflow", test_distance_extremes_no_overflow},
        {"mesh_bytes_checked", test_mesh_bytes_checked},
        {"upload_budget_reasons", test_upload_budget_reasons},
        {"task_mailbox_deterministic_order", test_task_mailbox_deterministic_order},
        {"mailbox_capacity_enforced", test_mailbox_capacity_enforced},
        {"unload_reload_rejects_old_generation", test_unload_reload_rejects_old_generation},
        {"revision_bumps_reject_old_mesh", test_revision_bumps_reject_old_mesh},
        {"queue_full_never_blocks_main", test_queue_full_never_blocks_main},
        {"priority_end_to_end_order", test_priority_end_to_end_order},
        {"worker_exception_observable", test_worker_exception_observable},
        {"stop_barrier_semantics", test_stop_barrier_semantics},
        {"stop_wakes_full_result_queue_and_joins", test_stop_wakes_full_result_queue_and_joins},
        {"stop_interleavings_accounted", test_stop_interleavings_accounted},
        {"wrong_thread_contract", test_wrong_thread_contract},
        {"thread_factory_failure_injection", test_thread_factory_failure_injection},
        {"callable_dtor_reentrancy_no_deadlock", test_callable_dtor_reentrancy_no_deadlock},
        {"sequence_never_wraps", test_sequence_never_wraps},
        {"concurrent_pending_never_negative", test_concurrent_pending_never_negative},
        {"config_and_stats_basics", test_config_and_stats_basics},
        {"submit_invalid_rejections", test_submit_invalid_rejections},
        {"queue_level_dtor_reentrancy_no_deadlock", test_queue_level_dtor_reentrancy_no_deadlock},
        {"reentrant_owner_stop_does_not_finish_early", test_reentrant_owner_stop_does_not_finish_early},
        {"submit_sequence_consumed_only_on_accept_and_busy_is_deterministic", test_submit_sequence_consumed_only_on_accept_and_busy_is_deterministic},
        {"result_cutoff_and_worker_fault_boundaries", test_result_cutoff_and_worker_fault_boundaries},
        {"upload_frame_queue_retains_and_accounts_once", test_upload_frame_queue_retains_and_accounts_once},
    };

    int failed_tests = 0;
    for (const NamedTest& test : tests) {
        std::printf("[RUN] %s\n", test.name);
        std::fflush(stdout);
        const int failures_before = g_failures;
        test.fn();
        if (g_failures == failures_before) {
            std::printf("[PASS] %s\n", test.name);
        } else {
            std::printf("[FAIL] %s\n", test.name);
            ++failed_tests;
        }
    }

    std::printf("%zu tests, %d checks, %d failed checks, %d failed tests\n",
                sizeof(tests) / sizeof(tests[0]), g_checks, g_failures,
                failed_tests);
    return failed_tests == 0 ? 0 : 1;
}
