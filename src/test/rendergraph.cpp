#include "../modules/render/Render.h"
#include "../modules/render/render.ves.inc"

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <fstream>

#include <assert.h>

static void error_callback(int error, const char *msg) {
    std::cerr << "GLWT error " << error << ": " << msg << std::endl;
}

static void read_module_complete(const char* name, struct VesselLoadModuleResult result)
{
    if (result.source) {
        free((void*)result.source);
        result.source = NULL;
    }
}

VesselLoadModuleResult read_module(const char* module)
{
    const char* source = nullptr;
    if (strcmp(module, "render") == 0)
    {
        source = _strdup(renderModuleSource);
    }
    else
    {
        std::string path = module;
        for (auto& c : path) {
            if (c == '.') {
                c = '/';
            }
        }
        path.insert(0, "src/modules/");
        path.append(".ves");
        std::ifstream fin(path.c_str());
        if (!fin.fail()) {
            std::string str((std::istreambuf_iterator<char>(fin)),
                std::istreambuf_iterator<char>());
            source = _strdup(str.c_str());
        }
        fin.close();
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

    vessel_init_vm();

    VesselConfiguration cfg;
    cfg.load_module_fn = read_module;
    cfg.bind_foreign_class_fn = bind_foreign_class;
    cfg.bind_foreign_method_fn = bind_foreign_method;
    vessel_set_config(&cfg);

    vessel_interpret("test", R"(
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

import "rendergraph.shader" for Shader
var shader = Shader()
shader.vs = vs
shader.fs = fs
shader.execute()

import "rendergraph.vertex_array" for VertexArray
var va = VertexArray()
va.data = [
    //  X    Y    Z          R    G    B
       0.8, 0.8, 0.0,       1.0, 0.0, 0.0, // vertex 0
      -0.8, 0.8, 0.0,       0.0, 1.0, 0.0, // vertex 1
       0.8,-0.8, 0.0,       0.0, 0.0, 1.0, // vertex 2
       0.8,-0.8, 0.0,       0.0, 0.0, 1.0, // vertex 3
      -0.8, 0.8, 0.0,       0.0, 1.0, 0.0, // vertex 4
      -0.8,-0.8, 0.0,       1.0, 0.0, 0.0, // vertex 5
]
va.attrs = [3, 3]
va.execute()

)");

    while(!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

    vessel_interpret("test", R"(
var nodes = []

import "rendergraph.clear" for Clear
var clear = Clear()
clear.masks = ["color"]
clear.values = { "color" : [255,255,0,0] }

import "rendergraph.draw" for Draw
var draw = Draw()
draw.render_state = { "depth_test" : false }

import "blueprint.Blueprint" for Blueprint
Blueprint.connect(clear.exports[0], draw.imports[0])

nodes.add(draw)
nodes.add(clear)

var sorted = Blueprint.topoSort(nodes)
for (var node in sorted) {
	node.execute()
}

)");

        glfwSwapBuffers(window);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}