#include "voxel_game/ves/VoxelSessionRegistry.h"

#include <limits>

namespace tt::voxel_game
{

namespace
{

bool advance_generation(std::uint32_t& generation)
{
    if (generation == (std::numeric_limits<std::uint32_t>::max)())
    {
        return false;
    }

    ++generation;
    return true;
}

} // namespace

VoxelSessionRegistry::VoxelSessionRegistry(std::uint64_t owner)
    : m_owner(owner)
{
}

std::optional<VoxelSessionHandle> VoxelSessionRegistry::reserve()
{
    for (std::size_t index = 0; index < m_slots.size(); ++index)
    {
        Slot& slot = m_slots[index];
        if (!slot.live && !slot.retired)
        {
            return VoxelSessionHandle{m_owner, static_cast<std::uint32_t>(index), slot.generation};
        }
    }

    if (m_slots.size() >= (std::numeric_limits<std::uint32_t>::max)())
    {
        return std::nullopt;
    }

    m_slots.push_back({});
    const std::size_t index = m_slots.size() - 1;
    return VoxelSessionHandle{m_owner, static_cast<std::uint32_t>(index), m_slots[index].generation};
}

bool VoxelSessionRegistry::activate(VoxelSessionHandle handle) noexcept
{
    if (handle.owner != m_owner || !handle.is_valid() || handle.slot >= m_slots.size())
    {
        return false;
    }

    Slot& slot = m_slots[handle.slot];
    if (slot.live || slot.retired || slot.generation != handle.generation)
    {
        return false;
    }

    slot.live = true;
    return true;
}

void VoxelSessionRegistry::abandon(VoxelSessionHandle handle) noexcept
{
    // reserve() deliberately leaves the slot non-live, so abandoning a
    // reservation has no mutating rollback step and can never throw. A later
    // reserve() will reuse this exact inactive generation.
    (void)handle;
}

bool VoxelSessionRegistry::destroy(VoxelSessionHandle handle)
{
    if (!is_live(handle))
    {
        return false;
    }

    Slot& slot = m_slots[handle.slot];
    if (!advance_generation(slot.generation))
    {
        // Generation exhaustion permanently retires this slot rather than
        // allowing a stale handle to become valid again through ABA reuse.
        slot.live = false;
        slot.retired = true;
        return true;
    }

    slot.live = false;
    return true;
}

std::optional<VoxelSessionHandle> VoxelSessionRegistry::reload(VoxelSessionHandle handle)
{
    if (!is_live(handle))
    {
        return std::nullopt;
    }

    Slot& slot = m_slots[handle.slot];
    if (!advance_generation(slot.generation))
    {
        slot.live = false;
        slot.retired = true;
        return std::nullopt;
    }

    return VoxelSessionHandle{m_owner, handle.slot, slot.generation};
}

bool VoxelSessionRegistry::is_live(VoxelSessionHandle handle) const
{
    return handle.owner == m_owner && handle.is_valid() && handle.slot < m_slots.size() &&
           m_slots[handle.slot].live && m_slots[handle.slot].generation == handle.generation;
}

} // namespace tt::voxel_game
