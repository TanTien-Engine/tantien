#pragma once

#include "voxel_game/voxel/World.h"

#include <array>

namespace tt::voxel_game
{

// Player position is the centre of the soles. The collider extends half_width
// on X/Z and height upward on Y. All collision boxes are half-open: touching a
// voxel face is allowed, but a positive-volume intersection is not.
struct PlayerAabb
{
    std::array<double, 3> minimum{};
    std::array<double, 3> maximum{};
};

struct PlayerState
{
    std::array<double, 3> feet_position{0.5, 1.0, 0.5};
    std::array<double, 3> velocity{};
    bool grounded = false;
};

struct PlayerMoveIntent
{
    // World-space horizontal direction. It is normalized by the controller,
    // so diagonal input does not increase the walking speed.
    double x = 0.0;
    double z = 0.0;
};

struct PlayerMotionConfig
{
    double half_width = 0.30;
    double height = 1.80;
    double eye_height = 1.62;
    double walk_speed = 4.50;
    double gravity = 25.0;
    double terminal_fall_speed = 42.0;
    double jump_speed = 8.50;
};

PlayerAabb player_aabb(const PlayerState& player, const PlayerMotionConfig& config = {});
bool player_aabb_intersects_block(const PlayerAabb& aabb, BlockCoord block);
bool player_intersects_solid_world(const World& world, const PlayerAabb& aabb);
bool player_can_place_block(const PlayerState& player,
                            BlockCoord target,
                            const PlayerMotionConfig& config = {});

class PlayerController
{
public:
    explicit PlayerController(PlayerMotionConfig config = {});

    PlayerState& state();
    const PlayerState& state() const;
    const PlayerMotionConfig& config() const;
    std::array<double, 3> eye_position() const;

    // Queues one jump edge. The edge is retained until the next fixed tick,
    // where it is consumed exactly once. Airborne requests are rejected.
    bool request_jump();
    bool jump_pending() const;

    // Call only from a deterministic fixed-step loop. Swept, axis-by-axis
    // clipping prevents a move from tunnelling through a one-voxel wall.
    void fixed_step(const World& world, double fixed_delta_seconds, PlayerMoveIntent intent);

private:
    double clip_axis(const World& world, int axis, double desired_delta, bool& blocked) const;

    PlayerMotionConfig m_config;
    PlayerState m_state;
    bool m_jump_pending = false;
};

} // namespace tt::voxel_game
