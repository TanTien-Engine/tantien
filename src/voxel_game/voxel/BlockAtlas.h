#pragma once

#include "voxel_game/voxel/BlockRegistry.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace tt::voxel_game
{

struct AtlasUvBounds
{
    float minimum_u = 0.0F;
    float minimum_v = 0.0F;
    float maximum_u = 0.0F;
    float maximum_v = 0.0F;
};

// A fixed, original RGBA8 atlas. Each 16px content tile is surrounded by a
// one-pixel extruded gutter. UVs use content pixel centres, and mipmaps stay
// disabled: a 1px gutter does not make every lower mip level tile-safe.
class BlockAtlas
{
public:
    static constexpr int ContentSize = 16;
    static constexpr int GutterSize = 1;
    static constexpr int TileExtent = ContentSize + GutterSize * 2;
    static constexpr std::size_t TileCount = static_cast<std::size_t>(AtlasTile::Count);
    static constexpr int Columns = static_cast<int>(TileCount);
    static constexpr int Rows = 1;
    static constexpr int Width = Columns * TileExtent;
    static constexpr int Height = Rows * TileExtent;

    BlockAtlas();

    const std::vector<std::uint8_t>& pixels() const noexcept { return m_pixels; }
    std::array<std::uint8_t, 4> pixel(int x, int y) const;
    // These metadata operations do not require an atlas pixel allocation.
    // Unknown tile values are strictly rejected.
    static AtlasUvBounds uv_bounds(AtlasTile tile);
    static std::array<int, 2> content_origin(AtlasTile tile);
    std::uint64_t hash() const noexcept;
    bool gutters_are_extruded() const noexcept;

private:
    std::vector<std::uint8_t> m_pixels;
};

} // namespace tt::voxel_game
