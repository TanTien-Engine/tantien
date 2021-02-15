#include "modules/render/wrap_Render.h"
#include "modules/render/render.ves.inc"
#include "modules/graphics/wrap_Graphics.h"
#include "modules/graphics/graphics.ves.inc"
#include "modules/maths/wrap_Maths.h"
#include "modules/maths/Maths.ves.inc"
#include "modules/geometry/wrap_Geometry.h"
#include "modules/geometry/Geometry.ves.inc"
#include "modules/gui/wrap_GUI.h"
#include "modules/gui/gui.ves.inc"
#include "modules/image/wrap_Image.h"
#include "modules/image/image.ves.inc"

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <fstream>
#include <sstream>

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
    } else if (strcmp(module, "maths") == 0) {
        source = mathsModuleSource;
    } else if (strcmp(module, "geometry") == 0) {
        source = geometryModuleSource;
    } else if (strcmp(module, "gui") == 0) {
        source = guiModuleSource;
    } else if (strcmp(module, "image") == 0) {
        source = imageModuleSource;
    } else {
        source = file_search(module, "src/script/");
        if (!source) {
            source = file_search(module, "src/modules/");
        }
        if (!source) {
            source = file_search(module, "src/");
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

    tt::MathsBindClass(className, &methods);
    if (methods.allocate != NULL) return methods;

    tt::GeometryBindClass(className, &methods);
    if (methods.allocate != NULL) return methods;

    tt::GUIBindClass(className, &methods);
    if (methods.allocate != NULL) return methods;

    tt::ImageBindClass(className, &methods);
    if (methods.allocate != NULL) return methods;

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

    method = tt::MathsBindMethod(fullName);
    if (method != NULL) return method;

    method = tt::GeometryBindMethod(fullName);
    if (method != NULL) return method;

    method = tt::GUIBindMethod(fullName);
    if (method != NULL) return method;

    method = tt::ImageBindMethod(fullName);
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

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    const bool ctrl_pressed =
        glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;
    if (key == GLFW_KEY_O && action == GLFW_PRESS && ctrl_pressed) {
        ves_pushstring("loadfromfile()");
        ves_call(0, 0);
    } else if (key == GLFW_KEY_S && action == GLFW_PRESS && ctrl_pressed) {
        ves_pushstring("savetofile()");
        ves_call(0, 0);
    }
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

void show_fps(GLFWwindow* window)
{
    static double last_time = 0;
    static int frames = 0;

    double curr_time = glfwGetTime();
    double delta = curr_time - last_time;
    frames++;
    if (delta >= 1.0)
    {
        //std::cout << 1000.0/double(frames) << std::endl;

        double fps = double(frames) / delta;

        std::stringstream ss;
        ss << " [" << fps << " FPS]";

        glfwSetWindowTitle(window, ss.str().c_str());

        frames = 0;
        last_time = curr_time;
    }
}

void limit_fps(int fps)
{
    static double last_time = 0;
    double curr_time = glfwGetTime();
    double delta = curr_time - last_time;
    if (delta < 1.0f / fps) {
        Sleep((1.0f / fps - delta) * 1000);
    }
    last_time = glfwGetTime();
}

}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        std::cerr << "Need test file name!" << std::endl;
        glfwTerminate();
        return 1;
    }

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
    glfwSetWindowSizeCallback(window, window_size_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);

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

    char code[255];
    sprintf(code, "import \"editor.%s\" for Test\nvar test = Test()", argv[1]);
    ves_interpret("test", code);

    ves_getglobal("test");
    ves_pushstring("load()");
    ves_call(0, 0);

    call_sizechanged(width, height);

    while(!glfwWindowShouldClose(window))
    {
        process_input(window);

        ves_pushstring("update()");
        ves_call(0, 0);

        ves_pushstring("draw()");
        ves_call(0, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();

        //show_fps(window);
        limit_fps(60);
    }

    ves_free_vm();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}