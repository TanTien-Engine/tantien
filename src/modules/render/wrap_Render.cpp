#include "modules/render/wrap_Render.h"
#include "modules/render/Render.h"
#include "modules/image/ImageData.h"
#include "modules/model/Model.h"
#include "modules/script/Proxy.h"

#include <unirender/Device.h>
#include <unirender/Context.h>
#include <unirender/VertexArray.h>
#include <unirender/VertexBuffer.h>
#include <unirender/IndexBuffer.h>
#include <unirender/ComponentDataType.h>
#include <unirender/VertexInputAttribute.h>
#include <unirender/DrawState.h>
#include <unirender/ClearState.h>
#include <unirender/PrimitiveType.h>
#include <unirender/ShaderProgram.h>
#include <unirender/Uniform.h>
#include <unirender/Texture.h>
#include <unirender/Framebuffer.h>
#include <unirender/TextureUtility.h>
#include <unirender/TextureDescription.h>
#include <unirender/RenderBuffer.h>
#include <shadertrans/ShaderTrans.h>
#include <SM_Matrix.h>
#include <gimg_typedef.h>
#include <guard/check.h>
#include <model/Model.h>

#include <glslang/glslang/Public/ShaderLang.h>

#include <spirv-cross/spirv.hpp>
#include <spirv-cross/spirv_glsl.hpp>

#include <array>

//#define SHADER_DEBUG_PRINT

namespace
{

void w_Shader_allocate()
{
    std::shared_ptr<ur::ShaderProgram> prog = nullptr;

    const char* vs_str = ves_tostring(1);
    const char* fs_str = ves_tostring(2);
    const char* gs_str = ves_tostring(3);

    if (strlen(vs_str) == 0) 
    {
        auto proxy = (tt::Proxy<ur::ShaderProgram>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<ur::ShaderProgram>));
        proxy->obj = prog;
        return;
    }

    if (strlen(fs_str) == 0)
    {
        std::vector<unsigned int> cs;
        shadertrans::ShaderTrans::GLSL2SpirV(shadertrans::ShaderStage::ComputeShader, vs_str, cs);

#ifdef SHADER_DEBUG_PRINT
        std::string cs_glsl;
        shadertrans::ShaderTrans::SpirV2GLSL(shadertrans::ShaderStage::ComputeShader, cs, cs_glsl);
        printf("cs:\n%s\nfs:\n%s\n", cs_glsl.c_str(), cs_glsl.c_str());
#endif // SHADER_DEBUG_PRINT

        prog = tt::Render::Instance()->Device()->CreateShaderProgram(cs);
    }
    else
    {
        std::vector<unsigned int> vs, fs, gs;
        shadertrans::ShaderTrans::GLSL2SpirV(shadertrans::ShaderStage::VertexShader, vs_str, vs);
        shadertrans::ShaderTrans::GLSL2SpirV(shadertrans::ShaderStage::PixelShader, fs_str, fs);
        shadertrans::ShaderTrans::GLSL2SpirV(shadertrans::ShaderStage::GeometryShader, gs_str, gs);

#ifdef SHADER_DEBUG_PRINT
        std::string vs_glsl, fs_glsl;
        shadertrans::ShaderTrans::SpirV2GLSL(shadertrans::ShaderStage::VertexShader, vs, vs_glsl);
        shadertrans::ShaderTrans::SpirV2GLSL(shadertrans::ShaderStage::PixelShader, fs, fs_glsl);
        printf("vs:\n%s\nfs:\n%s\n", vs_glsl.c_str(), fs_glsl.c_str());
#endif // SHADER_DEBUG_PRINT

        prog = tt::Render::Instance()->Device()->CreateShaderProgram(vs, fs, std::vector<uint32_t>(), std::vector<uint32_t>(), gs);
    }

    auto proxy = (tt::Proxy<ur::ShaderProgram>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<ur::ShaderProgram>));
    proxy->obj = prog;
}

int w_Shader_finalize(void* data)
{
    auto proxy = (tt::Proxy<ur::ShaderProgram>*)(data);
    proxy->~Proxy();
    return sizeof(tt::Proxy<ur::ShaderProgram>);
}

int get_value_number_size(const std::string& type)
{
    int size = 0;
    if (type == "float") {
        size = 1;
    } else if (type == "float2") {
        size = 2;
    } else if (type == "float3") {
        size = 3;
    } else if (type == "float4") {
        size = 4;
    } else if (type == "mat2") {
        size = 4;
    } else if (type == "mat3") {
        size = 9;
    } else if (type == "mat4") {
        size = 16;
    } else if (type == "int") {
        size = 1;
    } else if (type == "int2") {
        size = 2;
    } else if (type == "int3") {
        size = 3;
    } else if (type == "int4") {
        size = 4;
    }
    return size;
}

void w_Shader_setUniformValue()
{
    auto prog = ((tt::Proxy<ur::ShaderProgram>*)ves_toforeign(0))->obj;

    GD_ASSERT(ves_type(1) == VES_TYPE_LIST, "unknown type");

    ves_geti(1, 0);
    const char* name = ves_tostring(-1);
    ves_pop(1);

    ves_geti(1, 1);
    const char* type = ves_tostring(-1);
    ves_pop(1);
    GD_ASSERT(strcmp(type, "unknown") != 0, "unknown type");

    if (strcmp(type, "sampler") == 0)
    {
        int num = ves_len(1);

        auto slot = prog->QueryTexSlot(name);
        if (slot >= 0)
        {
            auto ctx = tt::Render::Instance()->Context();

            ves_geti(1, 2);
            if (ves_type(-1) != VES_TYPE_NULL)
            {
                auto tex = ((tt::Proxy<ur::Texture>*)ves_toforeign(-1))->obj;
                if (slot >= 0) {
                    ctx->SetTexture(slot, tex);
                }
            }
            ves_pop(1);

            // texture sampler
            if (ves_len(1) > 3)
            {
                ves_geti(1, 3);
                if (ves_type(-1) != VES_TYPE_NULL)
                {
                    ur::Device::TextureSamplerType type = static_cast<ur::Device::TextureSamplerType>(ves_tonumber(-1));
                    auto dev = tt::Render::Instance()->Device();
                    ctx->SetTextureSampler(slot, dev->GetTextureSampler(type));
                }
                ves_pop(1);
            }
        }
    }
    else if (strcmp(type, "image") == 0)
    {
        int num = ves_len(1);

        auto slot = prog->QueryImgSlot(name);
        if (slot >= 0)
        {
            auto ctx = tt::Render::Instance()->Context();

            ves_geti(1, 2);
            if (ves_type(-1) != VES_TYPE_NULL)
            {
                auto tex = ((tt::Proxy<ur::Texture>*)ves_toforeign(-1))->obj;
                if (slot >= 0) {
                    ctx->SetImage(slot, tex, ur::AccessType::WriteOnly);
                }
            }
            ves_pop(1);
        }
    }
    else if (strcmp(type, "mat4") == 0)
    {
        auto unif = prog->QueryUniform(name);
        if (unif)
        {
            int num = ves_len(1);

            ves_geti(1, 2);
            sm::mat4* mt = static_cast<sm::mat4*>(ves_toforeign(-1));
            ves_pop(1);

            unif->SetValue(mt->x, 16);
        }
    }
    else
    {
        auto unif = prog->QueryUniform(name);
        if (unif)
        {
            auto unif_type = unif->GetType();
            if (unif_type >= ur::UniformType::Int1 &&
                unif_type <= ur::UniformType::UInt4) 
            {
                const int num = get_value_number_size(type);
                GD_ASSERT(num <= 4, "error num count");
                int val[4];
                for (int i = 0; i < num; ++i)
                {
                    ves_geti(1, 2 + i);
                    val[i] = (int)ves_tonumber(-1);
                    ves_pop(1);
                }

                unif->SetValue(val, num);
            }
            else 
            {
                const int num = get_value_number_size(type);
                GD_ASSERT(num <= 16, "error num count");
                float val[16];
                for (int i = 0; i < num; ++i)
                {
                    ves_geti(1, 2 + i);
                    val[i] = (float)ves_tonumber(-1);
                    ves_pop(1);
                }

                unif->SetValue(val, num);
            }
        }
    }
}

void w_VertexArray_allocate()
{
    std::shared_ptr<ur::VertexArray> va = nullptr;
    if (ves_type(1) == VES_TYPE_LIST)
    {
        std::vector<float> data;
        int data_num = ves_len(1);
        data.resize(data_num);
        for (int i = 0; i < data_num; ++i)
        {
            ves_geti(1, i);
            data[i] = static_cast<float>(ves_tonumber(-1));
            ves_pop(1);
        }

        std::vector<int> attrs;
        int attr_num = ves_len(2);
        attrs.resize(attr_num);
        for (int i = 0; i < attr_num; ++i)
        {
            ves_geti(2, i);
            attrs[i] = static_cast<int>(ves_tonumber(-1));
            ves_pop(1);
        }

        std::vector<unsigned short> index_data;
        if (ves_type(3) != VES_TYPE_NULL)
        {
            int num = ves_len(3);
            index_data.resize(num);
            for (int i = 0; i < num; ++i)
            {
                ves_geti(3, i);
                index_data[i] = static_cast<unsigned short>(ves_tonumber(-1));
                ves_pop(1);
            }
        }

        auto dev = tt::Render::Instance()->Device();

        va = dev->CreateVertexArray();

        int vbuf_sz = sizeof(float) * data_num;
        auto vbuf = dev->CreateVertexBuffer(ur::BufferUsageHint::StaticDraw, vbuf_sz);
        vbuf->ReadFromMemory(data.data(), vbuf_sz, 0);
        va->SetVertexBuffer(vbuf);

        int stride_in_bytes = 0;
        for (auto& attr : attrs) {
            stride_in_bytes += attr * sizeof(float);
        }

        std::vector<std::shared_ptr<ur::VertexInputAttribute>> vbuf_attrs;
        vbuf_attrs.resize(attrs.size());
        int offset_in_bytes = 0;
        for (size_t i = 0, n = attrs.size(); i < n; ++i)
        {
            vbuf_attrs[i] = std::make_shared<ur::VertexInputAttribute>(
                static_cast<int>(i), ur::ComponentDataType::Float, attrs[i], offset_in_bytes, stride_in_bytes
            );
            offset_in_bytes += attrs[i] * sizeof(float);
        }
        va->SetVertexBufferAttrs(vbuf_attrs);

        if (!index_data.empty())
        {
            auto ibuf_sz = sizeof(unsigned short) * index_data.size();
            auto ibuf = dev->CreateIndexBuffer(ur::BufferUsageHint::StaticDraw, static_cast<int>(ibuf_sz));
            ibuf->ReadFromMemory(index_data.data(), static_cast<int>(ibuf_sz), 0);
            ibuf->SetCount(static_cast<int>(index_data.size()));
            va->SetIndexBuffer(ibuf);
        }
    }
    else
    {
        const char* s_type = ves_tostring(1);
        tt::ShapeType type;
        if (strcmp(s_type, "quad") == 0) {
            type = tt::ShapeType::Quad;
        } else if (strcmp(s_type, "cube") == 0) {
            type = tt::ShapeType::Cube;
        } else if (strcmp(s_type, "sphere") == 0) {
            type = tt::ShapeType::Sphere;
        } else if (strcmp(s_type, "grids") == 0) {
            type = tt::ShapeType::Grids;
        } else {
            GD_REPORT_ASSERT("unknown type.");
        }

        bool p = false, t = false, n = false, tb = false;
        for (int i = 0, _n = ves_len(2); i < _n; ++i)
        {
            ves_geti(2, i);
            const char* layout = ves_tostring(-1);
            ves_pop(1);

            if (strcmp(layout, "position") == 0) {
                p = true;
            } else if (strcmp(layout, "texture") == 0) {
                t = true;
            } else if (strcmp(layout, "normal") == 0) {
                n = true;
            } else if (strcmp(layout, "tangent_bitangent") == 0) {
                tb = true;
            }
        }
        ur::VertexLayoutType layout;
        if (p && n && t && tb) {
            layout = ur::VertexLayoutType::PosNormTexTB;
        } else if (p && n && t) {
            layout = ur::VertexLayoutType::PosNormTex;
        } else if (p && n) {
            layout = ur::VertexLayoutType::PosNorm;
        } else if (p && t) {
            layout = ur::VertexLayoutType::PosTex;
        } else if (p) {
            layout = ur::VertexLayoutType::Pos;
        } else {
            GD_REPORT_ASSERT("unknown layout.");
        }

        auto dev = tt::Render::Instance()->Device();
        ur::PrimitiveType prim_type;
        va = tt::Model::Instance()->CreateShape(*dev, type, layout, prim_type);
    }

    auto proxy = (tt::Proxy<ur::VertexArray>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<ur::VertexArray>));
    proxy->obj = va;
}

int w_VertexArray_finalize(void* data)
{
    auto proxy = (tt::Proxy<ur::VertexArray>*)(data);
    proxy->~Proxy();
    return sizeof(tt::Proxy<ur::VertexArray>);
}

void w_Texture2D_allocate()
{
    ur::TexturePtr tex = nullptr;
    if (ves_type(1) == VES_TYPE_FOREIGN)
    {
        tt::ImageData* img = (tt::ImageData*)ves_toforeign(1);

        ur::TextureFormat tf;
	    switch (img->format)
	    {
        case GPF_INVALID:
            tf = ur::TextureFormat::RGBA8;
            break;
	    case GPF_ALPHA: case GPF_LUMINANCE: case GPF_LUMINANCE_ALPHA:
		    tf = ur::TextureFormat::A8;
		    break;
        case GPF_RED:
            tf = ur::TextureFormat::RED;
            break;
	    case GPF_RGB:
		    tf = ur::TextureFormat::RGB;
		    break;
        case GPF_RGB565:
            tf = ur::TextureFormat::RGB565;
            break;
	    case GPF_RGBA8:
		    tf = ur::TextureFormat::RGBA8;
		    break;
	    case GPF_BGRA_EXT:
		    tf = ur::TextureFormat::BGRA_EXT;
		    break;
	    case GPF_BGR_EXT:
		    tf = ur::TextureFormat::BGR_EXT;
		    break;
        case GPF_RGBA16F:
            tf = ur::TextureFormat::RGBA16F;
            break;
        case GPF_RGB16F:
            tf = ur::TextureFormat::RGB16F;
            break;
        case GPF_RGB32F:
            tf = ur::TextureFormat::RGB32F;
            break;
	    case GPF_COMPRESSED_RGBA_S3TC_DXT1_EXT:
		    tf = ur::TextureFormat::COMPRESSED_RGBA_S3TC_DXT1_EXT;
		    break;
	    case GPF_COMPRESSED_RGBA_S3TC_DXT3_EXT:
		    tf = ur::TextureFormat::COMPRESSED_RGBA_S3TC_DXT3_EXT;
		    break;
	    case GPF_COMPRESSED_RGBA_S3TC_DXT5_EXT:
		    tf = ur::TextureFormat::COMPRESSED_RGBA_S3TC_DXT5_EXT;
		    break;
	    default:
            GD_REPORT_ASSERT("unknown type.");
	    }

        bool gamma_correction = ves_toboolean(2);

        size_t buf_sz = ur::TextureUtility::RequiredSizeInBytes(img->width, img->height, tf, 4);
        tex = tt::Render::Instance()->Device()->CreateTexture(img->width, img->height, tf, img->pixels, buf_sz, gamma_correction);
    }
    else if (ves_type(1) == VES_TYPE_LIST)
    {
        GD_ASSERT(ves_len(1) == 6, "error number");
        std::array<ur::TexturePtr, 6> textures;
        for (int i = 0; i < 6; ++i)
        {
            ves_geti(1, i);
            textures[i] = ((tt::Proxy<ur::Texture>*)ves_toforeign(-1))->obj;
            ves_pop(1);
        }

        tex = tt::Render::Instance()->Device()->CreateTextureCubeMap(textures);
    }
    else
    {
        int width  = (int)ves_tonumber(1);
        int height = (int)ves_tonumber(2);
        const char* format = ves_tostring(3);
        ur::TextureFormat tf;
        if (strcmp(format, "rgb") == 0) {
            tf = ur::TextureFormat::RGB;
        } else if (strcmp(format, "rgba8") == 0) {
            tf = ur::TextureFormat::RGBA8;
        } else if (strcmp(format, "rgba16f") == 0) {
            tf = ur::TextureFormat::RGBA16F;
        } else if (strcmp(format, "rg16f") == 0) {
            tf = ur::TextureFormat::RG16F;
        } else if (strcmp(format, "r16") == 0) {
            tf = ur::TextureFormat::R16;
        } else if (strcmp(format, "depth") == 0) {
            tf = ur::TextureFormat::DEPTH;
        } else {
            GD_REPORT_ASSERT("unknown type.");
        }

        size_t buf_sz = ur::TextureUtility::RequiredSizeInBytes(width, height, tf, 4);
        tex = tt::Render::Instance()->Device()->CreateTexture(width, height, tf, nullptr, buf_sz);
    }

    auto proxy = (tt::Proxy<ur::Texture>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<ur::Texture>));
    proxy->obj = tex;
}

int w_Texture2D_finalize(void* data)
{
    auto proxy = (tt::Proxy<ur::Texture>*)(data);
    proxy->~Proxy();
    return sizeof(tt::Proxy<ur::Texture>);
}

void w_Texture2D_getWidth()
{
    auto tex = ((tt::Proxy<ur::Texture>*)ves_toforeign(0))->obj;
    ves_set_number(0, (double)tex->GetWidth());
}

void w_Texture2D_getHeight()
{
    auto tex = ((tt::Proxy<ur::Texture>*)ves_toforeign(0))->obj;
    ves_set_number(0, (double)tex->GetHeight());
}

void w_Cubemap_get_width()
{
    auto tex = ((tt::Proxy<ur::Texture>*)ves_toforeign(0))->obj;
    ves_set_number(0, (double)tex->GetWidth());
}

void w_Cubemap_get_height()
{
    auto tex = ((tt::Proxy<ur::Texture>*)ves_toforeign(0))->obj;
    ves_set_number(0, (double)tex->GetHeight());
}

void w_Cubemap_allocate()
{
    ur::TexturePtr tex = nullptr;
 
    if (ves_type(1) == VES_TYPE_LIST)
    {
        GD_ASSERT(ves_len(1) == 6, "error number");
        std::array<ur::TexturePtr, 6> textures;
        for (int i = 0; i < 6; ++i)
        {
            ves_geti(1, i);
            textures[i] = ((tt::Proxy<ur::Texture>*)ves_toforeign(-1))->obj;
            ves_pop(1);
        }

        tex = tt::Render::Instance()->Device()->CreateTextureCubeMap(textures);
    }
    else
    {
        int width  = (int)ves_tonumber(1);
        int height = (int)ves_tonumber(2);
        const char* format = ves_tostring(3);
        ur::TextureFormat tf;
        if (strcmp(format, "rgb") == 0) {
            tf = ur::TextureFormat::RGB;
        } else if (strcmp(format, "rgb16f") == 0) {
            tf = ur::TextureFormat::RGB16F;
        } else if (strcmp(format, "rgba8") == 0) {
            tf = ur::TextureFormat::RGBA8;
        } else if (strcmp(format, "rgba16f") == 0) {
            tf = ur::TextureFormat::RGBA16F;
        } else if (strcmp(format, "r16") == 0) {
            tf = ur::TextureFormat::R16;
        } else {
            GD_REPORT_ASSERT("unknown type.");
        }

        ur::TextureDescription desc;
        desc.target = ur::TextureTarget::TextureCubeMap;
        desc.width = width;
        desc.height = height;
        desc.format = tf;
        desc.gen_mipmaps = true;
        desc.sampler_type = ur::Device::TextureSamplerType::LinearClampMipmap;
        tex = tt::Render::Instance()->Device()->CreateTexture(desc);
    }

    auto proxy = (tt::Proxy<ur::Texture>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<ur::Texture>));
    proxy->obj = tex;
}

int w_Cubemap_finalize(void* data)
{
    auto proxy = (tt::Proxy<ur::Texture>*)(data);
    proxy->~Proxy();
    return sizeof(tt::Proxy<ur::Texture>);
}

void w_Framebuffer_allocate()
{
    auto fbo = tt::Render::Instance()->Device()->CreateFramebuffer();

    auto proxy = (tt::Proxy<ur::Framebuffer>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<ur::Framebuffer>));
    proxy->obj = fbo;
}

int w_Framebuffer_finalize(void* data)
{
    auto proxy = (tt::Proxy<ur::Framebuffer>*)(data);
    proxy->~Proxy();
    return sizeof(tt::Proxy<ur::Framebuffer>);
}

ur::AttachmentType strint2attachment(const std::string& str)
{
    ur::AttachmentType atta_type = ur::AttachmentType::Color0;
    if (str == "depth") {
        atta_type = ur::AttachmentType::Depth;
    } else if (str == "stencil") {
        atta_type = ur::AttachmentType::Stencil;
    } else {
        for (int i = 0; i < 16; ++i) {
            if (str == "col" + std::to_string(i)) {
                atta_type = static_cast<ur::AttachmentType>((int)ur::AttachmentType::Color0 + i);
                break;
            }
        }
    }
    return atta_type;
}

void w_Framebuffer_attach_tex()
{
    auto fbo = ((tt::Proxy<ur::Framebuffer>*)ves_toforeign(0))->obj;
    auto tex = ((tt::Proxy<ur::Texture>*)ves_toforeign(1))->obj;
    auto atta_type = strint2attachment(ves_tostring(2));

    const char* target = ves_tostring(3);
    ur::TextureTarget tex_target = ur::TextureTarget::Texture2D;
    if (strcmp(target, "tex2d") == 0) {
        tex_target = ur::TextureTarget::Texture2D;
    } else if (strcmp(target, "cubemap0") == 0) {
        tex_target = ur::TextureTarget::TextureCubeMap0;
    } else if (strcmp(target, "cubemap1") == 0) {
        tex_target = ur::TextureTarget::TextureCubeMap1;
    } else if (strcmp(target, "cubemap2") == 0) {
        tex_target = ur::TextureTarget::TextureCubeMap2;
    } else if (strcmp(target, "cubemap3") == 0) {
        tex_target = ur::TextureTarget::TextureCubeMap3;
    } else if (strcmp(target, "cubemap4") == 0) {
        tex_target = ur::TextureTarget::TextureCubeMap4;
    } else if (strcmp(target, "cubemap5") == 0) {
        tex_target = ur::TextureTarget::TextureCubeMap5;
    }

    int mipmap_level = (int)ves_tonumber(4);
    fbo->SetAttachment(atta_type, tex_target, tex, nullptr, mipmap_level);
}

void w_Framebuffer_attach_rbo()
{
    auto fbo = ((tt::Proxy<ur::Framebuffer>*)ves_toforeign(0))->obj;
    auto rbo = ((tt::Proxy<ur::RenderBuffer>*)ves_toforeign(1))->obj;
    auto atta_type = strint2attachment(ves_tostring(2));
    fbo->SetAttachment(atta_type, ur::TextureTarget::Texture2D, nullptr, rbo);
}

void w_RenderBuffer_allocate()
{
    int width = (int)ves_tonumber(1);
    int height = (int)ves_tonumber(2);

    ur::InternalFormat format = ur::InternalFormat::RGBA;
    const char* s_fmt = ves_tostring(3);
    if (strcmp(s_fmt, "depth_component") == 0) {
        format = ur::InternalFormat::DepthComponent;
    } else if (strcmp(s_fmt, "depth_stencil") == 0) {
        format = ur::InternalFormat::DepthStencil;
    } else if (strcmp(s_fmt, "red") == 0) {
        format = ur::InternalFormat::RED;
    } else if (strcmp(s_fmt, "rg") == 0) {
        format = ur::InternalFormat::RG;
    } else if (strcmp(s_fmt, "rgb") == 0) {
        format = ur::InternalFormat::RGB;
    } else if (strcmp(s_fmt, "rgba") == 0) {
        format = ur::InternalFormat::RGBA;
    }

    auto rbo = tt::Render::Instance()->Device()->CreateRenderBuffer(width, height, format);

    auto proxy = (tt::Proxy<ur::RenderBuffer>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<ur::RenderBuffer>));
    proxy->obj = rbo;
}

int w_RenderBuffer_finalize(void* data)
{
    auto proxy = (tt::Proxy<ur::RenderBuffer>*)(data);
    proxy->~Proxy();
    return sizeof(tt::Proxy<ur::RenderBuffer>);
}

void prepare_render_state(ur::RenderState& rs, int slot)
{
    if (ves_getfield(slot, "depth_test") == VES_TYPE_BOOL) {
        rs.depth_test.enabled = ves_toboolean(-1);
    }
    ves_pop(1);

    if (ves_getfield(slot, "depth_func") == VES_TYPE_STRING) 
    {
        const char* func = ves_tostring(-1);
        if (strcmp(func, "never") == 0) {
            rs.depth_test.function = ur::DepthTestFunc::Never;
        } else if (strcmp(func, "less") == 0) {
            rs.depth_test.function = ur::DepthTestFunc::Less;
        } else if (strcmp(func, "equal") == 0) {
            rs.depth_test.function = ur::DepthTestFunc::Equal;
        } else if (strcmp(func, "lequal") == 0) {
            rs.depth_test.function = ur::DepthTestFunc::LessThanOrEqual;
        } else if (strcmp(func, "greater") == 0) {
            rs.depth_test.function = ur::DepthTestFunc::Greater;
        } else if (strcmp(func, "notequal") == 0) {
            rs.depth_test.function = ur::DepthTestFunc::NotEqual;
        } else if (strcmp(func, "gequal") == 0) {
            rs.depth_test.function = ur::DepthTestFunc::GreaterThanOrEqual;
        } else if (strcmp(func, "always") == 0) {
            rs.depth_test.function = ur::DepthTestFunc::Always;
        }
    }
    ves_pop(1);

    if (ves_getfield(slot, "stencil_test") == VES_TYPE_BOOL) {
        rs.stencil_test.enabled = ves_toboolean(-1);
    }
    ves_pop(1);

    if (ves_getfield(slot, "stencil_func") == VES_TYPE_STRING) 
    {
        const char* func = ves_tostring(-1);
        if (strcmp(func, "never") == 0) {
            rs.stencil_test.front_face.function = ur::StencilTestFunction::Never;
        } else if (strcmp(func, "less") == 0) {
            rs.stencil_test.front_face.function = ur::StencilTestFunction::Less;
        } else if (strcmp(func, "equal") == 0) {
            rs.stencil_test.front_face.function = ur::StencilTestFunction::Equal;
        } else if (strcmp(func, "lequal") == 0) {
            rs.stencil_test.front_face.function = ur::StencilTestFunction::LessThanOrEqual;
        } else if (strcmp(func, "greater") == 0) {
            rs.stencil_test.front_face.function = ur::StencilTestFunction::Greater;
        } else if (strcmp(func, "notequal") == 0) {
            rs.stencil_test.front_face.function = ur::StencilTestFunction::NotEqual;
        } else if (strcmp(func, "gequal") == 0) {
            rs.stencil_test.front_face.function = ur::StencilTestFunction::GreaterThanOrEqual;
        } else if (strcmp(func, "always") == 0) {
            rs.stencil_test.front_face.function = ur::StencilTestFunction::Always;
        }
    }
    ves_pop(1);

    if (ves_getfield(slot, "stencil_ref") == VES_TYPE_NUM) {
        rs.stencil_test.front_face.reference_value = (int)(ves_tonumber(-1));
    }
    ves_pop(1);

    if (ves_getfield(slot, "stencil_mask") == VES_TYPE_NUM) {
        rs.stencil_test.front_face.mask = (int)(ves_tonumber(-1));
    }
    ves_pop(1);

    if (ves_getfield(slot, "cull") == VES_TYPE_STRING) 
    {
        const char* mode = ves_tostring(-1);
        if (strcmp(mode, "disable") == 0) {
            rs.facet_culling.enabled = false;
        } else if (strcmp(mode, "front") == 0) {
            rs.facet_culling.enabled = true;
            rs.facet_culling.face = ur::CullFace::Front;
        } else if (strcmp(mode, "back") == 0) {
            rs.facet_culling.enabled = true;
            rs.facet_culling.face = ur::CullFace::Back;
        } else if (strcmp(mode, "front_and_back") == 0) {
            rs.facet_culling.enabled = true;
            rs.facet_culling.face = ur::CullFace::FrontAndBack;
        }
    }
    ves_pop(1);
}

void w_Render_draw()
{
    ur::DrawState ds;

    ur::PrimitiveType prim_type = ur::PrimitiveType::Triangles;

    const char* prim_type_str = ves_tostring(1);
    if (strcmp(prim_type_str, "points") == 0) {
        prim_type = ur::PrimitiveType::Points;
    } else if (strcmp(prim_type_str, "triangles") == 0) {
        prim_type = ur::PrimitiveType::Triangles;
    } else if (strcmp(prim_type_str, "tri_strip") == 0) {
        prim_type = ur::PrimitiveType::TriangleStrip;
    } else {
        GD_REPORT_ASSERT("unknown prim type.");
    }

    ds.program = ((tt::Proxy<ur::ShaderProgram>*)ves_toforeign(2))->obj;
    ds.vertex_array = ((tt::Proxy<ur::VertexArray>*)ves_toforeign(3))->obj;

    prepare_render_state(ds.render_state, 4);

    tt::Render::Instance()->Context()->Draw(prim_type, ds, nullptr);
}

void w_Render_draw_model()
{
    ur::DrawState ds;

    ds.program = ((tt::Proxy<ur::ShaderProgram>*)ves_toforeign(1))->obj;

    auto model = ((tt::Proxy<model::Model>*)ves_toforeign(2))->obj;

    prepare_render_state(ds.render_state, 3);

    auto ctx = tt::Render::Instance()->Context();

    auto& meshes = model->meshes;
    for (auto& mesh : meshes)
    {
        auto& geo = mesh->geometry;
        ds.vertex_array = geo.vertex_array;
        GD_ASSERT(geo.sub_geometries.size() == geo.sub_geometry_materials.size(), "error size.");
        for (int i = 0, n = geo.sub_geometries.size(); i < n; ++i)
        {
            auto& mat = model->materials[geo.sub_geometry_materials[i]];
            if (mat->diffuse_tex >= 0) {
                ctx->SetTexture(ds.program->QueryTexSlot("texture_diffuse1"), model->textures[mat->diffuse_tex].second);
            }
            ds.offset = geo.sub_geometries[i].offset;
            ds.count = geo.sub_geometries[i].count;
            ctx->Draw(ur::PrimitiveType::Triangles, ds, nullptr);
        }
    }
}

void w_Render_compute()
{
    ur::DrawState ds;

    ds.program = ((tt::Proxy<ur::ShaderProgram>*)ves_toforeign(1))->obj;

    int x = (int)ves_tonumber(2);
    int y = (int)ves_tonumber(3);
    int z = (int)ves_tonumber(4);
    tt::Render::Instance()->Context()->Compute(ds, x, y, z);
}

void w_Render_clear()
{
    ur::ClearState clear;

    int clear_mask = 0;
    int mask_num = ves_len(1);
    for (int i = 0; i < mask_num; ++i)
    {
        ves_geti(1, i);
        auto mask_str = ves_tostring(-1);
        ves_pop(1);
        if (strcmp(mask_str, "color") == 0) {
            clear_mask |= static_cast<int>(ur::ClearBuffers::ColorBuffer);
        } else if (strcmp(mask_str, "depth") == 0) {
            clear_mask |= static_cast<int>(ur::ClearBuffers::DepthBuffer);
        } else if (strcmp(mask_str, "stencil") == 0) {
            clear_mask |= static_cast<int>(ur::ClearBuffers::StencilBuffer);
        } else {
            GD_REPORT_ASSERT("unknown type.");
        }
    }
    clear.buffers = static_cast<ur::ClearBuffers>(clear_mask);

    int type = ves_getfield(2, "color");
    if (type == VES_TYPE_LIST)
    {
        bool is01 = true;
        double rgba[4];
        GD_ASSERT(ves_len(-1) == 4, "error number");
        for (int i = 0; i < 4; ++i) 
        {
            ves_geti(-1, i);
            rgba[i] = ves_tonumber(-1);
            ves_pop(1);

            if (rgba[i] > 1.0) {
                is01 = false;
            }
        }

        if (is01)
        {
            clear.color.r = static_cast<uint8_t>(rgba[0] * 255);
            clear.color.g = static_cast<uint8_t>(rgba[1] * 255);
            clear.color.b = static_cast<uint8_t>(rgba[2] * 255);
            clear.color.a = static_cast<uint8_t>(rgba[3] * 255);
        }
        else
        {
            clear.color.r = static_cast<uint8_t>(rgba[0]);
            clear.color.g = static_cast<uint8_t>(rgba[1]);
            clear.color.b = static_cast<uint8_t>(rgba[2]);
            clear.color.a = static_cast<uint8_t>(rgba[3]);
        }
    }

    tt::Render::Instance()->Context()->Clear(clear);
}

void w_Render_get_fbo()
{
    std::shared_ptr<ur::Framebuffer>* fbo = (std::shared_ptr<ur::Framebuffer>*)ves_set_newforeign(0, 0, sizeof(std::shared_ptr<ur::Framebuffer>));
    *fbo = tt::Render::Instance()->Context()->GetFramebuffer();
}

void w_Render_set_fbo()
{
    auto fbo = ((tt::Proxy<ur::Framebuffer>*)ves_toforeign(1))->obj;
    tt::Render::Instance()->Context()->SetFramebuffer(fbo);
}

void w_Render_get_viewport()
{
    int x, y, w, h;
    tt::Render::Instance()->Context()->GetViewport(x, y, w, h);

    ves_pop(1);
    ves_newlist(4);

    ves_pushnumber(x);
    ves_seti(-2, 0);
    ves_pop(1);

    ves_pushnumber(y);
    ves_seti(-2, 1);
    ves_pop(1);

    ves_pushnumber(w);
    ves_seti(-2, 2);
    ves_pop(1);

    ves_pushnumber(h);
    ves_seti(-2, 3);
    ves_pop(1);
}

void w_Render_set_viewport()
{
    ves_geti(1, 0);
    int x = (int)ves_tonumber(-1);
    ves_pop(1);

    ves_geti(1, 1);
    int y = (int)ves_tonumber(-1);
    ves_pop(1);

    ves_geti(1, 2);
    int w = (int)ves_tonumber(-1);
    ves_pop(1);

    ves_geti(1, 3);
    int h = (int)ves_tonumber(-1);
    ves_pop(1);

    tt::Render::Instance()->Context()->SetViewport(x, y, w, h);
}

std::string parse_spir_type(const spirv_cross::SPIRType& type)
{
    std::string ret = "unknown";

	switch (type.basetype)
	{
	case spirv_cross::SPIRType::BaseType::Boolean:
		if (type.columns == 1 && type.vecsize == 1) {
			ret = "bool";
		}
		break;

	case spirv_cross::SPIRType::BaseType::Int:
		switch (type.vecsize) 
		{
		case 1: 
			ret = "int";
			break;
		case 2: 
			ret = "int2";
			break;
		case 3: 
			ret = "int3";
			break;
		case 4: 
			ret = "int4";
			break;
		}
		break;

    case spirv_cross::SPIRType::BaseType::UInt:
        switch (type.vecsize)
        {
        case 1:
            ret = "int";
            break;
        case 2:
            ret = "int2";
            break;
        case 3:
            ret = "int3";
            break;
        case 4:
            ret = "int4";
            break;
        }
        break;

	case spirv_cross::SPIRType::BaseType::Float:
		switch (type.columns) 
		{
		case 1:
			switch (type.vecsize) 
			{
			case 1: 
				ret = "float";
				break;
			case 2: 
				ret = "float2";
				break;
			case 3: 
				ret = "float3";
				break;
			case 4: 
				ret = "float4";
				break;
			}
			break;

		case 2:
			if (type.vecsize == 2) {
				ret = "mat2";
			}
			break;
				
		case 3:
			if (type.vecsize == 3) {
				ret = "mat3";
			}
			break;

		case 4:
			if (type.vecsize == 4) {
				ret = "mat4";
			}
			break;
		}
		break;
	}
    GD_ASSERT(ret != "unknown", "unknown type");
	return ret;
}

void get_struct_uniforms(const spirv_cross::CompilerGLSL& compiler, 
                         spirv_cross::TypeID base_type_id,
                         const spirv_cross::SPIRType& type,
                         std::vector<std::pair<std::string, std::string>>& uniforms,
                         const std::string& base_name)
{   
    auto member_count = type.member_types.size();
    if (!type.array.empty()) 
    {
        for (int i = 0, n = type.array[0]; i < n; ++i)
        {
            for (int j = 0; j < member_count; j++)
            {
                std::string full_name = base_name + "[" + std::to_string(i) + "]";
                auto name = compiler.get_member_name(type.parent_type, j);
                full_name.append("." + name);
                auto sub_type = compiler.get_type(type.member_types[j]);
                if (sub_type.basetype == spirv_cross::SPIRType::Struct)
                {
                    get_struct_uniforms(compiler, type.member_types[j], sub_type, uniforms, full_name);
                }
                else
                {
                    std::pair<std::string, std::string> unif;

                    unif.first = full_name;
                    unif.second = parse_spir_type(sub_type);

                    //size_t size = compiler.get_declared_struct_member_size(type, j);
                    //size_t offset = compiler.type_struct_member_offset(type, j);

                    uniforms.push_back(unif);
                }
            }
        }
    }
    else
    {
        for (int i = 0; i < member_count; i++)
        {
            auto name = compiler.get_member_name(base_type_id, i);
            if (!base_name.empty()) {
                name.insert(0, base_name + ".");
            }
            auto sub_type = compiler.get_type(type.member_types[i]);
            if (sub_type.basetype == spirv_cross::SPIRType::Struct)
            {
                get_struct_uniforms(compiler, type.member_types[i], sub_type, uniforms, name);
            }
            else if (!sub_type.array.empty())
            {
                for (int i = 0, n = sub_type.array[0]; i < n; ++i)
                {
                    std::string full_name = name + "[" + std::to_string(i) + "]";

                    std::pair<std::string, std::string> unif;

                    unif.first = full_name;
                    unif.second = parse_spir_type(sub_type);

                    uniforms.push_back(unif);
                }
            }
            else
            {
                std::pair<std::string, std::string> unif;

                unif.first = name;
                unif.second = parse_spir_type(sub_type);


                uniforms.push_back(unif);
            }
        }
    }
}

void get_shader_uniforms(EShLanguage stage, const std::vector<unsigned int>& shader,
                         std::vector<std::pair<std::string, std::string>>& uniforms)
{
    spirv_cross::CompilerGLSL compiler(shader);
    spirv_cross::ShaderResources resources = compiler.get_shader_resources();

	//// get_work_group_size_specialization_constants
	//auto entries = compiler.get_entry_points_and_stages();
	//for (auto& e : entries)
	//{
	//	if (e.execution_model == spv::ExecutionModelGLCompute)
	//	{
	//		const auto& spv_entry = compiler.get_entry_point(e.name, e.execution_model);
	//		m_props.insert({ "local_size_x", ShaderVariant(static_cast<int>(spv_entry.workgroup_size.x)) });
	//		m_props.insert({ "local_size_y", ShaderVariant(static_cast<int>(spv_entry.workgroup_size.y)) });
	//		m_props.insert({ "local_size_z", ShaderVariant(static_cast<int>(spv_entry.workgroup_size.z)) });
	//		break;
	//	}
	//}

	// uniforms
    for (auto& resource : resources.uniform_buffers)
    {
        auto ubo_name = compiler.get_name(resource.id);;
        spirv_cross::SPIRType type = compiler.get_type(resource.base_type_id);
        if (type.basetype == spirv_cross::SPIRType::Struct) {
            get_struct_uniforms(compiler, resource.base_type_id, type, uniforms, ubo_name);
        }

		uint32_t set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
		uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);		
    }

	for (auto& resource : resources.sampled_images)
	{
        std::pair<std::string, std::string> unif;

		spirv_cross::SPIRType type = compiler.get_type(resource.base_type_id);

		unif.first = compiler.get_name(resource.id);
		uint32_t set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
		uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);

		// todo
        unif.second = "sampler";

		uniforms.push_back(unif);
	}

	//for (auto& resource : resources.storage_images)
	//{
	//	ImageUnit img;

	//	spirv_cross::SPIRType type = compiler.get_type(resource.base_type_id);

	//	img.name = compiler.get_name(resource.id);
	//	uint32_t set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
	//	img.unit = compiler.get_decoration(resource.id, spv::DecorationBinding);
	//	img.fmt = parse_image_format(type.image.format);
	//	img.access = parse_image_access(type.image.access);

	//	m_images.push_back(img);

	//	Variable unif;
	//	unif.name = img.name;
	//	unif.type = VariableType::Sampler2D;
	//	m_uniforms.push_back(unif);
	//}
}

void get_shader_uniforms(const char* stage_str, const char* shader_str, const char* lang_str,
                         std::vector<std::pair<std::string, std::string>>& uniforms)
{
    EShLanguage stage;
    shadertrans::ShaderStage st_stage;
    if (strcmp(stage_str, "vertex") == 0) {
        stage = EShLangVertex;
        st_stage = shadertrans::ShaderStage::VertexShader;
    } else if (strcmp(stage_str, "tess_ctrl") == 0) {
        stage = EShLangTessControl;
        st_stage = shadertrans::ShaderStage::TessCtrlShader;
    } else if (strcmp(stage_str, "tess_eval") == 0) {
        stage = EShLangTessEvaluation;
        st_stage = shadertrans::ShaderStage::TessEvalShader;
    } else if (strcmp(stage_str, "geometry") == 0) {
        stage = EShLangGeometry;
        st_stage = shadertrans::ShaderStage::GeometryShader;
    } else if (strcmp(stage_str, "pixel") == 0) {
        stage = EShLangFragment;
        st_stage = shadertrans::ShaderStage::PixelShader;
    } else if (strcmp(stage_str, "compute") == 0) {
        stage = EShLangCompute;
        st_stage = shadertrans::ShaderStage::ComputeShader;
    } else {
        return;
    }

    std::vector<unsigned int> spirv;
    if (strcmp(lang_str, "glsl") == 0) {
        shadertrans::ShaderTrans::GLSL2SpirV(st_stage, shader_str, spirv);
    } else if (strcmp(lang_str, "hlsl") == 0) {
        shadertrans::ShaderTrans::HLSL2SpirV(st_stage, shader_str, spirv);
    }

    if (!spirv.empty()) {
        get_shader_uniforms(stage, spirv, uniforms);
    }
}

void w_Render_getShaderUniforms()
{
    const char* stage = ves_tostring(1);
    const char* code  = ves_tostring(2);
    const char* lang  = ves_tostring(3);

    std::vector<std::pair<std::string, std::string>> uniforms;
    if (strlen(code) != 0) {
        get_shader_uniforms(stage, code, lang, uniforms);
    }

    ves_pop(4);
    ves_newlist(static_cast<int>(uniforms.size()));
    for (int i = 0, n = static_cast<int>(uniforms.size()); i < n; ++i)
    {
        auto& unif = uniforms[i];

        int size = 2;
        if (unif.second == "sampler") {
            size += 2;  // texture and sampler
        } else if (unif.second == "mat2" || unif.second == "mat3" || unif.second == "mat4") {
            size += 1;
        } else {
            size += get_value_number_size(unif.second);
        }

        ves_newlist(size);

        // name
        ves_pushstring(unif.first.c_str());
        ves_seti(-2, 0);
        ves_pop(1);

        // type
        ves_pushstring(unif.second.c_str());
        ves_seti(-2, 1);
        ves_pop(1);

        // values
        if (unif.second == "sampler")
        {
        }
        else if (unif.second == "mat2" || unif.second == "mat3" || unif.second == "mat4")
        {
        }
        else
        {
            for (int i = 0, n = get_value_number_size(unif.second); i < n; ++i) {
                ves_pushnumber(0);
                ves_seti(-2, 2 + i);
                ves_pop(1);
            }
        }

        ves_seti(-2, i);
        ves_pop(1);
    }
}

}

namespace tt
{

VesselForeignMethodFn RenderBindMethod(const char* signature)
{
    if (strcmp(signature, "Shader.setUniformValue(_)") == 0) return w_Shader_setUniformValue;

    if (strcmp(signature, "Texture2D.getWidth()") == 0) return w_Texture2D_getWidth;
    if (strcmp(signature, "Texture2D.getHeight()") == 0) return w_Texture2D_getHeight;

    if (strcmp(signature, "Cubemap.get_width()") == 0) return w_Cubemap_get_width;
    if (strcmp(signature, "Cubemap.get_height()") == 0) return w_Cubemap_get_height;

    if (strcmp(signature, "Framebuffer.attach_tex(_,_,_,_)") == 0) return w_Framebuffer_attach_tex;
    if (strcmp(signature, "Framebuffer.attach_rbo(_,_)") == 0) return w_Framebuffer_attach_rbo;

    if (strcmp(signature, "static Render.draw(_,_,_,_)") == 0) return w_Render_draw;
    if (strcmp(signature, "static Render.draw_model(_,_,_)") == 0) return w_Render_draw_model;
    if (strcmp(signature, "static Render.compute(_,_,_,_)") == 0) return w_Render_compute;
    if (strcmp(signature, "static Render.clear(_,_)") == 0) return w_Render_clear;
    if (strcmp(signature, "static Render.getShaderUniforms(_,_,_)") == 0) return w_Render_getShaderUniforms;
    if (strcmp(signature, "static Render.get_fbo()") == 0) return w_Render_get_fbo;
    if (strcmp(signature, "static Render.set_fbo(_)") == 0) return w_Render_set_fbo;
    if (strcmp(signature, "static Render.get_viewport()") == 0) return w_Render_get_viewport;
    if (strcmp(signature, "static Render.set_viewport(_)") == 0) return w_Render_set_viewport;

    return NULL;
}

void RenderBindClass(const char* className, VesselForeignClassMethods* methods)
{
    if (strcmp(className, "Shader") == 0)
    {
        methods->allocate = w_Shader_allocate;
        methods->finalize = w_Shader_finalize;
        return;
    }

    if (strcmp(className, "VertexArray") == 0)
    {
        methods->allocate = w_VertexArray_allocate;
        methods->finalize = w_VertexArray_finalize;
        return;
    }

    if (strcmp(className, "Texture2D") == 0)
    {
        methods->allocate = w_Texture2D_allocate;
        methods->finalize = w_Texture2D_finalize;
        return;
    }

    if (strcmp(className, "Cubemap") == 0)
    {
        methods->allocate = w_Cubemap_allocate;
        methods->finalize = w_Cubemap_finalize;
        return;
    }

    if (strcmp(className, "Framebuffer") == 0)
    {
        methods->allocate = w_Framebuffer_allocate;
        methods->finalize = w_Framebuffer_finalize;
        return;
    }

    if (strcmp(className, "RenderBuffer") == 0)
    {
        methods->allocate = w_RenderBuffer_allocate;
        methods->finalize = w_RenderBuffer_finalize;
        return;
    }
}

}