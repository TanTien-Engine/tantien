#include "voxel_game/bridge/VoxelGameFacade.h"

#include "voxel_game/bridge/FixedRing.h"

#include <atomic>
#include <thread>
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
    case VoxelBridgeStatus::ReentrantCall:
        return "facade mutation rejected during adapter execution";
    case VoxelBridgeStatus::WrongThread:
        return "facade call rejected from a non-owner thread";
    case VoxelBridgeStatus::StorageFailure:
        return "bridge storage preparation failed";
    case VoxelBridgeStatus::GenerationExhausted:
        return "session generation space exhausted";
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
    SessionState(const CommandAdapter& next_adapter, VoxelBridgeConfig config)
        : adapter(next_adapter)
        , commands(config.command_capacity)
        , events(config.event_capacity)
    {
    }

    CommandAdapter adapter;
    FixedRing<VoxelCommand> commands;
    FixedRing<VoxelEvent> events;
    std::size_t command_overflow_count = 0;
    std::size_t event_overflow_count = 0;
    std::uint64_t next_event_sequence = 1;
    VoxelBridgeStatus last_status = VoxelBridgeStatus::Ok;
};

namespace
{

class AdapterInvocationGuard
{
public:
    explicit AdapterInvocationGuard(bool& invoking)
        : m_invoking(invoking)
        , m_previous(invoking)
    {
        m_invoking = true;
    }

    ~AdapterInvocationGuard()
    {
        m_invoking = m_previous;
    }

private:
    bool& m_invoking;
    bool m_previous = false;
};

} // namespace

VoxelGameFacade::~VoxelGameFacade() = default;

namespace
{

std::uint64_t next_facade_owner() noexcept
{
    static std::atomic<std::uint64_t> next_owner{1};
    std::uint64_t owner = next_owner.fetch_add(1, std::memory_order_relaxed);
    if (owner == 0)
    {
        owner = next_owner.fetch_add(1, std::memory_order_relaxed);
    }
    return owner;
}

} // namespace

VoxelGameFacade::VoxelGameFacade(VoxelBridgeConfig config)
    : m_config(config)
    , m_owner(next_facade_owner())
    , m_owner_thread(std::this_thread::get_id())
    , m_sessions(m_owner)
{
}

VoxelSessionHandle VoxelGameFacade::create_session(const CommandAdapter& adapter)
{
    if (!on_owner_thread())
    {
        return {};
    }

    if (m_adapter_invoking)
    {
        m_create_status = VoxelBridgeStatus::ReentrantCall;
        m_last_facade_status = m_create_status;
        return {};
    }

    if (m_config.command_capacity == 0 || m_config.event_capacity == 0)
    {
        m_create_status = VoxelBridgeStatus::InvalidInput;
        m_last_facade_status = m_create_status;
        return {};
    }

    if (!adapter)
    {
        m_create_status = VoxelBridgeStatus::CallbackMissing;
        m_last_facade_status = m_create_status;
        return {};
    }

    std::unique_ptr<SessionState> state;
    try
    {
        state = std::make_unique<SessionState>(adapter, m_config);
    }
    catch (...)
    {
        m_create_status = VoxelBridgeStatus::StorageFailure;
        m_last_facade_status = m_create_status;
        return {};
    }

    std::optional<VoxelSessionHandle> reservation;
    try
    {
        reservation = m_sessions.reserve();
        if (!reservation.has_value())
        {
            m_create_status = VoxelBridgeStatus::StorageFailure;
            m_last_facade_status = m_create_status;
            return {};
        }
        if (m_states.size() <= reservation->slot)
        {
            m_states.resize(static_cast<std::size_t>(reservation->slot) + 1);
        }
    }
    catch (...)
    {
        if (reservation.has_value())
        {
            m_sessions.abandon(*reservation);
        }
        m_create_status = VoxelBridgeStatus::StorageFailure;
        m_last_facade_status = m_create_status;
        return {};
    }

    m_states[reservation->slot] = std::move(state);
    if (!m_sessions.activate(*reservation))
    {
        m_states[reservation->slot].reset();
        m_sessions.abandon(*reservation);
        m_create_status = VoxelBridgeStatus::StorageFailure;
        m_last_facade_status = m_create_status;
        return {};
    }
    m_create_status = VoxelBridgeStatus::Ok;
    m_last_facade_status = m_create_status;
    return *reservation;
}

VoxelBridgeStatus VoxelGameFacade::create_status() const
{
    return on_owner_thread() ? m_create_status : VoxelBridgeStatus::WrongThread;
}

VoxelBridgeStatus VoxelGameFacade::last_facade_status() const
{
    return on_owner_thread() ? m_last_facade_status : VoxelBridgeStatus::WrongThread;
}

VoxelBridgeStatus VoxelGameFacade::destroy_session(VoxelSessionHandle handle)
{
    if (!on_owner_thread())
    {
        return VoxelBridgeStatus::WrongThread;
    }
    SessionState* state = find_state(handle);
    if (state == nullptr)
    {
        return VoxelBridgeStatus::InvalidHandle;
    }

    if (reject_reentrant_mutation(state))
    {
        return VoxelBridgeStatus::ReentrantCall;
    }

    if (!m_sessions.destroy(handle))
    {
        return VoxelBridgeStatus::InvalidHandle;
    }

    m_states[handle.slot].reset();
    return VoxelBridgeStatus::Ok;
}

std::optional<VoxelSessionHandle> VoxelGameFacade::reload_session(VoxelSessionHandle handle)
{
    if (!on_owner_thread())
    {
        return std::nullopt;
    }
    SessionState* old_state = find_state(handle);
    if (old_state == nullptr)
    {
        record_facade_status(VoxelBridgeStatus::InvalidHandle);
        return std::nullopt;
    }

    if (reject_reentrant_mutation(old_state))
    {
        record_facade_status(VoxelBridgeStatus::ReentrantCall);
        return std::nullopt;
    }

    std::unique_ptr<SessionState> replacement_state;
    try
    {
        replacement_state = std::make_unique<SessionState>(old_state->adapter, m_config);
    }
    catch (...)
    {
        old_state->last_status = VoxelBridgeStatus::StorageFailure;
        m_last_facade_status = old_state->last_status;
        return std::nullopt;
    }

    VoxelEvent reloaded;
    reloaded.type = VoxelEventType::WorldReloaded;
    reloaded.sequence = replacement_state->next_event_sequence++;
    reloaded.discarded_command_count = old_state->commands.size();
    reloaded.discarded_event_count = old_state->events.size();
    VoxelEvent* event_slot = replacement_state->events.reserve_back();
    if (event_slot == nullptr)
    {
        old_state->last_status = VoxelBridgeStatus::StorageFailure;
        m_last_facade_status = old_state->last_status;
        return std::nullopt;
    }
    *event_slot = reloaded;
    replacement_state->events.commit_reserved_back();

    const std::optional<VoxelSessionHandle> replacement = m_sessions.reload(handle);
    if (!replacement.has_value())
    {
        m_states[handle.slot].reset();
        record_facade_status(VoxelBridgeStatus::GenerationExhausted);
        return std::nullopt;
    }

    m_states[replacement->slot] = std::move(replacement_state);
    m_last_facade_status = VoxelBridgeStatus::Ok;
    return replacement;
}

bool VoxelGameFacade::is_session_live(VoxelSessionHandle handle) const
{
    if (!on_owner_thread())
    {
        return false;
    }

    const bool live = find_state(handle) != nullptr;
    record_facade_status(live ? VoxelBridgeStatus::Ok : VoxelBridgeStatus::InvalidHandle);
    return live;
}

bool VoxelGameFacade::is_owner_thread() const
{
    return on_owner_thread();
}

VoxelBridgeStatus VoxelGameFacade::enqueue_command(VoxelSessionHandle handle, const VoxelCommand& command)
{
    if (!on_owner_thread())
    {
        return VoxelBridgeStatus::WrongThread;
    }
    SessionState* state = find_state(handle);
    if (state == nullptr)
    {
        return VoxelBridgeStatus::InvalidHandle;
    }

    if (reject_reentrant_mutation(state))
    {
        return VoxelBridgeStatus::ReentrantCall;
    }

    if (!command_is_valid(command))
    {
        state->last_status = VoxelBridgeStatus::InvalidInput;
        return state->last_status;
    }

    if (!state->commands.try_push_back(command))
    {
        ++state->command_overflow_count;
        state->last_status = VoxelBridgeStatus::CommandQueueFull;
        return state->last_status;
    }

    state->last_status = VoxelBridgeStatus::Ok;
    return state->last_status;
}

VoxelDrainResult VoxelGameFacade::drain_fixed_tick(VoxelSessionHandle handle, std::size_t maximum_commands)
{
    if (!on_owner_thread())
    {
        return {VoxelBridgeStatus::WrongThread, 0};
    }
    SessionState* state = find_state(handle);
    if (state == nullptr)
    {
        return {VoxelBridgeStatus::InvalidHandle, 0};
    }

    if (reject_reentrant_mutation(state))
    {
        return {VoxelBridgeStatus::ReentrantCall, 0};
    }

    VoxelDrainResult result;
    while (result.consumed < maximum_commands && !state->commands.empty())
    {
        // A command has no externally observable completion if there is no
        // room for its result event. Leave it at the FIFO head for a later
        // fixed tick instead of applying an unreportable edit.
        VoxelEvent* event_slot = state->events.reserve_back();
        if (event_slot == nullptr)
        {
            ++state->event_overflow_count;
            state->last_status = VoxelBridgeStatus::EventQueueFull;
            result.status = state->last_status;
            break;
        }

        const VoxelCommand command = state->commands.front();

        VoxelCommandResult command_result;
        try
        {
            AdapterInvocationGuard guard(m_adapter_invoking);
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

        *event_slot = VoxelEvent{};
        event_slot->type = command_result.accepted ? VoxelEventType::EditAccepted : VoxelEventType::EditRejected;
        event_slot->command = command.type;
        event_slot->status = command_result.accepted ? VoxelBridgeStatus::Ok : command_result.status;
        event_slot->sequence = state->next_event_sequence++;
        state->events.commit_reserved_back();
        state->commands.pop_front();
        state->last_status = event_slot->status;
        result.status = state->last_status;
        ++result.consumed;
    }

    return result;
}

VoxelBridgeStatus VoxelGameFacade::publish_action_pressed(VoxelSessionHandle handle, VoxelAction action)
{
    if (!on_owner_thread())
    {
        return VoxelBridgeStatus::WrongThread;
    }
    SessionState* state = find_state(handle);
    if (state == nullptr)
    {
        return VoxelBridgeStatus::InvalidHandle;
    }

    if (reject_reentrant_mutation(state))
    {
        return VoxelBridgeStatus::ReentrantCall;
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
    if (!on_owner_thread())
    {
        return VoxelBridgeStatus::WrongThread;
    }
    SessionState* state = find_state(handle);
    if (state == nullptr)
    {
        return VoxelBridgeStatus::InvalidHandle;
    }

    if (reject_reentrant_mutation(state))
    {
        return VoxelBridgeStatus::ReentrantCall;
    }

    const bool target_is_consistent = target.has_target
                                          ? target.face >= 0 && target.face <= 5
                                          : target.face == -1;
    if (!target_is_consistent)
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
    if (!on_owner_thread())
    {
        return false;
    }
    SessionState* state = find_state(handle);
    if (state == nullptr)
    {
        record_facade_status(VoxelBridgeStatus::InvalidHandle);
        return false;
    }

    if (reject_reentrant_mutation(state))
    {
        record_facade_status(VoxelBridgeStatus::ReentrantCall);
        return false;
    }

    if (state->events.empty())
    {
        record_facade_status(VoxelBridgeStatus::Ok);
        return false;
    }

    event = state->events.front();
    state->events.pop_front();
    record_facade_status(VoxelBridgeStatus::Ok);
    return true;
}

std::optional<VoxelQueueStats> VoxelGameFacade::queue_stats(VoxelSessionHandle handle) const
{
    if (!on_owner_thread())
    {
        return std::nullopt;
    }
    const SessionState* state = find_state(handle);
    if (state == nullptr)
    {
        record_facade_status(VoxelBridgeStatus::InvalidHandle);
        return std::nullopt;
    }

    record_facade_status(VoxelBridgeStatus::Ok);
    return VoxelQueueStats{
        state->commands.size(),
        state->events.size(),
        state->command_overflow_count,
        state->event_overflow_count};
}

VoxelBridgeStatus VoxelGameFacade::last_status(VoxelSessionHandle handle) const
{
    if (!on_owner_thread())
    {
        return VoxelBridgeStatus::WrongThread;
    }
    const SessionState* state = find_state(handle);
    return state == nullptr ? VoxelBridgeStatus::InvalidHandle : state->last_status;
}

std::string VoxelGameFacade::last_error(VoxelSessionHandle handle) const
{
    return voxel_bridge_status_message(last_status(handle));
}

bool VoxelGameFacade::on_owner_thread() const
{
    return std::this_thread::get_id() == m_owner_thread;
}

void VoxelGameFacade::record_facade_status(VoxelBridgeStatus status) const
{
    m_last_facade_status = status;
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

bool VoxelGameFacade::reject_reentrant_mutation(SessionState* state)
{
    if (!m_adapter_invoking)
    {
        return false;
    }

    if (state != nullptr)
    {
        state->last_status = VoxelBridgeStatus::ReentrantCall;
    }
    return true;
}

VoxelBridgeStatus VoxelGameFacade::push_event(SessionState& state, VoxelEvent event)
{
    event.sequence = state.next_event_sequence;
    if (!state.events.try_push_back(event))
    {
        ++state.event_overflow_count;
        state.last_status = VoxelBridgeStatus::EventQueueFull;
        return state.last_status;
    }

    ++state.next_event_sequence;
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
