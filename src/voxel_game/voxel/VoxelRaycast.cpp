#include "voxel_game/voxel/VoxelRaycast.h"

#include <cmath>
#include <limits>

namespace tt::voxel_game
{
namespace
{

int sign_of(double value)
{
    return value > 0.0 ? 1 : (value < 0.0 ? -1 : 0);
}

} // namespace

std::optional<VoxelRaycastHit> raycast_voxels(const World& world,
                                              std::array<double, 3> origin,
                                              std::array<double, 3> direction,
                                              double max_distance)
{
    const double length = std::sqrt(direction[0] * direction[0] +
                                    direction[1] * direction[1] +
                                    direction[2] * direction[2]);
    if (!(length > 0.0) || !(max_distance >= 0.0) ||
        !std::isfinite(length) || !std::isfinite(max_distance) ||
        !std::isfinite(origin[0]) || !std::isfinite(origin[1]) || !std::isfinite(origin[2]))
    {
        return std::nullopt;
    }
    for (double& component : direction)
    {
        component /= length;
    }

    BlockCoord current{
        static_cast<int>(std::floor(origin[0])),
        static_cast<int>(std::floor(origin[1])),
        static_cast<int>(std::floor(origin[2])),
    };
    if (is_opaque(world.get(current)))
    {
        return VoxelRaycastHit{current, {}, {}, 0.0, false};
    }

    std::array<int, 3> step{};
    std::array<double, 3> t_max{};
    std::array<double, 3> t_delta{};
    for (int axis = 0; axis < 3; ++axis)
    {
        step[axis] = sign_of(direction[axis]);
        if (step[axis] == 0)
        {
            t_max[axis] = std::numeric_limits<double>::infinity();
            t_delta[axis] = std::numeric_limits<double>::infinity();
            continue;
        }
        const double next_boundary = step[axis] > 0
            ? std::floor(origin[axis]) + 1.0
            : std::floor(origin[axis]);
        t_max[axis] = (next_boundary - origin[axis]) / direction[axis];
        t_delta[axis] = 1.0 / std::abs(direction[axis]);
    }

    double distance = 0.0;
    while (true)
    {
        int axis = 0;
        if (t_max[1] < t_max[axis])
        {
            axis = 1;
        }
        if (t_max[2] < t_max[axis])
        {
            axis = 2;
        }
        distance = t_max[axis];
        if (distance > max_distance)
        {
            return std::nullopt;
        }

        const BlockCoord previous = current;
        if (axis == 0)
        {
            current.x += step[axis];
        }
        else if (axis == 1)
        {
            current.y += step[axis];
        }
        else
        {
            current.z += step[axis];
        }
        t_max[axis] += t_delta[axis];

        if (is_opaque(world.get(current)))
        {
            BlockCoord normal{};
            if (axis == 0)
            {
                normal.x = -step[axis];
            }
            else if (axis == 1)
            {
                normal.y = -step[axis];
            }
            else
            {
                normal.z = -step[axis];
            }
            return VoxelRaycastHit{current, normal, previous, distance, true};
        }
    }
}

} // namespace tt::voxel_game
