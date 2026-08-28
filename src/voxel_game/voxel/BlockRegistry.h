#pragma once

#include "voxel_game/voxel/BlockState.h"

#include <array>
#include <cstddef>
#include <stdexcept>

namespace tt::voxel_game
{

// Face order is deliberately shared by the registry and mesher. It remains
// independent of storage layout so render buckets can grow without changing
// Section data.
enum class BlockFace : std::size_t
{
    NegativeX = 0,
    PositiveX = 1,
    NegativeY = 2,
    PositiveY = 3,
    NegativeZ = 4,
    PositiveZ = 5,
};

enum class AtlasTile : std::size_t
{
    Stone = 0,
    GrassTop = 1,
    GrassSide = 2,
    Dirt = 3,
    Clay = 4,
    Count = 5,
};

struct BlockDefinition
{
    const char* name = "Air";
    bool occludes = false;
    bool collidable = false;
    RenderBucket bucket = RenderBucket::Opaque;
    std::array<AtlasTile, 6> face_tiles{};

    AtlasTile tile_for(BlockFace face) const
    {
        const std::size_t index = static_cast<std::size_t>(face);
        if (index >= face_tiles.size())
        {
            throw std::out_of_range("BlockDefinition::tile_for rejects an unknown BlockFace");
        }
        return face_tiles[index];
    }
};

// Returns nullptr for an unknown serialized value. Mutation APIs reject such
// values; read-only consumers use block_definition_or_air as a safe fallback.
const BlockDefinition* find_block_definition(BlockState state) noexcept;
const BlockDefinition& block_definition_or_air(BlockState state) noexcept;
bool is_valid_block_state(BlockState state) noexcept;
bool is_valid_atlas_tile(AtlasTile tile) noexcept;
bool is_opaque(BlockState state) noexcept;
bool is_collidable(BlockState state) noexcept;
RenderBucket render_bucket(BlockState state) noexcept;

} // namespace tt::voxel_game
