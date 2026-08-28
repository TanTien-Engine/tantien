#pragma once

#include "voxel_game/voxel/Section.h"
#include "voxel_game/voxel/World.h"

#include <cstdint>

namespace tt::voxel_game
{

// Version 1 is one octave of 2-D lattice value noise.  Lattice values are
// derived by a fixed uint64_t mixer and interpolated entirely in Q16 fixed
// point, so the result does not depend on a platform RNG or floating-point
// rounding mode.
struct TerrainConfig
{
    std::uint64_t seed = 0;
    int base_surface_height = 8;
    std::uint32_t height_amplitude = 4;
    std::uint32_t lattice_spacing = 32;
    std::uint32_t soil_depth = 3;
};

class TerrainGenerator
{
public:
    static constexpr std::uint32_t AlgorithmVersion = 1;

    explicit TerrainGenerator(TerrainConfig config);

    const TerrainConfig& config() const noexcept;
    int height_at(int world_x, int world_z) const noexcept;
    BlockState block_at(BlockCoord coordinate) const noexcept;

    // Throws std::out_of_range if a SectionCoord would address blocks outside
    // BlockCoord's int domain.  This is a strict rejection rather than a
    // narrowing conversion; accepted Sections equal per-cell block_at calls.
    Section generate_section(SectionCoord coordinate) const;

private:
    static std::uint64_t mix64(std::uint64_t value) noexcept;
    std::uint32_t lattice_value(std::int64_t lattice_x,
                                std::int64_t lattice_z) const noexcept;
    std::uint32_t smoothed_fraction(std::uint32_t fraction) const noexcept;
    std::uint32_t interpolated_noise(int world_x, int world_z) const noexcept;

    TerrainConfig m_config;
};

} // namespace tt::voxel_game
