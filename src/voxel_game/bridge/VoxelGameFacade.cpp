#include "voxel_game/bridge/VoxelGameFacade.h"

#include <deque>
#include <utility>

namespace tt::voxel_game
{

const char* voxel_bridge_status_message(VoxelBridgeStatus status)
{
    switch (status)
    {
    case VoxelBridgeStatus::Ok:
        return "ok";
    case VoxelBridgeStatus::InvalidHandle:
        return "invalid or stale session handle";
    case VoxelBridgeStatus::InvalidInput:
        return "invalid bridge input";
    case VoxelBridgeStatus::CommandQueueFull:
        return "command queue capacity reached";
    case VoxelBridgeStatus::EventQueueFull:
        return "event queue capacity reached";
    case VoxelBridgeStatus::CallbackMissing:
        return "command adapter is required";
    case VoxelBridgeStatus::AdapterRejected:
        return "command rejected by adapter";
    case VoxelBridgeStatus::AdapterException:
        return "command adapter raised an exception";
    }

    return "unknown bridge status";
}

struct VoxelGameFacade::SessionState
{
    CommandAdapter adapter;
    std::deque<VoxelCommand> commands;
    std::deque<VoxelEvent> events;
    std::size_t command_overflow_count = 0;
    std::size_t event_overflow_count = 0;
    std::uint64_t next_event_sequence = 1;
    VoxelBridgeStatus last_status = VoxelBridgeStatus::Ok;
};

VoxelGameFacade::~VoxelGameFacade() = default;

VoxelGameFacade::VoxelGameFacade(VoxelBridgeConfig config)
    : m_config(config)
{
}

VoxelSessionHandle VoxelGameFacade::create_session(CommandAdapter adapter)
{
    if (m_config.command_capacity == 0 || m_config.event_capacity == 0)
    {
        m_create_status = VoxelBridgeStatus::InvalidInput;
        return {};
    }

    if (!adapter)
    {
        m_create_status = VoxelBridgeStatus::CallbackMissing;
        return {};
    }

    const VoxelSessionHandle handle = m_sessions.create();
    if (!handle.is_valid())
    {
        m_create_status = VoxelBridgeStatus::InvalidInput;
        return {};
    }

    if (m_states.size() <= handle.slot)
    {
        m_states.resize(static_cast<std::size_t>(handle.slot) + 1);
    }

    auto state = std::make_unique<SessionState>();
    state->adapter = std::move(adapter);
    m_states[handle.slot] = std::move(state);
    m_create_status = VoxelBridgeStatus::Ok;
    return handle;
}

VoxelBridgeStatus VoxelGameFacade::create_status() const
{
    return m_create_status;
}

VoxelBridgeStatus VoxelGameFacade::destroy_session(VoxelSessionHandle handle)
{
    if (!m_sessions.destroy(handle))
    {
        return VoxelBridgeStatus::InvalidHandle;
    }

    m_states[handle.slot].reset();
    return VoxelBridgeStatus::Ok;
}

std::optional<VoxelSessionHandle> VoxelGameFacade::reload_session(VoxelSessionHandle handle)
{
    SessionState* old_state = find_state(handle);
    if (old_state == nullptr)
    {
        return std::nullopt;
    }

    CommandAdapter adapter = old_state->adapter;
    const std::optional<VoxelSessionHandle> replacement = m_sessions.reload(handle);
    if (!replacement.has_value())
    {
        m_states[handle.slot].reset();
        return std::nullopt;
    }

    auto replacement_state = std::make_unique<SessionState>();
    replacement_state->adapter = std::move(adapter);
    VoxelEvent reloaded;
    reloaded.type = VoxelEventType::WorldReloaded;
    push_event(*replacement_state, reloaded);
    m_states[replacement->slot] = std::move(replacement_state);
    return replacement;
}

bool VoxelGameFacade::is_session_live(VoxelSessionHandle handle) const
{
    return find_state(handle) != nullptr;
}

VoxelBridgeStatus VoxelGameFacade::enqueue_command(VoxelSessionHandle handle, const VoxelCommand& command)
{
    SessionState* state = find_state(handle);
    if (state == nullptr)
    {
        return VoxelBridgeStatus::InvalidHandle;
    }

    if (!command_is_valid(command))
    {
        state->last_status = VoxelBridgeStatus::InvalidInput;
        return state->last_status;
    }

    if (state->commands.size() >= m_config.command_capacity)
    {
        ++state->command_overflow_count;
        state->last_status = VoxelBridgeStatus::CommandQueueFull;
        return state->last_status;
    }

    state->commands.push_back(command);
    state->last_status = VoxelBridgeStatus::Ok;
    return state->last_status;
}

VoxelDrainResult VoxelGameFacade::drain_fixed_tick(VoxelSessionHandle handle, std::size_t maximum_commands)
{
    SessionState* state = find_state(handle);
    if (state == nullptr)
    {
        return {VoxelBridgeStatus::InvalidHandle, 0};
    }

    VoxelDrainResult result;
    while (result.consumed < maximum_commands && !state->commands.empty())
    {
        // A command has no externally observable completion if there is no
        // room for its result event. Leave it at the FIFO head for a later
        // fixed tick instead of applying an unreportable edit.
        if (state->events.size() >= m_config.event_capacity)
        {
            ++state->event_overflow_count;
            state->last_status = VoxelBridgeStatus::EventQueueFull;
            result.status = state->last_status;
            break;
        }

        const VoxelCommand command = state->commands.front();
        state->commands.pop_front();

        VoxelCommandResult command_result;
        try
        {
            command_result = state->adapter(command);
        }
        catch (...)
        {
            command_result = VoxelCommandResult::reject(VoxelBridgeStatus::AdapterException);
        }

        if (!command_result.accepted && command_result.status == VoxelBridgeStatus::Ok)
        {
            command_result.status = VoxelBridgeStatus::AdapterRejected;
        }
        if (command_result.accepted)
        {
            command_result.status = VoxelBridgeStatus::Ok;
        }

        VoxelEvent event;
        event.type = command_result.accepted ? VoxelEventType::EditAccepted : VoxelEventType::EditRejected;
        event.command = command.type;
        event.status = command_result.accepted ? VoxelBridgeStatus::Ok : command_result.status;
        const VoxelBridgeStatus event_status = push_event(*state, event);
        state->last_status = event_status == VoxelBridgeStatus::Ok ? event.status : event_status;
        result.status = state->last_status;
        ++result.consumed;
    }

    return result;
}

VoxelBridgeStatus VoxelGameFacade::publish_action_pressed(VoxelSessionHandle handle, VoxelAction action)
{
    SessionState* state = find_state(handle);
    if (state == nullptr)
    {
        return VoxelBridgeStatus::InvalidHandle;
    }

    if (!action_is_valid(action))
    {
        state->last_status = VoxelBridgeStatus::InvalidInput;
        return state->last_status;
    }

    VoxelEvent event;
    event.type = VoxelEventType::ActionPressed;
    event.action = action;
    return push_event(*state, event);
}

VoxelBridgeStatus VoxelGameFacade::publish_target_changed(VoxelSessionHandle handle, VoxelTargetSnapshot target)
{
    SessionState* state = find_state(handle);
    if (state == nullptr)
    {
        return VoxelBridgeStatus::InvalidHandle;
    }

    if (target.face < -1 || target.face > 5)
    {
        state->last_status = VoxelBridgeStatus::InvalidInput;
        return state->last_status;
    }

    VoxelEvent event;
    event.type = VoxelEventType::TargetChanged;
    event.target = target;
    return push_event(*state, event);
}

bool VoxelGameFacade::try_pop_event(VoxelSessionHandle handle, VoxelEvent& event)
{
    SessionState* state = find_state(handle);
    if (state == nullptr || state->events.empty())
    {
        return false;
    }

    event = state->events.front();
    state->events.pop_front();
    return true;
}

std::optional<VoxelQueueStats> VoxelGameFacade::queue_stats(VoxelSessionHandle handle) const
{
    const SessionState* state = find_state(handle);
    if (state == nullptr)
    {
        return std::nullopt;
    }

    return VoxelQueueStats{
        state->commands.size(),
        state->events.size(),
        state->command_overflow_count,
        state->event_overflow_count};
}

VoxelBridgeStatus VoxelGameFacade::last_status(VoxelSessionHandle handle) const
{
    const SessionState* state = find_state(handle);
    return state == nullptr ? VoxelBridgeStatus::InvalidHandle : state->last_status;
}

std::string VoxelGameFacade::last_error(VoxelSessionHandle handle) const
{
    return voxel_bridge_status_message(last_status(handle));
}

VoxelGameFacade::SessionState* VoxelGameFacade::find_state(VoxelSessionHandle handle)
{
    if (!m_sessions.is_live(handle) || handle.slot >= m_states.size())
    {
        return nullptr;
    }

    return m_states[handle.slot].get();
}

const VoxelGameFacade::SessionState* VoxelGameFacade::find_state(VoxelSessionHandle handle) const
{
    if (!m_sessions.is_live(handle) || handle.slot >= m_states.size())
    {
        return nullptr;
    }

    return m_states[handle.slot].get();
}

VoxelBridgeStatus VoxelGameFacade::push_event(SessionState& state, VoxelEvent event)
{
    if (state.events.size() >= m_config.event_capacity)
    {
        ++state.event_overflow_count;
        state.last_status = VoxelBridgeStatus::EventQueueFull;
        return state.last_status;
    }

    event.sequence = state.next_event_sequence++;
    state.events.push_back(event);
    state.last_status = VoxelBridgeStatus::Ok;
    return state.last_status;
}

bool VoxelGameFacade::command_is_valid(const VoxelCommand& command)
{
    switch (command.type)
    {
    case VoxelCommandType::BreakCurrentTarget:
    case VoxelCommandType::PlaceSelectedBlock:
        return true;
    case VoxelCommandType::SetSelectedBlock:
        return command.selected_block != 0;
    case VoxelCommandType::SetRuleFlag:
        return command.rule_flag == VoxelRuleFlag::AllowEdits ||
               command.rule_flag == VoxelRuleFlag::StrictPlacement;
    }

    return false;
}

bool VoxelGameFacade::action_is_valid(VoxelAction action)
{
    return action == VoxelAction::Primary || action == VoxelAction::Secondary ||
           action == VoxelAction::SelectNext || action == VoxelAction::SelectPrevious;
}

} // namespace tt::voxel_game
