#include "voxel_game/runtime/FpsCamera.h"

#include <algorithm>
#include <cmath>

namespace tt::voxel_game
{
namespace
{

constexpr float DegreesToRadians = 0.01745329251994329577F;

} // namespace

void FpsCamera::update_look(const InputState& input)
{
    if (input.mouse_captured())
    {
        const MouseDelta mouse_delta = input.relative_mouse_delta();
        m_yaw_degrees += static_cast<float>(mouse_delta.x) * m_mouse_sensitivity;
        m_pitch_degrees -= static_cast<float>(mouse_delta.y) * m_mouse_sensitivity;
        m_pitch_degrees = std::clamp(m_pitch_degrees, -89.0F, 89.0F);
    }

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
