#include "modules/model/wrap_Model.h"
#include "modules/script/Proxy.h"
#include "modules/render/Render.h"
#include "modules/filesystem/Filesystem.h"

#include <model/ParametricEquations.h>
#include <model/Model.h>
#include <model/gltf/Model.h>
#include <model/GltfLoader.h>
#include <unirender/VertexArray.h>

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
        if (textures_idx[i] >= 0) 
        {
            ves_import_class("render", "Texture2D");
            ur::TexturePtr* tex = (std::shared_ptr<ur::Texture>*)ves_set_newforeign(2, 3, sizeof(std::shared_ptr<ur::Texture>));
            ves_pop(1);
            *tex = model->textures[textures_idx[i]].second;
        }
        ves_seti(-2, i);
        ves_pop(1);
    }

    ves_insert(0);
}

void w_glTF_allocate()
{
    auto dev = tt::Render::Instance()->Device();
    auto model = std::make_shared<model::gltf::Model>();

    const char* filepath = ves_tostring(1);
    if (tt::Filesystem::IsExists(filepath)) {
        model::GltfLoader::Load(*dev, *model, filepath);
    } else {
        std::string path = tt::Filesystem::Instance()->GetAssetBaseDir() + "/" + filepath;
        if (tt::Filesystem::IsExists(path.c_str())) {
            model::GltfLoader::Load(*dev, *model, path);
        }
    }

    auto proxy = (tt::Proxy<model::gltf::Model>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<model::gltf::Model>));
    proxy->obj = model;
}

int w_glTF_finalize(void* data)
{
    auto proxy = (tt::Proxy<model::gltf::Model>*)(data);
    proxy->~Proxy();
    return sizeof(tt::Proxy<model::gltf::Model>);
}

void w_glTF_get_desc()
{
    auto model = ((tt::Proxy<model::gltf::Model>*)ves_toforeign(0))->obj;
    if (!model->scene) {
        ves_set_nil(0);
        return;
    }

    ves_pop(1);
    ves_newmap();
    for (auto& node : model->scene->nodes)
    {
        ves_newmap();

        assert(node->mesh->primitives.size() == 1);
        auto& prim = node->mesh->primitives.front();

        auto material = prim->material;

        // va
        {
            ves_pushnil();
            ves_import_class("render", "VertexArray");
            std::shared_ptr<ur::VertexArray>* va = (std::shared_ptr<ur::VertexArray>*)ves_set_newforeign(2, 3, sizeof(std::shared_ptr<ur::VertexArray>));
            ves_pop(1);
            *va = prim->va;
            ves_setfield(-2, "va");
            ves_pop(1);
        }
        // emissive
        if (material->emissive)
        {
            auto& emissive = material->emissive;
            ves_newmap();
            // factor
            {
                ves_pushnil();
                ves_import_class("maths", "Float3");
                sm::vec3* factor = (sm::vec3*)ves_set_newforeign(3, 4, sizeof(sm::vec3));
                memcpy(factor->xyz, emissive->factor.xyz, sizeof(float) * 3);
                ves_pop(1);
                ves_setfield(-2, "factor");
                ves_pop(1);
            }
            // texture
            {
                ves_pushnil();
                ves_import_class("render", "Texture2D");
                ur::TexturePtr* tex = (std::shared_ptr<ur::Texture>*)ves_set_newforeign(3, 4, sizeof(std::shared_ptr<ur::Texture>));
                ves_pop(1);
                *tex = emissive->texture->image;
                ves_setfield(-2, "texture");
                ves_pop(1);
            }
            // tex_coord
            {
                ves_pushnumber(emissive->tex_coord);
                ves_setfield(-2, "tex_coord");
                ves_pop(1);
            }
            ves_setfield(-2, "emissive");
            ves_pop(1);
        }
        // normal
        if (material->normal)
        {
            auto& normal = material->normal;
            ves_newmap();
            // texture
            {
                ves_pushnil();
                ves_import_class("render", "Texture2D");
                ur::TexturePtr* tex = (std::shared_ptr<ur::Texture>*)ves_set_newforeign(3, 4, sizeof(std::shared_ptr<ur::Texture>));
                ves_pop(1);
                *tex = normal->texture->image;
                ves_setfield(-2, "texture");
                ves_pop(1);
            }
            // tex_coord
            {
                ves_pushnumber(normal->tex_coord);
                ves_setfield(-2, "tex_coord");
                ves_pop(1);
            }
            ves_setfield(-2, "normal");
            ves_pop(1);
        }
        // occlusion
        if (material->occlusion)
        {
            auto& occlusion = material->occlusion;
            ves_newmap();
            // texture
            {
                ves_pushnil();
                ves_import_class("render", "Texture2D");
                ur::TexturePtr* tex = (std::shared_ptr<ur::Texture>*)ves_set_newforeign(3, 4, sizeof(std::shared_ptr<ur::Texture>));
                ves_pop(1);
                *tex = occlusion->texture->image;
                ves_setfield(-2, "texture");
                ves_pop(1);
            }
            // tex_coord
            {
                ves_pushnumber(occlusion->tex_coord);
                ves_setfield(-2, "tex_coord");
                ves_pop(1);
            }
            ves_setfield(-2, "occlusion");
            ves_pop(1);
        }
        // metallic_roughness
        if (material->metallic_roughness)
        {
            auto& metallic_roughness = material->metallic_roughness;
            ves_newmap();
            // metallic_factor
            {
                ves_pushnumber(metallic_roughness->metallic_factor);
                ves_setfield(-2, "metallic_factor");
                ves_pop(1);
            }
            // roughness_factor
            {
                ves_pushnumber(metallic_roughness->roughness_factor);
                ves_setfield(-2, "roughness_factor");
                ves_pop(1);
            }
            // texture
            {
                ves_pushnil();
                ves_import_class("render", "Texture2D");
                ur::TexturePtr* tex = (std::shared_ptr<ur::Texture>*)ves_set_newforeign(3, 4, sizeof(std::shared_ptr<ur::Texture>));
                ves_pop(1);
                *tex = metallic_roughness->texture->image;
                ves_setfield(-2, "texture");
                ves_pop(1);
            }
            // tex_coord
            {
                ves_pushnumber(metallic_roughness->tex_coord);
                ves_setfield(-2, "tex_coord");
                ves_pop(1);
            }
            ves_setfield(-2, "metallic_roughness");
            ves_pop(1);
        }
        // base_color
        if (material->base_color)
        {
            auto& base_color = material->base_color;
            ves_newmap();
            // factor
            {
                ves_pushnil();
                ves_import_class("maths", "Float4");
                sm::vec4* factor = (sm::vec4*)ves_set_newforeign(3, 4, sizeof(sm::vec4));
                memcpy(factor->xyzw, base_color->factor.xyzw, sizeof(float) * 4);
                ves_pop(1);
                ves_setfield(-2, "factor");
                ves_pop(1);
            }
            // texture
            {
                ves_pushnil();
                ves_import_class("render", "Texture2D");
                ur::TexturePtr* tex = (std::shared_ptr<ur::Texture>*)ves_set_newforeign(3, 4, sizeof(std::shared_ptr<ur::Texture>));
                ves_pop(1);
                *tex = base_color->texture->image;
                ves_setfield(-2, "texture");
                ves_pop(1);
            }
            // tex_coord
            {
                ves_pushnumber(base_color->tex_coord);
                ves_setfield(-2, "tex_coord");
                ves_pop(1);
            }
            ves_setfield(-2, "base_color");
            ves_pop(1);
        }
        // translation
        {
            ves_pushnil();
            ves_import_class("maths", "Float3");
            sm::vec3* translation = (sm::vec3*)ves_set_newforeign(2, 3, sizeof(sm::vec3));
            memcpy(translation->xyz, node->translation.xyz, sizeof(float) * 3);
            ves_pop(1);
            ves_setfield(-2, "translation");
            ves_pop(1);
        }
        // rotation
        {
            ves_pushnil();
            ves_import_class("maths", "Float4");
            sm::vec4* rotation = (sm::vec4*)ves_set_newforeign(2, 3, sizeof(sm::vec4));
            memcpy(rotation->xyzw, node->rotation.xyzw, sizeof(float) * 4);
            ves_pop(1);
            ves_setfield(-2, "rotation");
            ves_pop(1);
        }

        ves_setfield(-2, node->name.c_str());
        ves_pop(1);
    }
}

}

namespace tt
{

VesselForeignMethodFn ModelBindMethod(const char* signature)
{
    if (strcmp(signature, "Model.get_pbr_textures()") == 0) return w_Model_get_pbr_textures;
    if (strcmp(signature, "glTF.get_desc()") == 0) return w_glTF_get_desc;

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

    if (strcmp(class_name, "glTF") == 0)
    {
        methods->allocate = w_glTF_allocate;
        methods->finalize = w_glTF_finalize;
        return;
    }
}

}