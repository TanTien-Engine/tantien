#include "voxel_game/bridge/VoxelGameFacade.h"

#include <exception>
#include <iostream>
#include <stdexcept>
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
using tt::voxel_game::VoxelTargetSnapshot;

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
    return value;
}

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

void test_strict_rejections_and_exception_recovery()
{
    VoxelGameFacade invalid_config({0, 1});
    require(!invalid_config.create_session([](const VoxelCommand&) { return VoxelCommandResult::accept(); }).is_valid(),
            "zero queue capacity must fail closed");
    require(invalid_config.create_status() == VoxelBridgeStatus::InvalidInput,
            "invalid configuration must expose status");

    VoxelGameFacade facade;
    require(!facade.create_session({}).is_valid(), "missing adapter must fail closed");
    require(facade.create_status() == VoxelBridgeStatus::CallbackMissing, "missing adapter must expose status");

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
    require(facade.publish_action_pressed(session, static_cast<VoxelAction>(99)) == VoxelBridgeStatus::InvalidInput,
            "unknown action enum must be strictly rejected");
    require(facade.last_status(session) == VoxelBridgeStatus::InvalidInput,
            "invalid action must update observable last status");
    require(facade.enqueue_command(session, command(VoxelCommandType::BreakCurrentTarget)) == VoxelBridgeStatus::Ok,
            "valid command must remain queueable after rejected input");
    const auto drain = facade.drain_fixed_tick(session, 1);
    require(drain.status == VoxelBridgeStatus::AdapterException && drain.consumed == 1,
            "adapter exception must fail closed at the fixed tick boundary");
    VoxelEvent event;
    require(facade.try_pop_event(session, event) && event.type == VoxelEventType::EditRejected &&
                event.status == VoxelBridgeStatus::AdapterException,
            "exception recovery must expose a reject event rather than throw across boundary");
}

} // namespace

int main()
{
    try
    {
        test_handle_generation_destroy_reuse_and_reload();
        test_command_and_event_fifo_at_fixed_tick_boundary();
        test_bounded_queues_and_result_events();
        test_result_observability_and_rejection_normalization();
        test_strict_rejections_and_exception_recovery();
        std::cout << "voxel_bridge_tests: PASS" << std::endl;
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "voxel_bridge_tests: FAIL: " << error.what() << std::endl;
        return 1;
    }
}
