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
    explicit VoxelSessionRegistry(std::uint64_t owner);

    // Reservation is not live. It lets the facade complete all potentially
    // throwing storage preparation before activating a visible handle.
    std::optional<VoxelSessionHandle> reserve();
    bool activate(VoxelSessionHandle handle) noexcept;
    void abandon(VoxelSessionHandle handle) noexcept;
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
    std::uint64_t m_owner = 0;
};

} // namespace tt::voxel_game
