#pragma once

#include "voxel_game/voxel/BlockState.h"

#include <array>
#include <cstddef>

namespace tt::voxel_game
{

struct BlockCoord
{
    int x = 0;
    int y = 0;
    int z = 0;
};

// A Section is 16 blocks on each axis. Its linear storage is x-fastest:
// index = x + 16 * (z + 16 * y). This is deliberately local-coordinate only;
// the mesher can ask a caller for coordinates beyond the local Section.
class Section
{
public:
    static constexpr int Size = 16;
    static constexpr std::size_t BlockCount = Size * Size * Size;

    static bool contains(BlockCoord coordinate);
    static std::size_t index_of(BlockCoord coordinate);

    BlockState get(BlockCoord coordinate) const;
    BlockState get_or_air(BlockCoord coordinate) const;
    void set(BlockCoord coordinate, BlockState state);
    void fill(BlockState state);

private:
    std::array<BlockState, BlockCount> m_blocks{};
};

} // namespace tt::voxel_game
