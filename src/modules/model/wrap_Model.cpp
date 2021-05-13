#include "modules/model/wrap_Model.h"
#include "modules/script/Proxy.h"
#include "modules/render/Render.h"
#include "modules/filesystem/Filesystem.h"

#include <model/ParametricEquations.h>
#include <model/Model.h>

#include <memory>
#include <array>

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
        if (tt::Filesystem::IsExists(path.c_str())) {
            model->LoadFromFile(path);
        }
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

void w_Model_get_pbr_textures()
{
    auto model = ((tt::Proxy<model::Model>*)ves_toforeign(0))->obj;

    ves_newlist(5);

    if (model->materials.empty()) 
    {
        for (int i = 0; i < 5; ++i)
        {
            ves_pushnil();
            ves_seti(-2, i);
            ves_pop(1);
        }

        ves_insert(0);
        return;
    }

    auto& material = *model->materials.front();
    std::array<int, 5> textures_idx = {
        material.diffuse_tex,
        material.metallic_roughness_tex,
        material.emissive_tex,
        material.occlusion_tex,
        material.normal_tex,
    };
    for (int i = 0; i < 5; ++i)
    {
        ves_pushnil();
        if (textures_idx[i] >= 0) {
            ur::TexturePtr* tex = (std::shared_ptr<ur::Texture>*)ves_set_newforeign(3, 1, sizeof(std::shared_ptr<ur::Texture>));
            *tex = model->textures[textures_idx[i]].second;
        }
        ves_seti(-2, i);
        ves_pop(1);
    }

    ves_insert(0);
}

}

namespace tt
{

VesselForeignMethodFn ModelBindMethod(const char* signature)
{
    if (strcmp(signature, "Model.get_pbr_textures(_)") == 0) return w_Model_get_pbr_textures;

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