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

void load_tex_trans(const model::gltf::Texture::Transform& trans, int slot)
{
    ves_newmap();
    { 
        ves_pushnil();
        ves_import_class("maths", "Float2");
        sm::vec2* scale = (sm::vec2*)ves_set_newforeign(slot + 1, slot + 2, sizeof(sm::vec2));
        memcpy(scale->xy, trans.offset.xy, sizeof(float) * 2);
        ves_pop(1);
        ves_setfield(-2, "offset");
        ves_pop(1);
    }
    {
        ves_pushnumber(trans.rotation);
        ves_setfield(-2, "rotation");
        ves_pop(1);
    }
    {
        ves_pushnil();
        ves_import_class("maths", "Float2");
        sm::vec2* scale = (sm::vec2*)ves_set_newforeign(slot + 1, slot + 2, sizeof(sm::vec2));
        memcpy(scale->xy, trans.scale.xy, sizeof(float) * 2);
        ves_pop(1);
        ves_setfield(-2, "scale");
        ves_pop(1);
    }
    ves_setfield(-2, "tex_trans");
    ves_pop(1);
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
        if (!node->mesh) {
            continue;
        }

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
        if (emissive->texture)
        {
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
            // tex_trans
            if (emissive->texture->transform) {
                load_tex_trans(*emissive->texture->transform, 3);
            }
        }
        ves_setfield(-2, "emissive");
        ves_pop(1);
        // normal
        auto& normal = material->normal;
        ves_newmap();
        if (normal->texture)
        {
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
            // tex_trans
            if (normal->texture->transform) {
                load_tex_trans(*normal->texture->transform, 3);
            }
        }
        ves_setfield(-2, "normal");
        ves_pop(1);
        // occlusion
        auto& occlusion = material->occlusion;
        ves_newmap();
        if (occlusion->texture)
        {
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
            // tex_trans
            if (occlusion->texture->transform) {
                load_tex_trans(*occlusion->texture->transform, 3);
            }
        }
        ves_setfield(-2, "occlusion");
        ves_pop(1);
        // metallic_roughness
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
        if (metallic_roughness->texture)
        {
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
            // tex_trans
            if (metallic_roughness->texture->transform) {
                load_tex_trans(*metallic_roughness->texture->transform, 3);
            }
        }
        ves_setfield(-2, "metallic_roughness");
        ves_pop(1);
        // base_color
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
        if (base_color->texture)
        {
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
            // tex_trans
            if (base_color->texture->transform) {
                load_tex_trans(*base_color->texture->transform, 3);
            }
        }
        ves_setfield(-2, "base_color");
        ves_pop(1);
        // sheen
        if (material->sheen)
        {
            ves_newmap();
            // color_factor
            {
                ves_pushnil();
                ves_import_class("maths", "Float3");
                sm::vec3* factor = (sm::vec3*)ves_set_newforeign(3, 4, sizeof(sm::vec3));
                memcpy(factor->xyz, material->sheen->color_factor.xyz, sizeof(float) * 3);
                ves_pop(1);
                ves_setfield(-2, "color_factor");
                ves_pop(1);
            }
            // color_texture
            if (material->sheen->color_texture)
            {
                ves_newmap();
                // texture
                {
                    ves_pushnil();
                    ves_import_class("render", "Texture2D");
                    ur::TexturePtr* tex = (std::shared_ptr<ur::Texture>*)ves_set_newforeign(4, 5, sizeof(std::shared_ptr<ur::Texture>));
                    ves_pop(1);
                    *tex = material->sheen->color_texture->image;
                    ves_setfield(-2, "texture");
                    ves_pop(1);
                }
                // tex_coord
                {
                    ves_pushnumber(0);
                    ves_setfield(-2, "tex_coord");
                    ves_pop(1);
                }
                // tex_trans
                if (base_color->texture->transform) {
                    load_tex_trans(*material->sheen->color_texture->transform, 4);
                }
                ves_setfield(-2, "color_texture");
                ves_pop(1);
            }
            // roughness_factor
            {
                ves_pushnumber(material->sheen->roughness_factor);
                ves_setfield(-2, "roughness_factor");
                ves_pop(1);
            }
            // roughness_texture
            if (material->sheen->roughness_texture)
            {
                ves_newmap();
                // texture
                {
                    ves_pushnil();
                    ves_import_class("render", "Texture2D");
                    ur::TexturePtr* tex = (std::shared_ptr<ur::Texture>*)ves_set_newforeign(4, 5, sizeof(std::shared_ptr<ur::Texture>));
                    ves_pop(1);
                    *tex = material->sheen->roughness_texture->image;
                    ves_setfield(-2, "texture");
                    ves_pop(1);
                }
                // tex_coord
                {
                    ves_pushnumber(0);
                    ves_setfield(-2, "tex_coord");
                    ves_pop(1);
                }
                // tex_trans
                if (base_color->texture->transform) {
                    load_tex_trans(*material->sheen->roughness_texture->transform, 4);
                }
                ves_setfield(-2, "roughness_texture");
                ves_pop(1);
            }
            ves_setfield(-2, "sheen");
            ves_pop(1);
        }
        // clearcoat
        if (material->clearcoat)
        {
            ves_newmap();
            // factor
            {
                ves_pushnumber(material->clearcoat->factor);
                ves_setfield(-2, "factor");
                ves_pop(1);
            }
            // texture
            if (material->clearcoat->texture)
            {
                ves_newmap();
                // texture
                {
                    ves_pushnil();
                    ves_import_class("render", "Texture2D");
                    ur::TexturePtr* tex = (std::shared_ptr<ur::Texture>*)ves_set_newforeign(4, 5, sizeof(std::shared_ptr<ur::Texture>));
                    ves_pop(1);
                    *tex = material->clearcoat->texture->image;
                    ves_setfield(-2, "texture");
                    ves_pop(1);
                }
                // tex_coord
                {
                    ves_pushnumber(material->clearcoat->tex_coord);
                    ves_setfield(-2, "tex_coord");
                    ves_pop(1);
                }
                ves_setfield(-2, "texture");
                ves_pop(1);
            }
            ves_setfield(-2, "clearcoat");
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
        // scale
        {
            ves_pushnil();
            ves_import_class("maths", "Float3");
            sm::vec3* scale = (sm::vec3*)ves_set_newforeign(2, 3, sizeof(sm::vec3));
            memcpy(scale->xyz, node->scale.xyz, sizeof(float) * 3);
            ves_pop(1);
            ves_setfield(-2, "scale");
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