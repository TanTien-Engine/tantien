#include "voxel_game/voxel/World.h"

#include <algorithm>
#include <functional>

namespace tt::voxel_game
{
namespace
{

int floor_divide_by_section_size(int value)
{
    int remainder = value % Section::Size;
    if (remainder < 0)
    {
        remainder += Section::Size;
    }
    return (value - remainder) / Section::Size;
}

int local_modulo_section_size(int value)
{
    int remainder = value % Section::Size;
    if (remainder < 0)
    {
        remainder += Section::Size;
    }
    return remainder;
}

} // namespace

std::size_t SectionCoordHash::operator()(const SectionCoord& coordinate) const
{
    std::size_t seed = std::hash<int>{}(coordinate.x);
    seed ^= std::hash<int>{}(coordinate.y) + 0x9e3779b9U + (seed << 6U) + (seed >> 2U);
    seed ^= std::hash<int>{}(coordinate.z) + 0x9e3779b9U + (seed << 6U) + (seed >> 2U);
    return seed;
}

WorldBlockLocation locate_world_block(BlockCoord world_coordinate)
{
    return {{floor_divide_by_section_size(world_coordinate.x),
             floor_divide_by_section_size(world_coordinate.y),
             floor_divide_by_section_size(world_coordinate.z)},
            {local_modulo_section_size(world_coordinate.x),
             local_modulo_section_size(world_coordinate.y),
             local_modulo_section_size(world_coordinate.z)}};
}

BlockState World::get(BlockCoord world_coordinate) const
{
    const WorldBlockLocation location = locate_world_block(world_coordinate);
    const auto found = m_sections.find(location.section);
    if (found == m_sections.end())
    {
        return BlockState::Air;
    }
    return found->second.blocks.get(location.local);
}

bool World::set(BlockCoord world_coordinate, BlockState state)
{
    const WorldBlockLocation location = locate_world_block(world_coordinate);
    LoadedSection& section = m_sections[location.section];
    const BlockState previous = section.blocks.get(location.local);
    if (previous == state)
    {
        return false;
    }

    section.blocks.set(location.local, state);
    section.dirty = true;
    mark_boundary_neighbors_dirty(location);
    return true;
}

const Section* World::find_section(SectionCoord coordinate) const
{
    const auto found = m_sections.find(coordinate);
    return found == m_sections.end() ? nullptr : &found->second.blocks;
}

const OpaqueMesh* World::find_mesh(SectionCoord coordinate) const
{
    const auto found = m_sections.find(coordinate);
    return found == m_sections.end() ? nullptr : &found->second.mesh;
}

std::uint64_t World::mesh_revision(SectionCoord coordinate) const
{
    const auto found = m_sections.find(coordinate);
    return found == m_sections.end() ? 0 : found->second.mesh_revision;
}

bool World::section_dirty(SectionCoord coordinate) const
{
    const auto found = m_sections.find(coordinate);
    return found != m_sections.end() && found->second.dirty;
}

std::size_t World::section_count() const
{
    return m_sections.size();
}

std::vector<SectionCoord> World::rebuild_dirty_meshes()
{
    std::vector<SectionCoord> rebuilt;
    rebuilt.reserve(m_sections.size());
    for (const auto& pair : m_sections)
    {
        if (pair.second.dirty)
        {
            rebuilt.push_back(pair.first);
        }
    }
    std::sort(rebuilt.begin(), rebuilt.end());

    for (const SectionCoord coordinate : rebuilt)
    {
        LoadedSection& section = m_sections.at(coordinate);
        const BlockCoord origin{
            coordinate.x * Section::Size,
            coordinate.y * Section::Size,
            coordinate.z * Section::Size,
        };
        section.mesh = mesh_opaque_visible_faces(section.blocks,
            [this, origin](BlockCoord local_neighbor)
            {
                return get({origin.x + local_neighbor.x,
                            origin.y + local_neighbor.y,
                            origin.z + local_neighbor.z});
            });
        ++section.mesh_revision;
        section.dirty = false;
    }
    return rebuilt;
}

void World::mark_dirty_if_loaded(SectionCoord coordinate)
{
    const auto found = m_sections.find(coordinate);
    if (found != m_sections.end())
    {
        found->second.dirty = true;
    }
}

void World::mark_boundary_neighbors_dirty(WorldBlockLocation location)
{
    if (location.local.x == 0)
    {
        mark_dirty_if_loaded({location.section.x - 1, location.section.y, location.section.z});
    }
    if (location.local.x == Section::Size - 1)
    {
        mark_dirty_if_loaded({location.section.x + 1, location.section.y, location.section.z});
    }
    if (location.local.y == 0)
    {
        mark_dirty_if_loaded({location.section.x, location.section.y - 1, location.section.z});
    }
    if (location.local.y == Section::Size - 1)
    {
        mark_dirty_if_loaded({location.section.x, location.section.y + 1, location.section.z});
    }
    if (location.local.z == 0)
    {
        mark_dirty_if_loaded({location.section.x, location.section.y, location.section.z - 1});
    }
    if (location.local.z == Section::Size - 1)
    {
        mark_dirty_if_loaded({location.section.x, location.section.y, location.section.z + 1});
    }
}

} // namespace tt::voxel_game
