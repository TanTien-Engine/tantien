#include "voxel_game/voxel/VisibleFaceMesher.h"

#include <array>

namespace tt::voxel_game
{
namespace
{

struct FaceDefinition
{
    BlockCoord neighbor_offset;
    std::array<float, 3> normal;
    std::array<std::array<float, 3>, 4> corners;
};

constexpr std::array<FaceDefinition, 6> Faces = {{
    {{-1, 0, 0}, {-1.0F, 0.0F, 0.0F}, {{{0.0F, 0.0F, 1.0F}, {0.0F, 1.0F, 1.0F}, {0.0F, 1.0F, 0.0F}, {0.0F, 0.0F, 0.0F}}}},
    {{1, 0, 0}, {1.0F, 0.0F, 0.0F}, {{{1.0F, 0.0F, 0.0F}, {1.0F, 1.0F, 0.0F}, {1.0F, 1.0F, 1.0F}, {1.0F, 0.0F, 1.0F}}}},
    {{0, -1, 0}, {0.0F, -1.0F, 0.0F}, {{{0.0F, 0.0F, 0.0F}, {1.0F, 0.0F, 0.0F}, {1.0F, 0.0F, 1.0F}, {0.0F, 0.0F, 1.0F}}}},
    {{0, 1, 0}, {0.0F, 1.0F, 0.0F}, {{{0.0F, 1.0F, 1.0F}, {1.0F, 1.0F, 1.0F}, {1.0F, 1.0F, 0.0F}, {0.0F, 1.0F, 0.0F}}}},
    {{0, 0, -1}, {0.0F, 0.0F, -1.0F}, {{{1.0F, 0.0F, 0.0F}, {0.0F, 0.0F, 0.0F}, {0.0F, 1.0F, 0.0F}, {1.0F, 1.0F, 0.0F}}}},
    {{0, 0, 1}, {0.0F, 0.0F, 1.0F}, {{{0.0F, 0.0F, 1.0F}, {1.0F, 0.0F, 1.0F}, {1.0F, 1.0F, 1.0F}, {0.0F, 1.0F, 1.0F}}}},
}};

std::array<float, 4> color_for(BlockState state)
{
    switch (state)
    {
    case BlockState::Grass:
        return {0.31F, 0.72F, 0.28F, 1.0F};
    case BlockState::Clay:
        return {0.70F, 0.34F, 0.20F, 1.0F};
    case BlockState::Stone:
        return {0.52F, 0.55F, 0.60F, 1.0F};
    case BlockState::Air:
        break;
    }
    return {1.0F, 0.0F, 1.0F, 1.0F};
}

void append_face(OpaqueMesh& mesh,
                 BlockCoord coordinate,
                 BlockState state,
                 const FaceDefinition& face)
{
    const std::uint32_t base_index = static_cast<std::uint32_t>(mesh.vertices.size());
    const std::array<float, 4> color = color_for(state);
    for (const auto& corner : face.corners)
    {
        mesh.vertices.push_back(VoxelVertex{
            {static_cast<float>(coordinate.x) + corner[0],
             static_cast<float>(coordinate.y) + corner[1],
             static_cast<float>(coordinate.z) + corner[2]},
            face.normal,
            color,
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
                if (!is_opaque(state) || render_bucket(state) != RenderBucket::Opaque)
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
                    if (!is_opaque(neighbor_state))
                    {
                        append_face(mesh, coordinate, state, face);
                    }
                }
            }
        }
    }

    return mesh;
}

} // namespace tt::voxel_game
