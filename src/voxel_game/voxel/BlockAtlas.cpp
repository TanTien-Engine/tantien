#include "voxel_game/voxel/BlockAtlas.h"

#include <algorithm>
#include <stdexcept>

namespace tt::voxel_game
{
namespace
{

std::size_t tile_index(AtlasTile tile)
{
    if (!is_valid_atlas_tile(tile))
    {
        throw std::invalid_argument("BlockAtlas rejects an unknown AtlasTile");
    }
    return static_cast<std::size_t>(tile);
}

std::array<std::uint8_t, 3> base_color(AtlasTile tile)
{
    switch (tile)
    {
    case AtlasTile::Stone: return {104, 112, 120};
    case AtlasTile::GrassTop: return {74, 142, 67};
    case AtlasTile::GrassSide: return {100, 150, 70};
    case AtlasTile::Dirt: return {124, 82, 48};
    case AtlasTile::Clay: return {159, 103, 86};
    case AtlasTile::Count: break;
    }
    return {255, 0, 255};
}

std::uint32_t noise(AtlasTile tile, int x, int y)
{
    std::uint32_t value = static_cast<std::uint32_t>(tile_index(tile) + 1U) * 0x9e3779b9U;
    value ^= static_cast<std::uint32_t>(x + 31) * 0x85ebca6bU;
    value ^= static_cast<std::uint32_t>(y + 17) * 0xc2b2ae35U;
    value ^= value >> 16;
    return value * 0x7feb352dU;
}

std::array<std::uint8_t, 4> content_pixel(AtlasTile tile, int x, int y)
{
    const auto base = base_color(tile);
    const int variation = static_cast<int>(noise(tile, x, y) % 19U) - 9;
    std::array<std::uint8_t, 4> result = {
        static_cast<std::uint8_t>(std::clamp<int>(base[0] + variation, 0, 255)),
        static_cast<std::uint8_t>(std::clamp<int>(base[1] + variation, 0, 255)),
        static_cast<std::uint8_t>(std::clamp<int>(base[2] + variation, 0, 255)), 255,
    };
    // Atlas row zero is sampled at v_min (the bottom of the rendered face).
    // Keep dirt at the bottom/body and a grass band at the v_max/top edge.
    if (tile == AtlasTile::GrassSide && y <= 10)
    {
        const auto dirt = base_color(AtlasTile::Dirt);
        result = {static_cast<std::uint8_t>(dirt[0] + variation / 2),
                  static_cast<std::uint8_t>(dirt[1] + variation / 2),
                  static_cast<std::uint8_t>(dirt[2] + variation / 2), 255};
    }
    if (tile == AtlasTile::GrassSide && y >= 8 && y <= 10 && ((x * 5 + y * 3) % 7) < 3)
    {
        result[0] = 72; result[1] = 132; result[2] = 57;
    }
    return result;
}

} // namespace

BlockAtlas::BlockAtlas() : m_pixels(static_cast<std::size_t>(Width) * Height * 4U, 0U)
{
    for (std::size_t index = 0; index < TileCount; ++index)
    {
        const AtlasTile tile = static_cast<AtlasTile>(index);
        const auto origin = content_origin(tile);
        for (int y = 0; y < ContentSize; ++y)
        {
            for (int x = 0; x < ContentSize; ++x)
            {
                const auto color = content_pixel(tile, x, y);
                const std::size_t offset = (static_cast<std::size_t>(origin[1] + y) * Width +
                                            static_cast<std::size_t>(origin[0] + x)) * 4U;
                std::copy(color.begin(), color.end(), m_pixels.begin() + static_cast<std::ptrdiff_t>(offset));
            }
        }
        const int left = origin[0] - GutterSize;
        const int top = origin[1] - GutterSize;
        for (int y = 0; y < TileExtent; ++y)
        {
            for (int x = 0; x < TileExtent; ++x)
            {
                const int source_x = origin[0] + std::clamp(x - GutterSize, 0, ContentSize - 1);
                const int source_y = origin[1] + std::clamp(y - GutterSize, 0, ContentSize - 1);
                const std::size_t source = (static_cast<std::size_t>(source_y) * Width + source_x) * 4U;
                const std::size_t target = (static_cast<std::size_t>(top + y) * Width + left + x) * 4U;
                std::copy_n(m_pixels.begin() + static_cast<std::ptrdiff_t>(source), 4,
                            m_pixels.begin() + static_cast<std::ptrdiff_t>(target));
            }
        }
    }
}

std::array<std::uint8_t, 4> BlockAtlas::pixel(int x, int y) const
{
    if (x < 0 || x >= Width || y < 0 || y >= Height)
    {
        throw std::out_of_range("atlas pixel coordinate is outside the fixed atlas");
    }
    const std::size_t offset = (static_cast<std::size_t>(y) * Width + x) * 4U;
    return {m_pixels[offset], m_pixels[offset + 1], m_pixels[offset + 2], m_pixels[offset + 3]};
}

AtlasUvBounds BlockAtlas::uv_bounds(AtlasTile tile)
{
    const auto origin = content_origin(tile);
    return {(static_cast<float>(origin[0]) + 0.5F) / static_cast<float>(Width),
            (static_cast<float>(origin[1]) + 0.5F) / static_cast<float>(Height),
            (static_cast<float>(origin[0] + ContentSize) - 0.5F) / static_cast<float>(Width),
            (static_cast<float>(origin[1] + ContentSize) - 0.5F) / static_cast<float>(Height)};
}

std::array<int, 2> BlockAtlas::content_origin(AtlasTile tile)
{
    const std::size_t index = tile_index(tile);
    return {static_cast<int>(index % Columns) * TileExtent + GutterSize,
            static_cast<int>(index / Columns) * TileExtent + GutterSize};
}

std::uint64_t BlockAtlas::hash() const noexcept
{
    std::uint64_t result = 1469598103934665603ULL;
    for (const std::uint8_t value : m_pixels)
    {
        result ^= value;
        result *= 1099511628211ULL;
    }
    return result;
}

bool BlockAtlas::gutters_are_extruded() const noexcept
{
    for (std::size_t index = 0; index < TileCount; ++index)
    {
        const auto origin = content_origin(static_cast<AtlasTile>(index));
        for (int y = -GutterSize; y < ContentSize + GutterSize; ++y)
        {
            for (int x = -GutterSize; x < ContentSize + GutterSize; ++x)
            {
                const int expected_x = origin[0] + std::clamp(x, 0, ContentSize - 1);
                const int expected_y = origin[1] + std::clamp(y, 0, ContentSize - 1);
                if (pixel(origin[0] + x, origin[1] + y) != pixel(expected_x, expected_y))
                {
                    return false;
                }
            }
        }
    }
    return true;
}

} // namespace tt::voxel_game
