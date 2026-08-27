#include "voxel_game/runtime/FpsCamera.h"
#include "voxel_game/runtime/FrameClock.h"
#include "voxel_game/runtime/InputState.h"
#include "voxel_game/voxel/Section.h"
#include "voxel_game/voxel/VisibleFaceMesher.h"

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

    section.set({1, 0, 0}, BlockState::Grass);
    const auto paired_block_mesh = mesh_opaque_visible_faces(section);
    require_mesh_shape(paired_block_mesh, 10, "two adjacent blocks hide their shared face");

    section.fill(BlockState::Stone);
    const auto solid_mesh = mesh_opaque_visible_faces(section);
    require_mesh_shape(solid_mesh, 6 * Section::Size * Section::Size,
                       "solid 16 cubed section has only exterior faces");

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
    section.set({15, 0, 0}, BlockState::Stone);
    const auto boundary_mesh = mesh_opaque_visible_faces(
        section,
        [](BlockCoord coordinate)
        {
            return coordinate.x == 16 && coordinate.y == 0 && coordinate.z == 0
                ? BlockState::Stone
                : BlockState::Air;
        });
    require_mesh_shape(boundary_mesh, 5, "neighbor callback can hide a boundary face");
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
