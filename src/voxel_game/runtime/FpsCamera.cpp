#include "voxel_game/runtime/FpsCamera.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace tt::voxel_game
{
namespace
{

constexpr float DegreesToRadians = 0.01745329251994329577F;

} // namespace

FpsCamera::FpsCamera(FpsActionMap actions)
    : m_actions(actions)
{
}

void FpsCamera::update(const InputState& input, double delta_seconds)
{
    if (delta_seconds < 0.0)
    {
        throw std::invalid_argument("camera delta must not be negative");
    }

    if (input.mouse_captured())
    {
        const MouseDelta mouse_delta = input.relative_mouse_delta();
        m_yaw_degrees += static_cast<float>(mouse_delta.x) * m_mouse_sensitivity;
        m_pitch_degrees -= static_cast<float>(mouse_delta.y) * m_mouse_sensitivity;
        m_pitch_degrees = std::clamp(m_pitch_degrees, -89.0F, 89.0F);
    }

    std::array<float, 3> facing = forward();
    facing[1] = 0.0F;
    const float horizontal_length = std::sqrt(facing[0] * facing[0] + facing[2] * facing[2]);
    facing[0] /= horizontal_length;
    facing[2] /= horizontal_length;
    const std::array<float, 3> right{-facing[2], 0.0F, facing[0]};
    float forward_axis = 0.0F;
    float right_axis = 0.0F;
    if (input.held(m_actions.forward_key))
    {
        forward_axis += 1.0F;
    }
    if (input.held(m_actions.backward_key))
    {
        forward_axis -= 1.0F;
    }
    if (input.held(m_actions.right_key))
    {
        right_axis += 1.0F;
    }
    if (input.held(m_actions.left_key))
    {
        right_axis -= 1.0F;
    }

    const float axis_length = std::sqrt(forward_axis * forward_axis + right_axis * right_axis);
    if (axis_length > 0.0F)
    {
        forward_axis /= axis_length;
        right_axis /= axis_length;
        const float movement = m_move_speed * static_cast<float>(delta_seconds);
        m_position[0] += (facing[0] * forward_axis + right[0] * right_axis) * movement;
        m_position[2] += (facing[2] * forward_axis + right[2] * right_axis) * movement;
    }
}

void FpsCamera::set_position(std::array<float, 3> position)
{
    m_position = position;
}

const std::array<float, 3>& FpsCamera::position() const
{
    return m_position;
}

float FpsCamera::yaw_degrees() const
{
    return m_yaw_degrees;
}

float FpsCamera::pitch_degrees() const
{
    return m_pitch_degrees;
}

std::array<float, 3> FpsCamera::forward() const
{
    const float yaw_radians = m_yaw_degrees * DegreesToRadians;
    const float pitch_radians = m_pitch_degrees * DegreesToRadians;
    const float horizontal_length = std::cos(pitch_radians);
    return {
        horizontal_length * std::cos(yaw_radians),
        std::sin(pitch_radians),
        horizontal_length * std::sin(yaw_radians),
    };
}

} // namespace tt::voxel_game
