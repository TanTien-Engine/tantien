#include "voxel_game/bridge/VoxelGameFacade.h"

#include <exception>
#include <iostream>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace
{

using tt::voxel_game::VoxelAction;
using tt::voxel_game::VoxelBridgeConfig;
using tt::voxel_game::VoxelBridgeStatus;
using tt::voxel_game::VoxelCommand;
using tt::voxel_game::VoxelCommandResult;
using tt::voxel_game::VoxelCommandType;
using tt::voxel_game::VoxelEvent;
using tt::voxel_game::VoxelEventType;
using tt::voxel_game::VoxelGameFacade;
using tt::voxel_game::VoxelRuleFlag;
using tt::voxel_game::VoxelSessionHandle;
using tt::voxel_game::VoxelTargetSnapshot;

static_assert(std::is_nothrow_default_constructible<VoxelCommand>::value,
              "VoxelCommand must satisfy FixedRing's no-throw storage contract");
static_assert(std::is_nothrow_copy_assignable<VoxelCommand>::value,
              "VoxelCommand must satisfy FixedRing's no-throw storage contract");
static_assert(std::is_nothrow_default_constructible<VoxelEvent>::value,
              "VoxelEvent must satisfy FixedRing's no-throw storage contract");
static_assert(std::is_nothrow_copy_assignable<VoxelEvent>::value,
              "VoxelEvent must satisfy FixedRing's no-throw storage contract");

void require(bool condition, const char* message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

VoxelCommand command(VoxelCommandType type)
{
    VoxelCommand value;
    value.type = type;
    if (type == VoxelCommandType::BreakCurrentTarget || type == VoxelCommandType::PlaceSelectedBlock)
    {
        value.edit.target_x = 3;
        value.edit.target_y = 4;
        value.edit.target_z = 5;
        value.edit.expected_target_block = 2;
        value.edit.has_target = true;
    }
    if (type == VoxelCommandType::PlaceSelectedBlock)
    {
        value.edit.previous_x = 3;
        value.edit.previous_y = 3;
        value.edit.previous_z = 5;
        value.edit.expected_previous_block = 0;
        value.edit.has_previous_empty = true;
        value.selected_block = 2;
    }
    return value;
}

struct ThrowingAdapterCopy
{
    ThrowingAdapterCopy() = default;
    ThrowingAdapterCopy(const ThrowingAdapterCopy&)
    {
        throw std::runtime_error("adapter copy failure");
    }
    ThrowingAdapterCopy(ThrowingAdapterCopy&&) noexcept = default;

    VoxelCommandResult operator()(const VoxelCommand&) const
    {
        return VoxelCommandResult::accept();
    }
};

void test_handle_generation_destroy_reuse_and_reload()
{
    VoxelGameFacade facade;
    const auto adapter = [](const VoxelCommand&) { return VoxelCommandResult::accept(); };
    const auto first = facade.create_session(adapter);
    require(first.is_valid(), "session creation must return a valid opaque handle");
    require(facade.destroy_session(first) == VoxelBridgeStatus::Ok, "destroy must accept a live handle");
    require(!facade.is_session_live(first), "destroyed handle must be stale");
    require(facade.enqueue_command(first, command(VoxelCommandType::BreakCurrentTarget)) ==
                VoxelBridgeStatus::InvalidHandle,
            "destroyed handle must be strictly rejected");

    const auto reused = facade.create_session(adapter);
    require(reused.slot == first.slot, "destroyed slot should be reusable");
    require(reused.generation != first.generation, "reused slot must have a new generation");
    require(!facade.is_session_live(first), "old generation must not revive after slot reuse");

    const auto replacement = facade.reload_session(reused);
    require(replacement.has_value(), "reload must return a replacement handle");
    require(replacement->slot == reused.slot, "reload should preserve the opaque slot");
    require(replacement->generation != reused.generation, "reload must invalidate old proxies");
    require(!facade.is_session_live(reused), "pre-reload handle must be stale");
    require(facade.enqueue_command(reused, command(VoxelCommandType::BreakCurrentTarget)) ==
                VoxelBridgeStatus::InvalidHandle,
            "old reload proxy must be strictly rejected");

    VoxelEvent event;
    require(facade.try_pop_event(*replacement, event), "replacement session must receive reload event");
    require(event.type == VoxelEventType::WorldReloaded, "reload event type must be explicit");
    require(!facade.try_pop_event(*replacement, event), "reload event must be consumed exactly once");
}

void test_facade_ownership_and_create_exception_safety()
{
    VoxelGameFacade facade;
    std::function<VoxelCommandResult(const VoxelCommand&)> throwing_adapter{ThrowingAdapterCopy{}};
    require(!facade.create_session(throwing_adapter).is_valid(),
            "adapter copy failure must fail closed without creating a live session");
    require(facade.create_status() == VoxelBridgeStatus::StorageFailure,
            "adapter copy failure must expose storage status");

    const auto accepted_adapter = [](const VoxelCommand&) { return VoxelCommandResult::accept(); };
    const auto first = facade.create_session(accepted_adapter);
    require(first.is_valid() && first.slot == 0 && first.generation == 1,
            "failed creation must leave no ghost registry slot");

    VoxelGameFacade other_facade;
    const auto other = other_facade.create_session(accepted_adapter);
    require(other.is_valid() && other.slot == first.slot && other.generation == first.generation,
            "separate facades may reuse local slot and generation values");
    require(other.owner != first.owner, "facade identity must distinguish otherwise equal local handles");
    require(facade.enqueue_command(other, command(VoxelCommandType::BreakCurrentTarget)) ==
                VoxelBridgeStatus::InvalidHandle,
            "foreign facade handle must never be accepted");
    require(!facade.is_session_live(other), "foreign facade handle must fail closed for read queries");
    require(facade.is_owner_thread(), "construction thread must be the owner thread");
}

void test_reload_discards_queues_and_reports_counts()
{
    VoxelGameFacade facade({3, 3});
    const auto adapter = [](const VoxelCommand&) { return VoxelCommandResult::accept(); };
    const auto session = facade.create_session(adapter);
    require(facade.enqueue_command(session, command(VoxelCommandType::BreakCurrentTarget)) == VoxelBridgeStatus::Ok,
            "pending command must queue before reload");
    require(facade.enqueue_command(session, command(VoxelCommandType::PlaceSelectedBlock)) == VoxelBridgeStatus::Ok,
            "second pending command must queue before reload");
    require(facade.publish_action_pressed(session, VoxelAction::Primary) == VoxelBridgeStatus::Ok,
            "pending event must queue before reload");
    const auto replacement = facade.reload_session(session);
    require(replacement.has_value(), "reload must create replacement after preallocating state");
    require(!facade.is_session_live(session), "reload must invalidate old handle after commit");
    const auto stats = facade.queue_stats(*replacement);
    require(stats.has_value() && stats->command_count == 0 && stats->event_count == 1,
            "replacement must discard old queues and retain only reload report");
    VoxelEvent event;
    require(facade.try_pop_event(*replacement, event) && event.type == VoxelEventType::WorldReloaded &&
                event.discarded_command_count == 2 && event.discarded_event_count == 1,
            "reload event must report exactly which pending work was discarded");
}

void test_command_and_event_fifo_at_fixed_tick_boundary()
{
    std::vector<VoxelCommandType> applied;
    VoxelGameFacade facade;
    const auto session = facade.create_session([&applied](const VoxelCommand& value) {
        applied.push_back(value.type);
        return VoxelCommandResult::accept();
    });

    VoxelCommand selected = command(VoxelCommandType::SetSelectedBlock);
    selected.selected_block = 7;
    VoxelCommand rule = command(VoxelCommandType::SetRuleFlag);
    rule.rule_flag = VoxelRuleFlag::StrictPlacement;
    rule.rule_enabled = true;
    require(facade.enqueue_command(session, command(VoxelCommandType::BreakCurrentTarget)) == VoxelBridgeStatus::Ok,
            "break command must queue");
    require(facade.enqueue_command(session, selected) == VoxelBridgeStatus::Ok,
            "selection command must queue");
    require(facade.enqueue_command(session, rule) == VoxelBridgeStatus::Ok,
            "rule command must queue");
    require(applied.empty(), "queueing must not invoke the game adapter before fixed tick drain");

    const auto first_drain = facade.drain_fixed_tick(session, 2);
    require(first_drain.consumed == 2, "bounded fixed drain must consume its requested prefix");
    require(applied.size() == 2 && applied[0] == VoxelCommandType::BreakCurrentTarget &&
                applied[1] == VoxelCommandType::SetSelectedBlock,
            "commands must arrive at adapter in FIFO order");
    require(facade.drain_fixed_tick(session, 8).consumed == 1, "second drain must consume remaining command");
    require(applied[2] == VoxelCommandType::SetRuleFlag, "remaining command must preserve FIFO order");

    VoxelEvent event;
    require(facade.try_pop_event(session, event) && event.type == VoxelEventType::EditAccepted &&
                event.command == VoxelCommandType::BreakCurrentTarget,
            "first result event must match first command");
    require(facade.try_pop_event(session, event) && event.command == VoxelCommandType::SetSelectedBlock,
            "second result event must match second command");
    require(facade.try_pop_event(session, event) && event.command == VoxelCommandType::SetRuleFlag,
            "third result event must match third command");

    require(facade.publish_action_pressed(session, VoxelAction::Secondary) == VoxelBridgeStatus::Ok,
            "coarse action event must enqueue");
    require(facade.publish_target_changed(session, {2, 3, 4, 1, true}) == VoxelBridgeStatus::Ok,
            "coarse target event must enqueue");
    require(facade.try_pop_event(session, event) && event.type == VoxelEventType::ActionPressed &&
                event.action == VoxelAction::Secondary,
            "action event must be FIFO first");
    require(facade.try_pop_event(session, event) && event.type == VoxelEventType::TargetChanged &&
                event.target.x == 2 && event.target.face == 1,
            "target snapshot must be FIFO second");
}

void test_bounded_queues_and_result_events()
{
    VoxelGameFacade facade({2, 2});
    const auto session = facade.create_session([](const VoxelCommand& value) {
        return value.type == VoxelCommandType::PlaceSelectedBlock
                   ? VoxelCommandResult::reject(VoxelBridgeStatus::AdapterRejected)
                   : VoxelCommandResult::accept();
    });

    require(facade.enqueue_command(session, command(VoxelCommandType::BreakCurrentTarget)) == VoxelBridgeStatus::Ok,
            "first command must fit bounded queue");
    require(facade.enqueue_command(session, command(VoxelCommandType::PlaceSelectedBlock)) == VoxelBridgeStatus::Ok,
            "second command must fit bounded queue");
    require(facade.enqueue_command(session, command(VoxelCommandType::BreakCurrentTarget)) ==
                VoxelBridgeStatus::CommandQueueFull,
            "command overflow must be non-blocking and observable");
    const auto queued = facade.queue_stats(session);
    require(queued.has_value() && queued->command_count == 2 && queued->command_overflow_count == 1,
            "command queue must never exceed explicit capacity");

    const auto drain = facade.drain_fixed_tick(session, 2);
    require(drain.consumed == 2, "both queued commands must drain");
    const auto events = facade.queue_stats(session);
    require(events.has_value() && events->event_count == 2, "one result event per applied command must fit");

    VoxelEvent event;
    require(facade.try_pop_event(session, event) && event.type == VoxelEventType::EditAccepted,
            "accepted adapter result must publish EditAccepted");
    require(facade.try_pop_event(session, event) && event.type == VoxelEventType::EditRejected &&
                event.status == VoxelBridgeStatus::AdapterRejected,
            "rejected adapter result must publish EditRejected");

    require(facade.publish_action_pressed(session, VoxelAction::Primary) == VoxelBridgeStatus::Ok,
            "first event must fit bounded queue");
    require(facade.publish_action_pressed(session, VoxelAction::Secondary) == VoxelBridgeStatus::Ok,
            "second event must fit bounded queue");
    require(facade.publish_action_pressed(session, VoxelAction::SelectNext) == VoxelBridgeStatus::EventQueueFull,
            "event overflow must be non-blocking and observable");
    const auto overflow = facade.queue_stats(session);
    require(overflow.has_value() && overflow->event_count == 2 && overflow->event_overflow_count == 1,
            "event queue must never exceed explicit capacity");
}

void test_result_observability_and_rejection_normalization()
{
    std::vector<VoxelCommandType> applied;
    VoxelGameFacade facade({2, 1});
    const auto session = facade.create_session([&applied](const VoxelCommand& value) {
        applied.push_back(value.type);
        return VoxelCommandResult{false, VoxelBridgeStatus::Ok};
    });

    require(facade.publish_action_pressed(session, VoxelAction::Primary) == VoxelBridgeStatus::Ok,
            "setup event must fill bounded event queue");
    require(facade.enqueue_command(session, command(VoxelCommandType::BreakCurrentTarget)) == VoxelBridgeStatus::Ok,
            "queued edit must wait for an observable completion slot");
    const auto blocked_drain = facade.drain_fixed_tick(session, 1);
    require(blocked_drain.consumed == 0 && blocked_drain.status == VoxelBridgeStatus::EventQueueFull,
            "full event queue must stop fixed drain before applying edit");
    require(applied.empty(), "adapter must not run while completion event cannot be queued");
    const auto blocked_stats = facade.queue_stats(session);
    require(blocked_stats.has_value() && blocked_stats->command_count == 1 &&
                blocked_stats->event_count == 1 && blocked_stats->event_overflow_count == 1,
            "blocked command and event overflow must remain observable within capacity");

    VoxelEvent event;
    require(facade.try_pop_event(session, event) && event.type == VoxelEventType::ActionPressed,
            "freeing an event slot must preserve existing FIFO event");
    const auto recovered_drain = facade.drain_fixed_tick(session, 1);
    require(recovered_drain.consumed == 1 && applied.size() == 1,
            "command must apply only after an observable result slot exists");
    require(facade.try_pop_event(session, event) && event.type == VoxelEventType::EditRejected &&
                event.status == VoxelBridgeStatus::AdapterRejected,
            "rejected adapter result with Ok status must normalize to AdapterRejected");
}

void test_adapter_reentrancy_is_strictly_rejected()
{
    VoxelGameFacade facade({2, 1});
    VoxelSessionHandle session;
    std::vector<VoxelBridgeStatus> statuses;
    bool reload_returned_handle = true;
    bool pop_returned_event = true;
    VoxelBridgeStatus reload_facade_status = VoxelBridgeStatus::Ok;
    const auto adapter = [&facade, &session, &statuses, &reload_returned_handle, &pop_returned_event,
                          &reload_facade_status](
                             const VoxelCommand&) {
        statuses.push_back(facade.destroy_session(session));
        reload_returned_handle = facade.reload_session(session).has_value();
        reload_facade_status = facade.last_facade_status();
        statuses.push_back(facade.last_status(session));
        const auto recursive_drain = facade.drain_fixed_tick(session, 1);
        statuses.push_back(recursive_drain.status);
        VoxelCommand nested = command(VoxelCommandType::SetSelectedBlock);
        nested.selected_block = 2;
        statuses.push_back(facade.enqueue_command(session, nested));
        statuses.push_back(facade.publish_action_pressed(session, VoxelAction::Secondary));
        VoxelEvent ignored;
        pop_returned_event = facade.try_pop_event(session, ignored);
        statuses.push_back(facade.last_status(session));
        return VoxelCommandResult::accept();
    };
    session = facade.create_session(adapter);

    require(facade.enqueue_command(session, command(VoxelCommandType::BreakCurrentTarget)) == VoxelBridgeStatus::Ok,
            "outer command must queue before adapter reentrancy test");
    const auto outer_drain = facade.drain_fixed_tick(session, 1);
    require(outer_drain.status == VoxelBridgeStatus::Ok && outer_drain.consumed == 1,
            "outer drain must complete despite rejected adapter reentrancy");
    require(statuses.size() == 6, "every nested mutation must report an explicit status");
    for (VoxelBridgeStatus status : statuses)
    {
        require(status == VoxelBridgeStatus::ReentrantCall,
                "adapter mutation must be rejected by the non-reentrant boundary");
    }
    require(!reload_returned_handle, "adapter reload must not replace the active session");
    require(reload_facade_status == VoxelBridgeStatus::ReentrantCall,
            "reentrant optional reload must update facade diagnostics");
    require(!pop_returned_event, "adapter pop must not remove a reserved result slot");
    require(facade.is_session_live(session), "rejected adapter destroy must leave outer session live");
    const auto stats = facade.queue_stats(session);
    require(stats.has_value() && stats->command_count == 0 && stats->event_count == 1 &&
                stats->event_overflow_count == 0,
            "adapter reentrancy must neither reorder queues nor consume result capacity");
    VoxelEvent event;
    require(facade.try_pop_event(session, event) && event.type == VoxelEventType::EditAccepted &&
                event.command == VoxelCommandType::BreakCurrentTarget,
            "outer command must retain its observable result event in FIFO order");
}

void test_strict_rejections_and_exception_recovery()
{
    VoxelGameFacade invalid_config({0, 1});
    require(!invalid_config.create_session([](const VoxelCommand&) { return VoxelCommandResult::accept(); }).is_valid(),
            "zero queue capacity must fail closed");
    require(invalid_config.create_status() == VoxelBridgeStatus::InvalidInput,
            "invalid configuration must expose status");
    require(invalid_config.last_facade_status() == VoxelBridgeStatus::InvalidInput,
            "invalid handle-returning create must update facade diagnostics");

    VoxelGameFacade facade;
    require(!facade.create_session({}).is_valid(), "missing adapter must fail closed");
    require(facade.create_status() == VoxelBridgeStatus::CallbackMissing, "missing adapter must expose status");
    require(facade.last_facade_status() == VoxelBridgeStatus::CallbackMissing,
            "missing adapter create must update facade diagnostics");

    const auto session = facade.create_session([](const VoxelCommand&) -> VoxelCommandResult {
        throw std::runtime_error("adapter failure");
    });
    VoxelCommand zero_block = command(VoxelCommandType::SetSelectedBlock);
    require(facade.enqueue_command(session, zero_block) == VoxelBridgeStatus::InvalidInput,
            "empty block selection must be rejected before queueing");
    VoxelCommand invalid_rule = command(VoxelCommandType::SetRuleFlag);
    invalid_rule.rule_flag = static_cast<VoxelRuleFlag>(99);
    require(facade.enqueue_command(session, invalid_rule) == VoxelBridgeStatus::InvalidInput,
            "unknown rule flag must be rejected before queueing");
    require(facade.publish_target_changed(session, {0, 0, 0, 6, true}) == VoxelBridgeStatus::InvalidInput,
            "invalid target face must be strictly rejected");
    require(facade.last_status(session) == VoxelBridgeStatus::InvalidInput,
            "invalid target must update observable last status");
    require(facade.publish_target_changed(session, {0, 0, 0, -1, true}) == VoxelBridgeStatus::InvalidInput,
            "present target must carry a concrete face");
    require(facade.publish_target_changed(session, {0, 0, 0, 0, false}) == VoxelBridgeStatus::InvalidInput,
            "absent target must carry only face -1");
    require(facade.publish_target_changed(session, {0, 0, 0, -1, false}) == VoxelBridgeStatus::Ok,
            "absent target with face -1 must remain representable");
    VoxelEvent target_event;
    require(facade.try_pop_event(session, target_event) && target_event.type == VoxelEventType::TargetChanged,
            "valid absent target must publish its normal coarse event");
    require(facade.publish_action_pressed(session, static_cast<VoxelAction>(99)) == VoxelBridgeStatus::InvalidInput,
            "unknown action enum must be strictly rejected");
    require(facade.last_status(session) == VoxelBridgeStatus::InvalidInput,
            "invalid action must update observable last status");
    VoxelCommand missing_edit_snapshot;
    missing_edit_snapshot.type = VoxelCommandType::BreakCurrentTarget;
    require(facade.enqueue_command(session, missing_edit_snapshot) == VoxelBridgeStatus::InvalidInput,
            "edit command without an immutable target snapshot must reject before adapter invocation");
    VoxelCommand invalid_place_snapshot = command(VoxelCommandType::PlaceSelectedBlock);
    invalid_place_snapshot.edit.expected_previous_block = 2;
    require(facade.enqueue_command(session, invalid_place_snapshot) == VoxelBridgeStatus::InvalidInput,
            "place snapshot with a non-Air expected previous state must reject before adapter invocation");
    require(facade.enqueue_command(session, command(VoxelCommandType::BreakCurrentTarget)) == VoxelBridgeStatus::Ok,
            "valid command must remain queueable after rejected input");
    const auto drain = facade.drain_fixed_tick(session, 1);
    require(drain.status == VoxelBridgeStatus::AdapterException && drain.consumed == 1,
            "adapter exception must fail closed at the fixed tick boundary");
    VoxelEvent event;
    require(facade.try_pop_event(session, event) && event.type == VoxelEventType::EditRejected &&
                event.status == VoxelBridgeStatus::AdapterException,
            "exception recovery must expose a reject event rather than throw across boundary");
    require(facade.enqueue_command(session, command(VoxelCommandType::PlaceSelectedBlock)) == VoxelBridgeStatus::Ok,
            "exception must restore the adapter guard for subsequent fixed ticks");
    require(facade.drain_fixed_tick(session, 1).status == VoxelBridgeStatus::AdapterException,
            "restored guard must permit subsequent adapter invocation");
}

void test_facade_diagnostics_and_event_slot_reset()
{
    VoxelGameFacade diagnostics;
    VoxelEvent ignored;
    require(!diagnostics.reload_session({}).has_value() &&
                diagnostics.last_facade_status() == VoxelBridgeStatus::InvalidHandle,
            "invalid reload must update facade diagnostics");
    require(!diagnostics.is_session_live({}) &&
                diagnostics.last_facade_status() == VoxelBridgeStatus::InvalidHandle,
            "false liveness result must update facade diagnostics");
    require(!diagnostics.try_pop_event({}, ignored) &&
                diagnostics.last_facade_status() == VoxelBridgeStatus::InvalidHandle,
            "false pop result from invalid handle must update facade diagnostics");
    require(!diagnostics.queue_stats({}).has_value() &&
                diagnostics.last_facade_status() == VoxelBridgeStatus::InvalidHandle,
            "empty stats result must update facade diagnostics");

    VoxelGameFacade facade({1, 1});
    const auto session = facade.create_session([](const VoxelCommand&) { return VoxelCommandResult::accept(); });
    require(facade.publish_target_changed(session, {7, 8, 9, 4, true}) == VoxelBridgeStatus::Ok,
            "first event must populate every non-default payload field");
    require(facade.try_pop_event(session, ignored) && ignored.type == VoxelEventType::TargetChanged,
            "first event must release the only ring slot");
    require(facade.enqueue_command(session, command(VoxelCommandType::BreakCurrentTarget)) == VoxelBridgeStatus::Ok,
            "second event must reuse the released ring slot");
    require(facade.drain_fixed_tick(session, 1).consumed == 1,
            "fixed tick must write an accepted result into the reused slot");
    require(facade.try_pop_event(session, ignored) && ignored.type == VoxelEventType::EditAccepted &&
                ignored.action == VoxelAction::Primary && !ignored.target.has_target &&
                ignored.target.face == -1 && ignored.discarded_command_count == 0 &&
                ignored.discarded_event_count == 0,
            "reused result slot must not retain stale target, action, or reload payload");
}

} // namespace

int main()
{
    try
    {
        test_handle_generation_destroy_reuse_and_reload();
        test_facade_ownership_and_create_exception_safety();
        test_reload_discards_queues_and_reports_counts();
        test_command_and_event_fifo_at_fixed_tick_boundary();
        test_bounded_queues_and_result_events();
        test_result_observability_and_rejection_normalization();
        test_adapter_reentrancy_is_strictly_rejected();
        test_strict_rejections_and_exception_recovery();
        test_facade_diagnostics_and_event_slot_reset();
        std::cout << "voxel_bridge_tests: PASS" << std::endl;
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "voxel_bridge_tests: FAIL: " << error.what() << std::endl;
        return 1;
    }
}
