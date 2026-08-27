#include "voxel_game/runtime/FrameClock.h"
#include "voxel_game/runtime/InputState.h"

#include <cmath>
#include <exception>
#include <iostream>
#include <stdexcept>

namespace
{

void require(bool condition, const char* message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

void require_near(double actual, double expected, const char* message)
{
    require(std::abs(actual - expected) < 1e-9, message);
}

void test_fixed_tick_accumulator()
{
    tt::voxel_game::FixedStepAccumulator ticks(0.1, 2);
    double simulated_seconds = 0.0;

    const auto first = ticks.advance(0.05, [&simulated_seconds](double delta)
    {
        simulated_seconds += delta;
    });
    require(first.tick_count == 0, "partial frame must not tick");
    require_near(first.interpolation_alpha, 0.5, "partial frame alpha");

    const auto second = ticks.advance(0.15, [&simulated_seconds](double delta)
    {
        simulated_seconds += delta;
    });
    require(second.tick_count == 2, "accumulator must run two fixed ticks");
    require_near(simulated_seconds, 0.2, "fixed tick simulation duration");
    require_near(second.interpolation_alpha, 0.0, "exact tick boundary alpha");

    const auto capped = ticks.advance(0.35, [&simulated_seconds](double delta)
    {
        simulated_seconds += delta;
    });
    require(capped.tick_count == 2, "per-frame tick cap");
    require_near(capped.dropped_seconds, 0.1, "overrun must drop full ticks only");
    require_near(capped.interpolation_alpha, 0.5, "overrun retains interpolation remainder");
}

void test_input_transitions()
{
    tt::voxel_game::InputState input;
    input.begin_frame();
    input.on_key(87, tt::voxel_game::ButtonAction::Press);
    input.on_key(65, tt::voxel_game::ButtonAction::Press);
    require(input.held(87) && input.held(65), "simultaneous held keys");
    require(input.pressed(87) && input.pressed(65), "simultaneous pressed keys");

    input.on_key(87, tt::voxel_game::ButtonAction::Repeat);
    require(input.pressed(87), "repeat must not erase first press");

    input.begin_frame();
    require(input.held(87) && input.held(65), "held keys persist across frames");
    require(!input.pressed(87), "pressed state lasts one frame");

    input.on_key(87, tt::voxel_game::ButtonAction::Release);
    require(!input.held(87) && input.released(87), "release transition");
    require(input.held(65), "unrelated key remains held");

    input.set_mouse_captured(true);
    input.on_cursor_position(100.0, 100.0);
    input.on_cursor_position(103.0, 96.0);
    const auto delta = input.relative_mouse_delta();
    require_near(delta.x, 3.0, "captured mouse x delta");
    require_near(delta.y, -4.0, "captured mouse y delta");

    input.on_key(tt::voxel_game::InputState::EscapeKey, tt::voxel_game::ButtonAction::Press);
    require(!input.mouse_captured(), "Escape releases mouse capture");
}

} // namespace

int main()
{
    try
    {
        test_fixed_tick_accumulator();
        test_input_transitions();
        std::cout << "voxel_runtime_tests: PASS" << std::endl;
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "voxel_runtime_tests: FAIL: " << error.what() << std::endl;
        return 1;
    }
}
