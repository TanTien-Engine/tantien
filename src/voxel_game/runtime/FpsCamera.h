#pragma once

#include "voxel_game/runtime/InputState.h"

#include <array>

namespace tt::voxel_game
{

class FpsCamera
{
public:
    // The camera owns only first-person view orientation. PlayerController
    // owns all translation so a render frame cannot bypass fixed-step physics.
    void update_look(const InputState& input);

    float yaw_degrees() const;
    float pitch_degrees() const;
    std::array<float, 3> forward() const;

private:
    float m_yaw_degrees = -90.0F;
    float m_pitch_degrees = -16.0F;
    float m_mouse_sensitivity = 0.10F;
};

} // namespace tt::voxel_game
