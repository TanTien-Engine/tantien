#pragma once

#include "voxel_game/bridge/VoxelBridgeTypes.h"
#include "voxel_game/ves/VoxelSessionRegistry.h"

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <thread>
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
// cannot access World, Sections, renderer resources, or a VM callback. A
// facade is single-threaded: all calls belong to one game update/render control
// thread. Concurrent calls are not supported. Adapter execution is a strict
// non-reentrant boundary for every mutating facade operation.
class VoxelGameFacade
{
public:
    using CommandAdapter = std::function<VoxelCommandResult(const VoxelCommand&)>;

    explicit VoxelGameFacade(VoxelBridgeConfig config = {});
    ~VoxelGameFacade();

    VoxelSessionHandle create_session(const CommandAdapter& adapter);
    VoxelBridgeStatus create_status() const;
    // Reports the most recent owner-thread result from create_session(),
    // reload_session(), is_session_live(), try_pop_event(), or queue_stats().
    // Calls from another thread return WrongThread without mutating this value.
    VoxelBridgeStatus last_facade_status() const;
    VoxelBridgeStatus destroy_session(VoxelSessionHandle handle);
    std::optional<VoxelSessionHandle> reload_session(VoxelSessionHandle handle);
    bool is_session_live(VoxelSessionHandle handle) const;
    bool is_owner_thread() const;

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
    bool on_owner_thread() const;
    void record_facade_status(VoxelBridgeStatus status) const;
    bool reject_reentrant_mutation(SessionState* state);
    VoxelBridgeStatus push_event(SessionState& state, VoxelEvent event);
    static bool command_is_valid(const VoxelCommand& command);
    static bool action_is_valid(VoxelAction action);

    VoxelBridgeConfig m_config;
    std::uint64_t m_owner = 0;
    std::thread::id m_owner_thread;
    VoxelSessionRegistry m_sessions;
    std::vector<std::unique_ptr<SessionState>> m_states;
    VoxelBridgeStatus m_create_status = VoxelBridgeStatus::Ok;
    mutable VoxelBridgeStatus m_last_facade_status = VoxelBridgeStatus::Ok;
    bool m_adapter_invoking = false;
};

} // namespace tt::voxel_game
