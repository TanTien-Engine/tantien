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

    bool operator==(const VoxelVertex& other) const
    {
        return position == other.position &&
               normal == other.normal &&
               color == other.color;
    }
};

struct OpaqueMesh
{
    std::vector<VoxelVertex> vertices;
    std::vector<std::uint32_t> indices;
    std::size_t face_count = 0;
};

// Coordinates presented to this callback are Section-local and may be one
// block outside [0, 16). A future World can resolve them to neighboring
// Sections without changing the mesher or its opaque/cutout/translucent split.
using BlockLookup = std::function<BlockState(BlockCoord)>;

OpaqueMesh mesh_opaque_visible_faces(const Section& section,
                                     const BlockLookup& block_lookup = {});

} // namespace tt::voxel_game
