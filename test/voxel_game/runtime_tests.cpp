#include "voxel_game/runtime/FpsCamera.h"
#include "voxel_game/runtime/FrameClock.h"
#include "voxel_game/runtime/InputState.h"
#include "voxel_game/voxel/Section.h"
#include "voxel_game/voxel/VisibleFaceMesher.h"
#include "voxel_game/voxel/VoxelRaycast.h"
#include "voxel_game/voxel/World.h"

#include <array>
#include <cmath>
#include <exception>
#include <iostream>
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
    require(!raycast_voxels(axis_world, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, 1.0),
            "zero direction is rejected");

    World tie_world;
    tie_world.set({1, 0, 0}, BlockState::Stone);
    tie_world.set({0, 1, 0}, BlockState::Stone);
    const auto tie_hit = raycast_voxels(tie_world, {0.0, 0.0, 0.0}, {1.0, 1.0, 0.0}, 8.0);
    require(tie_hit.has_value() && equal_coordinate(tie_hit->block, {1, 0, 0}),
            "edge crossing uses documented X-before-Y tie priority");
}

void test_fps_camera_actions()
{
    using tt::voxel_game::ButtonAction;
    using tt::voxel_game::FpsCamera;
    using tt::voxel_game::InputState;

    InputState input;
    FpsCamera camera;
    camera.set_position({0.0F, 0.0F, 0.0F});
    input.begin_frame();
    input.on_key(87, ButtonAction::Press);
    camera.update(input, 0.5);
    require_near(camera.position()[0], 0.0, "forward movement x component");
    require_near(camera.position()[2], -3.5, "WASD movement uses delta time");

    input.set_mouse_captured(true);
    input.on_cursor_position(10.0, 10.0);
    input.on_cursor_position(20.0, 0.0);
    camera.update(input, 0.0);
    require_near(camera.yaw_degrees(), -89.0, "mouse delta changes yaw");
    require_near(camera.pitch_degrees(), -15.0, "mouse delta changes pitch");
}

} // namespace

int main()
{
    try
    {
        test_fixed_tick_accumulator();
        test_input_transitions();
        test_section_coordinates();
        test_visible_face_mesher();
        test_neighbor_lookup_boundary();
        test_world_coordinates_and_dirty_meshes();
        test_voxel_raycast();
        test_fps_camera_actions();
        std::cout << "voxel_runtime_tests: PASS" << std::endl;
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "voxel_runtime_tests: FAIL: " << error.what() << std::endl;
        return 1;
    }
}
