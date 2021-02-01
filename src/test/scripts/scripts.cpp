﻿#include "modules/render/Render.h"
#include "modules/render/render.ves.inc"
#include "modules/graphics/Graphics.h"
#include "modules/graphics/graphics.ves.inc"

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <fstream>

#include <assert.h>

namespace
{

void error_callback(int error, const char *msg) {
    std::cerr << "GLWT error " << error << ": " << msg << std::endl;
}

void write(const char* text)
{
    printf("%s", text);
}

const char* file_search(const char* module, const char* dir)
{
    const char* ret = nullptr;

    std::string path = module;
    for (auto& c : path) {
        if (c == '.') {
            c = '/';
        }
    }
    path.insert(0, dir);
    path.append(".ves");
    std::ifstream fin(path.c_str());
    if (!fin.fail()) {
        std::string str((std::istreambuf_iterator<char>(fin)),
            std::istreambuf_iterator<char>());
        ret = _strdup(str.c_str());
    }
    fin.close();

    return ret;
}

VesselLoadModuleResult read_module(const char* module)
{
    const char* source = nullptr;
    if (strcmp(module, "render") == 0) {
        source = renderModuleSource;
    } else if (strcmp(module, "graphics") == 0) {
        source = graphicsModuleSource;
    } else {
        source = file_search(module, "src/script/");
        if (!source) {
            source = file_search(module, "src/test/scripts/");
        }
    }

    VesselLoadModuleResult result;
    result.source = source;
    result.on_complete = NULL;
    return result;

}

VesselForeignClassMethods bind_foreign_class(const char* module, const char* className)
{
    VesselForeignClassMethods methods = { NULL, NULL };

    tt::RenderBindClass(className, &methods);
    if (methods.allocate != NULL) return methods;

    tt::GraphicsBindClass(className, &methods);
    if (methods.allocate != NULL) return methods;

    //assert(0);
    return methods;
}

VesselForeignMethodFn bind_foreign_method(const char* module, const char* className, bool isStatic, const char* signature)
{
    // For convenience, concatenate all of the method qualifiers into a single
    // signature string.
    char fullName[256];
    fullName[0] = '\0';
    if (isStatic) strcat(fullName, "static ");
    strcat(fullName, className);
    strcat(fullName, ".");
    strcat(fullName, signature);

    VesselForeignMethodFn method = NULL;

    method = tt::RenderBindMethod(fullName);
    if (method != NULL) return method;

    method = tt::GraphicsBindMethod(fullName);
    if (method != NULL) return method;

    return NULL;
}

void call_sizechanged(int w, int h)
{
    ves_pushnumber(w);
    ves_pushnumber(h);
    ves_pushstring("sizechanged(_,_)");
    ves_call(2, 0);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void window_size_callback(GLFWwindow* window, int width, int height)
{
    call_sizechanged(width, height);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    double x, y;
    glfwGetCursorPos(window, &x, &y);

    ves_pushnumber(x);
    ves_pushnumber(y);
    ves_pushnumber(xoffset);
    ves_pushnumber(yoffset);
    ves_pushstring("mousescrolled(_,_,_,_)");
    ves_call(4, 0);
}

void call_keypressed(char c)
{
    ves_pushlstring(&c, 1);
    ves_pushstring("keypressed(_)");
    ves_call(1, 0);
}

void call_mousemoved(double x, double y, int button)
{
    ves_pushnumber(x);
    ves_pushnumber(y);
    ves_pushnumber(button);
    ves_pushstring("mousemoved(_,_,_)");
    ves_call(3, 0);
}

void call_mousedragged(double x, double y, int button)
{
    ves_pushnumber(x);
    ves_pushnumber(y);
    ves_pushnumber(button);
    ves_pushstring("mousedragged(_,_,_)");
    ves_call(3, 0);
}

void call_mousepressed(double x, double y, int button)
{
    ves_pushnumber(x);
    ves_pushnumber(y);
    ves_pushnumber(button);
    ves_pushstring("mousepressed(_,_,_)");
    ves_call(3, 0);
}

void call_mousereleased(double x, double y, int button)
{
    ves_pushnumber(x);
    ves_pushnumber(y);
    ves_pushnumber(button);
    ves_pushstring("mousereleased(_,_,_)");
    ves_call(3, 0);
}

enum class MouseButton
{
    None   = 0,
    Left   = 1,
    Right  = 2,
    Middle = 3,
};

enum class MouseStatus
{
    Default,
    LeftPressed,
    RightPressed,
    MiddlePressed,
};

MouseStatus mouse_status = MouseStatus::Default;

void process_input(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
    } else if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        call_keypressed('w');
    } else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        call_keypressed('s');
    } else if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        call_keypressed('a');
    } else if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        call_keypressed('d');
    }

    double x, y;
    glfwGetCursorPos(window, &x, &y);
    switch (mouse_status)
    {
    case MouseStatus::Default:
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            mouse_status = MouseStatus::LeftPressed;
            call_mousepressed(x, y, static_cast<int>(MouseButton::Left));
        } else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
            mouse_status = MouseStatus::RightPressed;
            call_mousepressed(x, y, static_cast<int>(MouseButton::Right));
        } else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS) {
            mouse_status = MouseStatus::MiddlePressed;
            call_mousepressed(x, y, static_cast<int>(MouseButton::Middle));
        } else {
            call_mousemoved(x, y, static_cast<int>(MouseButton::None));
        }
        break;
    case MouseStatus::LeftPressed:
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE) {
            mouse_status = MouseStatus::Default;
            call_mousereleased(x, y, static_cast<int>(MouseButton::Left));
        } else {
            call_mousedragged(x, y, static_cast<int>(MouseButton::Left));
        }
        break;
    case MouseStatus::RightPressed:
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_RELEASE) {
            mouse_status = MouseStatus::Default;
            call_mousereleased(x, y, static_cast<int>(MouseButton::Right));
        } else {
            call_mousedragged(x, y, static_cast<int>(MouseButton::Right));
        }
        break;
    case MouseStatus::MiddlePressed:
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_RELEASE) {
            mouse_status = MouseStatus::Default;
            call_mousereleased(x, y, static_cast<int>(MouseButton::Middle));
        } else {
            call_mousedragged(x, y, static_cast<int>(MouseButton::Middle));
        }
        break;
    default:
        assert(0);
    }
}

}

int main()
{
    int width = 640;
    int height = 480;

    glfwSetErrorCallback(error_callback);

    if(glfwInit() == GL_FALSE) {
        std::cerr << "failed to init GLFW" << std::endl;
        return 1;
    }

    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    GLFWwindow *window;
    if((window = glfwCreateWindow(width, height, "glfw3", 0, 0)) == 0) {
        std::cerr << "failed to open window" << std::endl;
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetWindowSizeCallback(window, window_size_callback);
    glfwSetScrollCallback(window, scroll_callback);

    if(gl3wInit()) {
        std::cerr << "failed to init GL3W" << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    ves_init_vm();

    VesselConfiguration cfg;
    cfg.load_module_fn = read_module;
    cfg.bind_foreign_class_fn = bind_foreign_class;
    cfg.bind_foreign_method_fn = bind_foreign_method;
    cfg.write_fn = write;
    ves_set_config(&cfg);

    ves_interpret("test", R"(
//import "test_render" for Test
//import "test_rendergraph" for Test
//import "test_graphics" for Test
import "test_blueprint" for Test
var test = Test()
)");

    ves_getglobal("test");
    ves_pushstring("load()");
    ves_call(0, 0);

    call_sizechanged(width, height);

    while(!glfwWindowShouldClose(window))
    {
        process_input(window);

        ves_pushstring("draw()");
        ves_call(0, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    ves_free_vm();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}