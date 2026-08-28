#include "voxel_game/voxel/VisibleFaceMesher.h"

#include "voxel_game/voxel/BlockAtlas.h"
#include "voxel_game/voxel/BlockRegistry.h"

#include <array>

namespace tt::voxel_game
{
namespace
{

struct FaceDefinition
{
    BlockFace face;
    BlockCoord neighbor_offset;
    std::array<float, 3> normal;
    std::array<std::array<float, 3>, 4> corners;
};

constexpr std::array<FaceDefinition, 6> Faces = {{
    {BlockFace::NegativeX, {-1, 0, 0}, {-1.0F, 0.0F, 0.0F}, {{{0.0F, 0.0F, 1.0F}, {0.0F, 1.0F, 1.0F}, {0.0F, 1.0F, 0.0F}, {0.0F, 0.0F, 0.0F}}}},
    {BlockFace::PositiveX, {1, 0, 0}, {1.0F, 0.0F, 0.0F}, {{{1.0F, 0.0F, 0.0F}, {1.0F, 1.0F, 0.0F}, {1.0F, 1.0F, 1.0F}, {1.0F, 0.0F, 1.0F}}}},
    {BlockFace::NegativeY, {0, -1, 0}, {0.0F, -1.0F, 0.0F}, {{{0.0F, 0.0F, 0.0F}, {1.0F, 0.0F, 0.0F}, {1.0F, 0.0F, 1.0F}, {0.0F, 0.0F, 1.0F}}}},
    {BlockFace::PositiveY, {0, 1, 0}, {0.0F, 1.0F, 0.0F}, {{{0.0F, 1.0F, 1.0F}, {1.0F, 1.0F, 1.0F}, {1.0F, 1.0F, 0.0F}, {0.0F, 1.0F, 0.0F}}}},
    {BlockFace::NegativeZ, {0, 0, -1}, {0.0F, 0.0F, -1.0F}, {{{1.0F, 0.0F, 0.0F}, {0.0F, 0.0F, 0.0F}, {0.0F, 1.0F, 0.0F}, {1.0F, 1.0F, 0.0F}}}},
    {BlockFace::PositiveZ, {0, 0, 1}, {0.0F, 0.0F, 1.0F}, {{{0.0F, 0.0F, 1.0F}, {1.0F, 0.0F, 1.0F}, {1.0F, 1.0F, 1.0F}, {0.0F, 1.0F, 1.0F}}}},
}};

void append_face(OpaqueMesh& mesh,
                 BlockCoord coordinate,
                 const BlockDefinition& definition,
                 const FaceDefinition& face)
{
    const std::uint32_t base_index = static_cast<std::uint32_t>(mesh.vertices.size());
    const AtlasUvBounds bounds = BlockAtlas::uv_bounds(definition.tile_for(face.face));
    constexpr std::array<std::array<float, 2>, 4> Uvs = {{{{0.0F, 0.0F}}, {{1.0F, 0.0F}},
                                                            {{1.0F, 1.0F}}, {{0.0F, 1.0F}}}};
    for (std::size_t corner_index = 0; corner_index < face.corners.size(); ++corner_index)
    {
        const auto& corner = face.corners[corner_index];
        const auto& normalized_uv = Uvs[corner_index];
        mesh.vertices.push_back(VoxelVertex{
            {static_cast<float>(coordinate.x) + corner[0],
             static_cast<float>(coordinate.y) + corner[1],
             static_cast<float>(coordinate.z) + corner[2]},
            face.normal,
            {1.0F, 1.0F, 1.0F, 1.0F},
            {bounds.minimum_u + (bounds.maximum_u - bounds.minimum_u) * normalized_uv[0],
             bounds.minimum_v + (bounds.maximum_v - bounds.minimum_v) * normalized_uv[1]},
        });
    }

    mesh.indices.insert(mesh.indices.end(), {
        base_index, base_index + 1, base_index + 2,
        base_index, base_index + 2, base_index + 3,
    });
    ++mesh.face_count;
}

} // namespace

OpaqueMesh mesh_opaque_visible_faces(const Section& section,
                                     const BlockLookup& block_lookup)
{
    OpaqueMesh mesh;
    for (int y = 0; y < Section::Size; ++y)
    {
        for (int z = 0; z < Section::Size; ++z)
        {
            for (int x = 0; x < Section::Size; ++x)
            {
                const BlockCoord coordinate{x, y, z};
                const BlockState state = section.get(coordinate);
                const BlockDefinition* definition = find_block_definition(state);
                if (definition == nullptr || !definition->occludes ||
                    definition->bucket != RenderBucket::Opaque)
                {
                    continue;
                }

                for (const FaceDefinition& face : Faces)
                {
                    const BlockCoord neighbor{
                        x + face.neighbor_offset.x,
                        y + face.neighbor_offset.y,
                        z + face.neighbor_offset.z,
                    };
                    const BlockState neighbor_state = Section::contains(neighbor)
                        ? section.get(neighbor)
                        : (block_lookup ? block_lookup(neighbor) : BlockState::Air);
                    if (!block_definition_or_air(neighbor_state).occludes)
                    {
                        append_face(mesh, coordinate, *definition, face);
                    }
                }
            }
        }
    }

    return mesh;
}

} // namespace tt::voxel_game
