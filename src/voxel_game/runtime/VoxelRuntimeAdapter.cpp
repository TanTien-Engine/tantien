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

std::optional<VoxelCommand> VoxelRuntimeAdapter::capture_edit_command(
    VoxelCommandType type,
    BlockState requested_block) const
{
    if ((type != VoxelCommandType::BreakCurrentTarget && type != VoxelCommandType::PlaceSelectedBlock) ||
        !m_target.has_value())
    {
        return std::nullopt;
    }
    const BlockState target_block = m_world.get(m_target->block);
    if (target_block == BlockState::Air || !is_valid_block_state(target_block))
    {
        return std::nullopt;
    }

    VoxelCommand command;
    command.type = type;
    command.edit.target_x = m_target->block.x;
    command.edit.target_y = m_target->block.y;
    command.edit.target_z = m_target->block.z;
    command.edit.expected_target_block = static_cast<std::uint16_t>(target_block);
    command.edit.has_target = true;
    command.edit.has_previous_empty = m_target->has_previous_empty;
    if (m_target->has_previous_empty)
    {
        const BlockState previous_block = m_world.get(m_target->previous_empty);
        if (previous_block != BlockState::Air)
        {
            return std::nullopt;
        }
        command.edit.previous_x = m_target->previous_empty.x;
        command.edit.previous_y = m_target->previous_empty.y;
        command.edit.previous_z = m_target->previous_empty.z;
        command.edit.expected_previous_block = static_cast<std::uint16_t>(previous_block);
    }
    if (type == VoxelCommandType::PlaceSelectedBlock)
    {
        if (!m_target->has_previous_empty || requested_block == BlockState::Air ||
            !is_valid_block_state(requested_block))
        {
            return std::nullopt;
        }
        command.selected_block = static_cast<std::uint16_t>(requested_block);
    }
    return command;
}

VoxelCommandResult VoxelRuntimeAdapter::apply(const VoxelCommand& command)
{
    switch (command.type)
    {
    case VoxelCommandType::BreakCurrentTarget:
        return break_current_target(command);
    case VoxelCommandType::PlaceSelectedBlock:
        return place_selected_block(command);
    case VoxelCommandType::SetSelectedBlock:
        return set_selected_block(command.selected_block);
    case VoxelCommandType::SetRuleFlag:
        return set_rule_flag(command.rule_flag, command.rule_enabled);
    }
    return VoxelCommandResult::reject(VoxelBridgeStatus::InvalidInput);
}

VoxelCommandResult VoxelRuntimeAdapter::break_current_target(const VoxelCommand& command)
{
    if (!edit_precondition_is_well_formed(command.edit, false))
    {
        return VoxelCommandResult::reject(VoxelBridgeStatus::InvalidInput);
    }
    if (!m_allow_edits || !edit_target_matches(command.edit))
    {
        return VoxelCommandResult::reject();
    }
    return m_world.set({command.edit.target_x, command.edit.target_y, command.edit.target_z}, BlockState::Air)
               ? VoxelCommandResult::accept()
               : VoxelCommandResult::reject();
}

VoxelCommandResult VoxelRuntimeAdapter::place_selected_block(const VoxelCommand& command)
{
    if (!edit_precondition_is_well_formed(command.edit, true))
    {
        return VoxelCommandResult::reject(VoxelBridgeStatus::InvalidInput);
    }
    const BlockState requested_block = static_cast<BlockState>(command.selected_block);
    if (requested_block == BlockState::Air || !is_valid_block_state(requested_block))
    {
        return VoxelCommandResult::reject(VoxelBridgeStatus::InvalidInput);
    }
    if (!m_allow_edits || !edit_target_matches(command.edit) ||
        requested_block != m_selected_block || m_selected_block == BlockState::Air ||
        !is_valid_block_state(m_selected_block))
    {
        return VoxelCommandResult::reject();
    }
    const BlockCoord previous = {command.edit.previous_x, command.edit.previous_y, command.edit.previous_z};
    if (m_strict_placement && !player_can_place_block(m_player.state(), previous, m_player.config()))
    {
        return VoxelCommandResult::reject();
    }
    return m_world.set(previous, m_selected_block)
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

bool VoxelRuntimeAdapter::edit_target_matches(const VoxelEditPrecondition& precondition) const
{
    if (!m_target.has_value() || !precondition.has_target ||
        m_target->block.x != precondition.target_x || m_target->block.y != precondition.target_y ||
        m_target->block.z != precondition.target_z ||
        m_target->has_previous_empty != precondition.has_previous_empty)
    {
        return false;
    }
    if (m_target->has_previous_empty &&
        (m_target->previous_empty.x != precondition.previous_x ||
         m_target->previous_empty.y != precondition.previous_y ||
         m_target->previous_empty.z != precondition.previous_z))
    {
        return false;
    }
    const BlockState current_target = m_world.get(m_target->block);
    if (static_cast<std::uint16_t>(current_target) != precondition.expected_target_block)
    {
        return false;
    }
    if (!m_target->has_previous_empty)
    {
        return true;
    }
    const BlockState current_previous = m_world.get(m_target->previous_empty);
    return static_cast<std::uint16_t>(current_previous) == precondition.expected_previous_block;
}

bool VoxelRuntimeAdapter::edit_precondition_is_well_formed(
    const VoxelEditPrecondition& precondition,
    bool requires_previous_empty) noexcept
{
    if (!precondition.has_target || precondition.expected_target_block == 0)
    {
        return false;
    }
    if (!precondition.has_previous_empty)
    {
        return !requires_previous_empty && precondition.expected_previous_block == 0 &&
               precondition.previous_x == 0 && precondition.previous_y == 0 &&
               precondition.previous_z == 0;
    }
    return precondition.expected_previous_block == 0 &&
           (precondition.previous_x != precondition.target_x ||
            precondition.previous_y != precondition.target_y ||
            precondition.previous_z != precondition.target_z);
}

} // namespace tt::voxel_game
