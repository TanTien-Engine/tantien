#include "modules/model/wrap_Model.h"
#include "modules/script/Proxy.h"
#include "modules/render/Render.h"
#include "modules/filesystem/Filesystem.h"

#include <model/ParametricEquations.h>
#include <model/Model.h>

#include <memory>

#include <string.h>

namespace
{

void w_Model_allocate()
{
    auto dev = tt::Render::Instance()->Device();
    auto model = std::make_shared<model::Model>(dev.get());

    const char* filepath = ves_tostring(1);
    if (tt::Filesystem::IsExists(filepath)) {
        model->LoadFromFile(filepath);
    } else {
        std::string path = tt::Filesystem::Instance()->GetAssetBaseDir() + "/" + filepath;
        model->LoadFromFile(path);
    }    

    auto proxy = (tt::Proxy<model::Model>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<model::Model>));
    proxy->obj = model;
}

int w_Model_finalize(void* data)
{
    auto proxy = (tt::Proxy<model::Model>*)(data);
    proxy->~Proxy();
    return sizeof(tt::Proxy<model::Model>);
}

}

namespace tt
{

VesselForeignMethodFn ModelBindMethod(const char* signature)
{
    return NULL;
}

void ModelBindClass(const char* class_name, VesselForeignClassMethods* methods)
{
    if (strcmp(class_name, "Model") == 0)
    {
        methods->allocate = w_Model_allocate;
        methods->finalize = w_Model_finalize;
        return;
    }
}

}