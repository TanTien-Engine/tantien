#pragma once

#include "voxel_game/voxel/World.h"

#include <array>
#include <optional>

namespace tt::voxel_game
{

struct VoxelRaycastHit
{
    BlockCoord block;
    BlockCoord entered_face_normal;
    BlockCoord previous_empty;
    double distance = 0.0;
    bool has_previous_empty = false;
};

// Casts from origin in direction for at most max_distance world units. The
// direction is normalized internally, so distance and max_distance are world
// distances. A starting solid block is a distance-zero hit with a zero normal
// and no previous empty block. Hits exactly at max_distance are included. For
// a ray starting on a grid boundary, the initial floor(origin) cell is checked
// first; if it is air, a negative direction may cross into its neighbor at 0.
// Exact simultaneous grid-plane crossings use a deterministic X, then Y, then
// Z tie priority, visiting one face-neighbor per iteration.
// The complete finite ray segment must map to BlockCoord's int domain; rays
// outside that domain are rejected instead of narrowing or overflowing.
std::optional<VoxelRaycastHit> raycast_voxels(const World& world,
                                              std::array<double, 3> origin,
                                              std::array<double, 3> direction,
                                              double max_distance);

} // namespace tt::voxel_game
