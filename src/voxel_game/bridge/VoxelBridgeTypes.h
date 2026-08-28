#pragma once

#include <cstddef>
#include <cstdint>

namespace tt::voxel_game
{

// An opaque, generation-checked identifier for one script-facing game session.
// Neither field is a pointer or a reference to a World or renderer object.
struct VoxelSessionHandle
{
    std::uint64_t owner = 0;
    std::uint32_t slot = 0;
    std::uint32_t generation = 0;

    bool operator==(const VoxelSessionHandle& other) const
    {
        return owner == other.owner && slot == other.slot && generation == other.generation;
    }

    bool is_valid() const
    {
        return owner != 0 && generation != 0;
    }
};

enum class VoxelBridgeStatus
{
    Ok,
    InvalidHandle,
    InvalidInput,
    CommandQueueFull,
    EventQueueFull,
    ReentrantCall,
    WrongThread,
    StorageFailure,
    GenerationExhausted,
    CallbackMissing,
    AdapterRejected,
    AdapterException,
};

const char* voxel_bridge_status_message(VoxelBridgeStatus status);

enum class VoxelCommandType : std::uint8_t
{
    BreakCurrentTarget,
    PlaceSelectedBlock,
    SetSelectedBlock,
    SetRuleFlag,
};

enum class VoxelRuleFlag : std::uint8_t
{
    AllowEdits,
    StrictPlacement,
};

// Immutable scalar edit evidence captured when a command is queued. It never
// carries a World, a Section, or a storage reference across the bridge.
struct VoxelEditPrecondition
{
    int target_x = 0;
    int target_y = 0;
    int target_z = 0;
    int previous_x = 0;
    int previous_y = 0;
    int previous_z = 0;
    std::uint16_t expected_target_block = 0;
    std::uint16_t expected_previous_block = 0;
    bool has_target = false;
    bool has_previous_empty = false;
};

// Only scalar gameplay intent crosses this boundary. The adapter owns any
// interpretation involving a World, player, mesh, GPU resource, or VM object.
struct VoxelCommand
{
    VoxelCommandType type = VoxelCommandType::BreakCurrentTarget;
    std::uint16_t selected_block = 0;
    VoxelRuleFlag rule_flag = VoxelRuleFlag::AllowEdits;
    bool rule_enabled = false;
    VoxelEditPrecondition edit;
};

enum class VoxelAction : std::uint8_t
{
    Primary,
    Secondary,
    SelectNext,
    SelectPrevious,
};

// A target is reported as a small immutable snapshot rather than a World or
// voxel-storage reference. Consumers can use it only for coarse UI updates.
struct VoxelTargetSnapshot
{
    int x = 0;
    int y = 0;
    int z = 0;
    int face = -1;
    bool has_target = false;
};

enum class VoxelEventType : std::uint8_t
{
    ActionPressed,
    TargetChanged,
    EditAccepted,
    EditRejected,
    WorldReloaded,
};

struct VoxelEvent
{
    VoxelEventType type = VoxelEventType::ActionPressed;
    std::uint64_t sequence = 0;
    VoxelCommandType command = VoxelCommandType::BreakCurrentTarget;
    VoxelAction action = VoxelAction::Primary;
    VoxelTargetSnapshot target;
    VoxelBridgeStatus status = VoxelBridgeStatus::Ok;
    std::size_t discarded_command_count = 0;
    std::size_t discarded_event_count = 0;
};

struct VoxelCommandResult
{
    bool accepted = false;
    VoxelBridgeStatus status = VoxelBridgeStatus::AdapterRejected;

    static VoxelCommandResult accept()
    {
        return {true, VoxelBridgeStatus::Ok};
    }

    static VoxelCommandResult reject(VoxelBridgeStatus rejection_status = VoxelBridgeStatus::AdapterRejected)
    {
        return {false, rejection_status};
    }
};

struct VoxelQueueStats
{
    std::size_t command_count = 0;
    std::size_t event_count = 0;
    std::size_t command_overflow_count = 0;
    std::size_t event_overflow_count = 0;
};

} // namespace tt::voxel_game
