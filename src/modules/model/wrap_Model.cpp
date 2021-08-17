#include "modules/model/wrap_Model.h"
#include "modules/script/Proxy.h"
#include "modules/render/Render.h"
#include "modules/filesystem/Filesystem.h"
#include "modules/script/TransHelper.h"

#include <model/ParametricEquations.h>
#include <model/Model.h>
#include <model/gltf/Model.h>
#include <model/GltfLoader.h>
#include <model/BrushModel.h>
#include <model/BrushBuilder.h>
#include <unirender/VertexArray.h>
#include <polymesh3/Polytope.h>

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

void w_Model_create_from_polytope()
{
    std::vector<model::BrushModel::Brush> brushes;

    std::vector<std::shared_ptr<pm3::Polytope>> polys;
    tt::list_to_foreigns(1, polys);
    for (auto& poly : polys)
    {
        model::BrushModel::Brush brush;

        brush.desc.mesh_begin = 0;
        brush.desc.mesh_end = 1;
        brush.desc.meshes.push_back({ 0, 0, 0, (int)(poly->Faces().size()) });
        brush.impl = poly;

        brushes.push_back(brush);
    }
    
    auto brush_model = std::make_shared<model::BrushModel>();
    brush_model->SetBrushes(brushes);

    auto dev = tt::Render::Instance()->Device();
    std::shared_ptr<model::Model> model = model::BrushBuilder::PolymeshFromBrushPN(*dev, *brush_model);

    auto proxy = (tt::Proxy<model::Model>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<model::Model>));
    proxy->obj = model;
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

void load_texture(const model::gltf::Texture& texture, int slot, int tex_coord = 0)
{
    // texture
    {
        ves_pushnil();
        ves_import_class("render", "Texture2D");
        ur::TexturePtr* tex = (std::shared_ptr<ur::Texture>*)ves_set_newforeign(slot, slot + 1, sizeof(std::shared_ptr<ur::Texture>));
        ves_pop(1);
        *tex = texture.image;
        ves_setfield(-2, "texture");
        ves_pop(1);
    }
    // sampler
    //if (texture.sampler)
    //{
    //    ves_pushnil();
    //    ves_import_class("render", "TextureSampler");
    //    std::shared_ptr<ur::TextureSampler>* sampler = (std::shared_ptr<ur::TextureSampler>*)ves_set_newforeign(slot, slot + 1, sizeof(std::shared_ptr<ur::TextureSampler>));
    //    ves_pop(1);
    //    *sampler = texture.sampler;
    //    ves_setfield(-2, "sampler");
    //    ves_pop(1);
    //}
    // tex_coord
    {
        ves_pushnumber(tex_coord);
        ves_setfield(-2, "tex_coord");
        ves_pop(1);
    }
    // tex_trans
    if (texture.transform) {
        load_tex_trans(*texture.transform, slot);
    }
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
    int idx = 0;
    for (auto& node : model->scene->nodes)
    {
        if (!node->mesh) {
            continue;
        }

        for (auto& prim : node->mesh->primitives)
        {
            ves_newmap();

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
            // size
            {
                ves_pushnil();
                ves_import_class("maths", "Float3");
                sm::vec3* size = (sm::vec3*)ves_set_newforeign(2, 3, sizeof(sm::vec3));
                memcpy(size->xyz, prim->size.xyz, sizeof(float) * 3);
                ves_pop(1);
                ves_setfield(-2, "size");
                ves_pop(1);
            }
            // double sided
            ves_pushboolean(material->double_sided);
            ves_setfield(-2, "double_sided");
            ves_pop(1);
            // alpha mode
            switch (material->alpha_mode)
            {
            case model::gltf::Material::AlphaMode::Opaque:
                ves_pushstring("opaque");
                break;
            case model::gltf::Material::AlphaMode::Mask:
                ves_pushstring("mask");
                break;
            case model::gltf::Material::AlphaMode::Blend:
                ves_pushstring("blend");
                break;
            default:
                assert(0);
            }
            ves_setfield(-2, "alpha_mode");
            ves_pop(1);
            // alpha cutoff
            ves_pushnumber(material->alpha_cutoff);
            ves_setfield(-2, "alpha_cutoff");
            ves_pop(1);
            // emissive
            auto& emissive = material->emissive;
            if (emissive)
            {
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
                if (emissive->texture) {
                    load_texture(*emissive->texture, 3, emissive->tex_coord);
                }
                ves_setfield(-2, "emissive");
                ves_pop(1);
            }
            // normal
            auto& normal = material->normal;
            if (normal)
            {
                ves_newmap();
                if (normal->texture) {
                    load_texture(*normal->texture, 3, normal->tex_coord);
                }
                ves_setfield(-2, "normal");
                ves_pop(1);
            }
            // occlusion
            auto& occlusion = material->occlusion;
            if (occlusion)
            {
                ves_newmap();
                if (occlusion->texture) {
                    load_texture(*occlusion->texture, 3, occlusion->tex_coord);
                }
                ves_setfield(-2, "occlusion");
                ves_pop(1);
            }
            // metallic_roughness
            auto& metallic_roughness = material->metallic_roughness;
            if (metallic_roughness)
            {
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
                if (metallic_roughness->texture) {
                    load_texture(*metallic_roughness->texture, 3, metallic_roughness->tex_coord);
                }
                ves_setfield(-2, "metallic_roughness");
                ves_pop(1);
            }
            // base_color
            auto& base_color = material->base_color;
            if (base_color)
            {
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
                if (base_color->texture) {
                    load_texture(*base_color->texture, 3, base_color->tex_coord);
                }
                ves_setfield(-2, "base_color");
                ves_pop(1);
            }
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
                    load_texture(*material->sheen->color_texture, 4);
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
                    load_texture(*material->sheen->roughness_texture, 4);
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
                    load_texture(*material->clearcoat->texture, 4);
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

            if (node->name.empty()) {
                std::string name = "node" + std::to_string(idx++);
                ves_setfield(-2, name.c_str());
            } else {
                ves_setfield(-2, node->name.c_str());
            }
            ves_pop(1);
        }
    }
}

void w_glTF_create_from_polytope()
{
    std::vector<std::shared_ptr<pm3::Polytope>> polys;
    tt::list_to_foreigns(1, polys);

    auto dev = tt::Render::Instance()->Device();
    auto model = std::make_shared<model::gltf::Model>();
    model::BrushBuilder::PolymeshFromBrush(*dev, polys, *model);

    auto proxy = (tt::Proxy<model::gltf::Model>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<model::gltf::Model>));
    proxy->obj = model;
}

}

namespace tt
{

VesselForeignMethodFn ModelBindMethod(const char* signature)
{
    if (strcmp(signature, "static Model.create_from_polytope(_)") == 0) return w_Model_create_from_polytope;

    if (strcmp(signature, "glTF.get_desc()") == 0) return w_glTF_get_desc;
    if (strcmp(signature, "static glTF.create_from_polytope(_)") == 0) return w_glTF_create_from_polytope;

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