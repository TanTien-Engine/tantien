#include "voxel_game/runtime/FpsCamera.h"
#include "voxel_game/runtime/FrameClock.h"
#include "voxel_game/runtime/InputState.h"
#include "voxel_game/runtime/PlayerController.h"
#include "voxel_game/voxel/BlockAtlas.h"
#include "voxel_game/voxel/BlockRegistry.h"
#include "voxel_game/voxel/Section.h"
#include "voxel_game/voxel/VisibleFaceMesher.h"
#include "voxel_game/voxel/VoxelRaycast.h"
#include "voxel_game/voxel/World.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <exception>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace
{

void require(bool condition, const char* message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

void require_near(double actual, double expected, const char* message)
{
    require(std::abs(actual - expected) < 1e-6, message);
}

void test_fixed_tick_accumulator()
{
    tt::voxel_game::FixedStepAccumulator ticks(0.1, 2);
    double simulated_seconds = 0.0;

    const auto first = ticks.advance(0.05, [&simulated_seconds](double delta)
    {
        simulated_seconds += delta;
    });
    require(first.tick_count == 0, "partial frame must not tick");
    require_near(first.interpolation_alpha, 0.5, "partial frame alpha");

    const auto second = ticks.advance(0.15, [&simulated_seconds](double delta)
    {
        simulated_seconds += delta;
    });
    require(second.tick_count == 2, "accumulator must run two fixed ticks");
    require_near(simulated_seconds, 0.2, "fixed tick simulation duration");
    require_near(second.interpolation_alpha, 0.0, "exact tick boundary alpha");

    const auto capped = ticks.advance(0.35, [&simulated_seconds](double delta)
    {
        simulated_seconds += delta;
    });
    require(capped.tick_count == 2, "per-frame tick cap");
    require_near(capped.dropped_seconds, 0.1, "overrun must drop full ticks only");
    require_near(capped.interpolation_alpha, 0.5, "overrun retains interpolation remainder");
}

void test_input_transitions()
{
    tt::voxel_game::InputState input;
    input.begin_frame();
    input.on_key(87, tt::voxel_game::ButtonAction::Press);
    input.on_key(65, tt::voxel_game::ButtonAction::Press);
    require(input.held(87) && input.held(65), "simultaneous held keys");
    require(input.pressed(87) && input.pressed(65), "simultaneous pressed keys");

    input.on_key(87, tt::voxel_game::ButtonAction::Repeat);
    require(input.pressed(87), "repeat must not erase first press");

    input.begin_frame();
    require(input.held(87) && input.held(65), "held keys persist across frames");
    require(!input.pressed(87), "pressed state lasts one frame");

    input.on_key(87, tt::voxel_game::ButtonAction::Release);
    require(!input.held(87) && input.released(87), "release transition");
    require(input.held(65), "unrelated key remains held");

    input.set_mouse_captured(true);
    input.on_cursor_position(100.0, 100.0);
    input.on_cursor_position(103.0, 96.0);
    const auto delta = input.relative_mouse_delta();
    require_near(delta.x, 3.0, "captured mouse x delta");
    require_near(delta.y, -4.0, "captured mouse y delta");

    input.on_key(tt::voxel_game::InputState::EscapeKey, tt::voxel_game::ButtonAction::Press);
    require(!input.mouse_captured(), "Escape releases mouse capture");

    input.begin_frame();
    input.on_mouse_button(0, tt::voxel_game::ButtonAction::Press);
    require(input.mouse_button_held(0), "mouse held state starts on press");
    require(input.mouse_button_pressed(0), "mouse pressed state lasts current frame");
    input.on_mouse_button(0, tt::voxel_game::ButtonAction::Repeat);
    require(input.mouse_button_pressed(0), "mouse repeat retains first press edge");
    input.begin_frame();
    require(input.mouse_button_held(0), "mouse held state persists across frames");
    require(!input.mouse_button_pressed(0), "mouse press edge expires next frame");
    input.on_mouse_button(0, tt::voxel_game::ButtonAction::Release);
    require(!input.mouse_button_held(0), "mouse release clears held state");
    require(input.mouse_button_released(0), "mouse release edge lasts current frame");
}

void require_mesh_shape(const tt::voxel_game::OpaqueMesh& mesh,
                        std::size_t expected_faces,
                        const char* message)
{
    require(mesh.face_count == expected_faces, message);
    require(mesh.vertices.size() == expected_faces * 4, "four vertices per face");
    require(mesh.indices.size() == expected_faces * 6, "six uint32 indices per face");
    for (const std::uint32_t index : mesh.indices)
    {
        require(index < mesh.vertices.size(), "mesh index must address a vertex");
    }
}

std::array<float, 3> subtract(const std::array<float, 3>& left,
                              const std::array<float, 3>& right)
{
    return {left[0] - right[0], left[1] - right[1], left[2] - right[2]};
}

std::array<float, 3> cross(const std::array<float, 3>& left,
                           const std::array<float, 3>& right)
{
    return {
        left[1] * right[2] - left[2] * right[1],
        left[2] * right[0] - left[0] * right[2],
        left[0] * right[1] - left[1] * right[0],
    };
}

float dot(const std::array<float, 3>& left, const std::array<float, 3>& right)
{
    return left[0] * right[0] + left[1] * right[1] + left[2] * right[2];
}

void require_positive_triangle_winding(const tt::voxel_game::OpaqueMesh& mesh)
{
    require(mesh.indices.size() == mesh.face_count * 6, "mesh contains two triangles per face");
    for (std::size_t face = 0; face < mesh.face_count; ++face)
    {
        const std::size_t index_offset = face * 6;
        const std::uint32_t index0 = mesh.indices[index_offset];
        const std::uint32_t index1 = mesh.indices[index_offset + 1];
        const std::uint32_t index2 = mesh.indices[index_offset + 2];
        const std::uint32_t index3 = mesh.indices[index_offset + 5];
        const auto& normal = mesh.vertices[index0].normal;
        const float first_winding = dot(
            cross(subtract(mesh.vertices[index1].position, mesh.vertices[index0].position),
                  subtract(mesh.vertices[index2].position, mesh.vertices[index0].position)),
            normal);
        const float second_winding = dot(
            cross(subtract(mesh.vertices[index2].position, mesh.vertices[index0].position),
                  subtract(mesh.vertices[index3].position, mesh.vertices[index0].position)),
            normal);
        require(first_winding > 0.0F, "first quad triangle must wind toward its normal");
        require(second_winding > 0.0F, "second quad triangle must wind toward its normal");
    }
}

void test_section_coordinates()
{
    using tt::voxel_game::BlockCoord;
    using tt::voxel_game::BlockState;
    using tt::voxel_game::Section;

    require(Section::contains({0, 0, 0}), "section origin is valid");
    require(Section::contains({15, 15, 15}), "section maximum coordinate is valid");
    require(!Section::contains({16, 0, 0}), "section x upper bound");
    require(Section::index_of({0, 0, 0}) == 0, "section origin index");
    require(Section::index_of({1, 0, 0}) == 1, "x is fastest axis");
    require(Section::index_of({0, 0, 1}) == 16, "z is second storage axis");
    require(Section::index_of({0, 1, 0}) == 256, "y is third storage axis");

    Section section;
    section.set({15, 15, 15}, BlockState::Clay);
    require(section.get({15, 15, 15}) == BlockState::Clay, "section read/write round trip");
    require(section.get_or_air({16, 15, 15}) == BlockState::Air, "out of bounds is air lookup");

    bool threw = false;
    try
    {
        static_cast<void>(section.get({-1, 0, 0}));
    }
    catch (const std::out_of_range&)
    {
        threw = true;
    }
    require(threw, "strict get rejects invalid coordinate");

    bool rejected_unknown_state = false;
    try
    {
        section.set({0, 0, 0}, static_cast<BlockState>(99));
    }
    catch (const std::invalid_argument&)
    {
        rejected_unknown_state = true;
    }
    require(rejected_unknown_state, "section mutation strictly rejects an unknown block state");

    tt::voxel_game::World world;
    bool world_rejected_unknown_state = false;
    try
    {
        world.set({0, 0, 0}, static_cast<BlockState>(99));
    }
    catch (const std::invalid_argument&)
    {
        world_rejected_unknown_state = true;
    }
    require(world_rejected_unknown_state && world.section_count() == 0,
            "world strictly rejects unknown state without loading a section");
}

void test_block_registry()
{
    using tt::voxel_game::AtlasTile;
    using tt::voxel_game::BlockFace;
    using tt::voxel_game::BlockState;
    using tt::voxel_game::RenderBucket;
    using tt::voxel_game::block_definition_or_air;
    using tt::voxel_game::find_block_definition;
    using tt::voxel_game::is_collidable;
    using tt::voxel_game::is_opaque;
    using tt::voxel_game::is_valid_block_state;

    constexpr std::array<BlockState, 5> States = {
        BlockState::Air, BlockState::Grass, BlockState::Stone, BlockState::Clay, BlockState::Dirt};
    for (BlockState state : States)
    {
        const auto* definition = find_block_definition(state);
        require(definition != nullptr, "every declared BlockState has a registry definition");
        require(definition->name[0] != '\0', "registered BlockState has a name");
        require(definition->bucket == RenderBucket::Opaque, "M2C registry uses opaque bucket only");
        require(is_valid_block_state(state), "registered BlockState validates");
    }
    require(!is_opaque(BlockState::Air) && !is_collidable(BlockState::Air), "air does not block mesh or player");
    require(is_opaque(BlockState::Stone) && is_collidable(BlockState::Stone), "stone blocks mesh and player");
    const auto* grass = find_block_definition(BlockState::Grass);
    require(grass->tile_for(BlockFace::PositiveY) == AtlasTile::GrassTop, "grass top tile");
    require(grass->tile_for(BlockFace::NegativeY) == AtlasTile::Dirt, "grass bottom tile");
    require(grass->tile_for(BlockFace::NegativeX) == AtlasTile::GrassSide &&
            grass->tile_for(BlockFace::PositiveX) == AtlasTile::GrassSide &&
            grass->tile_for(BlockFace::NegativeZ) == AtlasTile::GrassSide &&
            grass->tile_for(BlockFace::PositiveZ) == AtlasTile::GrassSide,
            "grass four side faces share grass-side tile");
    const BlockState invalid = static_cast<BlockState>(99);
    require(find_block_definition(invalid) == nullptr && !is_valid_block_state(invalid),
            "unknown BlockState has explicit strict-rejection identity");
    require(&block_definition_or_air(invalid) == &block_definition_or_air(BlockState::Air),
            "read-only unknown BlockState safely falls back to air");
    bool rejected_unknown_face = false;
    try
    {
        static_cast<void>(grass->tile_for(static_cast<BlockFace>(42)));
    }
    catch (const std::out_of_range&)
    {
        rejected_unknown_face = true;
    }
    require(rejected_unknown_face, "registry strictly rejects an unknown face enum");
}

void test_block_atlas()
{
    using tt::voxel_game::AtlasTile;
    using tt::voxel_game::BlockAtlas;

    const BlockAtlas first;
    const BlockAtlas second;
    require(first.pixels().size() == static_cast<std::size_t>(BlockAtlas::Width * BlockAtlas::Height * 4),
            "atlas is exactly RGBA8 dimensions");
    require(first.hash() == second.hash(), "atlas hash is deterministic");
    require(first.gutters_are_extruded(), "every atlas tile has 1px edge extrusion");
    require(first.pixel(1, 1) == second.pixel(1, 1), "atlas exact sample is deterministic");
    for (std::size_t left = 0; left < BlockAtlas::TileCount; ++left)
    {
        const auto left_bounds = first.uv_bounds(static_cast<AtlasTile>(left));
        require(left_bounds.minimum_u < left_bounds.maximum_u && left_bounds.minimum_v < left_bounds.maximum_v,
                "tile UVs cover their content region");
        for (std::size_t right = left + 1; right < BlockAtlas::TileCount; ++right)
        {
            const auto right_bounds = first.uv_bounds(static_cast<AtlasTile>(right));
            require(left_bounds.maximum_u < right_bounds.minimum_u || right_bounds.maximum_u < left_bounds.minimum_u ||
                    left_bounds.maximum_v < right_bounds.minimum_v || right_bounds.maximum_v < left_bounds.minimum_v,
                    "content UV regions do not overlap");
        }
    }
    bool rejected_unknown_tile = false;
    try
    {
        static_cast<void>(BlockAtlas::uv_bounds(AtlasTile::Count));
    }
    catch (const std::invalid_argument&)
    {
        rejected_unknown_tile = true;
    }
    require(rejected_unknown_tile, "atlas metadata strictly rejects the Count sentinel");
    const auto grass_side_origin = BlockAtlas::content_origin(AtlasTile::GrassSide);
    const auto grass_side_bottom = first.pixel(grass_side_origin[0] + 8, grass_side_origin[1]);
    const auto grass_side_top = first.pixel(grass_side_origin[0] + 8,
                                            grass_side_origin[1] + BlockAtlas::ContentSize - 1);
    require(grass_side_bottom[0] > grass_side_bottom[1] && grass_side_top[1] > grass_side_top[0],
            "GrassSide v_min bottom is dirt while v_max top is grass");
}

void test_mesher_uvs_and_vertex_layout()
{
    using tt::voxel_game::AtlasTile;
    using tt::voxel_game::BlockAtlas;
    using tt::voxel_game::BlockState;
    using tt::voxel_game::Section;
    using tt::voxel_game::VoxelVertex;
    using tt::voxel_game::mesh_opaque_visible_faces;

    const BlockAtlas atlas;
    Section section;
    section.set({0, 0, 0}, BlockState::Grass);
    const auto mesh = mesh_opaque_visible_faces(section);
    require_mesh_shape(mesh, 6, "grass block meshes six faces");
    constexpr std::array<AtlasTile, 6> ExpectedTiles = {
        AtlasTile::GrassSide, AtlasTile::GrassSide, AtlasTile::Dirt,
        AtlasTile::GrassTop, AtlasTile::GrassSide, AtlasTile::GrassSide};
    for (std::size_t face = 0; face < ExpectedTiles.size(); ++face)
    {
        const auto bounds = atlas.uv_bounds(ExpectedTiles[face]);
        for (std::size_t vertex = 0; vertex < 4; ++vertex)
        {
            const auto& uv = mesh.vertices[face * 4 + vertex].uv;
            require(uv[0] >= bounds.minimum_u && uv[0] <= bounds.maximum_u &&
                    uv[1] >= bounds.minimum_v && uv[1] <= bounds.maximum_v,
                    "mesher UV remains inside registry tile content");
        }
        require(mesh.vertices[face * 4].uv == std::array<float, 2>{bounds.minimum_u, bounds.minimum_v} &&
                mesh.vertices[face * 4 + 1].uv == std::array<float, 2>{bounds.maximum_u, bounds.minimum_v} &&
                mesh.vertices[face * 4 + 2].uv == std::array<float, 2>{bounds.maximum_u, bounds.maximum_v} &&
                mesh.vertices[face * 4 + 3].uv == std::array<float, 2>{bounds.minimum_u, bounds.maximum_v},
                "all face definitions use the locked consistent quad UV winding");
    }
    require_positive_triangle_winding(mesh);
    require(sizeof(VoxelVertex) <= static_cast<std::size_t>((std::numeric_limits<int>::max)()) &&
            offsetof(VoxelVertex, position) <= static_cast<std::size_t>((std::numeric_limits<int>::max)()) &&
            offsetof(VoxelVertex, normal) <= static_cast<std::size_t>((std::numeric_limits<int>::max)()) &&
            offsetof(VoxelVertex, color) <= static_cast<std::size_t>((std::numeric_limits<int>::max)()) &&
            offsetof(VoxelVertex, uv) <= static_cast<std::size_t>((std::numeric_limits<int>::max)()),
            "VoxelVertex layout can safely narrow to UniRender int attributes");
}

void test_visible_face_mesher()
{
    using tt::voxel_game::BlockState;
    using tt::voxel_game::Section;
    using tt::voxel_game::mesh_opaque_visible_faces;

    Section section;
    const auto air_mesh = mesh_opaque_visible_faces(section);
    require_mesh_shape(air_mesh, 0, "all-air section must produce no faces");

    section.set({0, 0, 0}, BlockState::Stone);
    const auto single_block_mesh = mesh_opaque_visible_faces(section);
    require_mesh_shape(single_block_mesh, 6, "one solid block has six visible faces");
    require_positive_triangle_winding(single_block_mesh);

    section.set({1, 0, 0}, BlockState::Grass);
    const auto paired_block_mesh = mesh_opaque_visible_faces(section);
    require_mesh_shape(paired_block_mesh, 10, "two adjacent blocks hide their shared face");
    require_positive_triangle_winding(paired_block_mesh);

    section.fill(BlockState::Stone);
    const auto solid_mesh = mesh_opaque_visible_faces(section);
    require_mesh_shape(solid_mesh, 6 * Section::Size * Section::Size,
                       "solid 16 cubed section has only exterior faces");
    require_positive_triangle_winding(solid_mesh);

    const auto repeat_mesh = mesh_opaque_visible_faces(section);
    require(repeat_mesh.vertices == solid_mesh.vertices, "mesher vertices are deterministic");
    require(repeat_mesh.indices == solid_mesh.indices, "mesher indices are deterministic");
}

void test_neighbor_lookup_boundary()
{
    using tt::voxel_game::BlockCoord;
    using tt::voxel_game::BlockState;
    using tt::voxel_game::Section;
    using tt::voxel_game::mesh_opaque_visible_faces;

    Section section;
    section.set({14, 0, 0}, BlockState::Stone);
    section.set({15, 0, 0}, BlockState::Stone);
    const auto boundary_mesh = mesh_opaque_visible_faces(
        section,
        [](BlockCoord coordinate)
        {
            return coordinate.x == 16 && coordinate.y == 0 && coordinate.z == 0
                ? BlockState::Stone
                : BlockState::Air;
        });
    require_mesh_shape(boundary_mesh, 9,
                       "neighbor callback hides external face without exposing internal face");
    require_positive_triangle_winding(boundary_mesh);
}

bool equal_coordinate(tt::voxel_game::BlockCoord actual,
                      tt::voxel_game::BlockCoord expected)
{
    return actual.x == expected.x && actual.y == expected.y && actual.z == expected.z;
}

void test_world_coordinates_and_dirty_meshes()
{
    using tt::voxel_game::BlockState;
    using tt::voxel_game::SectionCoord;
    using tt::voxel_game::World;
    using tt::voxel_game::locate_world_block;

    const auto negative = locate_world_block({-1, -16, -17});
    require(negative.section == SectionCoord{-1, -1, -2}, "negative floor section coordinates");
    require(equal_coordinate(negative.local, {15, 0, 15}), "negative local coordinates use nonnegative modulo");

    World world;
    require(world.get({-1, 0, 0}) == BlockState::Air, "missing world section reads as air");
    require(!world.set({32, 0, 0}, BlockState::Air), "missing-section air set is a no-op");
    require(world.section_count() == 0, "missing-section air set does not load a section");
    require(world.rebuild_dirty_meshes().empty(), "missing-section air set creates no dirty mesh");
    require(world.set({15, 0, 0}, BlockState::Stone), "world set creates first section");
    require(world.set({16, 0, 0}, BlockState::Stone), "world set creates neighbor section");
    require(world.section_count() == 2, "cross-boundary writes load two sections");
    const auto initial = world.rebuild_dirty_meshes();
    require(initial.size() == 2, "both new sections build once");
    const SectionCoord left{0, 0, 0};
    const SectionCoord right{1, 0, 0};
    require(world.find_mesh(left)->face_count == 5 && world.find_mesh(right)->face_count == 5,
            "cross-section neighbors cull their shared face");
    require(!world.set({15, 0, 0}, BlockState::Stone), "idempotent world set does not dirty meshes");
    require(world.rebuild_dirty_meshes().empty(), "idempotent world set rebuilds nothing");

    require(world.set({15, 0, 0}, BlockState::Air), "boundary deletion changes world");
    require(world.section_dirty(left) && world.section_dirty(right), "boundary edit dirties loaded face neighbor");
    const auto rebuilt = world.rebuild_dirty_meshes();
    require(rebuilt.size() == 2, "boundary edit rebuilds exactly both affected sections");
    require(world.find_mesh(left)->face_count == 0 && world.find_mesh(right)->face_count == 6,
            "boundary deletion reveals neighbor face");
    require(world.mesh_revision(left) == 2 && world.mesh_revision(right) == 2,
            "only dirty sections advance mesh revisions");

    World extrema_world;
    const int minimum = (std::numeric_limits<int>::min)();
    const int maximum = (std::numeric_limits<int>::max)();
    require(extrema_world.set({minimum, 0, 0}, BlockState::Stone), "minimum coordinate write");
    require(extrema_world.set({maximum, 0, 0}, BlockState::Stone), "maximum coordinate write");
    const auto extrema_rebuilt = extrema_world.rebuild_dirty_meshes();
    require(extrema_rebuilt.size() == 2, "coordinate extrema rebuild two sections");
    for (const SectionCoord coordinate : extrema_rebuilt)
    {
        require(extrema_world.find_mesh(coordinate)->face_count == 6,
                "out-of-domain mesh neighbor is treated as air");
    }
}

void test_voxel_raycast()
{
    using tt::voxel_game::BlockState;
    using tt::voxel_game::World;
    using tt::voxel_game::raycast_voxels;

    World world;
    world.set({-1, 0, 0}, BlockState::Stone);
    const auto negative_hit = raycast_voxels(world, {2.5, 0.5, 0.5}, {-2.0, 0.0, 0.0}, 3.0);
    require(negative_hit.has_value(), "negative direction ray finds solid block");
    require(equal_coordinate(negative_hit->block, {-1, 0, 0}), "negative direction block coordinate");
    require(equal_coordinate(negative_hit->entered_face_normal, {1, 0, 0}), "negative direction entered face normal");
    require(equal_coordinate(negative_hit->previous_empty, {0, 0, 0}), "ray reports placement cell");
    require_near(negative_hit->distance, 2.5, "ray normalizes direction before distance reporting");

    World axis_world;
    axis_world.set({0, 0, 0}, BlockState::Stone);
    const auto axis_hit = raycast_voxels(axis_world, {0.5, 0.5, -2.0}, {0.0, 0.0, 3.0}, 2.0);
    require(axis_hit.has_value(), "axis-aligned ray handles zero components");
    require_near(axis_hit->distance, 2.0, "maximum distance hit is inclusive");
    require(!raycast_voxels(axis_world, {0.5, 0.5, -2.0}, {0.0, 0.0, 1.0}, 1.999),
            "ray past maximum distance misses");
    const auto start_hit = raycast_voxels(axis_world, {0.5, 0.5, 0.5}, {0.0, 1.0, 0.0}, 8.0);
    require(start_hit.has_value() && !start_hit->has_previous_empty && start_hit->distance == 0.0,
            "starting in solid block is a distance-zero hit without placement cell");
    require(!raycast_voxels(axis_world, {NAN, 0.0, 0.0}, {1.0, 0.0, 0.0}, 1.0),
            "nonfinite ray origin is rejected");
    require(!raycast_voxels(axis_world, {1.0e100, 0.0, 0.0}, {1.0, 0.0, 0.0}, 1.0),
            "finite origin outside BlockCoord domain is rejected");
    require(!raycast_voxels(axis_world, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, 1.0),
            "zero direction is rejected");

    World maximum_world;
    const int maximum = (std::numeric_limits<int>::max)();
    maximum_world.set({maximum, 0, 0}, BlockState::Stone);
    require(!raycast_voxels(maximum_world,
                            {static_cast<double>(maximum) + 0.5, 0.5, 0.5},
                            {1.0, 0.0, 0.0},
                            0.5),
            "ray segment leaving BlockCoord domain is rejected before narrowing");

    World tie_world;
    tie_world.set({1, 0, 0}, BlockState::Stone);
    tie_world.set({0, 1, 0}, BlockState::Stone);
    const auto tie_hit = raycast_voxels(tie_world, {0.0, 0.0, 0.0}, {1.0, 1.0, 0.0}, 8.0);
    require(tie_hit.has_value() && equal_coordinate(tie_hit->block, {1, 0, 0}),
            "edge crossing uses documented X-before-Y tie priority");
}

void test_fps_camera_actions()
{
    using tt::voxel_game::FpsCamera;
    using tt::voxel_game::InputState;

    InputState input;
    FpsCamera camera;
    input.set_mouse_captured(true);
    input.on_cursor_position(10.0, 10.0);
    input.on_cursor_position(20.0, 0.0);
    camera.update_look(input);
    require_near(camera.yaw_degrees(), -89.0, "mouse delta changes yaw");
    require_near(camera.pitch_degrees(), -15.0, "mouse delta changes pitch");
    const auto forward = camera.forward();
    require_near(forward[0], 0.016858, "view forward reflects yaw without moving player");
    require_near(forward[1], -0.258819, "view forward reflects pitch without moving player");
}

void fill_floor(tt::voxel_game::World& world, int minimum_x, int maximum_x,
                int minimum_z, int maximum_z)
{
    for (int z = minimum_z; z <= maximum_z; ++z)
    {
        for (int x = minimum_x; x <= maximum_x; ++x)
        {
            world.set({x, 0, z}, tt::voxel_game::BlockState::Grass);
        }
    }
}

void step_player(tt::voxel_game::PlayerController& player, const tt::voxel_game::World& world,
                 int ticks, tt::voxel_game::PlayerMoveIntent intent = {})
{
    for (int tick = 0; tick < ticks; ++tick)
    {
        player.fixed_step(world, 1.0 / 60.0, intent);
    }
}

void test_player_collision_and_fixed_step()
{
    using tt::voxel_game::BlockState;
    using tt::voxel_game::PlayerController;
    using tt::voxel_game::PlayerMotionConfig;
    using tt::voxel_game::PlayerMoveIntent;
    using tt::voxel_game::World;
    using tt::voxel_game::player_aabb;
    using tt::voxel_game::player_can_place_block;
    using tt::voxel_game::player_intersects_solid_world;

    World falling_world;
    fill_floor(falling_world, -4, 4, -4, 4);
    PlayerController falling;
    falling.state().feet_position = {0.5, 5.0, 0.5};
    step_player(falling, falling_world, 180);
    require(falling.state().grounded, "falling player lands and is grounded");
    require_near(falling.state().feet_position[1], 1.0, "falling player rests on floor");
    require_near(falling.state().velocity[1], 0.0, "floor collision clears falling speed");
    require(!player_intersects_solid_world(falling_world, player_aabb(falling.state(), falling.config())),
            "landed player does not penetrate floor");

    World jump_world;
    fill_floor(jump_world, -4, 4, -4, 4);
    for (int x = -1; x <= 1; ++x)
    {
        for (int z = -1; z <= 1; ++z)
        {
            jump_world.set({x, 4, z}, BlockState::Stone);
        }
    }
    PlayerController jumper;
    jumper.state().feet_position = {0.5, 1.0, 0.5};
    step_player(jumper, jump_world, 1);
    require(jumper.state().grounded, "spawn settles before jump request");
    require(jumper.request_jump() && jumper.jump_pending(), "grounded jump edge queues before a fixed tick");
    tt::voxel_game::FixedStepAccumulator jump_ticks(1.0 / 60.0, 2);
    const auto no_tick = jump_ticks.advance(1.0 / 120.0, [&jumper, &jump_world](double tick_seconds)
    {
        jumper.fixed_step(jump_world, tick_seconds, {});
    });
    require(no_tick.tick_count == 0 && jumper.jump_pending(),
            "jump edge survives a render frame with no fixed tick");
    const auto one_tick = jump_ticks.advance(1.0 / 120.0, [&jumper, &jump_world](double tick_seconds)
    {
        jumper.fixed_step(jump_world, tick_seconds, {});
    });
    require(one_tick.tick_count == 1, "second partial frame runs one fixed tick");
    require(!jumper.jump_pending() && !jumper.state().grounded && jumper.state().velocity[1] > 0.0,
            "next fixed tick consumes exactly one jump edge");
    bool hit_ceiling = false;
    for (int tick = 0; tick < 240; ++tick)
    {
        jumper.fixed_step(jump_world, 1.0 / 60.0, {});
        hit_ceiling = hit_ceiling || (jumper.state().velocity[1] == 0.0 && !jumper.state().grounded &&
                                      jumper.state().feet_position[1] <= 2.200001);
    }
    require(hit_ceiling, "upward sweep clips at ceiling");
    require(jumper.state().grounded && jumper.state().feet_position[1] >= 0.999999,
            "jump returns to grounded floor state");
    require(jumper.request_jump(), "grounded player accepts the next distinct jump edge");
    jumper.fixed_step(jump_world, 1.0 / 60.0, {});
    require(!jumper.request_jump(), "airborne player cannot queue repeat jump");

    World wall_world;
    fill_floor(wall_world, -2, 5, -2, 5);
    for (int z = -2; z <= 5; ++z)
    {
        for (int y = 1; y <= 3; ++y)
        {
            wall_world.set({2, y, z}, BlockState::Stone);
        }
    }
    PlayerController slider;
    slider.state().feet_position = {0.5, 1.0, 0.5};
    step_player(slider, wall_world, 60, {1.0, 1.0});
    require(slider.state().feet_position[0] <= 1.700001, "wall blocks normal movement");
    require(slider.state().feet_position[2] > 2.0, "wall collision preserves tangential slide");
    require(!player_intersects_solid_world(wall_world, player_aabb(slider.state(), slider.config())),
            "wall slide leaves no penetration");

    PlayerMotionConfig fast_config;
    fast_config.walk_speed = 600.0;
    PlayerController fast(fast_config);
    fast.state().feet_position = {0.5, 1.0, 0.5};
    fast.fixed_step(wall_world, 1.0 / 60.0, {1.0, 0.0});
    require(fast.state().feet_position[0] <= 1.700001, "swept clipping prevents high-speed wall tunnelling");

    World negative_world;
    fill_floor(negative_world, -20, -10, -2, 2);
    for (int y = 1; y <= 3; ++y)
    {
        negative_world.set({-15, y, 0}, BlockState::Stone);
    }
    require(negative_world.section_count() >= 2,
            "negative collision fixture spans the -16 section boundary");
    PlayerController negative;
    negative.state().feet_position = {-17.5, 1.0, 0.5};
    step_player(negative, negative_world, 60, {1.0, 0.0});
    require(negative.state().feet_position[0] > -16.0 && negative.state().feet_position[0] >= -15.300001 &&
                negative.state().feet_position[0] <= -15.299999,
            "negative coordinates cross section boundary and stop at wall");
    require(!player_intersects_solid_world(negative_world, player_aabb(negative.state(), negative.config())),
            "negative-coordinate collision has no penetration");

    PlayerController airborne;
    airborne.state().feet_position = {0.5, 4.0, 0.5};
    require(!airborne.request_jump(), "airborne player cannot queue jump");
    PlayerController placement;
    placement.state().feet_position = {0.5, 1.0, 0.5};
    require(!player_can_place_block(placement.state(), {0, 1, 0}, placement.config()),
            "placing a block inside player AABB is rejected by runtime API");
    require(player_can_place_block(placement.state(), {0, 3, 0}, placement.config()),
            "non-overlapping placement target remains allowed");

    bool rejected_nonfinite_config = false;
    try
    {
        PlayerMotionConfig invalid;
        invalid.gravity = std::numeric_limits<double>::infinity();
        PlayerController invalid_player(invalid);
    }
    catch (const std::invalid_argument&)
    {
        rejected_nonfinite_config = true;
    }
    require(rejected_nonfinite_config, "nonfinite player configuration is strictly rejected");

    bool rejected_domain_escape = false;
    try
    {
        PlayerMotionConfig extreme_config;
        extreme_config.walk_speed = 600.0;
        PlayerController extreme(extreme_config);
        extreme.state().feet_position = {
            static_cast<double>((std::numeric_limits<int>::max)()) - 0.5, 1.0, 0.5};
        extreme.fixed_step(falling_world, 1.0 / 60.0, {1.0, 0.0});
    }
    catch (const std::out_of_range&)
    {
        rejected_domain_escape = true;
    }
    require(rejected_domain_escape, "player outside BlockCoord domain is strictly rejected");
}

} // namespace

int main()
{
    try
    {
        test_fixed_tick_accumulator();
        test_input_transitions();
        test_section_coordinates();
        test_block_registry();
        test_block_atlas();
        test_visible_face_mesher();
        test_mesher_uvs_and_vertex_layout();
        test_neighbor_lookup_boundary();
        test_world_coordinates_and_dirty_meshes();
        test_voxel_raycast();
        test_fps_camera_actions();
        test_player_collision_and_fixed_step();
        std::cout << "voxel_runtime_tests: PASS" << std::endl;
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "voxel_runtime_tests: FAIL: " << error.what() << std::endl;
        return 1;
    }
}
