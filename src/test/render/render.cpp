#include "modules/render/Render.h"
#include "modules/render/render.ves.inc"

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#include <iostream>

#include <assert.h>

static void error_callback(int error, const char *msg) {
    std::cerr << "GLWT error " << error << ": " << msg << std::endl;
}

VesselLoadModuleResult read_module(const char* module)
{
    const char* source = nullptr;
    if (strcmp(module, "render") == 0) {
        source = renderModuleSource;
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
import "render" for Render

var vs = "
#version 330
layout(location = 0) in vec4 vposition;
layout(location = 1) in vec4 vcolor;
out vec4 fcolor;
void main() {
   fcolor = vcolor;
   gl_Position = vposition;
}
"
var fs = "
#version 330
in vec4 fcolor;
layout(location = 0) out vec4 FragColor;
void main() {
   FragColor = fcolor;
}
"
var prog = Render.newShader(vs, fs)

var va = Render.newVertexArray([
    //  X    Y    Z          R    G    B
       0.8, 0.8, 0.0,       1.0, 0.0, 0.0, // vertex 0
      -0.8, 0.8, 0.0,       0.0, 1.0, 0.0, // vertex 1
       0.8,-0.8, 0.0,       0.0, 0.0, 1.0, // vertex 2
       0.8,-0.8, 0.0,       0.0, 0.0, 1.0, // vertex 3
      -0.8, 0.8, 0.0,       0.0, 1.0, 0.0, // vertex 4
      -0.8,-0.8, 0.0,       1.0, 0.0, 0.0, // vertex 5
], [3, 3])

)");

//    void* closure = ves_compile("test", R"(
//import "render" for Render
//Render.clear(["color"], { "color" : [255,255,0,0] })
//Render.draw("triangles", 0, 0, { "depth_test" : false })
//)");

    while(!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

    ves_interpret("test", R"(
import "render" for Render
Render.clear(["color"], { "color" : [255,255,0,0] })
Render.draw("triangles", 0, 0, { "depth_test" : false })
)");
        //ves_run(closure);

        glfwSwapBuffers(window);
    }

    ves_free_vm();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}