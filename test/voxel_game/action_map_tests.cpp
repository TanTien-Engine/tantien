#include "voxel_game/runtime/ActionMap.h"
#include "voxel_game/runtime/CreativeHotbar.h"
#include "voxel_game/runtime/InputState.h"

#include <exception>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace
{

using tt::voxel_game::ActionFrame;
using tt::voxel_game::ActionMap;
using tt::voxel_game::BlockState;
using tt::voxel_game::ButtonAction;
using tt::voxel_game::CreativeHotbar;
using tt::voxel_game::GameAction;
using tt::voxel_game::InputState;

void require(bool condition, const char* message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

template <typename Function>
void require_rejected(Function&& function, const char* message)
{
    try
    {
        function();
    }
    catch (const std::exception&)
    {
        return;
    }
    throw std::runtime_error(message);
}

void test_single_key_edges_and_repeat()
{
    ActionMap map;
    InputState input;
    map.bind_key(GameAction::Jump, 32);
    map.begin_frame();
    input.begin_frame();
    input.on_key(32, ButtonAction::Press);
    input.on_key(32, ButtonAction::Repeat);
    const ActionFrame pressed = map.sample(input);
    require(pressed.held(GameAction::Jump), "key press holds jump");
    require(pressed.pressed(GameAction::Jump), "key press creates jump edge");
    require(!pressed.released(GameAction::Jump), "key press does not release jump");

    map.begin_frame();
    input.begin_frame();
    input.on_key(32, ButtonAction::Repeat);
    const ActionFrame repeated = map.sample(input);
    require(repeated.held(GameAction::Jump), "repeat preserves held state");
    require(!repeated.pressed(GameAction::Jump), "repeat creates no new press edge");

    map.begin_frame();
    input.begin_frame();
    input.on_key(32, ButtonAction::Release);
    const ActionFrame released = map.sample(input);
    require(!released.held(GameAction::Jump), "release clears held state");
    require(released.released(GameAction::Jump), "release creates release edge");
}

void test_logical_edges_and_short_tap()
{
    ActionMap map;
    InputState input;
    map.bind_key(GameAction::MoveForward, 87);
    map.bind_key(GameAction::MoveForward, 38);
    map.begin_frame();
    input.begin_frame();
    input.on_key(87, ButtonAction::Press);
    const ActionFrame first_key = map.sample(input);
    require(first_key.pressed(GameAction::MoveForward), "first of two keys presses action");

    map.begin_frame();
    input.begin_frame();
    input.on_key(38, ButtonAction::Press);
    const ActionFrame second_key = map.sample(input);
    require(second_key.held(GameAction::MoveForward), "second bound key preserves action held");
    require(!second_key.pressed(GameAction::MoveForward),
            "pressing another bound key does not re-press an already held action");

    map.begin_frame();
    input.begin_frame();
    input.on_key(87, ButtonAction::Release);
    const ActionFrame one_key_remaining = map.sample(input);
    require(one_key_remaining.held(GameAction::MoveForward), "second bound key preserves action held");
    require(!one_key_remaining.released(GameAction::MoveForward), "releasing one of two keys does not release action");

    map.begin_frame();
    input.begin_frame();
    input.on_key(38, ButtonAction::Release);
    const ActionFrame all_released = map.sample(input);
    require(all_released.released(GameAction::MoveForward), "last bound key releases action");

    map.begin_frame();
    input.begin_frame();
    input.on_key(87, ButtonAction::Press);
    input.on_key(87, ButtonAction::Release);
    const ActionFrame tap = map.sample(input);
    require(!tap.held(GameAction::MoveForward), "short tap ends unheld");
    require(tap.pressed(GameAction::MoveForward), "short tap retains press edge");
    require(tap.released(GameAction::MoveForward), "short tap retains release edge");
}

void test_duplicate_and_invalid_bindings()
{
    ActionMap map;
    InputState input;
    map.bind_mouse_button(GameAction::BreakBlock, 0);
    map.bind_mouse_button(GameAction::BreakBlock, 0);
    map.begin_frame();
    input.begin_frame();
    input.on_mouse_button(0, ButtonAction::Press);
    const ActionFrame pressed = map.sample(input);
    require(pressed.pressed(GameAction::BreakBlock), "duplicate binding has one press edge");
    require_rejected([&map]() { map.bind_key(GameAction::Jump, -1); }, "negative key binding is rejected");
    require_rejected([&map]() { map.bind_key(GameAction::Count, 1); }, "invalid action binding is rejected");

    map.begin_frame();
    input.begin_frame();
    require_rejected([&map, &input]() { map.sample(input); map.sample(input); }, "a frame cannot be sampled twice");
    require_rejected([]() { ActionFrame().held(GameAction::Count); }, "invalid frame query is rejected");
}

void test_creative_hotbar()
{
    CreativeHotbar hotbar;
    require(hotbar.selected_slot() == 0 && hotbar.selected_block() == BlockState::Stone,
            "hotbar defaults to stone");
    require(hotbar.select_slot(1) && hotbar.selected_block() == BlockState::Dirt,
            "second slot selects dirt");
    require(!hotbar.select_slot(CreativeHotbar::SlotCount), "out of range slot is rejected");
    require(hotbar.selected_block() == BlockState::Dirt, "rejected slot leaves selection unchanged");

    ActionMap map;
    InputState input;
    map.bind_key(GameAction::SelectSlot2, 2);
    map.bind_key(GameAction::SelectSlot4, 4);
    map.begin_frame();
    input.begin_frame();
    input.on_key(4, ButtonAction::Press);
    input.on_key(2, ButtonAction::Press);
    hotbar.apply_slot_actions(map.sample(input));
    require(hotbar.selected_slot() == 1 && hotbar.selected_block() == BlockState::Dirt,
            "multiple slot presses use the lowest slot priority");
}

} // namespace

int main()
{
    try
    {
        test_single_key_edges_and_repeat();
        test_logical_edges_and_short_tap();
        test_duplicate_and_invalid_bindings();
        test_creative_hotbar();
        std::cout << "action map tests passed" << std::endl;
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "action map tests failed: " << error.what() << std::endl;
        return 1;
    }
}
