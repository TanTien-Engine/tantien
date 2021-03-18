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

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>

#include <assert.h>

namespace
{

bool error = false;

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

    tt::FilesystemBindClass(className, &methods);
    if (methods.allocate != NULL) return methods;

    tt::ModelBindClass(className, &methods);
    if (methods.allocate != NULL) return methods;

    tt::SystemBindClass(className, &methods);
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

    return NULL;
}

void save_dir(const char* dir_path)
{
    for (auto& p : std::filesystem::recursive_directory_iterator(dir_path)) 
    {
        auto filepath = std::filesystem::absolute(p).string();
        if (filepath.find(".ves") == std::string::npos) {
            continue;
        }

        ves_pushstring(filepath.c_str());
        ves_pushstring("loadfromfile(_)");
        ves_call(1, 0);

        ves_pushstring("savetofile()");
        ves_call(0, 0);
    }
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

    if(gl3wInit()) {
        std::cerr << "failed to init GL3W" << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    // Enable debug output
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(MessageCallback, 0);

    ves_init_vm();

    tt::System::Instance()->SetWindow(window);

    VesselConfiguration cfg;
    cfg.load_module_fn = read_module;
    cfg.bind_foreign_class_fn = bind_foreign_class;
    cfg.bind_foreign_method_fn = bind_foreign_method;
    cfg.write_fn = write;
    ves_set_config(&cfg);

    char code[255];
    std::string cls = argv[1];
    cls[0] = std::toupper(cls[0]);
    sprintf(code, "import \"editor.%s\" for %s\nvar _editor = %s()", argv[1], cls.c_str(), cls.c_str());
    ves_interpret("editor", code);

    ves_getglobal("_editor");
    ves_pushstring("load()");
    ves_call(0, 0);

    save_dir("samples");
    save_dir("assets");

    ves_free_vm();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}