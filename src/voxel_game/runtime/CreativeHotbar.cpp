#include "voxel_game/runtime/CreativeHotbar.h"

#include "voxel_game/voxel/BlockRegistry.h"

namespace tt::voxel_game
{

bool CreativeHotbar::select_slot(std::size_t index) noexcept
{
    if (index >= SlotCount)
    {
        return false;
    }
    m_selected_slot = index;
    return true;
}

void CreativeHotbar::apply_slot_actions(const ActionFrame& frame) noexcept
{
    constexpr std::array<GameAction, SlotCount> SlotActions = {
        GameAction::SelectSlot1,
        GameAction::SelectSlot2,
        GameAction::SelectSlot3,
        GameAction::SelectSlot4,
    };
    for (std::size_t index = 0; index < SlotActions.size(); ++index)
    {
        if (frame.pressed(SlotActions[index]))
        {
            m_selected_slot = index;
            return;
        }
    }
}

std::size_t CreativeHotbar::selected_slot() const noexcept
{
    return m_selected_slot;
}

BlockState CreativeHotbar::selected_block() const noexcept
{
    const BlockState selected = Slots[m_selected_slot];
    return selected != BlockState::Air && is_valid_block_state(selected) ? selected : BlockState::Stone;
}

} // namespace tt::voxel_game
