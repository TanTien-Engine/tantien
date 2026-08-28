#include "voxel_game/runtime/VoxelRuntimeAdapter.h"

#include "voxel_game/voxel/BlockRegistry.h"

namespace tt::voxel_game
{

VoxelRuntimeAdapter::VoxelRuntimeAdapter(World& world, PlayerController& player)
    : m_world(world)
    , m_player(player)
{
}

void VoxelRuntimeAdapter::set_target(std::optional<VoxelRaycastHit> target)
{
    m_target = target;
}

std::optional<VoxelRaycastHit> VoxelRuntimeAdapter::target() const
{
    return m_target;
}

BlockState VoxelRuntimeAdapter::selected_block() const noexcept
{
    return m_selected_block;
}

bool VoxelRuntimeAdapter::allow_edits() const noexcept
{
    return m_allow_edits;
}

bool VoxelRuntimeAdapter::strict_placement() const noexcept
{
    return m_strict_placement;
}

VoxelCommandResult VoxelRuntimeAdapter::apply(const VoxelCommand& command)
{
    switch (command.type)
    {
    case VoxelCommandType::BreakCurrentTarget:
        return break_current_target();
    case VoxelCommandType::PlaceSelectedBlock:
        return place_selected_block();
    case VoxelCommandType::SetSelectedBlock:
        return set_selected_block(command.selected_block);
    case VoxelCommandType::SetRuleFlag:
        return set_rule_flag(command.rule_flag, command.rule_enabled);
    }
    return VoxelCommandResult::reject(VoxelBridgeStatus::InvalidInput);
}

VoxelCommandResult VoxelRuntimeAdapter::break_current_target()
{
    if (!m_allow_edits || !m_target.has_value() ||
        m_world.get(m_target->block) == BlockState::Air)
    {
        return VoxelCommandResult::reject();
    }
    return m_world.set(m_target->block, BlockState::Air)
               ? VoxelCommandResult::accept()
               : VoxelCommandResult::reject();
}

VoxelCommandResult VoxelRuntimeAdapter::place_selected_block()
{
    if (!m_allow_edits || !m_target.has_value() || !m_target->has_previous_empty ||
        m_world.get(m_target->block) == BlockState::Air ||
        m_world.get(m_target->previous_empty) != BlockState::Air ||
        m_selected_block == BlockState::Air || !is_valid_block_state(m_selected_block))
    {
        return VoxelCommandResult::reject();
    }
    if (m_strict_placement && !player_can_place_block(
                                  m_player.state(), m_target->previous_empty, m_player.config()))
    {
        return VoxelCommandResult::reject();
    }
    return m_world.set(m_target->previous_empty, m_selected_block)
               ? VoxelCommandResult::accept()
               : VoxelCommandResult::reject();
}

VoxelCommandResult VoxelRuntimeAdapter::set_selected_block(std::uint16_t serialized_block)
{
    const BlockState selected = static_cast<BlockState>(serialized_block);
    if (selected == BlockState::Air || !is_valid_block_state(selected))
    {
        return VoxelCommandResult::reject(VoxelBridgeStatus::InvalidInput);
    }
    m_selected_block = selected;
    return VoxelCommandResult::accept();
}

VoxelCommandResult VoxelRuntimeAdapter::set_rule_flag(VoxelRuleFlag flag, bool enabled)
{
    switch (flag)
    {
    case VoxelRuleFlag::AllowEdits:
        m_allow_edits = enabled;
        return VoxelCommandResult::accept();
    case VoxelRuleFlag::StrictPlacement:
        m_strict_placement = enabled;
        return VoxelCommandResult::accept();
    }
    return VoxelCommandResult::reject(VoxelBridgeStatus::InvalidInput);
}

} // namespace tt::voxel_game
