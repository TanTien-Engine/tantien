#include "voxel_game/runtime/PlayerController.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>

namespace tt::voxel_game
{
namespace
{

constexpr double CollisionEpsilon = 1.0e-9;

std::int64_t floor_to_block_index(double value)
{
    const double floored = std::floor(value);
    if (!std::isfinite(floored) ||
        floored < static_cast<double>((std::numeric_limits<int>::min)()) ||
        floored > static_cast<double>((std::numeric_limits<int>::max)()))
    {
        throw std::out_of_range("player position is outside the BlockCoord domain");
    }
    return static_cast<std::int64_t>(floored);
}

int narrow_block_index(std::int64_t value)
{
    if (value < static_cast<std::int64_t>((std::numeric_limits<int>::min)()) ||
        value > static_cast<std::int64_t>((std::numeric_limits<int>::max)()))
    {
        throw std::out_of_range("player sweep leaves the BlockCoord domain");
    }
    return static_cast<int>(value);
}

bool finite_config(const PlayerMotionConfig& config)
{
    return std::isfinite(config.half_width) && std::isfinite(config.height) &&
           std::isfinite(config.eye_height) && std::isfinite(config.walk_speed) &&
           std::isfinite(config.gravity) && std::isfinite(config.terminal_fall_speed) &&
           std::isfinite(config.jump_speed);
}

bool finite_state(const PlayerState& state)
{
    for (double value : state.feet_position)
    {
        if (!std::isfinite(value))
        {
            return false;
        }
    }
    for (double value : state.velocity)
    {
        if (!std::isfinite(value))
        {
            return false;
        }
    }
    return true;
}

bool solid(const World& world, BlockCoord coordinate)
{
    return world.get(coordinate) != BlockState::Air;
}

} // namespace

PlayerAabb player_aabb(const PlayerState& player, const PlayerMotionConfig& config)
{
    return {{player.feet_position[0] - config.half_width,
             player.feet_position[1],
             player.feet_position[2] - config.half_width},
            {player.feet_position[0] + config.half_width,
             player.feet_position[1] + config.height,
             player.feet_position[2] + config.half_width}};
}

bool player_aabb_intersects_block(const PlayerAabb& aabb, BlockCoord block)
{
    return aabb.minimum[0] < static_cast<double>(block.x) + 1.0 &&
           aabb.maximum[0] > static_cast<double>(block.x) &&
           aabb.minimum[1] < static_cast<double>(block.y) + 1.0 &&
           aabb.maximum[1] > static_cast<double>(block.y) &&
           aabb.minimum[2] < static_cast<double>(block.z) + 1.0 &&
           aabb.maximum[2] > static_cast<double>(block.z);
}

bool player_intersects_solid_world(const World& world, const PlayerAabb& aabb)
{
    const std::int64_t min_x = floor_to_block_index(aabb.minimum[0] + CollisionEpsilon);
    const std::int64_t max_x = floor_to_block_index(aabb.maximum[0] - CollisionEpsilon);
    const std::int64_t min_y = floor_to_block_index(aabb.minimum[1] + CollisionEpsilon);
    const std::int64_t max_y = floor_to_block_index(aabb.maximum[1] - CollisionEpsilon);
    const std::int64_t min_z = floor_to_block_index(aabb.minimum[2] + CollisionEpsilon);
    const std::int64_t max_z = floor_to_block_index(aabb.maximum[2] - CollisionEpsilon);
    for (std::int64_t z = min_z; z <= max_z; ++z)
    {
        for (std::int64_t y = min_y; y <= max_y; ++y)
        {
            for (std::int64_t x = min_x; x <= max_x; ++x)
            {
                if (solid(world, {narrow_block_index(x), narrow_block_index(y), narrow_block_index(z)}))
                {
                    return true;
                }
            }
        }
    }
    return false;
}

bool player_can_place_block(const PlayerState& player, BlockCoord target, const PlayerMotionConfig& config)
{
    return !player_aabb_intersects_block(player_aabb(player, config), target);
}

PlayerController::PlayerController(PlayerMotionConfig config)
    : m_config(config)
{
    if (!finite_config(m_config) || m_config.half_width <= 0.0 || m_config.height <= 0.0 ||
        m_config.eye_height < 0.0 || m_config.eye_height > m_config.height ||
        m_config.walk_speed < 0.0 || m_config.gravity <= 0.0 ||
        m_config.terminal_fall_speed <= 0.0 || m_config.jump_speed <= 0.0)
    {
        throw std::invalid_argument("invalid player motion configuration");
    }
}

PlayerState& PlayerController::state()
{
    return m_state;
}

const PlayerState& PlayerController::state() const
{
    return m_state;
}

const PlayerMotionConfig& PlayerController::config() const
{
    return m_config;
}

std::array<double, 3> PlayerController::eye_position() const
{
    return {m_state.feet_position[0], m_state.feet_position[1] + m_config.eye_height,
            m_state.feet_position[2]};
}

bool PlayerController::request_jump()
{
    if (!m_state.grounded || m_jump_pending)
    {
        return false;
    }
    m_jump_pending = true;
    return true;
}

bool PlayerController::jump_pending() const
{
    return m_jump_pending;
}

void PlayerController::fixed_step(const World& world,
                                  double fixed_delta_seconds,
                                  PlayerMoveIntent intent)
{
    if (!std::isfinite(fixed_delta_seconds) || fixed_delta_seconds <= 0.0 ||
        !finite_state(m_state) || !std::isfinite(intent.x) || !std::isfinite(intent.z))
    {
        throw std::invalid_argument("player fixed delta must be positive and finite");
    }

    const double intent_length = std::sqrt(intent.x * intent.x + intent.z * intent.z);
    if (intent_length > 1.0)
    {
        intent.x /= intent_length;
        intent.z /= intent_length;
    }
    m_state.velocity[0] = intent.x * m_config.walk_speed;
    m_state.velocity[2] = intent.z * m_config.walk_speed;

    if (m_jump_pending)
    {
        m_state.velocity[1] = m_config.jump_speed;
        m_state.grounded = false;
        m_jump_pending = false;
    }
    else
    {
        m_state.velocity[1] = std::max(m_state.velocity[1] - m_config.gravity * fixed_delta_seconds,
                                       -m_config.terminal_fall_speed);
        m_state.grounded = false;
    }

    for (int axis = 0; axis < 3; ++axis)
    {
        const double desired_delta = m_state.velocity[axis] * fixed_delta_seconds;
        bool blocked = false;
        const double actual_delta = clip_axis(world, axis, desired_delta, blocked);
        m_state.feet_position[axis] += actual_delta;
        if (blocked)
        {
            m_state.velocity[axis] = 0.0;
            if (axis == 1 && desired_delta < 0.0)
            {
                m_state.grounded = true;
            }
        }
    }
}

double PlayerController::clip_axis(const World& world, int axis, double desired_delta, bool& blocked) const
{
    blocked = false;
    if (desired_delta == 0.0)
    {
        return 0.0;
    }
    const PlayerAabb aabb = player_aabb(m_state, m_config);
    const double minimum = aabb.minimum[axis];
    const double maximum = aabb.maximum[axis];
    const double end = (desired_delta > 0.0 ? maximum : minimum) + desired_delta;

    std::int64_t ranges[3][2] = {
        {floor_to_block_index(aabb.minimum[0] + CollisionEpsilon), floor_to_block_index(aabb.maximum[0] - CollisionEpsilon)},
        {floor_to_block_index(aabb.minimum[1] + CollisionEpsilon), floor_to_block_index(aabb.maximum[1] - CollisionEpsilon)},
        {floor_to_block_index(aabb.minimum[2] + CollisionEpsilon), floor_to_block_index(aabb.maximum[2] - CollisionEpsilon)},
    };
    std::int64_t first = 0;
    std::int64_t last = -1;
    std::int64_t increment = 1;
    if (desired_delta > 0.0)
    {
        first = floor_to_block_index(maximum - CollisionEpsilon) + 1;
        last = floor_to_block_index(end - CollisionEpsilon);
    }
    else
    {
        first = floor_to_block_index(minimum + CollisionEpsilon) - 1;
        last = floor_to_block_index(end + CollisionEpsilon);
        increment = -1;
    }

    double clipped_delta = desired_delta;
    for (int moving_cell = first;
         increment > 0 ? moving_cell <= last : moving_cell >= last;
         moving_cell += increment)
    {
        ranges[axis][0] = moving_cell;
        ranges[axis][1] = moving_cell;
        for (std::int64_t z = ranges[2][0]; z <= ranges[2][1]; ++z)
        {
            for (std::int64_t y = ranges[1][0]; y <= ranges[1][1]; ++y)
            {
                for (std::int64_t x = ranges[0][0]; x <= ranges[0][1]; ++x)
                {
                    if (!solid(world, {narrow_block_index(x), narrow_block_index(y), narrow_block_index(z)}))
                    {
                        continue;
                    }
                    const std::int64_t coordinate = axis == 0 ? x : (axis == 1 ? y : z);
                    const double boundary = desired_delta > 0.0
                        ? static_cast<double>(coordinate) - maximum
                        : static_cast<double>(coordinate) + 1.0 - minimum;
                    if ((desired_delta > 0.0 && boundary < clipped_delta) ||
                        (desired_delta < 0.0 && boundary > clipped_delta))
                    {
                        clipped_delta = boundary;
                        blocked = true;
                    }
                }
            }
        }
    }
    return clipped_delta;
}

} // namespace tt::voxel_game
