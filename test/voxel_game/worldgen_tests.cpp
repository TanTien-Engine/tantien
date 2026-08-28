#include "voxel_game/worldgen/TerrainGenerator.h"

#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace
{

using tt::voxel_game::BlockCoord;
using tt::voxel_game::BlockState;
using tt::voxel_game::Section;
using tt::voxel_game::SectionCoord;
using tt::voxel_game::TerrainConfig;
using tt::voxel_game::TerrainGenerator;

void require(bool condition, const char* message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

template <typename Exception, typename Callable>
void require_throws(Callable callable, const char* message)
{
    try
    {
        callable();
    }
    catch (const Exception&)
    {
        return;
    }
    throw std::runtime_error(message);
}

std::uint64_t stable_section_hash(const Section& section)
{
    std::uint64_t hash = UINT64_C(1469598103934665603);
    for (int y = 0; y < Section::Size; ++y)
    {
        for (int z = 0; z < Section::Size; ++z)
        {
            for (int x = 0; x < Section::Size; ++x)
            {
                hash ^= static_cast<std::uint16_t>(section.get({x, y, z}));
                hash *= UINT64_C(1099511628211);
            }
        }
    }
    return hash;
}

TerrainConfig golden_config()
{
    TerrainConfig config;
    config.seed = UINT64_C(0x4d32465f7631a9c3);
    config.base_surface_height = 9;
    config.height_amplitude = 5;
    config.lattice_spacing = 23;
    config.soil_depth = 3;
    return config;
}

void test_determinism_and_order_independence()
{
    const TerrainGenerator first(golden_config());
    const TerrainGenerator second(golden_config());
    constexpr BlockCoord coordinates[] = {
        {0, 0, 0}, {-1, 7, -1}, {31, -8, 47},
        {(std::numeric_limits<int>::min)(), 0, (std::numeric_limits<int>::max)()},
        {(std::numeric_limits<int>::max)(), -12, (std::numeric_limits<int>::min)()},
    };
    BlockState expected[sizeof(coordinates) / sizeof(coordinates[0])]{};
    for (std::size_t index = 0; index < sizeof(coordinates) / sizeof(coordinates[0]); ++index)
    {
        expected[index] = first.block_at(coordinates[index]);
    }
    for (std::size_t reverse = sizeof(coordinates) / sizeof(coordinates[0]); reverse > 0; --reverse)
    {
        const std::size_t index = reverse - 1;
        require(second.block_at(coordinates[index]) == expected[index],
                "same config must be independent of call order");
        require(first.height_at(coordinates[index].x, coordinates[index].z) ==
                    second.height_at(coordinates[index].x, coordinates[index].z),
                "same config height must be deterministic");
    }
    require(stable_section_hash(first.generate_section({-1, 0, 2})) ==
                stable_section_hash(second.generate_section({-1, 0, 2})),
            "same config section must be deterministic");
}

void test_distinct_seeds_change_representative_area()
{
    TerrainConfig alternate = golden_config();
    alternate.seed ^= UINT64_C(0xa4093822299f31d0);
    const TerrainGenerator first(golden_config());
    const TerrainGenerator second(alternate);
    bool found_difference = false;
    for (int z = -96; z <= 96 && !found_difference; z += 7)
    {
        for (int x = -96; x <= 96; x += 5)
        {
            if (first.height_at(x, z) != second.height_at(x, z))
            {
                found_difference = true;
                break;
            }
        }
    }
    require(found_difference, "different seeds must differ in a representative area");
}

void test_negative_boundaries_and_section_equivalence()
{
    const TerrainGenerator generator(golden_config());
    for (const SectionCoord coordinate : {SectionCoord{-1, -1, -1}, SectionCoord{0, 0, 0},
                                          SectionCoord{1, 0, -1}})
    {
        const Section generated = generator.generate_section(coordinate);
        for (int y = 0; y < Section::Size; ++y)
        {
            for (int z = 0; z < Section::Size; ++z)
            {
                for (int x = 0; x < Section::Size; ++x)
                {
                    const BlockCoord world{coordinate.x * Section::Size + x,
                                           coordinate.y * Section::Size + y,
                                           coordinate.z * Section::Size + z};
                    const BlockState state = generated.get({x, y, z});
                    require(state == generator.block_at(world),
                            "generated Section must equal block_at at every local coordinate");
                    require(state != BlockState::Clay,
                            "terrain generation must not emit Clay");
                }
            }
        }
    }
    require(generator.height_at(-1, -1) == generator.height_at(-1, -1),
            "negative coordinate sample must be repeatable");
}

void test_layering()
{
    TerrainConfig config = golden_config();
    config.height_amplitude = 0;
    config.base_surface_height = 20;
    config.soil_depth = 3;
    const TerrainGenerator generator(config);
    require(generator.block_at({7, 21, -9}) == BlockState::Air, "above surface is Air");
    require(generator.block_at({7, 20, -9}) == BlockState::Grass, "surface is Grass");
    require(generator.block_at({7, 19, -9}) == BlockState::Dirt, "first subsurface layer is Dirt");
    require(generator.block_at({7, 17, -9}) == BlockState::Dirt, "soil_depth layers are Dirt");
    require(generator.block_at({7, 16, -9}) == BlockState::Stone, "deeper terrain is Stone");
}

void test_invalid_config_and_extrema()
{
    TerrainConfig invalid = golden_config();
    invalid.lattice_spacing = 0;
    require_throws<std::invalid_argument>([invalid] { TerrainGenerator rejected(invalid); },
                                          "zero lattice spacing must reject");
    invalid = golden_config();
    invalid.soil_depth = 0;
    require_throws<std::invalid_argument>([invalid] { TerrainGenerator rejected(invalid); },
                                          "zero soil depth must reject");
    invalid = golden_config();
    invalid.base_surface_height = (std::numeric_limits<int>::max)();
    invalid.height_amplitude = 1;
    require_throws<std::invalid_argument>([invalid] { TerrainGenerator rejected(invalid); },
                                          "overflowing height range must reject");
    invalid = golden_config();
    invalid.height_amplitude = static_cast<std::uint32_t>((std::numeric_limits<int>::max)()) + 1U;
    require_throws<std::invalid_argument>([invalid] { TerrainGenerator rejected(invalid); },
                                          "oversize amplitude must reject");

    const TerrainGenerator generator(golden_config());
    const int min_value = (std::numeric_limits<int>::min)();
    const int max_value = (std::numeric_limits<int>::max)();
    require(generator.height_at(min_value, max_value) == generator.height_at(min_value, max_value),
            "INT_MIN/INT_MAX height sample must be repeatable");
    require(generator.block_at({min_value, min_value, max_value}) ==
                generator.block_at({min_value, min_value, max_value}),
            "INT_MIN/INT_MAX block sample must be repeatable");
    require_throws<std::out_of_range>([&generator]
    {
        generator.generate_section({(std::numeric_limits<int>::max)(), 0, 0});
    }, "unrepresentable extreme SectionCoord must reject");
}

void test_golden()
{
    const TerrainGenerator generator(golden_config());
    require(generator.height_at(-37, 19) == 12, "golden height -37,19");
    require(generator.height_at(0, 0) == 9, "golden height 0,0");
    require(generator.height_at(48, -64) == 11, "golden height 48,-64");
    require(stable_section_hash(generator.generate_section({-1, 0, 1})) == UINT64_C(10335942788010201761),
            "golden section hash");
}

} // namespace

int main()
{
    try
    {
        test_determinism_and_order_independence();
        test_distinct_seeds_change_representative_area();
        test_negative_boundaries_and_section_equivalence();
        test_layering();
        test_invalid_config_and_extrema();
        test_golden();
        std::cout << "worldgen tests passed" << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "worldgen test failure: " << exception.what() << std::endl;
        return 1;
    }
}
