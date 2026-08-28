#pragma once

#include "voxel_game/bridge/VoxelBridgeTypes.h"
#include "voxel_game/ves/VoxelSessionRegistry.h"

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace tt::voxel_game
{

struct VoxelBridgeConfig
{
    std::size_t command_capacity = 64;
    std::size_t event_capacity = 64;
};

struct VoxelDrainResult
{
    VoxelBridgeStatus status = VoxelBridgeStatus::Ok;
    std::size_t consumed = 0;
};

// Future VES/Blueprint code should own one opaque VoxelGame instance backed by
// this facade. It can enqueue gameplay intent and consume coarse events, but
// cannot access World, Sections, renderer resources, or a VM callback.
class VoxelGameFacade
{
public:
    using CommandAdapter = std::function<VoxelCommandResult(const VoxelCommand&)>;

    explicit VoxelGameFacade(VoxelBridgeConfig config = {});
    ~VoxelGameFacade();

    VoxelSessionHandle create_session(CommandAdapter adapter);
    VoxelBridgeStatus create_status() const;
    VoxelBridgeStatus destroy_session(VoxelSessionHandle handle);
    std::optional<VoxelSessionHandle> reload_session(VoxelSessionHandle handle);
    bool is_session_live(VoxelSessionHandle handle) const;

    // Queueing is the only command submission path. The adapter is called only
    // from drain_fixed_tick(), never from this method.
    VoxelBridgeStatus enqueue_command(VoxelSessionHandle handle, const VoxelCommand& command);
    VoxelDrainResult drain_fixed_tick(VoxelSessionHandle handle, std::size_t maximum_commands);

    VoxelBridgeStatus publish_action_pressed(VoxelSessionHandle handle, VoxelAction action);
    VoxelBridgeStatus publish_target_changed(VoxelSessionHandle handle, VoxelTargetSnapshot target);
    bool try_pop_event(VoxelSessionHandle handle, VoxelEvent& event);

    std::optional<VoxelQueueStats> queue_stats(VoxelSessionHandle handle) const;
    VoxelBridgeStatus last_status(VoxelSessionHandle handle) const;
    std::string last_error(VoxelSessionHandle handle) const;

private:
    struct SessionState;

    SessionState* find_state(VoxelSessionHandle handle);
    const SessionState* find_state(VoxelSessionHandle handle) const;
    VoxelBridgeStatus push_event(SessionState& state, VoxelEvent event);
    static bool command_is_valid(const VoxelCommand& command);
    static bool action_is_valid(VoxelAction action);

    VoxelBridgeConfig m_config;
    VoxelSessionRegistry m_sessions;
    std::vector<std::unique_ptr<SessionState>> m_states;
    VoxelBridgeStatus m_create_status = VoxelBridgeStatus::Ok;
};

} // namespace tt::voxel_game
