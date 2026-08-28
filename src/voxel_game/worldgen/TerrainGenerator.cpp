#include "voxel_game/worldgen/TerrainGenerator.h"

#include <limits>
#include <stdexcept>

namespace tt::voxel_game
{
namespace
{

constexpr std::uint32_t FixedOne = 65536U;
constexpr std::uint32_t NoiseCentre = 32768U;

std::int64_t floor_divide(std::int64_t value, std::int64_t divisor) noexcept
{
    std::int64_t quotient = value / divisor;
    if (value % divisor < 0)
    {
        --quotient;
    }
    return quotient;
}

std::uint32_t positive_remainder(std::int64_t value, std::int64_t divisor) noexcept
{
    std::int64_t remainder = value % divisor;
    if (remainder < 0)
    {
        remainder += divisor;
    }
    return static_cast<std::uint32_t>(remainder);
}

std::uint32_t interpolate(std::uint32_t first, std::uint32_t second,
                          std::uint32_t fraction) noexcept
{
    const std::uint64_t weighted =
        static_cast<std::uint64_t>(first) * (FixedOne - fraction) +
        static_cast<std::uint64_t>(second) * fraction + NoiseCentre;
    return static_cast<std::uint32_t>(weighted >> 16U);
}

void validate_config(const TerrainConfig& config)
{
    if (config.height_amplitude > static_cast<std::uint32_t>((std::numeric_limits<int>::max)()))
    {
        throw std::invalid_argument("TerrainConfig.height_amplitude exceeds int height range");
    }
    if (config.lattice_spacing == 0 ||
        config.lattice_spacing > static_cast<std::uint32_t>((std::numeric_limits<int>::max)()))
    {
        throw std::invalid_argument("TerrainConfig.lattice_spacing must be in [1, INT_MAX]");
    }
    if (config.soil_depth == 0 ||
        config.soil_depth > static_cast<std::uint32_t>((std::numeric_limits<int>::max)()))
    {
        throw std::invalid_argument("TerrainConfig.soil_depth must be in [1, INT_MAX]");
    }

    const std::int64_t amplitude = static_cast<std::int64_t>(config.height_amplitude);
    const std::int64_t lowest = static_cast<std::int64_t>(config.base_surface_height) - amplitude;
    const std::int64_t highest = static_cast<std::int64_t>(config.base_surface_height) + amplitude;
    if (lowest < (std::numeric_limits<int>::min)() ||
        highest > (std::numeric_limits<int>::max)())
    {
        throw std::invalid_argument("TerrainConfig base_surface_height plus amplitude leaves int height range");
    }
}

} // namespace

TerrainGenerator::TerrainGenerator(TerrainConfig config)
    : m_config(config)
{
    validate_config(m_config);
}

const TerrainConfig& TerrainGenerator::config() const noexcept
{
    return m_config;
}

std::uint64_t TerrainGenerator::mix64(std::uint64_t value) noexcept
{
    value += UINT64_C(0x9e3779b97f4a7c15);
    value = (value ^ (value >> 30U)) * UINT64_C(0xbf58476d1ce4e5b9);
    value = (value ^ (value >> 27U)) * UINT64_C(0x94d049bb133111eb);
    return value ^ (value >> 31U);
}

std::uint32_t TerrainGenerator::lattice_value(std::int64_t lattice_x,
                                              std::int64_t lattice_z) const noexcept
{
    const std::uint64_t x = static_cast<std::uint64_t>(lattice_x);
    const std::uint64_t z = static_cast<std::uint64_t>(lattice_z);
    const std::uint64_t mixed = mix64(m_config.seed ^ mix64(x) ^
                                      (mix64(z) + UINT64_C(0xd1b54a32d192ed03)));
    return static_cast<std::uint32_t>(mixed >> 48U);
}

std::uint32_t TerrainGenerator::smoothed_fraction(std::uint32_t fraction) const noexcept
{
    const std::uint64_t t = fraction;
    const std::uint64_t smooth = t * t * (static_cast<std::uint64_t>(3U * FixedOne) - 2U * t);
    return static_cast<std::uint32_t>((smooth + UINT64_C(0x80000000)) >> 32U);
}

std::uint32_t TerrainGenerator::interpolated_noise(int world_x, int world_z) const noexcept
{
    const std::int64_t spacing = static_cast<std::int64_t>(m_config.lattice_spacing);
    const std::int64_t x = static_cast<std::int64_t>(world_x);
    const std::int64_t z = static_cast<std::int64_t>(world_z);
    const std::int64_t lattice_x = floor_divide(x, spacing);
    const std::int64_t lattice_z = floor_divide(z, spacing);
    const std::uint32_t fraction_x = static_cast<std::uint32_t>(
        (static_cast<std::uint64_t>(positive_remainder(x, spacing)) * FixedOne) /
        static_cast<std::uint64_t>(spacing));
    const std::uint32_t fraction_z = static_cast<std::uint32_t>(
        (static_cast<std::uint64_t>(positive_remainder(z, spacing)) * FixedOne) /
        static_cast<std::uint64_t>(spacing));
    const std::uint32_t smooth_x = smoothed_fraction(fraction_x);
    const std::uint32_t smooth_z = smoothed_fraction(fraction_z);
    const std::uint32_t x0 = interpolate(lattice_value(lattice_x, lattice_z),
                                         lattice_value(lattice_x + 1, lattice_z), smooth_x);
    const std::uint32_t x1 = interpolate(lattice_value(lattice_x, lattice_z + 1),
                                         lattice_value(lattice_x + 1, lattice_z + 1), smooth_x);
    return interpolate(x0, x1, smooth_z);
}

int TerrainGenerator::height_at(int world_x, int world_z) const noexcept
{
    const std::int64_t centered_noise =
        static_cast<std::int64_t>(interpolated_noise(world_x, world_z)) - NoiseCentre;
    const std::int64_t scaled =
        (centered_noise * static_cast<std::int64_t>(m_config.height_amplitude)) / NoiseCentre;
    return static_cast<int>(static_cast<std::int64_t>(m_config.base_surface_height) + scaled);
}

BlockState TerrainGenerator::block_at(BlockCoord coordinate) const noexcept
{
    const std::int64_t surface = height_at(coordinate.x, coordinate.z);
    const std::int64_t y = static_cast<std::int64_t>(coordinate.y);
    if (y > surface)
    {
        return BlockState::Air;
    }
    if (y == surface)
    {
        return BlockState::Grass;
    }
    if (y >= surface - static_cast<std::int64_t>(m_config.soil_depth))
    {
        return BlockState::Dirt;
    }
    return BlockState::Stone;
}

Section TerrainGenerator::generate_section(SectionCoord coordinate) const
{
    const std::int64_t base_x = static_cast<std::int64_t>(coordinate.x) * Section::Size;
    const std::int64_t base_y = static_cast<std::int64_t>(coordinate.y) * Section::Size;
    const std::int64_t base_z = static_cast<std::int64_t>(coordinate.z) * Section::Size;
    const std::int64_t last_offset = Section::Size - 1;
    if (base_x < (std::numeric_limits<int>::min)() || base_x + last_offset > (std::numeric_limits<int>::max)() ||
        base_y < (std::numeric_limits<int>::min)() || base_y + last_offset > (std::numeric_limits<int>::max)() ||
        base_z < (std::numeric_limits<int>::min)() || base_z + last_offset > (std::numeric_limits<int>::max)())
    {
        throw std::out_of_range("TerrainGenerator::generate_section rejects a SectionCoord outside the BlockCoord domain");
    }

    Section section;
    for (int y = 0; y < Section::Size; ++y)
    {
        for (int z = 0; z < Section::Size; ++z)
        {
            for (int x = 0; x < Section::Size; ++x)
            {
                const BlockCoord world_coordinate{
                    static_cast<int>(base_x + x), static_cast<int>(base_y + y),
                    static_cast<int>(base_z + z)};
                section.set({x, y, z}, block_at(world_coordinate));
            }
        }
    }
    return section;
}

} // namespace tt::voxel_game
