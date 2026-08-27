#include "voxel_game/runtime/FrameClock.h"
#include "voxel_game/runtime/InputState.h"

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#include <unirender/Context.h>
#include <unirender/Device.h>
#include <unirender/opengl/Context.h>
#include <unirender/opengl/Device.h>

#include <cstdlib>
#include <iostream>
#include <memory>

namespace
{

using tt::voxel_game::ButtonAction;
using tt::voxel_game::FrameClock;
using tt::voxel_game::FixedStepAccumulator;
using tt::voxel_game::InputState;

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

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Voxel Game M0", nullptr, nullptr);
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

    InputState input;
    glfwSetWindowUserPointer(window, &input);
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetWindowSizeCallback(window, window_size_callback);
    input.set_mouse_captured(true);
    apply_cursor_mode(window, input);

    FrameClock frame_clock;
    FixedStepAccumulator fixed_tick;
    std::size_t rendered_frames = 0;
    while (!glfwWindowShouldClose(window))
    {
        input.begin_frame();
        glfwPollEvents();

        const double frame_delta_seconds = frame_clock.sample();
        fixed_tick.advance(frame_delta_seconds, [](double)
        {
            // M1 will place game simulation here. M0 owns only the schedule.
        });

        int width = 0;
        int height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        context->SetViewport(0, 0, width, height);
        glClearColor(0.08F, 0.12F, 0.18F, 1.0F);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glfwSwapBuffers(window);

        ++rendered_frames;
        if (smoke && rendered_frames >= 3)
        {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
    }

    context.reset();
    device.reset();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
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
