#pragma once

#include "voxel_game/bridge/VoxelBridgeTypes.h"

#include <cstdint>
#include <optional>
#include <vector>

namespace tt::voxel_game
{

// Owns only session lifetimes. The facade keeps its command/event state
// separately, and no gameplay object can be recovered from a handle.
class VoxelSessionRegistry
{
public:
    VoxelSessionHandle create();
    bool destroy(VoxelSessionHandle handle);
    std::optional<VoxelSessionHandle> reload(VoxelSessionHandle handle);
    bool is_live(VoxelSessionHandle handle) const;

private:
    struct Slot
    {
        std::uint32_t generation = 1;
        bool live = false;
        bool retired = false;
    };

    std::vector<Slot> m_slots;
};

} // namespace tt::voxel_game
