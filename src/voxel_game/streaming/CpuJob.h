#pragma once

#include "voxel_game/streaming/ChunkToken.h"
#include "voxel_game/streaming/SectionSnapshot.h"
#include "voxel_game/voxel/VisibleFaceMesher.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>

namespace tt::voxel_game::streaming
{

enum class MailboxStatus : std::uint8_t
{
    Accepted,
    Full,
    Busy,   // owner-side try_lock lost the race with a worker's critical section
    Closed
};

// CPU-only section function executed on worker threads. Purity is a CALLER
// CONTRACT: the std::function mechanism cannot verify that a captured lambda
// touches no engine state, allocates no GPU resources, or captures by value
// only. The queue validates what it can (non-null snapshot, non-empty
// callable, token consistency); behavioral purity stays with the integrator.
using CpuSectionFn = std::function<OpaqueMesh(const SectionSnapshot&)>;

// One queued unit of work. The snapshot keeps the job self-contained with
// respect to game state; no pointers back into live sections.
struct VoxelJob
{
    ChunkToken token{};
    SectionSnapshotPtr snapshot;
    CpuSectionFn function;
};

// Worker output. `ok == false` means the function threw; `error` carries the
// exception text when it could be copied (allocation failure during the copy
// leaves it empty — see the worker loop). The token always echoes the
// submitted one.
struct MeshResult
{
    ChunkToken token{};
    bool ok = false;
    std::string error;
    OpaqueMesh mesh;
    unsigned worker_id = 0;
};

// Saturating byte size with an overflow flag; the two counts are multiplied
// by their element sizes and summed. Realistic meshes never overflow, but
// the upload-budget decision must not wrap on hostile counts.
struct MeshSizeBytes
{
    std::size_t bytes = 0;
    bool overflow = false;
};

constexpr std::size_t kVoxelVertexSize = sizeof(VoxelVertex);
constexpr std::size_t kMeshIndexSize = sizeof(std::uint32_t);

inline MeshSizeBytes checked_mesh_bytes(std::size_t vertex_count,
                                        std::size_t index_count) noexcept
{
    MeshSizeBytes out;
    if (vertex_count != 0 && vertex_count > SIZE_MAX / kVoxelVertexSize) {
        out.overflow = true;
    }
    if (index_count != 0 && index_count > SIZE_MAX / kMeshIndexSize) {
        out.overflow = true;
    }
    if (out.overflow) {
        out.bytes = SIZE_MAX;
        return out;
    }
    const std::size_t vertex_bytes = vertex_count * kVoxelVertexSize;
    const std::size_t index_bytes = index_count * kMeshIndexSize;
    if (vertex_bytes > SIZE_MAX - index_bytes) {
        out.overflow = true;
        out.bytes = SIZE_MAX;
        return out;
    }
    out.bytes = vertex_bytes + index_bytes;
    return out;
}

inline MeshSizeBytes checked_mesh_bytes(const OpaqueMesh& mesh) noexcept
{
    return checked_mesh_bytes(mesh.vertices.size(), mesh.indices.size());
}

} // namespace tt::voxel_game::streaming
