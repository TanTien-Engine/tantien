#include "voxel_game/bridge/VoxelGameFacade.h"
#include "voxel_game/runtime/VoxelRuntimeAdapter.h"

#include <exception>
#include <iostream>
#include <optional>
#include <stdexcept>

namespace
{

using tt::voxel_game::BlockCoord;
using tt::voxel_game::BlockState;
using tt::voxel_game::PlayerController;
using tt::voxel_game::VoxelBridgeConfig;
using tt::voxel_game::VoxelBridgeStatus;
using tt::voxel_game::VoxelCommand;
using tt::voxel_game::VoxelCommandType;
using tt::voxel_game::VoxelEvent;
using tt::voxel_game::VoxelEventType;
using tt::voxel_game::VoxelGameFacade;
using tt::voxel_game::VoxelRaycastHit;
using tt::voxel_game::VoxelRuleFlag;
using tt::voxel_game::VoxelRuntimeAdapter;
using tt::voxel_game::World;

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

VoxelRaycastHit target(BlockCoord block, BlockCoord previous_empty)
{
    VoxelRaycastHit value;
    value.block = block;
    value.previous_empty = previous_empty;
    value.has_previous_empty = true;
    return value;
}

void select_block(VoxelRuntimeAdapter& adapter, BlockState block)
{
    VoxelCommand selection = command(VoxelCommandType::SetSelectedBlock);
    selection.selected_block = static_cast<std::uint16_t>(block);
    require(adapter.apply(selection).accepted, "registered hotbar block must be selectable");
}

void set_rule(VoxelRuntimeAdapter& adapter, VoxelRuleFlag flag, bool enabled)
{
    VoxelCommand rule = command(VoxelCommandType::SetRuleFlag);
    rule.rule_flag = flag;
    rule.rule_enabled = enabled;
    require(adapter.apply(rule).accepted, "known rule flag must update deterministically");
}

void test_direct_adapter_rules_selection_and_stale_targets()
{
    World world;
    PlayerController player;
    player.state().feet_position = {100.5, 100.0, 100.5};
    VoxelRuntimeAdapter adapter(world, player);

    require(adapter.selected_block() == BlockState::Stone && adapter.allow_edits() && adapter.strict_placement(),
            "adapter defaults must be Stone, edits enabled, and strict placement enabled");
    require(!adapter.apply(command(VoxelCommandType::BreakCurrentTarget)).accepted &&
                !adapter.apply(command(VoxelCommandType::PlaceSelectedBlock)).accepted,
            "missing target must reject both edits");

    world.set({0, 4, 0}, BlockState::Stone);
    adapter.set_target(target({0, 4, 0}, {0, 3, 0}));
    require(adapter.apply(command(VoxelCommandType::BreakCurrentTarget)).accepted &&
                world.get({0, 4, 0}) == BlockState::Air,
            "break must mutate a live non-Air target");
    require(!adapter.apply(command(VoxelCommandType::BreakCurrentTarget)).accepted,
            "a stale second break must fail closed");
    require(!adapter.apply(command(VoxelCommandType::PlaceSelectedBlock)).accepted,
            "a stale target after break must not be repurposed for placement");

    world.set({2, 4, 0}, BlockState::Stone);
    adapter.set_target(target({2, 4, 0}, {2, 3, 0}));
    require(adapter.apply(command(VoxelCommandType::PlaceSelectedBlock)).accepted &&
                world.get({2, 3, 0}) == BlockState::Stone,
            "default Stone selection must place through the adapter");

    const BlockState selectable[] = {BlockState::Dirt, BlockState::Grass, BlockState::Clay};
    for (std::size_t index = 0; index < 3; ++index)
    {
        const int x = 4 + static_cast<int>(index) * 2;
        select_block(adapter, selectable[index]);
        world.set({x, 4, 0}, BlockState::Stone);
        adapter.set_target(target({x, 4, 0}, {x, 3, 0}));
        require(adapter.apply(command(VoxelCommandType::PlaceSelectedBlock)).accepted &&
                    world.get({x, 3, 0}) == selectable[index],
                "selected registered block must control subsequent placement");
    }

    const BlockState before_invalid = adapter.selected_block();
    VoxelCommand air = command(VoxelCommandType::SetSelectedBlock);
    air.selected_block = static_cast<std::uint16_t>(BlockState::Air);
    require(adapter.apply(air).status == VoxelBridgeStatus::InvalidInput &&
                adapter.selected_block() == before_invalid,
            "Air selection must reject without changing current selection");
    VoxelCommand unknown = command(VoxelCommandType::SetSelectedBlock);
    unknown.selected_block = 99;
    require(adapter.apply(unknown).status == VoxelBridgeStatus::InvalidInput &&
                adapter.selected_block() == before_invalid,
            "unknown selection must reject without changing current selection");

    set_rule(adapter, VoxelRuleFlag::AllowEdits, false);
    world.set({10, 4, 0}, BlockState::Stone);
    adapter.set_target(target({10, 4, 0}, {10, 3, 0}));
    require(!adapter.apply(command(VoxelCommandType::BreakCurrentTarget)).accepted &&
                !adapter.apply(command(VoxelCommandType::PlaceSelectedBlock)).accepted &&
                world.get({10, 4, 0}) == BlockState::Stone &&
                world.get({10, 3, 0}) == BlockState::Air,
            "disabled edits must block both world edit commands");
    select_block(adapter, BlockState::Stone);
    set_rule(adapter, VoxelRuleFlag::StrictPlacement, false);
    set_rule(adapter, VoxelRuleFlag::AllowEdits, true);
    require(adapter.strict_placement() == false && adapter.allow_edits(),
            "selection and rules must remain available while edits are disabled");

    world.set({12, 4, 0}, BlockState::Stone);
    world.set({12, 3, 0}, BlockState::Dirt);
    adapter.set_target(target({12, 4, 0}, {12, 3, 0}));
    require(!adapter.apply(command(VoxelCommandType::PlaceSelectedBlock)).accepted &&
                world.get({12, 3, 0}) == BlockState::Dirt,
            "occupied previous-empty cell must reject stale placement without overwrite");

    world.set({14, 4, 0}, BlockState::Stone);
    adapter.set_target(target({14, 4, 0}, {14, 3, 0}));
    world.set({14, 4, 0}, BlockState::Air);
    require(!adapter.apply(command(VoxelCommandType::PlaceSelectedBlock)).accepted &&
                world.get({14, 3, 0}) == BlockState::Air,
            "an Air original target must reject stale placement without acting elsewhere");
}

void test_strict_placement_and_invalid_rule()
{
    World world;
    PlayerController player;
    VoxelRuntimeAdapter adapter(world, player);
    world.set({0, 3, 0}, BlockState::Stone);
    adapter.set_target(target({0, 3, 0}, {0, 1, 0}));
    require(!adapter.apply(command(VoxelCommandType::PlaceSelectedBlock)).accepted &&
                world.get({0, 1, 0}) == BlockState::Air,
            "strict placement must reject a block intersecting the player AABB");
    set_rule(adapter, VoxelRuleFlag::StrictPlacement, false);
    require(adapter.apply(command(VoxelCommandType::PlaceSelectedBlock)).accepted &&
                world.get({0, 1, 0}) == BlockState::Stone,
            "disabling strict placement must permit the identical otherwise-live command");

    VoxelCommand invalid_rule = command(VoxelCommandType::SetRuleFlag);
    invalid_rule.rule_flag = static_cast<VoxelRuleFlag>(99);
    require(adapter.apply(invalid_rule).status == VoxelBridgeStatus::InvalidInput,
            "adapter-recognizable unknown rule flag must be InvalidInput");
    VoxelCommand invalid_type = command(static_cast<VoxelCommandType>(99));
    require(adapter.apply(invalid_type).status == VoxelBridgeStatus::InvalidInput,
            "adapter-recognizable unknown command type must be InvalidInput");
}

void test_facade_fifo_and_side_effect_gates()
{
    World world;
    PlayerController player;
    player.state().feet_position = {100.5, 100.0, 100.5};
    VoxelRuntimeAdapter adapter(world, player);
    world.set({0, 4, 0}, BlockState::Stone);
    adapter.set_target(target({0, 4, 0}, {0, 3, 0}));
    VoxelGameFacade facade({4, 4});
    const auto session = facade.create_session([&adapter](const VoxelCommand& value) {
        return adapter.apply(value);
    });
    require(session.is_valid(), "facade session must outlive neither adapter nor borrowed world/player");
    VoxelCommand selection = command(VoxelCommandType::SetSelectedBlock);
    selection.selected_block = static_cast<std::uint16_t>(BlockState::Dirt);
    require(facade.enqueue_command(session, selection) == VoxelBridgeStatus::Ok &&
                facade.enqueue_command(session, command(VoxelCommandType::PlaceSelectedBlock)) == VoxelBridgeStatus::Ok,
            "selection and edit must queue in FIFO order");
    require(facade.drain_fixed_tick(session, 2).consumed == 2 &&
                world.get({0, 3, 0}) == BlockState::Dirt,
            "FIFO drain must apply selected block before same-frame placement");
    VoxelEvent event;
    require(facade.try_pop_event(session, event) && event.type == VoxelEventType::EditAccepted &&
                event.command == VoxelCommandType::SetSelectedBlock &&
                facade.try_pop_event(session, event) && event.type == VoxelEventType::EditAccepted &&
                event.command == VoxelCommandType::PlaceSelectedBlock,
            "completion events must preserve command FIFO order and accepted state");

    World queue_world;
    PlayerController queue_player;
    queue_player.state().feet_position = {100.5, 100.0, 100.5};
    VoxelRuntimeAdapter queue_adapter(queue_world, queue_player);
    queue_world.set({2, 4, 0}, BlockState::Stone);
    queue_adapter.set_target(target({2, 4, 0}, {2, 3, 0}));
    VoxelGameFacade queue_facade({1, 2});
    const auto queue_session = queue_facade.create_session([&queue_adapter](const VoxelCommand& value) {
        return queue_adapter.apply(value);
    });
    require(queue_facade.enqueue_command(queue_session, command(VoxelCommandType::PlaceSelectedBlock)) ==
                VoxelBridgeStatus::Ok &&
                queue_facade.enqueue_command(queue_session, command(VoxelCommandType::BreakCurrentTarget)) ==
                    VoxelBridgeStatus::CommandQueueFull &&
                queue_world.get({2, 3, 0}) == BlockState::Air,
            "a full command queue must not apply world mutations eagerly");

    World event_world;
    PlayerController event_player;
    event_player.state().feet_position = {100.5, 100.0, 100.5};
    VoxelRuntimeAdapter event_adapter(event_world, event_player);
    event_world.set({4, 4, 0}, BlockState::Stone);
    event_adapter.set_target(target({4, 4, 0}, {4, 3, 0}));
    VoxelGameFacade event_facade({1, 1});
    const auto event_session = event_facade.create_session([&event_adapter](const VoxelCommand& value) {
        return event_adapter.apply(value);
    });
    require(event_facade.publish_action_pressed(event_session, tt::voxel_game::VoxelAction::Primary) ==
                VoxelBridgeStatus::Ok &&
                event_facade.enqueue_command(event_session, command(VoxelCommandType::BreakCurrentTarget)) ==
                    VoxelBridgeStatus::Ok &&
                event_facade.drain_fixed_tick(event_session, 1).consumed == 0 &&
                event_world.get({4, 4, 0}) == BlockState::Stone,
            "a full completion queue must prevent adapter execution before observable completion exists");
    require(event_facade.try_pop_event(event_session, event) &&
                event_facade.drain_fixed_tick(event_session, 1).consumed == 1 &&
                event_world.get({4, 4, 0}) == BlockState::Air,
            "freeing completion capacity must allow the retained FIFO command to execute");

    VoxelCommand zero_selection = command(VoxelCommandType::SetSelectedBlock);
    require(facade.enqueue_command(session, zero_selection) == VoxelBridgeStatus::InvalidInput,
            "facade must strictly reject its directly-invalid bridge command before adapter invocation");
    VoxelCommand bad_rule = command(VoxelCommandType::SetRuleFlag);
    bad_rule.rule_flag = static_cast<VoxelRuleFlag>(99);
    require(facade.enqueue_command(session, bad_rule) == VoxelBridgeStatus::InvalidInput,
            "facade must strictly reject unknown bridge rule flags before adapter invocation");
}

} // namespace

int main()
{
    try
    {
        test_direct_adapter_rules_selection_and_stale_targets();
        test_strict_placement_and_invalid_rule();
        test_facade_fifo_and_side_effect_gates();
        std::cout << "voxel_runtime_adapter_tests: PASS" << std::endl;
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "voxel_runtime_adapter_tests: FAIL: " << error.what() << std::endl;
        return 1;
    }
}
