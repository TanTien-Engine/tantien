#include "ves_host.h"

// Windows _strdup shim for POSIX (matches the one main.cpp uses).
#ifndef _WIN32
#define _strdup strdup
#endif

// --- module foreign binders + compiled-in .ves sources -----------------------
// Each pair below provides a module's BindClass/BindMethod (the wrap_*.h) and
// its compiled-in <name>ModuleSource byte array (the *.ves.inc). Keep this list
// in sync with MODULE_REGISTRY. Runtime-only headers (System.h, Keyboard.h,
// BrepGraph.h, BrepDBInit.h, Render.h) stay in main.cpp — the host only needs
// the binders and sources.
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
#include "modules/system/wrap_System.h"
#include "modules/system/system.ves.inc"
#include "modules/shader/wrap_Shader.h"
#include "modules/shader/shader.ves.inc"
#include "modules/io/wrap_Keyboard.h"
#include "modules/io/keyboard.ves.inc"
#include "modules/scene/wrap_Scene.h"
#include "modules/scene/scene.ves.inc"
#include "modules/vm/wrap_VM.h"
#include "modules/vm/VM.ves.inc"
#include "modules/db/wrap_DB.h"
#include "modules/db/DB.ves.inc"
#include "modules/om/wrap_OM.h"
#include "modules/om/OM.ves.inc"
#include "modules/regen/wrap_Regen.h"
#include "modules/regen/regen.ves.inc"
#include "modules/graph/wrap_Graph.h"
#include "modules/graph/graph.ves.inc"

#include "voxel_game/ves/wrap_GameClock.h"
#include "voxel_game/ves/gameclock.ves.inc"

#include "cax/brepkit_c/wrap_BrepKit.h"
#include "cax/brepkit_c/brepkit.ves.inc"
#include "cax/brepgraph_c/script/wrap_BrepGraph.h"
#include "cax/brepgraph_c/script/brepgraph.ves.inc"
#include "cax/brepdb_c/wrap_BrepDB.h"
#include "cax/brepdb_c/brepdb.ves.inc"
#include "cax/deepbrep_c/wrap_DeepBrep.h"
#include "cax/deepbrep_c/deepbrep.ves.inc"
#include "cax/cadcvt_c/cadcvt.ves.inc"
#include "cax/cadcvt_c/wrap_CadCvt.h"
#include "cax/sketchlib/wrap_SketchLib.h"
#include "cax/sketchlib/sketchlib.ves.inc"

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <filesystem>

namespace tt_ves_host {

std::vector<std::string> search_paths;

namespace {

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

// ---- module registry --------------------------------------------------------
// Single source of truth for every statically linked script module: its
// compiled-in .ves source and its foreign class/method binders. read_module,
// read_module_complete, bind_foreign_class and bind_foreign_method used to keep
// four hand-written copies of this list, and they drifted (stale entries in the
// don't-free list leaked every file-loaded source). Register new modules HERE
// and nowhere else. Foreign binders are selected by their exact module name,
// matching Vessel's callback contract and allowing different modules to use
// the same class name without an order-dependent collision.
typedef void (*BindClassFn)(const char*, VesselForeignClassMethods*);
typedef VesselForeignMethodFn (*BindMethodFn)(const char*);

struct ModuleEntry
{
    const char*  name;
    const char*  source;      // compiled-in module source; never freed
    BindClassFn  bind_class;
    BindMethodFn bind_method;
};

const ModuleEntry MODULE_REGISTRY[] = {
    { "render",     renderModuleSource,     tt::RenderBindClass,           tt::RenderBindMethod },
    { "graphics",   graphicsModuleSource,   tt::GraphicsBindClass,         tt::GraphicsBindMethod },
    { "maths",      mathsModuleSource,      tt::MathsBindClass,            tt::MathsBindMethod },
    { "geometry",   geometryModuleSource,   tt::GeometryBindClass,         tt::GeometryBindMethod },
    { "gui",        guiModuleSource,        tt::GUIBindClass,              tt::GUIBindMethod },
    { "image",      imageModuleSource,      tt::ImageBindClass,            tt::ImageBindMethod },
    { "filesystem", filesystemModuleSource, tt::FilesystemBindClass,       tt::FilesystemBindMethod },
    { "model",      modelModuleSource,      tt::ModelBindClass,            tt::ModelBindMethod },
    { "system",     systemModuleSource,     tt::SystemBindClass,           tt::SystemBindMethod },
    { "shader",     shaderModuleSource,     tt::ShaderBindClass,           tt::ShaderBindMethod },
    //{ "physics",  physicsModuleSource,    tt::PhysicsBindClass,          tt::PhysicsBindMethod },
    { "keyboard",   keyboardModuleSource,   tt::KeyboardBindClass,         tt::KeyboardBindMethod },
    { "scene",      sceneModuleSource,      tt::SceneBindClass,            tt::SceneBindMethod },
    { "vm",         vmModuleSource,         tt::VmBindClass,               tt::VmBindMethod },
    { "db",         dbModuleSource,         tt::DbBindClass,               tt::DbBindMethod },
    { "om",         omModuleSource,         tt::OmBindClass,               tt::OmBindMethod },
    { "regen",      regenModuleSource,      tt::RegenBindClass,            tt::RegenBindMethod },
    { "graph",      graphModuleSource,      tt::GraphBindClass,            tt::GraphBindMethod },
    { "gameclock",  gameclockModuleSource,  tt::voxel_game::GameClockBindClass,
                                               tt::voxel_game::GameClockBindMethod },
    { "brepkit",    brepkitModuleSource,    brepkit::BrepKitBindClass,     brepkit::BrepKitBindMethod },
    { "brepgraph",  brepgraphModuleSource,  brepgraph::BrepGraphBindClass, brepgraph::BrepGraphBindMethod },
    { "brepdb",     brepdbModuleSource,     brepdb::BrepDBBindClass,       brepdb::BrepDBBindMethod },
    { "deepbrep",   deepbrepModuleSource,   deepbrep::DeepBrepBindClass,   deepbrep::DeepBrepBindMethod },
    { "cadcvt",     cadcvtModuleSource,     cadcvt::CadCvtBindClass,       cadcvt::CadCvtBindMethod },
    { "sketchlib",  sketchlibModuleSource,  sketchlib::SketchLibBindClass, sketchlib::SketchLibBindMethod },
};

const ModuleEntry* find_module(const char* module)
{
    for (auto& m : MODULE_REGISTRY) {
        if (strcmp(module, m.name) == 0) {
            return &m;
        }
    }
    return nullptr;
}

void read_module_complete(const char* module, VesselLoadModuleResult result)
{
    if (!result.source) {
        return;
    }

    // Registry sources are compiled-in arrays; everything else came from
    // file_search's strdup and must be freed.
    if (!find_module(module)) {
        free((void*)result.source);
        result.source = NULL;
    }
}

void expand_modules_complete(VesselExpandModulesResult result)
{
    for (int i = 0; i < result.num; ++i) {
        free((char*)result.modules[i]);
    }
    free((char**)result.modules);
}

} // anonymous namespace

VesselLoadModuleResult read_module(const char* module)
{
    const char* source = nullptr;
    if (auto m = find_module(module)) {
        source = m->source;
    }
    else {
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

    VesselLoadModuleResult result{};
    result.source = source;
    result.on_complete = read_module_complete;
    return result;
}

VesselExpandModulesResult expand_modules(const char* path)
{
    VesselExpandModulesResult ret{};
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

    const ModuleEntry* entry = module != nullptr ? find_module(module) : nullptr;
    if (entry != nullptr && entry->bind_class != nullptr) {
        entry->bind_class(className, &methods);
    }
    return methods;
}

VesselForeignMethodFn bind_foreign_method(const char* module, const char* className, bool isStatic, const char* signature)
{
    // For convenience, concatenate all of the method qualifiers into a single
    // signature string.
    char fullName[256];
    snprintf(fullName, sizeof(fullName), "%s%s.%s", isStatic ? "static " : "", className, signature);

    const ModuleEntry* entry = module != nullptr ? find_module(module) : nullptr;
    return entry != nullptr && entry->bind_method != nullptr
        ? entry->bind_method(fullName)
        : NULL;
}

void write(const char* text)
{
    printf("%s", text);
}

void configure(VesselConfiguration* cfg)
{
    *cfg = VesselConfiguration{};

    cfg->load_module_fn = read_module;
    cfg->expand_modules_fn = expand_modules;
    cfg->bind_foreign_class_fn = bind_foreign_class;
    cfg->bind_foreign_method_fn = bind_foreign_method;
    cfg->write_fn = write;
}

} // namespace tt_ves_host
