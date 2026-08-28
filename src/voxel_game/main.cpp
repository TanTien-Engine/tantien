#include "voxel_game/render/VoxelGpuMesh.h"
#include "voxel_game/runtime/FpsCamera.h"
#include "voxel_game/runtime/FrameClock.h"
#include "voxel_game/runtime/InputState.h"
#include "voxel_game/runtime/PlayerController.h"
#include "voxel_game/voxel/VoxelRaycast.h"
#include "voxel_game/voxel/BlockAtlas.h"
#include "voxel_game/voxel/World.h"

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#include <unirender/Context.h>
#include <unirender/Device.h>
#include <unirender/TextureDescription.h>
#include <unirender/Texture.h>
#include <unirender/opengl/Context.h>
#include <unirender/opengl/Device.h>

#include <array>
#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace
{

using tt::voxel_game::BlockCoord;
using tt::voxel_game::BlockAtlas;
using tt::voxel_game::BlockState;
using tt::voxel_game::ButtonAction;
using tt::voxel_game::FpsCamera;
using tt::voxel_game::FrameClock;
using tt::voxel_game::FixedStepAccumulator;
using tt::voxel_game::InputState;
using tt::voxel_game::OpaqueMesh;
using tt::voxel_game::PlayerController;
using tt::voxel_game::PlayerMoveIntent;
using tt::voxel_game::Section;
using tt::voxel_game::SectionCoord;
using tt::voxel_game::SectionCoordHash;
using tt::voxel_game::VoxelGpuMesh;
using tt::voxel_game::World;

constexpr float DegreesToRadians = 0.01745329251994329577F;
constexpr double EditReach = 8.0;
constexpr int SpaceKey = 32;

struct Mat4
{
    std::array<float, 16> values{};
};

ButtonAction to_button_action(int glfw_action)
{
    if (glfw_action == GLFW_PRESS)
    {
        return ButtonAction::Press;
    }
    if (glfw_action == GLFW_RELEASE)
    {
        return ButtonAction::Release;
    }
    return ButtonAction::Repeat;
}

InputState* input_for(GLFWwindow* window)
{
    return static_cast<InputState*>(glfwGetWindowUserPointer(window));
}

void apply_cursor_mode(GLFWwindow* window, const InputState& input)
{
    glfwSetInputMode(window, GLFW_CURSOR,
                     input.mouse_captured() ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    if (glfwRawMouseMotionSupported())
    {
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION,
                         input.mouse_captured() ? GLFW_TRUE : GLFW_FALSE);
    }
}

void key_callback(GLFWwindow* window, int key, int, int action, int)
{
    InputState* input = input_for(window);
    input->on_key(key, to_button_action(action));
    apply_cursor_mode(window, *input);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int)
{
    InputState* input = input_for(window);
    input->on_mouse_button(button, to_button_action(action));
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && !input->mouse_captured())
    {
        input->set_mouse_captured(true);
        apply_cursor_mode(window, *input);
    }
}

void cursor_position_callback(GLFWwindow* window, double x, double y)
{
    input_for(window)->on_cursor_position(x, y);
}

void window_size_callback(GLFWwindow*, int width, int height)
{
    glViewport(0, 0, width, height);
}

bool initialize_unirender(std::shared_ptr<ur::Device>& device,
                          std::shared_ptr<ur::Context>& context)
{
    device = std::make_shared<ur::opengl::Device>();
    if (!device)
    {
        std::cerr << "failed to create OpenGL UniRender device" << std::endl;
        return false;
    }
    device->Init();
    context = std::make_shared<ur::opengl::Context>(*device);
    if (!context)
    {
        std::cerr << "failed to create OpenGL UniRender context" << std::endl;
        return false;
    }
    return true;
}

World make_playable_world()
{
    World world;
    for (int z = -8; z <= 17; ++z)
    {
        for (int x = -10; x <= 25; ++x)
        {
            world.set({x, -1, z}, BlockState::Dirt);
            world.set({x, 0, z}, BlockState::Grass);
        }
    }
    for (int z = 3; z <= 8; ++z)
    {
        for (int x = 3; x <= 9; ++x)
        {
            world.set({x, 1, z}, (x + z) % 3 == 0 ? BlockState::Clay : BlockState::Stone);
        }
    }
    for (int y = 1; y <= 4; ++y)
    {
        world.set({-1, y, 4}, BlockState::Clay);
        world.set({0, y, 4}, BlockState::Clay);
        world.set({15, y, 1}, BlockState::Stone);
        world.set({16, y, 1}, BlockState::Stone);
    }
    for (int z = -2; z <= 1; ++z)
    {
        for (int x = 8; x <= 12; ++x)
        {
            world.set({x, 1, z}, BlockState::Dirt);
            world.set({x, 2, z}, BlockState::Grass);
        }
    }
    return world;
}

World make_smoke_world()
{
    World world;
    world.set({15, 1, 0}, BlockState::Stone);
    world.set({16, 1, 0}, BlockState::Stone);
    return world;
}

World make_smoke_physics_world()
{
    World world;
    for (int z = -3; z <= 3; ++z)
    {
        for (int x = -3; x <= 3; ++x)
        {
            world.set({x, 0, z}, BlockState::Grass);
        }
    }
    for (int y = 1; y <= 3; ++y)
    {
        world.set({2, y, 0}, BlockState::Stone);
    }
    return world;
}

PlayerMoveIntent player_move_intent(const InputState& input, const FpsCamera& camera)
{
    const std::array<float, 3> view_forward = camera.forward();
    const double horizontal_length = std::sqrt(static_cast<double>(view_forward[0]) * view_forward[0] +
                                               static_cast<double>(view_forward[2]) * view_forward[2]);
    const double forward_x = static_cast<double>(view_forward[0]) / horizontal_length;
    const double forward_z = static_cast<double>(view_forward[2]) / horizontal_length;
    const double right_x = -forward_z;
    const double right_z = forward_x;
    double forward_axis = 0.0;
    double right_axis = 0.0;
    if (input.held(87)) { forward_axis += 1.0; }
    if (input.held(83)) { forward_axis -= 1.0; }
    if (input.held(68)) { right_axis += 1.0; }
    if (input.held(65)) { right_axis -= 1.0; }
    return {forward_x * forward_axis + right_x * right_axis,
            forward_z * forward_axis + right_z * right_axis};
}

std::array<float, 3> as_float3(const std::array<double, 3>& value)
{
    return {static_cast<float>(value[0]), static_cast<float>(value[1]), static_cast<float>(value[2])};
}

OpaqueMesh make_selection_mesh()
{
    Section section;
    section.set({0, 0, 0}, BlockState::Clay);
    OpaqueMesh mesh = tt::voxel_game::mesh_opaque_visible_faces(section);
    constexpr float OutlineExpansion = 0.015F;
    for (auto& vertex : mesh.vertices)
    {
        for (float& component : vertex.position)
        {
            component += component < 0.5F ? -OutlineExpansion : OutlineExpansion;
        }
    }
    return mesh;
}

bool shader_log(GLuint shader, std::string& error)
{
    GLint success = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success == GL_TRUE)
    {
        return true;
    }
    char log[1024]{};
    glGetShaderInfoLog(shader, static_cast<GLsizei>(sizeof(log)), nullptr, log);
    error = log;
    return false;
}

bool create_program(GLuint& program, std::string& error)
{
    constexpr const char* VertexSource = R"(
        #version 330 core
        layout(location = 0) in vec3 a_position;
        layout(location = 1) in vec3 a_normal;
        layout(location = 2) in vec4 a_color;
        layout(location = 3) in vec2 a_uv;
        uniform mat4 u_mvp;
        uniform mat4 u_model;
        out vec3 v_normal;
        out vec4 v_color;
        out vec2 v_uv;
        void main()
        {
            gl_Position = u_mvp * u_model * vec4(a_position, 1.0);
            v_normal = a_normal;
            v_color = a_color;
            v_uv = a_uv;
        }
    )";
    constexpr const char* FragmentSource = R"(
        #version 330 core
        in vec3 v_normal;
        in vec4 v_color;
        in vec2 v_uv;
        uniform sampler2D u_atlas;
        uniform bool u_selection;
        out vec4 out_color;
        void main()
        {
            if (u_selection)
            {
                out_color = vec4(1.0, 0.93, 0.18, 1.0);
                return;
            }
            vec3 light_direction = normalize(vec3(0.45, 0.80, 0.35));
            float brightness = 0.30 + 0.70 * max(dot(normalize(v_normal), light_direction), 0.0);
            vec4 texel = texture(u_atlas, v_uv);
            out_color = vec4(texel.rgb * v_color.rgb * brightness, texel.a * v_color.a);
        }
    )";
    const GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    const GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(vertex_shader, 1, &VertexSource, nullptr);
    glCompileShader(vertex_shader);
    if (!shader_log(vertex_shader, error))
    {
        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);
        return false;
    }
    glShaderSource(fragment_shader, 1, &FragmentSource, nullptr);
    glCompileShader(fragment_shader);
    if (!shader_log(fragment_shader, error))
    {
        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);
        return false;
    }
    program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    GLint linked = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (linked == GL_TRUE)
    {
        return true;
    }
    char log[1024]{};
    glGetProgramInfoLog(program, static_cast<GLsizei>(sizeof(log)), nullptr, log);
    error = log;
    glDeleteProgram(program);
    program = 0;
    return false;
}

std::array<float, 3> normalize(std::array<float, 3> value)
{
    const float length = std::sqrt(value[0] * value[0] + value[1] * value[1] + value[2] * value[2]);
    return {value[0] / length, value[1] / length, value[2] / length};
}

std::array<float, 3> cross(const std::array<float, 3>& left,
                           const std::array<float, 3>& right)
{
    return {left[1] * right[2] - left[2] * right[1],
            left[2] * right[0] - left[0] * right[2],
            left[0] * right[1] - left[1] * right[0]};
}

float dot(const std::array<float, 3>& left, const std::array<float, 3>& right)
{
    return left[0] * right[0] + left[1] * right[1] + left[2] * right[2];
}

Mat4 perspective(float vertical_fov_degrees, float aspect_ratio, float near_plane, float far_plane)
{
    const float focal_length = 1.0F / std::tan(vertical_fov_degrees * DegreesToRadians * 0.5F);
    Mat4 result;
    result.values[0] = focal_length / aspect_ratio;
    result.values[5] = focal_length;
    result.values[10] = (far_plane + near_plane) / (near_plane - far_plane);
    result.values[11] = -1.0F;
    result.values[14] = (2.0F * far_plane * near_plane) / (near_plane - far_plane);
    return result;
}

Mat4 look_at(const std::array<float, 3>& eye, const std::array<float, 3>& forward)
{
    const std::array<float, 3> side = normalize(cross(forward, {0.0F, 1.0F, 0.0F}));
    const std::array<float, 3> up = cross(side, forward);
    Mat4 result;
    result.values[0] = side[0];
    result.values[1] = up[0];
    result.values[2] = -forward[0];
    result.values[4] = side[1];
    result.values[5] = up[1];
    result.values[6] = -forward[1];
    result.values[8] = side[2];
    result.values[9] = up[2];
    result.values[10] = -forward[2];
    result.values[12] = -dot(side, eye);
    result.values[13] = -dot(up, eye);
    result.values[14] = dot(forward, eye);
    result.values[15] = 1.0F;
    return result;
}

Mat4 multiply(const Mat4& left, const Mat4& right)
{
    Mat4 result;
    for (int column = 0; column < 4; ++column)
    {
        for (int row = 0; row < 4; ++row)
        {
            for (int term = 0; term < 4; ++term)
            {
                result.values[column * 4 + row] +=
                    left.values[term * 4 + row] * right.values[column * 4 + term];
            }
        }
    }
    return result;
}

Mat4 translation(BlockCoord coordinate)
{
    Mat4 result;
    result.values[0] = 1.0F;
    result.values[5] = 1.0F;
    result.values[10] = 1.0F;
    result.values[15] = 1.0F;
    result.values[12] = static_cast<float>(coordinate.x);
    result.values[13] = static_cast<float>(coordinate.y);
    result.values[14] = static_cast<float>(coordinate.z);
    return result;
}

BlockCoord section_origin(SectionCoord coordinate)
{
    return {coordinate.x * Section::Size,
            coordinate.y * Section::Size,
            coordinate.z * Section::Size};
}

bool is_polygon_mode(GLint mode)
{
    return mode == GL_POINT || mode == GL_LINE || mode == GL_FILL;
}

bool restore_polygon_modes(const std::array<GLint, 2>& polygon_modes)
{
    // In the requested OpenGL 3.3 core profile glPolygonMode accepts only
    // GL_FRONT_AND_BACK, so a legal snapshot is necessarily uniform. Capture
    // both GL_POLYGON_MODE values as required, then restore the shared mode.
    const bool valid = is_polygon_mode(polygon_modes[0]) &&
                       is_polygon_mode(polygon_modes[1]) &&
                       polygon_modes[0] == polygon_modes[1];
    glPolygonMode(GL_FRONT_AND_BACK, valid ? polygon_modes[0] : GL_FILL);
    return valid;
}

bool smoke_framebuffer_has_voxel_pixels(int width, int height)
{
    if (width <= 0 || height <= 0)
    {
        return false;
    }
    const std::size_t pixel_count = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
    std::vector<unsigned char> colors(pixel_count * 4);
    std::vector<float> depths(pixel_count);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, colors.data());
    glReadPixels(0, 0, width, height, GL_DEPTH_COMPONENT, GL_FLOAT, depths.data());
    bool has_non_clear_color = false;
    bool has_depth_written = false;
    for (std::size_t pixel = 0; pixel < pixel_count; ++pixel)
    {
        const std::size_t offset = pixel * 4;
        if (std::abs(colors[offset] - 20) > 1 || std::abs(colors[offset + 1] - 31) > 1 ||
            std::abs(colors[offset + 2] - 46) > 1)
        {
            has_non_clear_color = true;
        }
        has_depth_written = has_depth_written || depths[pixel] < 0.99999F;
    }
    return has_non_clear_color && has_depth_written;
}

using GpuMeshes = std::unordered_map<SectionCoord, VoxelGpuMesh, SectionCoordHash>;

bool upload_rebuilt_meshes(const World& world, const std::vector<SectionCoord>& rebuilt,
                           const ur::Device& device, GpuMeshes& gpu_meshes, std::string& error)
{
    for (const SectionCoord coordinate : rebuilt)
    {
        const OpaqueMesh* mesh = world.find_mesh(coordinate);
        if (mesh == nullptr || !gpu_meshes[coordinate].upload(device, *mesh, error))
        {
            error = mesh == nullptr ? "world rebuilt a missing section mesh" : error;
            return false;
        }
    }
    return true;
}

bool run_smoke_world_edits(World& world, std::vector<SectionCoord>& initial_rebuilt)
{
    initial_rebuilt = world.rebuild_dirty_meshes();
    const SectionCoord left{0, 0, 0};
    const SectionCoord right{1, 0, 0};
    if (initial_rebuilt.size() != 2 || world.find_mesh(left)->face_count != 5 ||
        world.find_mesh(right)->face_count != 5)
    {
        return false;
    }
    const auto hit = tt::voxel_game::raycast_voxels(world, {15.5, 1.5, -2.0}, {0.0, 0.0, 1.0}, EditReach);
    if (!hit || hit->block.x != 15 || hit->block.y != 1 || hit->block.z != 0 ||
        !hit->has_previous_empty || hit->previous_empty.z != -1)
    {
        return false;
    }
    world.set(hit->block, BlockState::Air);
    const std::vector<SectionCoord> after_break = world.rebuild_dirty_meshes();
    if (after_break.size() != 2 || world.find_mesh(left)->face_count != 0 ||
        world.find_mesh(right)->face_count != 6 || world.mesh_revision(left) != 2 ||
        world.mesh_revision(right) != 2)
    {
        return false;
    }
    world.set(hit->previous_empty, BlockState::Stone);
    const std::vector<SectionCoord> after_place = world.rebuild_dirty_meshes();
    const SectionCoord placed{0, 0, -1};
    if (after_place.size() != 2 || world.find_mesh(placed)->face_count != 6 ||
        world.find_mesh(left)->face_count != 0 || world.find_mesh(right)->face_count != 6 ||
        world.mesh_revision(left) != 3 || world.mesh_revision(right) != 2)
    {
        return false;
    }
    std::cout << "smoke world: sections=" << world.section_count()
              << " initial_faces=10 after_break_faces=6 after_place_faces=12 revisions="
              << world.mesh_revision(left) << "," << world.mesh_revision(right) << ","
              << world.mesh_revision(placed) << std::endl;
    return true;
}

bool mesh_uses_tile(const OpaqueMesh& mesh, const BlockAtlas& atlas, tt::voxel_game::AtlasTile tile)
{
    const auto bounds = atlas.uv_bounds(tile);
    for (const auto& vertex : mesh.vertices)
    {
        if (vertex.uv[0] >= bounds.minimum_u && vertex.uv[0] <= bounds.maximum_u &&
            vertex.uv[1] >= bounds.minimum_v && vertex.uv[1] <= bounds.maximum_v)
        {
            return true;
        }
    }
    return false;
}

bool smoke_scene_has_multiple_visible_tiles(const World& world, const BlockAtlas& atlas)
{
    std::size_t tiles = 0;
    for (std::size_t index = 0; index < BlockAtlas::TileCount; ++index)
    {
        bool found = false;
        for (int z = -1; z <= 0 && !found; ++z)
        {
            for (int x = 0; x <= 1 && !found; ++x)
            {
                const OpaqueMesh* mesh = world.find_mesh({x, 0, z});
                found = mesh != nullptr && mesh_uses_tile(
                    *mesh, atlas, static_cast<tt::voxel_game::AtlasTile>(index));
            }
        }
        tiles += found ? 1U : 0U;
    }
    std::cout << "smoke atlas: size=" << BlockAtlas::Width << "x" << BlockAtlas::Height
              << " tiles=" << BlockAtlas::TileCount << " hash=" << atlas.hash()
              << " mesh_tiles=" << tiles << " mipmaps=disabled" << std::endl;
    return tiles >= 2;
}

bool run_smoke_physics_script()
{
    World world = make_smoke_physics_world();
    PlayerController player;
    player.state().feet_position = {0.5, 4.0, 0.5};
    constexpr double TickSeconds = 1.0 / 60.0;
    std::size_t ticks = 0;
    while (!player.state().grounded && ticks < 240)
    {
        player.fixed_step(world, TickSeconds, {});
        ++ticks;
    }
    if (!player.state().grounded || tt::voxel_game::player_intersects_solid_world(
                                       world, tt::voxel_game::player_aabb(player.state(), player.config())))
    {
        return false;
    }
    if (!player.request_jump() || !player.jump_pending())
    {
        return false;
    }
    bool was_airborne = false;
    for (std::size_t step = 0; step < 240; ++step)
    {
        player.fixed_step(world, TickSeconds, {1.0, 0.0});
        ++ticks;
        was_airborne = was_airborne || !player.state().grounded;
    }
    const auto& state = player.state();
    const bool valid = was_airborne && state.grounded && state.feet_position[0] <= 1.700001 &&
                       !tt::voxel_game::player_intersects_solid_world(
                           world, tt::voxel_game::player_aabb(state, player.config()));
    std::cout << "smoke physics: ticks=" << ticks << " final_feet="
              << state.feet_position[0] << "," << state.feet_position[1] << ","
              << state.feet_position[2] << " grounded=" << (state.grounded ? 1 : 0) << std::endl;
    return valid;
}

int run(bool smoke)
{
    if (glfwInit() == GLFW_FALSE)
    {
        std::cerr << "failed to initialize GLFW" << std::endl;
        return 1;
    }
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    if (smoke)
    {
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    }
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Voxel Game M2B", nullptr, nullptr);
    if (window == nullptr)
    {
        std::cerr << "failed to create GLFW window" << std::endl;
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    if (gl3wInit() != 0)
    {
        std::cerr << "failed to initialize GL3W" << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }
    std::shared_ptr<ur::Device> device;
    std::shared_ptr<ur::Context> context;
    if (!initialize_unirender(device, context))
    {
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }
    std::shared_ptr<ur::Texture> atlas_texture;
    std::shared_ptr<ur::TextureSampler> atlas_sampler;
    const auto release_atlas_resources = [&context, &atlas_texture, &atlas_sampler]()
    {
        // Context owns its own shared references. Release those before the
        // local references while the GLFW/OpenGL context is still current.
        if (context)
        {
            context->SetTexture(0, nullptr);
            context->SetTextureSampler(0, nullptr);
        }
        atlas_sampler.reset();
        atlas_texture.reset();
    };
    const BlockAtlas atlas;
    if (!atlas.gutters_are_extruded() || atlas.pixels().size() !=
        static_cast<std::size_t>(BlockAtlas::Width * BlockAtlas::Height * 4))
    {
        std::cerr << "generated block atlas failed its CPU invariants" << std::endl;
        context.reset();
        device.reset();
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }
    const auto mesh_start = std::chrono::steady_clock::now();
    World world = smoke ? make_smoke_world() : make_playable_world();
    std::vector<SectionCoord> rebuilt;
    if (smoke)
    {
        if (!run_smoke_physics_script())
        {
            std::cerr << "smoke physics assertions failed" << std::endl;
            context.reset();
            device.reset();
            glfwDestroyWindow(window);
            glfwTerminate();
            return 1;
        }
        if (!run_smoke_world_edits(world, rebuilt))
        {
            std::cerr << "smoke world edit assertions failed" << std::endl;
            context.reset();
            device.reset();
            glfwDestroyWindow(window);
            glfwTerminate();
            return 1;
        }
        world.set({14, 1, 0}, BlockState::Grass);
        world.set({14, 1, 1}, BlockState::Dirt);
        world.set({14, 2, 0}, BlockState::Clay);
        rebuilt = world.rebuild_dirty_meshes();
        if (!smoke_scene_has_multiple_visible_tiles(world, atlas))
        {
            std::cerr << "smoke scene did not mesh multiple atlas tiles" << std::endl;
            context.reset();
            device.reset();
            glfwDestroyWindow(window);
            glfwTerminate();
            return 1;
        }
    }
    else
    {
        rebuilt = world.rebuild_dirty_meshes();
    }
    const double mesh_milliseconds = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - mesh_start).count();
    std::size_t total_faces = 0;
    for (const SectionCoord coordinate : rebuilt)
    {
        total_faces += world.find_mesh(coordinate)->face_count;
    }
    std::cout << "voxel world: sections=" << world.section_count() << " rebuilt=" << rebuilt.size()
              << " faces=" << total_faces << " cpu_mesh_ms=" << mesh_milliseconds << std::endl;
    GpuMeshes gpu_meshes;
    std::string error;
    if (!upload_rebuilt_meshes(world, rebuilt, *device, gpu_meshes, error))
    {
        std::cerr << "failed to upload voxel mesh: " << error << std::endl;
        context.reset();
        device.reset();
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }
    VoxelGpuMesh selection_mesh;
    if (!selection_mesh.upload(*device, make_selection_mesh(), error))
    {
        std::cerr << "failed to upload selection mesh: " << error << std::endl;
        context.reset();
        device.reset();
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }
    GLuint program = 0;
    if (!create_program(program, error))
    {
        std::cerr << "failed to create voxel shader: " << error << std::endl;
        selection_mesh.reset();
        context.reset();
        device.reset();
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }
    const GLint mvp_location = glGetUniformLocation(program, "u_mvp");
    const GLint model_location = glGetUniformLocation(program, "u_model");
    const GLint atlas_location = glGetUniformLocation(program, "u_atlas");
    const GLint selection_location = glGetUniformLocation(program, "u_selection");
    if (mvp_location < 0 || model_location < 0 || atlas_location < 0 || selection_location < 0)
    {
        std::cerr << "voxel shader is missing a transform uniform" << std::endl;
        glDeleteProgram(program);
        selection_mesh.reset();
        context.reset();
        device.reset();
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }
    ur::TextureDescription atlas_description;
    atlas_description.width = BlockAtlas::Width;
    atlas_description.height = BlockAtlas::Height;
    atlas_description.format = ur::TextureFormat::RGBA8;
    atlas_description.sampler_type = ur::Device::TextureSamplerType::NearestClamp;
    atlas_description.gen_mipmaps = false;
    atlas_texture = device->CreateTexture(atlas_description, atlas.pixels().data());
    atlas_sampler = device->GetTextureSampler(ur::Device::TextureSamplerType::NearestClamp);
    if (!atlas_texture || atlas_texture->GetTexID() <= 0 || !atlas_sampler)
    {
        std::cerr << "failed to create nearest-filtered UniRender block atlas" << std::endl;
        glDeleteProgram(program);
        selection_mesh.reset();
        release_atlas_resources();
        context.reset();
        device.reset();
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }
    context->SetTexture(0, atlas_texture);
    context->SetTextureSampler(0, atlas_sampler);
    // This host uses direct OpenGL draw calls for the voxel VAOs. UniRender's
    // Context queues texture-unit changes until Context::Draw/Compute, so make
    // the equivalent backend bind immediate for this raw-draw boundary.
    GLint active_texture = GL_TEXTURE0;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &active_texture);
    atlas_texture->ApplySampler(atlas_sampler);
    glActiveTexture(GL_TEXTURE0);
    atlas_texture->Bind();
    glActiveTexture(static_cast<GLenum>(active_texture));
    glUseProgram(program);
    glUniform1i(atlas_location, 0);
    if (glGetError() != GL_NO_ERROR)
    {
        std::cerr << "OpenGL error while creating or binding the block atlas" << std::endl;
        glDeleteProgram(program);
        selection_mesh.reset();
        release_atlas_resources();
        context.reset();
        device.reset();
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }
    InputState input;
    FpsCamera camera;
    PlayerController player;
    player.state().feet_position = smoke ? std::array<double, 3>{15.5, 2.88, 8.0}
                                         : std::array<double, 3>{8.5, 1.0, 14.5};
    if (smoke)
    {
        player.state().grounded = false;
    }
    glfwSetWindowUserPointer(window, &input);
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetWindowSizeCallback(window, window_size_callback);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    FrameClock frame_clock;
    FixedStepAccumulator fixed_tick;
    std::size_t rendered_frames = 0;
    bool draw_failed = false;
    bool smoke_pixels_verified = false;
    bool smoke_selection_verified = false;
    while (!glfwWindowShouldClose(window))
    {
        input.begin_frame();
        const bool was_mouse_captured = input.mouse_captured();
        glfwPollEvents();
        const double frame_delta_seconds = frame_clock.sample();
        camera.update_look(input);
        if (input.pressed(SpaceKey))
        {
            player.request_jump();
        }
        const PlayerMoveIntent move_intent = player_move_intent(input, camera);
        fixed_tick.advance(frame_delta_seconds, [&world, &player, move_intent](double tick_seconds)
        {
            player.fixed_step(world, tick_seconds, move_intent);
        });
        const std::array<double, 3> player_eye = player.eye_position();
        const auto selection = smoke
            ? tt::voxel_game::raycast_voxels(world, {15.5, 1.5, -2.0}, {0.0, 0.0, 1.0}, EditReach)
            : tt::voxel_game::raycast_voxels(
                world, player_eye,
                {camera.forward()[0], camera.forward()[1], camera.forward()[2]}, EditReach);
        smoke_selection_verified = smoke_selection_verified || selection.has_value();
        if (!smoke && was_mouse_captured && selection)
        {
            bool edited = false;
            if (input.mouse_button_pressed(GLFW_MOUSE_BUTTON_LEFT))
            {
                edited = world.set(selection->block, BlockState::Air);
            }
            else if (input.mouse_button_pressed(GLFW_MOUSE_BUTTON_RIGHT) && selection->has_previous_empty)
            {
                edited = tt::voxel_game::player_can_place_block(
                    player.state(), selection->previous_empty, player.config()) &&
                    world.set(selection->previous_empty, BlockState::Stone);
            }
            if (edited)
            {
                const std::vector<SectionCoord> changed = world.rebuild_dirty_meshes();
                if (!upload_rebuilt_meshes(world, changed, *device, gpu_meshes, error))
                {
                    std::cerr << "failed to upload edited voxel mesh: " << error << std::endl;
                    draw_failed = true;
                    glfwSetWindowShouldClose(window, GLFW_TRUE);
                }
            }
        }
        int width = 0;
        int height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        context->SetViewport(0, 0, width, height);
        glClearColor(0.08F, 0.12F, 0.18F, 1.0F);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        const float aspect_ratio = static_cast<float>(width) / static_cast<float>(height > 0 ? height : 1);
        const Mat4 mvp = multiply(perspective(65.0F, aspect_ratio, 0.1F, 100.0F),
                                  look_at(as_float3(player_eye), camera.forward()));
        glUseProgram(program);
        glUniform1i(atlas_location, 0);
        glUniform1i(selection_location, GL_FALSE);
        glUniformMatrix4fv(mvp_location, 1, GL_FALSE, mvp.values.data());
        for (const auto& pair : gpu_meshes)
        {
            const Mat4 model = translation(section_origin(pair.first));
            glUniformMatrix4fv(model_location, 1, GL_FALSE, model.values.data());
            pair.second.draw();
        }
        if (selection)
        {
            const Mat4 model = translation(selection->block);
            glUniformMatrix4fv(model_location, 1, GL_FALSE, model.values.data());
            GLboolean cull_was_enabled = glIsEnabled(GL_CULL_FACE);
            // GL_POLYGON_MODE returns two values: front, then back.
            std::array<GLint, 2> polygon_modes = {GL_FILL, GL_FILL};
            GLfloat line_width = 1.0F;
            glGetIntegerv(GL_POLYGON_MODE, polygon_modes.data());
            glGetFloatv(GL_LINE_WIDTH, &line_width);
            glUniform1i(selection_location, GL_TRUE);
            glDisable(GL_CULL_FACE);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glLineWidth(2.0F);
            selection_mesh.draw();
            glLineWidth(line_width);
            if (!restore_polygon_modes(polygon_modes))
            {
                std::cerr << "OpenGL returned a non-uniform polygon-mode snapshot" << std::endl;
                draw_failed = true;
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            }
            if (cull_was_enabled)
            {
                glEnable(GL_CULL_FACE);
            }
            glUniform1i(selection_location, GL_FALSE);
        }
        if (smoke && rendered_frames == 2)
        {
            smoke_pixels_verified = smoke_framebuffer_has_voxel_pixels(width, height);
            if (!smoke_pixels_verified)
            {
                std::cerr << "smoke framebuffer readback found no visible voxel pixels" << std::endl;
                draw_failed = true;
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            }
        }
        const GLenum draw_error = glGetError();
        if (draw_error != GL_NO_ERROR)
        {
            std::cerr << "OpenGL error while drawing voxel mesh: 0x" << std::hex
                      << static_cast<unsigned int>(draw_error) << std::dec << std::endl;
            draw_failed = true;
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
        glfwSwapBuffers(window);
        ++rendered_frames;
        if (smoke && rendered_frames >= 3)
        {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
    }
    glDeleteProgram(program);
    selection_mesh.reset();
    gpu_meshes.clear();
    release_atlas_resources();
    context.reset();
    device.reset();
    glfwDestroyWindow(window);
    glfwTerminate();
    return draw_failed || (smoke && (!smoke_pixels_verified || !smoke_selection_verified)) ? 1 : 0;
}

} // namespace

int main(int argc, char* argv[])
{
    const bool smoke = argc == 2 && std::string(argv[1]) == "--smoke";
    if (argc > 1 && !smoke)
    {
        std::cerr << "usage: voxel_game [--smoke]" << std::endl;
        return 2;
    }
    return run(smoke);
}
