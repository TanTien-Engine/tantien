#include "voxel_game/runtime/FrameClock.h"

#include <algorithm>

namespace tt::voxel_game
{

FrameClock::FrameClock(double maximum_delta_seconds)
    : m_maximum_delta_seconds(maximum_delta_seconds)
{
    if (maximum_delta_seconds <= 0.0)
    {
        throw std::invalid_argument("maximum frame delta must be positive");
    }
}

double FrameClock::sample()
{
    const Clock::time_point now = Clock::now();
    if (!m_started)
    {
        m_previous = now;
        m_started = true;
        return 0.0;
    }

    const double elapsed_seconds = std::chrono::duration<double>(now - m_previous).count();
    m_previous = now;
    return std::clamp(elapsed_seconds, 0.0, m_maximum_delta_seconds);
}

FixedStepAccumulator::FixedStepAccumulator(double tick_seconds,
                                           std::size_t maximum_ticks_per_frame)
    : m_tick_seconds(tick_seconds)
    , m_maximum_ticks_per_frame(maximum_ticks_per_frame)
{
    if (tick_seconds <= 0.0)
    {
        throw std::invalid_argument("fixed tick duration must be positive");
    }
    if (maximum_ticks_per_frame == 0)
    {
        throw std::invalid_argument("maximum ticks per frame must be positive");
    }
}

double FixedStepAccumulator::tick_seconds() const
{
    return m_tick_seconds;
}

double FixedStepAccumulator::pending_seconds() const
{
    return m_accumulator_seconds;
}

} // namespace tt::voxel_game
