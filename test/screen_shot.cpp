﻿#include "tantien.h"
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
        !strcmp(module, "sketchlib") == 0) {
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

    return NULL;
}

void auto_test(const char* editor_path, const char* samples_path)
{
    ves_init_vm();

    tt_on_size(1024, 1024);

    VesselConfiguration cfg;
    cfg.load_module_fn = read_module;
    cfg.expand_modules_fn = expand_modules;
    cfg.bind_foreign_class_fn = bind_foreign_class;
    cfg.bind_foreign_method_fn = bind_foreign_method;
    cfg.write_fn = write;
    ves_set_config(&cfg);

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
        std::string cls_name = editor_path;
        cls_name[0] = std::toupper(cls_name[0]);

        char code[255];
        sprintf(code, "import \"editor.%s\" for %s\nvar _editor = %s()", editor_path, cls_name.c_str(), cls_name.c_str());
        ves_interpret("editor", code);
    }

    ves_getglobal("_editor");
    ves_pushstring("load()");
    ves_call(0, 0);

    std::string dir_path;
    if (samples_path) {
        dir_path = samples_path;
    } else {
        dir_path = std::string("samples/") + editor_path;
    }
    for (auto& p : std::filesystem::recursive_directory_iterator(dir_path)) 
    {
        auto filepath = std::filesystem::absolute(p).string();
        if (filepath.find(".ves") == std::string::npos) {
            continue;
        }

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
       
        printf("++ %s\n", filepath.c_str());
        ves_pushstring(filepath.c_str());
        ves_pushstring("loadfromfile(_)");
        ves_call(1, 0);

        ves_pushstring("print_screen()");
        ves_call(0, 0);        
    }    

    ves_free_vm();
}

}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        std::cerr << "Need test file name!" << std::endl;
        glfwTerminate();
        return 1;
    }

    glfwSetErrorCallback(error_callback);

    if(glfwInit() == GL_FALSE) {
        std::cerr << "failed to init GLFW" << std::endl;
        return 1;
    }

    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    GLFWwindow *window;
    if((window = glfwCreateWindow(512, 512, "screen_shot", 0, 0)) == 0) {
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

    tt::System::Instance()->SetWindow(window);

    const char* samples_path = nullptr;
    if (argc > 2) {
        samples_path = argv[2];
    }

    if (strcmp(argv[1], "all") == 0)
    {
        for (const auto& entry : std::filesystem::directory_iterator("samples/")) {
            const auto name = entry.path().filename().string();
            if (entry.is_directory()) {
                auto_test(name.c_str(), samples_path);
            }
        }
    }
    else
    {
        auto_test(argv[1], samples_path);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}