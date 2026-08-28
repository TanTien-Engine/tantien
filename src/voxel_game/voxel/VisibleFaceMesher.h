#pragma once

#include "voxel_game/voxel/BlockState.h"
#include "voxel_game/voxel/Section.h"

#include <array>
#include <cstdint>
#include <functional>
#include <vector>

namespace tt::voxel_game
{

struct VoxelVertex
{
    std::array<float, 3> position{};
    std::array<float, 3> normal{};
    std::array<float, 4> color{};
    std::array<float, 2> uv{};

    bool operator==(const VoxelVertex& other) const
    {
        return position == other.position &&
               normal == other.normal &&
               color == other.color &&
               uv == other.uv;
    }
};

struct OpaqueMesh
{
    std::vector<VoxelVertex> vertices;
    std::vector<std::uint32_t> indices;
    std::size_t face_count = 0;
};

// This callback is invoked only for a one-block-outside Section-local
// coordinate. Coordinates inside [0, 16) always resolve from the supplied
// Section; without a callback, out-of-bounds neighbors are Air. A future World
// can resolve the callback coordinates to neighboring Sections without changing
// the mesher or its opaque/cutout/translucent split.
using BlockLookup = std::function<BlockState(BlockCoord)>;

OpaqueMesh mesh_opaque_visible_faces(const Section& section,
                                     const BlockLookup& block_lookup = {});

} // namespace tt::voxel_game
