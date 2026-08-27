#pragma once

#include <chrono>
#include <cstddef>
#include <stdexcept>

namespace tt::voxel_game
{

class FrameClock
{
public:
    explicit FrameClock(double maximum_delta_seconds = 0.25);

    // The first sample establishes the origin and returns zero. Subsequent
    // samples use steady_clock, never the VM's process CPU clock.
    double sample();

private:
    using Clock = std::chrono::steady_clock;

    Clock::time_point m_previous;
    double m_maximum_delta_seconds;
    bool m_started = false;
};

struct FixedStepResult
{
    std::size_t tick_count = 0;
    double interpolation_alpha = 0.0;
    double dropped_seconds = 0.0;
};

class FixedStepAccumulator
{
public:
    explicit FixedStepAccumulator(double tick_seconds = 1.0 / 60.0,
                                  std::size_t maximum_ticks_per_frame = 8);

    template<typename TickFunction>
    FixedStepResult advance(double frame_delta_seconds, TickFunction&& on_tick)
    {
        if (frame_delta_seconds < 0.0)
        {
            throw std::invalid_argument("frame delta must not be negative");
        }

        m_accumulator_seconds += frame_delta_seconds;

        FixedStepResult result;
        while (m_accumulator_seconds >= m_tick_seconds &&
               result.tick_count < m_maximum_ticks_per_frame)
        {
            on_tick(m_tick_seconds);
            m_accumulator_seconds -= m_tick_seconds;
            ++result.tick_count;
        }

        if (m_accumulator_seconds >= m_tick_seconds)
        {
            const std::size_t dropped_ticks =
                static_cast<std::size_t>(m_accumulator_seconds / m_tick_seconds);
            result.dropped_seconds = static_cast<double>(dropped_ticks) * m_tick_seconds;
            m_accumulator_seconds -= result.dropped_seconds;
        }

        result.interpolation_alpha = m_accumulator_seconds / m_tick_seconds;
        return result;
    }

    double tick_seconds() const;
    double pending_seconds() const;

private:
    double m_tick_seconds;
    double m_accumulator_seconds = 0.0;
    std::size_t m_maximum_ticks_per_frame;
};

} // namespace tt::voxel_game
