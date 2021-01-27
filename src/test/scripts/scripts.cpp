#include "modules/render/Render.h"
#include "modules/render/render.ves.inc"
#include "modules/graphics/Graphics.h"
#include "modules/graphics/graphics.ves.inc"

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <fstream>

#include <assert.h>

static void error_callback(int error, const char *msg) {
    std::cerr << "GLWT error " << error << ": " << msg << std::endl;
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

    assert(0);

    return NULL;
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
    ves_set_config(&cfg);

    ves_interpret("test", R"(
//import "test_render" for Test
import "test_rendergraph" for Test
//import "test_graphics" for Test
var test = Test()
)");

    ves_getglobal("test");
    ves_pushstring("load()");
    ves_call(0, 0);

    while(!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        ves_pushstring("draw()");
        ves_call(0, 0);

        glfwSwapBuffers(window);
    }

    ves_free_vm();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}