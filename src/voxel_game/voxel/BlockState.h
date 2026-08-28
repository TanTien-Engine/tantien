#pragma once

#include <cstdint>

namespace tt::voxel_game
{

// The storage value remains a fixed-width block-state handle. Later palette
// packing may change Section storage without changing mesher-facing APIs.
enum class BlockState : std::uint16_t
{
    Air = 0,
    Grass = 1,
    Stone = 2,
    Clay = 3,
    Dirt = 4,
};

enum class RenderBucket : std::uint8_t
{
    Opaque,
    Cutout,
    Translucent,
};

} // namespace tt::voxel_game
