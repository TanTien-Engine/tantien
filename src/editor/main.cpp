#define _NO_OCCT_

#include "tantien.h"
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
#include "modules/filesystem/wrap_Filesystem.h"
#include "modules/filesystem/filesystem.ves.inc"
#include "modules/model/wrap_Model.h"
#include "modules/model/model.ves.inc"
#include "modules/system/System.h"
#include "modules/system/wrap_System.h"
#include "modules/system/system.ves.inc"
#include "modules/shader/wrap_Shader.h"
#include "modules/shader/shader.ves.inc"
#include "modules/physics/wrap_Physics.h"
#include "modules/physics/physics.ves.inc"
#include "modules/io/wrap_Keyboard.h"
#include "modules/io/Keyboard.h"
#include "modules/io/keyboard.ves.inc"

// fixme
#include "archgen/wrap_ArchGen.h"
#include "archgen/archgen.ves.inc"
#include "citygen/wrap_CityGen.h"
#include "citygen/citygen.ves.inc"
#include "globegen/wrap_GlobeGen.h"
#include "globegen/globegen.ves.inc"
#include "pathtracer/src/wrap_PathTracer.h"
#include "pathtracer/src/pathtracer.ves.inc"
#include "sketchlib/wrap_SketchLib.h"
#include "sketchlib/sketchlib.ves.inc"
#include "nurbslib/wrap_NurbsLib.h"
#include "nurbslib/nurbslib.ves.inc"
#ifndef _NO_OCCT_
#include "partgraph_c/wrap_PartGraph.h"
#include "partgraph_c/partgraph.ves.inc"
#endif // _NO_OCCT_
#include "loggraph_c/wrap_LogGraph.h"
#include "loggraph_c/loggraph.ves.inc"
#include "codegraph_c/wrap_CodeGraph.h"
#include "codegraph_c/codegraph.ves.inc"
#include "brepdb_c/wrap_BrepDB.h"
#include "brepdb_c/brepdb.ves.inc"

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <filesystem>
#include <string>

#include <assert.h>

namespace
{

bool error = false;

std::vector<std::string> search_paths;

void GLAPIENTRY MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
    error = true;
    std::cout << "[OpenGL Error](" << type << ") " << message << std::endl;
}

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

void read_module_complete(const char* module, VesselLoadModuleResult result)
{
    if (!result.source) {
        return;
    }

    if (!strcmp(module, "render") == 0 &&
        !strcmp(module, "graphics") == 0 &&
        !strcmp(module, "maths") == 0 &&
        !strcmp(module, "geometry") == 0 &&
        !strcmp(module, "gui") == 0 &&
        !strcmp(module, "image") == 0 &&
        !strcmp(module, "filesystem") == 0 &&
        !strcmp(module, "model") == 0 &&
        !strcmp(module, "system") == 0 &&
        !strcmp(module, "shader") == 0 &&
        !strcmp(module, "physics") == 0 &&
        !strcmp(module, "keyboard") == 0 &&
        !strcmp(module, "archgen") == 0 &&
        !strcmp(module, "citygen") == 0 &&
        !strcmp(module, "globegen") == 0 &&
        !strcmp(module, "pathtracer") == 0 &&
        !strcmp(module, "sketchlib") == 0 &&
        !strcmp(module, "nurbslib") == 0 &&
#ifndef _NO_OCCT_
        !strcmp(module, "partgraph") == 0 &&
#endif // _NO_OCCT_
        !strcmp(module, "loggraph") == 0 &&
        !strcmp(module, "codegraph") == 0 &&
        !strcmp(module, "brepdb") == 0) {
        free((void*)result.source);
        result.source = NULL;
    }
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
    } else if (strcmp(module, "filesystem") == 0) {
        source = filesystemModuleSource;
    } else if (strcmp(module, "model") == 0) {
        source = modelModuleSource;
    } else if (strcmp(module, "system") == 0) {
        source = systemModuleSource;
    } else if (strcmp(module, "shader") == 0) {
        source = shaderModuleSource;
    } else if (strcmp(module, "physics") == 0) {
        source = physicsModuleSource;
    } else if (strcmp(module, "keyboard") == 0) {
        source = keyboardModuleSource;
    } else if (strcmp(module, "archgen") == 0) {
        source = archgenModuleSource;
    } else if (strcmp(module, "citygen") == 0) {
        source = citygenModuleSource;
    } else if (strcmp(module, "globegen") == 0) {
        source = globegenModuleSource;
    } else if (strcmp(module, "pathtracer") == 0) {
        source = pathtracerModuleSource;
    } else if (strcmp(module, "sketchlib") == 0) {
        source = sketchlibModuleSource;
    } else if (strcmp(module, "nurbslib") == 0) {
        source = nurbslibModuleSource;
#ifndef _NO_OCCT_
    } else if (strcmp(module, "partgraph") == 0) {
        source = partgraphModuleSource;
#endif // _NO_OCCT_
    } else if (strcmp(module, "loggraph") == 0) {
        source = loggraphModuleSource;
    } else if (strcmp(module, "codegraph") == 0) {
        source = codegraphModuleSource;
    } else if (strcmp(module, "brepdb") == 0) {
        source = brepdbModuleSource;
    } else {
        source = file_search(module, "src/script/");
        if (!source) {
            source = file_search(module, "src/modules/");
        }
        if (!source) {
            source = file_search(module, "src/");
        }
        if (!source) {
            for (auto& path : search_paths) {
                source = file_search(module, path.c_str());
                if (source) {
                    break;
                }
            }
        }
    }

    VesselLoadModuleResult result;
    result.source = source;
    result.on_complete = read_module_complete;
    return result;
}

void expand_modules_complete(VesselExpandModulesResult result)
{
    for (int i = 0; i < result.num; ++i) {
        free((char*)result.modules[i]);
    }
    free((char**)result.modules);
}

VesselExpandModulesResult expand_modules(const char* path)
{
    VesselExpandModulesResult ret;
    ret.num = 0;
    ret.modules = nullptr;

    std::string relative = path;
    auto pos = relative.find_first_of('*');
    if (pos == std::string::npos) {
        return ret;
    }
    relative = relative.substr(0, pos);
    for (auto& c : relative) {
        if (c == '.') {
            c = '/';
        }
    }
    std::string absolute;
    if (std::filesystem::is_directory("src/script/" + relative)) {
        absolute = "src/script/" + relative;
    }

    if (absolute.empty()) 
    {
        for (auto& dir : search_paths) {
            if (std::filesystem::is_directory(dir + relative)) {
                absolute = dir + relative;
                break;
            }
        }
    }

    if (absolute.empty()) {
        return ret;
    }

    std::vector<std::string> paths;
    for (auto& p : std::filesystem::recursive_directory_iterator(absolute)) 
    {
        auto filename = p.path().filename().string();
        if (filename.size() > 4 && strncmp(&filename[filename.size() - 4], ".ves", 4) == 0) {
            paths.push_back(filename.substr(0, filename.size() - 4));
        }
    }

    ret.num = (int)paths.size();
    ret.modules = (const char**)malloc(sizeof(const char*) * ret.num);
    for (int i = 0; i < ret.num; ++i) {
        ret.modules[i] = _strdup(paths[i].c_str());
    }
    ret.on_complete = expand_modules_complete;

    return ret;
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

    tt::FilesystemBindClass(className, &methods);
    if (methods.allocate != NULL) return methods;

    tt::ModelBindClass(className, &methods);
    if (methods.allocate != NULL) return methods;

    tt::SystemBindClass(className, &methods);
    if (methods.allocate != NULL) return methods;

    tt::ShaderBindClass(className, &methods);
    if (methods.allocate != NULL) return methods;

    tt::PhysicsBindClass(className, &methods);
    if (methods.allocate != NULL) return methods;

    tt::KeyboardBindClass(className, &methods);
    if (methods.allocate != NULL) return methods;

    archgen::ArchGenBindClass(className, &methods);
    if (methods.allocate != NULL) return methods;

    citygen::CityGenBindClass(className, &methods);
    if (methods.allocate != NULL) return methods;

    globegen::GlobeGenBindClass(className, &methods);
    if (methods.allocate != NULL) return methods;

    pathtracer::PathTracerBindClass(className, &methods);
    if (methods.allocate != NULL) return methods;

    sketchlib::SketchLibBindClass(className, &methods);
    if (methods.allocate != NULL) return methods;

    nurbslib::NurbsLibBindClass(className, &methods);
    if (methods.allocate != NULL) return methods;

#ifndef _NO_OCCT_
    partgraph::PartGraphBindClass(className, &methods);
    if (methods.allocate != NULL) return methods;
#endif // _NO_OCCT_

    if (strcmp(module, "loggraph") == 0) {
        loggraph::LogGraphBindClass(className, &methods);
        if (methods.allocate != NULL) return methods;
    }

    if (strcmp(module, "codegraph") == 0) {
        codegraph::CodeGraphBindClass(className, &methods);
        if (methods.allocate != NULL) return methods;
    }

    brepdbgraph::BrepDBBindClass(className, &methods);
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

    method = tt::FilesystemBindMethod(fullName);
    if (method != NULL) return method;

    method = tt::ModelBindMethod(fullName);
    if (method != NULL) return method;

    method = tt::SystemBindMethod(fullName);
    if (method != NULL) return method;

    method = tt::ShaderBindMethod(fullName);
    if (method != NULL) return method;

    method = tt::PhysicsBindMethod(fullName);
    if (method != NULL) return method;

    method = tt::KeyboardBindMethod(fullName);
    if (method != NULL) return method;

    method = archgen::ArchGenBindMethod(fullName);
    if (method != NULL) return method;

    method = citygen::CityGenBindMethod(fullName);
    if (method != NULL) return method;

    method = globegen::GlobeGenBindMethod(fullName);
    if (method != NULL) return method;

    method = pathtracer::PathTracerBindMethod(fullName);
    if (method != NULL) return method;

    method = sketchlib::SketchLibBindMethod(fullName);
    if (method != NULL) return method;

    method = nurbslib::NurbsLibBindMethod(fullName);
    if (method != NULL) return method;

#ifndef _NO_OCCT_
    method = partgraph::PartGraphBindMethod(fullName);
    if (method != NULL) return method;
#endif // _NO_OCCT_

    if (strcmp(module, "loggraph") == 0) {
        method = loggraph::LogGraphBindMethod(fullName);
        if (method != NULL) return method;
    }

    if (strcmp(module, "codegraph") == 0) {
        method = codegraph::CodeGraphBindMethod(fullName);
        if (method != NULL) return method;
    }

    method = brepdbgraph::BrepDBBindMethod(fullName);
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

void call_keypressed(const char* str)
{
    ves_pushstring(str);
    ves_pushstring("keypressed(_)");
    ves_call(1, 0);
}

void call_keyreleased(const char* str)
{
    ves_pushstring(str);
    ves_pushstring("keyreleased(_)");
    ves_call(1, 0);
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
    } else if (key == GLFW_KEY_C && action == GLFW_PRESS && ctrl_pressed) {
        ves_pushstring("copy_to_clipboard()");
        ves_call(0, 0);
    } else if (key == GLFW_KEY_V && action == GLFW_PRESS && ctrl_pressed) {
        ves_pushstring("paste_from_clipboard()");
        ves_call(0, 0);
    } else if (key == GLFW_KEY_F5 && action == GLFW_PRESS) {
        ves_pushstring("refresh()");
        ves_call(0, 0);
    } else if (key == GLFW_KEY_DELETE && action == GLFW_PRESS) {
        call_keypressed("delete");
    } else if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
        call_keypressed("space");
    } else if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        call_keypressed("escape");
    } else if (key == GLFW_KEY_PRINT_SCREEN && action == GLFW_PRESS) {
        ves_pushstring("print_screen()");
        ves_call(0, 0);
    } else if (key >= GLFW_KEY_A && key <= GLFW_KEY_Z && action == GLFW_RELEASE) {
        std::string str;
        str.push_back('a' + (key - GLFW_KEY_A));
        call_keyreleased(str.c_str());
    }
}

void mouse_press_callback(GLFWwindow* window, int button, int action, int mods)
{
    // double click
    if (action == GLFW_RELEASE) 
    {
        static auto before = std::chrono::system_clock::now();
        auto now = std::chrono::system_clock::now();
        double diff_ms = std::chrono::duration <double, std::milli>(now - before).count();
        before = now;
        if (diff_ms > 10 && diff_ms < 250) 
        {
            double x, y;
            glfwGetCursorPos(window, &x, &y);

            ves_pushnumber(x);
            ves_pushnumber(y);
            ves_pushnumber(button);
            ves_pushstring("mouseclicked(_,_,_)");
            ves_call(3, 0);
        }
    }
}

void drop_callback(GLFWwindow* window, int count, const char** paths)
{
    if (count == 0) {
        return;
    }

    double x, y;
    glfwGetCursorPos(window, &x, &y);

    if (y < 50)
    {
        ves_pushstring("clear()");
        ves_call(0, 0);

        ves_pushstring(paths[0]);
        ves_pushstring("loadfromfile(_)");
        ves_call(1, 0);
    }
    else
    {
        ves_pushstring(paths[0]);
        ves_pushnumber(x);
        ves_pushnumber(y);
        ves_pushstring("add_node_from_file(_,_,_)");
        ves_call(3, 0);
    }
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
        call_keypressed("w");
    } else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        call_keypressed("s");
    } else if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        call_keypressed("a");
    } else if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        call_keypressed("d");
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
        std::cerr << "Need editor name!" << std::endl;
        glfwTerminate();
        return 1;
    }

    int width = 1024;
    int height = 768;

    glfwSetErrorCallback(error_callback);

    if(glfwInit() == GL_FALSE) {
        std::cerr << "failed to init GLFW" << std::endl;
        return 1;
    }

    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    GLFWwindow *window;
    if((window = glfwCreateWindow(width, height, argv[1], 0, 0)) == 0) {
        std::cerr << "failed to open window" << std::endl;
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSetWindowSizeCallback(window, window_size_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_press_callback);
    glfwSetDropCallback(window, drop_callback);

    if(gl3wInit()) {
        std::cerr << "failed to init GL3W" << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    // Enable debug output
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(MessageCallback, 0);

    tt::Keyboard::Funs keyboard_cb;
    keyboard_cb.is_ctrl_pressed = [&]()->bool {
        return
            glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;
    };
    keyboard_cb.is_shift_pressed = [&]()->bool {
        return
            glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
    };
    tt::Keyboard::RegisterCallback(keyboard_cb);

    ves_init_vm();

    tt::System::Instance()->SetWindow(window);
    tt_enable_dtex();

    VesselConfiguration cfg;
    cfg.load_module_fn = read_module;
    cfg.expand_modules_fn = expand_modules;
    cfg.bind_foreign_class_fn = bind_foreign_class;
    cfg.bind_foreign_method_fn = bind_foreign_method;
    cfg.write_fn = write;
    ves_set_config(&cfg);

    std::string editor_path = argv[1];
    if (std::filesystem::is_regular_file(editor_path))
    {
        auto filepath = std::filesystem::path(editor_path);

        auto dir = filepath.parent_path();
        search_paths.push_back(dir.string() + "/");

        auto file = filepath.stem().string();
        auto cls_name = file;
        cls_name[0] = std::toupper(cls_name[0]);

        char code[255];
        sprintf(code, "import \"%s\" for %s\nvar _editor = %s()", file.c_str(), cls_name.c_str(), cls_name.c_str());

        ves_interpret("editor", code);
    }
    else
    {
        auto cls_name = editor_path;
        cls_name[0] = std::toupper(cls_name[0]);

        char code[255];
        sprintf(code, "import \"editor.%s\" for %s\nvar _editor = %s()", argv[1], cls_name.c_str(), cls_name.c_str());
        ves_interpret("editor", code);
    }

    ves_getglobal("_editor");
    ves_pushstring("load()");
    ves_call(0, 0);

    call_sizechanged(width, height);

    if (argc > 2) {
        ves_pushstring(argv[2]);
        ves_pushstring("loadfromfile(_)");
        ves_call(1, 0);
    }

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