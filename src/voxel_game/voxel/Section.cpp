#include "voxel_game/voxel/Section.h"

#include <stdexcept>

namespace tt::voxel_game
{

bool Section::contains(BlockCoord coordinate)
{
    return coordinate.x >= 0 && coordinate.x < Size &&
           coordinate.y >= 0 && coordinate.y < Size &&
           coordinate.z >= 0 && coordinate.z < Size;
}

std::size_t Section::index_of(BlockCoord coordinate)
{
    if (!contains(coordinate))
    {
        throw std::out_of_range("section coordinate is outside [0, 16)");
    }

    return static_cast<std::size_t>(coordinate.x) +
           static_cast<std::size_t>(Size) *
               (static_cast<std::size_t>(coordinate.z) +
                static_cast<std::size_t>(Size) * static_cast<std::size_t>(coordinate.y));
}

BlockState Section::get(BlockCoord coordinate) const
{
    return m_blocks[index_of(coordinate)];
}

BlockState Section::get_or_air(BlockCoord coordinate) const
{
    if (!contains(coordinate))
    {
        return BlockState::Air;
    }
    return get(coordinate);
}

void Section::set(BlockCoord coordinate, BlockState state)
{
    m_blocks[index_of(coordinate)] = state;
}

void Section::fill(BlockState state)
{
    m_blocks.fill(state);
}

} // namespace tt::voxel_game
