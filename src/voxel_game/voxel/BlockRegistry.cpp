#include "voxel_game/voxel/BlockRegistry.h"

namespace tt::voxel_game
{
namespace
{

constexpr std::array<AtlasTile, 6> SameStone = {
    AtlasTile::Stone, AtlasTile::Stone, AtlasTile::Stone,
    AtlasTile::Stone, AtlasTile::Stone, AtlasTile::Stone,
};
constexpr std::array<AtlasTile, 6> SameDirt = {
    AtlasTile::Dirt, AtlasTile::Dirt, AtlasTile::Dirt,
    AtlasTile::Dirt, AtlasTile::Dirt, AtlasTile::Dirt,
};
constexpr std::array<AtlasTile, 6> SameClay = {
    AtlasTile::Clay, AtlasTile::Clay, AtlasTile::Clay,
    AtlasTile::Clay, AtlasTile::Clay, AtlasTile::Clay,
};
constexpr std::array<AtlasTile, 6> GrassTiles = {
    AtlasTile::GrassSide, AtlasTile::GrassSide, AtlasTile::Dirt,
    AtlasTile::GrassTop, AtlasTile::GrassSide, AtlasTile::GrassSide,
};

constexpr BlockDefinition Air{"Air", false, false, RenderBucket::Opaque, SameStone};
constexpr BlockDefinition Grass{"Grass", true, true, RenderBucket::Opaque, GrassTiles};
constexpr BlockDefinition Stone{"Stone", true, true, RenderBucket::Opaque, SameStone};
constexpr BlockDefinition Clay{"Clay", true, true, RenderBucket::Opaque, SameClay};
constexpr BlockDefinition Dirt{"Dirt", true, true, RenderBucket::Opaque, SameDirt};

} // namespace

const BlockDefinition* find_block_definition(BlockState state) noexcept
{
    switch (state)
    {
    case BlockState::Air: return &Air;
    case BlockState::Grass: return &Grass;
    case BlockState::Stone: return &Stone;
    case BlockState::Clay: return &Clay;
    case BlockState::Dirt: return &Dirt;
    }
    return nullptr;
}

const BlockDefinition& block_definition_or_air(BlockState state) noexcept
{
    const BlockDefinition* definition = find_block_definition(state);
    return definition == nullptr ? Air : *definition;
}

bool is_valid_block_state(BlockState state) noexcept
{
    return find_block_definition(state) != nullptr;
}

bool is_valid_atlas_tile(AtlasTile tile) noexcept
{
    return static_cast<std::size_t>(tile) < static_cast<std::size_t>(AtlasTile::Count);
}

bool is_opaque(BlockState state) noexcept
{
    return block_definition_or_air(state).occludes;
}

bool is_collidable(BlockState state) noexcept
{
    return block_definition_or_air(state).collidable;
}

RenderBucket render_bucket(BlockState state) noexcept
{
    return block_definition_or_air(state).bucket;
}

} // namespace tt::voxel_game
