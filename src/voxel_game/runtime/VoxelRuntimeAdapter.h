#pragma once

#include "voxel_game/bridge/VoxelBridgeTypes.h"
#include "voxel_game/runtime/PlayerController.h"
#include "voxel_game/voxel/VoxelRaycast.h"

#include <optional>

namespace tt::voxel_game
{

// Bridges scalar gameplay commands to the standalone world's CPU state. The
// caller retains ownership of World and PlayerController and must destroy this
// adapter before either borrowed object. All methods are single-threaded and
// belong to the same game-control thread as the facade drain; this type exposes
// neither storage nor GPU handles and never invokes a VM callback.
class VoxelRuntimeAdapter
{
public:
    VoxelRuntimeAdapter(World& world, PlayerController& player);

    void set_target(std::optional<VoxelRaycastHit> target);
    std::optional<VoxelRaycastHit> target() const;
    BlockState selected_block() const noexcept;
    bool allow_edits() const noexcept;
    bool strict_placement() const noexcept;

    // Captures only scalar evidence for a currently live edit target. For a
    // place command, requested_block is the hotbar value that must match the
    // adapter selection when the FIFO command eventually executes.
    std::optional<VoxelCommand> capture_edit_command(
        VoxelCommandType type,
        BlockState requested_block = BlockState::Air) const;
    VoxelCommandResult apply(const VoxelCommand& command);

private:
    VoxelCommandResult break_current_target(const VoxelCommand& command);
    VoxelCommandResult place_selected_block(const VoxelCommand& command);
    VoxelCommandResult set_selected_block(std::uint16_t serialized_block);
    VoxelCommandResult set_rule_flag(VoxelRuleFlag flag, bool enabled);
    bool edit_target_matches(const VoxelEditPrecondition& precondition) const;
    static bool edit_precondition_is_well_formed(const VoxelEditPrecondition& precondition,
                                                 bool requires_previous_empty) noexcept;

    World& m_world;
    PlayerController& m_player;
    std::optional<VoxelRaycastHit> m_target;
    BlockState m_selected_block = BlockState::Stone;
    bool m_allow_edits = true;
    bool m_strict_placement = true;
};

} // namespace tt::voxel_game
