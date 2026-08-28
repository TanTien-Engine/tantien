#pragma once

#include "voxel_game/voxel/Section.h"
#include "voxel_game/voxel/VisibleFaceMesher.h"

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace tt::voxel_game
{

struct SectionCoord
{
    int x = 0;
    int y = 0;
    int z = 0;

    bool operator==(const SectionCoord& other) const
    {
        return x == other.x && y == other.y && z == other.z;
    }

    bool operator<(const SectionCoord& other) const
    {
        if (x != other.x)
        {
            return x < other.x;
        }
        if (y != other.y)
        {
            return y < other.y;
        }
        return z < other.z;
    }
};

struct SectionCoordHash
{
    std::size_t operator()(const SectionCoord& coordinate) const;
};

struct WorldBlockLocation
{
    SectionCoord section;
    BlockCoord local;
};

// Converts every integer world coordinate using mathematical floor division.
// Therefore world x=-1 belongs to section x=-1 at local x=15, rather than
// incorrectly truncating into section zero.
WorldBlockLocation locate_world_block(BlockCoord world_coordinate);

// Owns loaded 16^3 Sections and their deterministic CPU mesh revisions.
// Missing Sections read as Air. A non-Air set() lazily loads the addressed
// Section, while setting Air in a missing Section is a no-op. Empty loaded
// Sections remain loaded after deletion. A changed boundary block marks any
// loaded face-neighbor Section dirty too.
class World
{
public:
    BlockState get(BlockCoord world_coordinate) const;
    bool set(BlockCoord world_coordinate, BlockState state);

    const Section* find_section(SectionCoord coordinate) const;
    const OpaqueMesh* find_mesh(SectionCoord coordinate) const;
    std::uint64_t mesh_revision(SectionCoord coordinate) const;
    bool section_dirty(SectionCoord coordinate) const;
    std::size_t section_count() const;

    // Returns a sorted snapshot of the loaded coordinates.  This is a
    // read-only enumeration boundary: it neither exposes LoadedSection nor
    // permits mutation of the world's storage.
    std::vector<SectionCoord> section_coordinates() const;

    // Rebuilds only dirty Sections. Returned coordinates are sorted so callers
    // can make GPU upload and smoke diagnostics deterministic.
    std::vector<SectionCoord> rebuild_dirty_meshes();

private:
    struct LoadedSection
    {
        Section blocks;
        OpaqueMesh mesh;
        std::uint64_t mesh_revision = 0;
        bool dirty = true;
    };

    void mark_dirty_if_loaded(SectionCoord coordinate);
    void mark_boundary_neighbors_dirty(WorldBlockLocation location);

    std::unordered_map<SectionCoord, LoadedSection, SectionCoordHash> m_sections;
};

} // namespace tt::voxel_game
