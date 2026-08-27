#pragma once

#include "voxel_game/runtime/InputState.h"

#include <array>

namespace tt::voxel_game
{

struct FpsActionMap
{
    int forward_key = 87;
    int backward_key = 83;
    int left_key = 65;
    int right_key = 68;
};

class FpsCamera
{
public:
    explicit FpsCamera(FpsActionMap actions = {});

    void update(const InputState& input, double delta_seconds);
    void set_position(std::array<float, 3> position);

    const std::array<float, 3>& position() const;
    float yaw_degrees() const;
    float pitch_degrees() const;
    std::array<float, 3> forward() const;

private:
    FpsActionMap m_actions;
    std::array<float, 3> m_position{8.0F, 7.0F, 22.0F};
    float m_yaw_degrees = -90.0F;
    float m_pitch_degrees = -16.0F;
    float m_move_speed = 7.0F;
    float m_mouse_sensitivity = 0.10F;
};

} // namespace tt::voxel_game
