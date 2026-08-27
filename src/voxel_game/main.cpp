#include "voxel_game/render/VoxelGpuMesh.h"
#include "voxel_game/runtime/FpsCamera.h"
#include "voxel_game/runtime/FrameClock.h"
#include "voxel_game/runtime/InputState.h"
#include "voxel_game/voxel/Section.h"
#include "voxel_game/voxel/VisibleFaceMesher.h"

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#include <unirender/Context.h>
#include <unirender/Device.h>
#include <unirender/opengl/Context.h>
#include <unirender/opengl/Device.h>

#include <array>
#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>

namespace
{

using tt::voxel_game::BlockState;
using tt::voxel_game::ButtonAction;
using tt::voxel_game::FpsCamera;
using tt::voxel_game::FrameClock;
using tt::voxel_game::FixedStepAccumulator;
using tt::voxel_game::InputState;
using tt::voxel_game::OpaqueMesh;
using tt::voxel_game::Section;
using tt::voxel_game::VoxelGpuMesh;

constexpr float DegreesToRadians = 0.01745329251994329577F;

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
    glfwSetInputMode(window,
                     GLFW_CURSOR,
                     input.mouse_captured() ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    if (input.mouse_captured() && glfwRawMouseMotionSupported())
    {
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
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

Section make_demo_section()
{
    Section section;
    for (int z = 1; z < 15; ++z)
    {
        for (int x = 1; x < 15; ++x)
        {
            section.set({x, 0, z}, BlockState::Grass);
        }
    }
    for (int z = 4; z < 12; ++z)
    {
        for (int x = 3; x < 11; ++x)
        {
            section.set({x, 1, z}, BlockState::Stone);
        }
    }
    for (int z = 6; z < 10; ++z)
    {
        for (int x = 5; x < 9; ++x)
        {
            section.set({x, 2, z}, BlockState::Clay);
        }
    }
    section.set({7, 1, 7}, BlockState::Air);
    section.set({7, 2, 7}, BlockState::Air);
    return section;
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
        uniform mat4 u_mvp;
        out vec3 v_normal;
        out vec4 v_color;
        void main()
        {
            gl_Position = u_mvp * vec4(a_position, 1.0);
            v_normal = a_normal;
            v_color = a_color;
        }
    )";
    constexpr const char* FragmentSource = R"(
        #version 330 core
        in vec3 v_normal;
        in vec4 v_color;
        out vec4 out_color;
        void main()
        {
            vec3 light_direction = normalize(vec3(0.45, 0.80, 0.35));
            float brightness = 0.30 + 0.70 * max(dot(normalize(v_normal), light_direction), 0.0);
            out_color = vec4(v_color.rgb * brightness, v_color.a);
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

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Voxel Game M1", nullptr, nullptr);
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

    const auto mesh_start = std::chrono::steady_clock::now();
    const OpaqueMesh cpu_mesh = tt::voxel_game::mesh_opaque_visible_faces(make_demo_section());
    const double mesh_milliseconds = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - mesh_start).count();
    std::cout << "voxel mesh: faces=" << cpu_mesh.face_count
              << " vertices=" << cpu_mesh.vertices.size()
              << " indices=" << cpu_mesh.indices.size()
              << " cpu_mesh_ms=" << mesh_milliseconds << std::endl;

    while (glGetError() != GL_NO_ERROR)
    {
    }
    VoxelGpuMesh gpu_mesh;
    std::string error;
    if (!gpu_mesh.upload(*device, cpu_mesh, error))
    {
        std::cerr << "failed to upload voxel mesh: " << error << std::endl;
        gpu_mesh.reset();
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
        gpu_mesh.reset();
        context.reset();
        device.reset();
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }
    const GLint mvp_location = glGetUniformLocation(program, "u_mvp");
    if (mvp_location < 0)
    {
        std::cerr << "voxel shader has no u_mvp uniform" << std::endl;
        glDeleteProgram(program);
        gpu_mesh.reset();
        context.reset();
        device.reset();
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    InputState input;
    FpsCamera camera;
    glfwSetWindowUserPointer(window, &input);
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetWindowSizeCallback(window, window_size_callback);
    input.set_mouse_captured(true);
    apply_cursor_mode(window, input);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    FrameClock frame_clock;
    FixedStepAccumulator fixed_tick;
    std::size_t rendered_frames = 0;
    bool draw_failed = false;
    while (!glfwWindowShouldClose(window))
    {
        input.begin_frame();
        glfwPollEvents();

        const double frame_delta_seconds = frame_clock.sample();
        fixed_tick.advance(frame_delta_seconds, [](double)
        {
            // M1 keeps M0's simulation schedule; gameplay arrives in M2.
        });
        camera.update(input, frame_delta_seconds);

        int width = 0;
        int height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        context->SetViewport(0, 0, width, height);
        glClearColor(0.08F, 0.12F, 0.18F, 1.0F);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const float aspect_ratio = static_cast<float>(width) /
                                   static_cast<float>(height > 0 ? height : 1);
        const Mat4 mvp = multiply(perspective(65.0F, aspect_ratio, 0.1F, 100.0F),
                                  look_at(camera.position(), camera.forward()));
        glUseProgram(program);
        glUniformMatrix4fv(mvp_location, 1, GL_FALSE, mvp.values.data());
        gpu_mesh.draw();
        if (glGetError() != GL_NO_ERROR)
        {
            std::cerr << "OpenGL error while drawing voxel mesh" << std::endl;
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
    gpu_mesh.reset();
    context.reset();
    device.reset();
    glfwDestroyWindow(window);
    glfwTerminate();
    return draw_failed ? 1 : 0;
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
