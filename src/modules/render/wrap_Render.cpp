#include "modules/render/wrap_Render.h"
#include "modules/render/Render.h"
#include "modules/image/ImageData.h"
#include "modules/model/Model.h"
#include "modules/script/Proxy.h"
#include "modules/script/TransHelper.h"
#include "modules/maths/float16.h"

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
#include <unirender/ComputeBuffer.h>
#include <unirender/WritePixelBuffer.h>
#include <shadertrans/ShaderTrans.h>
#include <shadertrans/ShaderReflection.h>
#include <shadertrans/ShaderPreprocess.h>
#include <shadertrans/ShaderBuilder.h>
#include <shadertrans/ShaderPreprocess.h>
#include <SM_Matrix.h>
#include <gimg_typedef.h>
#include <guard/check.h>
#include <model/Model.h>

#include <array>

//#define SHADER_DEBUG_PRINT

namespace
{

void read_shader(std::vector<unsigned int>& dst, int src, const char* inc_dir, shadertrans::ShaderStage stage)
{
    if (ves_type(src) == VES_TYPE_STRING) 
    {
        std::string str = ves_tostring(src);
        if (!str.empty()) {
            str = shadertrans::ShaderPreprocess::PrepareGLSL(stage, str);
            shadertrans::ShaderTrans::GLSL2SpirV(stage, str, inc_dir, dst);
        }
    } 
    else 
    {
        auto builder = ((tt::Proxy<shadertrans::ShaderBuilder>*)ves_toforeign(src))->obj;
        dst = builder->Link();
    }
}

void w_Shader_allocate()
{
    std::shared_ptr<ur::ShaderProgram> prog = nullptr;

    int num = ves_argnum();
    if (num == 7)
    {
        const char* inc_dir = ves_tostring(6);

        std::vector<unsigned int> vs, tcs, tes, gs, fs;
        read_shader(vs,  1, inc_dir, shadertrans::ShaderStage::VertexShader);
        read_shader(tcs, 2, inc_dir, shadertrans::ShaderStage::TessCtrlShader);
        read_shader(tes, 3, inc_dir, shadertrans::ShaderStage::TessEvalShader);
        read_shader(gs,  4, inc_dir, shadertrans::ShaderStage::GeometryShader);
        read_shader(fs,  5, inc_dir, shadertrans::ShaderStage::PixelShader);

        if (!vs.empty()) 
        {
#ifdef SHADER_DEBUG_PRINT
            std::string vs_glsl, gs_glsl, fs_glsl;
            shadertrans::ShaderTrans::SpirV2GLSL(shadertrans::ShaderStage::VertexShader, vs, vs_glsl);
            shadertrans::ShaderTrans::SpirV2GLSL(shadertrans::ShaderStage::GeometryShader, gs, gs_glsl);
            shadertrans::ShaderTrans::SpirV2GLSL(shadertrans::ShaderStage::PixelShader, fs, fs_glsl);
            printf("vs:\n%s\ngs:\n%s\nfs:\n%s\n", vs_glsl.c_str(), gs_glsl.c_str(), fs_glsl.c_str());
#endif // SHADER_DEBUG_PRINT
            prog = tt::Render::Instance()->Device()->CreateShaderProgram(vs, fs, tcs, tes, gs);
        }
    }
    else if (num == 3)
    {
        const char* inc_dir = ves_tostring(2);

        std::vector<unsigned int> cs;
        read_shader(cs, 1, inc_dir, shadertrans::ShaderStage::ComputeShader);
            
        if (!cs.empty())
        {
#ifdef SHADER_DEBUG_PRINT
            std::string cs_glsl;
            shadertrans::ShaderTrans::SpirV2GLSL(shadertrans::ShaderStage::ComputeShader, cs, cs_glsl);
            printf("cs:\n%s\nfs:\n%s\n", cs_glsl.c_str(), cs_glsl.c_str());
#endif // SHADER_DEBUG_PRINT
            
            prog = tt::Render::Instance()->Device()->CreateShaderProgram(cs);
        }
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

int get_value_number_size(shadertrans::ShaderReflection::VarType type)
{
    int size = 0;
    switch (type)
    {
    case shadertrans::ShaderReflection::VarType::Float:
        size = 1;
        break;
    case shadertrans::ShaderReflection::VarType::Float2:
        size = 2;
        break;
    case shadertrans::ShaderReflection::VarType::Float3:
        size = 3;
        break;
    case shadertrans::ShaderReflection::VarType::Float4:
        size = 4;
        break;
    case shadertrans::ShaderReflection::VarType::Mat2:
        size = 4;
        break;
    case shadertrans::ShaderReflection::VarType::Mat3:
        size = 9;
        break;
    case shadertrans::ShaderReflection::VarType::Mat4:
        size = 16;
        break;
    case shadertrans::ShaderReflection::VarType::Int:
        size = 1;
        break;
    case shadertrans::ShaderReflection::VarType::Int2:
        size = 2;
        break;
    case shadertrans::ShaderReflection::VarType::Int3:
        size = 3;
        break;
    case shadertrans::ShaderReflection::VarType::Int4:
        size = 4;
        break;
    }
    return size;
}

const char* unif_type_to_string(shadertrans::ShaderReflection::VarType type)
{
    const char* ret = nullptr;
    switch (type)
    {
    case shadertrans::ShaderReflection::VarType::Unknown:
        ret = "unknown";
        break;
    case shadertrans::ShaderReflection::VarType::Array:
        ret = "array";
        break;
    case shadertrans::ShaderReflection::VarType::Struct:
        ret = "struct";
        break;
    case shadertrans::ShaderReflection::VarType::Bool:
        ret = "bool";
        break;
    case shadertrans::ShaderReflection::VarType::Int:
        ret = "int";
        break;
    case shadertrans::ShaderReflection::VarType::Int2:
        ret = "int2";
        break;
    case shadertrans::ShaderReflection::VarType::Int3:
        ret = "int3";
        break;
    case shadertrans::ShaderReflection::VarType::Int4:
        ret = "int4";
        break;
    case shadertrans::ShaderReflection::VarType::Float:
        ret = "float";
        break;
    case shadertrans::ShaderReflection::VarType::Float2:
        ret = "float2";
        break;
    case shadertrans::ShaderReflection::VarType::Float3:
        ret = "float3";
        break;
    case shadertrans::ShaderReflection::VarType::Float4:
        ret = "float4";
        break;
    case shadertrans::ShaderReflection::VarType::Mat2:
        ret = "mat2";
        break;
    case shadertrans::ShaderReflection::VarType::Mat3:
        ret = "mat3";
        break;
    case shadertrans::ShaderReflection::VarType::Mat4:
        ret = "mat4";
        break;
    case shadertrans::ShaderReflection::VarType::Sampler:
        ret = "sampler";
        break;
    case shadertrans::ShaderReflection::VarType::Image:
        ret = "image";
        break;
    case shadertrans::ShaderReflection::VarType::Void:
        ret = "void";
        break;
    default:
        assert(0);
    }
    return ret;
}

shadertrans::ShaderReflection::VarType strint_to_unif_type(const char* type)
{
    auto ret = shadertrans::ShaderReflection::VarType::Unknown;
    if (strcmp(type, "unknown") == 0) {
        ret = shadertrans::ShaderReflection::VarType::Unknown;
    } else if (strcmp(type, "array") == 0) {
        ret = shadertrans::ShaderReflection::VarType::Array;
    } else if (strcmp(type, "struct") == 0) {
        ret = shadertrans::ShaderReflection::VarType::Struct;
    } else if (strcmp(type, "bool") == 0) {
        ret = shadertrans::ShaderReflection::VarType::Bool;
    } else if (strcmp(type, "int") == 0) {
        ret = shadertrans::ShaderReflection::VarType::Int;
    } else if (strcmp(type, "int2") == 0) {
        ret = shadertrans::ShaderReflection::VarType::Int2;
    } else if (strcmp(type, "int3") == 0) {
        ret = shadertrans::ShaderReflection::VarType::Int3;
    } else if (strcmp(type, "int4") == 0) {
        ret = shadertrans::ShaderReflection::VarType::Int4;
    } else if (strcmp(type, "float") == 0) {
        ret = shadertrans::ShaderReflection::VarType::Float;
    } else if (strcmp(type, "float2") == 0) {
        ret = shadertrans::ShaderReflection::VarType::Float2;
    } else if (strcmp(type, "float3") == 0) {
        ret = shadertrans::ShaderReflection::VarType::Float3;
    } else if (strcmp(type, "float4") == 0) {
        ret = shadertrans::ShaderReflection::VarType::Float4;
    } else if (strcmp(type, "mat2") == 0) {
        ret = shadertrans::ShaderReflection::VarType::Mat2;
    } else if (strcmp(type, "mat3") == 0) {
        ret = shadertrans::ShaderReflection::VarType::Mat3;
    } else if (strcmp(type, "mat4") == 0) {
        ret = shadertrans::ShaderReflection::VarType::Mat4;
    } else if (strcmp(type, "sampler") == 0) {
        ret = shadertrans::ShaderReflection::VarType::Sampler;
    } else if (strcmp(type, "image") == 0) {
        ret = shadertrans::ShaderReflection::VarType::Image;
    }
    return ret;
}

void set_uniform_value(const std::shared_ptr<ur::ShaderProgram>& prog, const char* name, shadertrans::ShaderReflection::VarType type)
{
    GD_ASSERT(ves_type(-1) == VES_TYPE_LIST, "value should be list");
    switch (type)
    {
    case shadertrans::ShaderReflection::VarType::Array:
    case shadertrans::ShaderReflection::VarType::Struct:
    {
        int num = ves_len(-1);
        for (int i = 0; i < num; ++i)
        {
            ves_geti(-1, i);

            ves_geti(-1, 0);
            const char* name = ves_tostring(-1);
            ves_pop(1);

            ves_geti(-1, 1);
            const char* type = ves_tostring(-1);
            ves_pop(1);
            GD_ASSERT(strcmp(type, "unknown") != 0, "unknown type");

            ves_geti(-1, 2);
            set_uniform_value(prog, name, strint_to_unif_type(type));
            ves_pop(1);

            ves_pop(1);
        }
    }
        break;
    case shadertrans::ShaderReflection::VarType::Sampler:
    {
        auto ctx = tt::Render::Instance()->Context();
        auto slot = prog->QueryTexSlot(name);
        if (slot >= 0)
        {
            int n = ves_len(-1);

            std::shared_ptr<ur::Texture> tex = nullptr;
            std::shared_ptr<ur::TextureSampler> sampler = nullptr;

            // texture
            if (n > 0)
            {
                ves_geti(-1, 0);
                if (ves_type(-1) != VES_TYPE_NULL) {
                    tex = ((tt::Proxy<ur::Texture>*)ves_toforeign(-1))->obj;
                }
                ves_pop(1);
            }

            // texture sampler
            if (tex && ves_len(-1) > 1)
            {
                ves_geti(-1, 1);
                if (ves_type(-1) != VES_TYPE_NULL) {
                    sampler = ((tt::Proxy<ur::TextureSampler>*)ves_toforeign(-1))->obj;
                }
                ves_pop(1);
            }

            ctx->SetTexture(slot, tex);
            ctx->SetTextureSampler(slot, sampler);
        }
    }
        break;
    case shadertrans::ShaderReflection::VarType::Image:
    {
        auto slot = prog->QueryImgSlot(name);
        if (slot >= 0)
        {
            int n = ves_len(-1);
            if (n > 0)
            {
                ves_geti(-1, 0);
                if (ves_type(-1) != VES_TYPE_NULL)
                {
                    auto tex = ((tt::Proxy<ur::Texture>*)ves_toforeign(-1))->obj;
                    if (slot >= 0) {
                        auto ctx = tt::Render::Instance()->Context();
                        ctx->SetImage(slot, tex, ur::AccessType::WriteOnly);
                    }
                }
                ves_pop(1);
            }
        }
    }
        break;
    case shadertrans::ShaderReflection::VarType::Mat4:
    {
        auto unif = prog->QueryUniform(name);
        if (unif && ves_len(-1) > 0)
        {
            ves_geti(-1, 0);
            if (ves_type(-1) != VES_TYPE_NULL) {
                sm::mat4* mt = static_cast<sm::mat4*>(ves_toforeign(-1));
                unif->SetValue(mt->x, 16);
            }
            ves_pop(1);
        }
    }
        break;
    default:
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
                    ves_geti(-1, i);
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
                    ves_geti(-1, i);
                    val[i] = (float)ves_tonumber(-1);
                    ves_pop(1);
                }
                unif->SetValue(val, num);
            }
        }
    }
    }
}

void w_Shader_set_uniform_value()
{
    auto prog = ((tt::Proxy<ur::ShaderProgram>*)ves_toforeign(0))->obj;
    if (!prog) {
        return;
    }

    GD_ASSERT(ves_type(1) == VES_TYPE_LIST, "unknown type");

    ves_geti(1, 0);
    const char* name = ves_tostring(-1);
    ves_pop(1);

    ves_geti(1, 1);
    const char* type = ves_tostring(-1);
    ves_pop(1);
    GD_ASSERT(strcmp(type, "unknown") != 0, "unknown type");

    ves_geti(1, 2);
    set_uniform_value(prog, name, strint_to_unif_type(type));
    ves_pop(1);
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

        int grids_size = -1;
        if (ves_getfield(3, "grids_size") == VES_TYPE_NUM) {
            grids_size = (int)ves_tonumber(-1);
        }
        ves_pop(1);

        auto dev = tt::Render::Instance()->Device();
        if (type == tt::ShapeType::Grids && grids_size > 0) {
            va = tt::Model::Instance()->CreateGrids(*dev, layout, grids_size);
        } else {
            ur::PrimitiveType prim_type;
            va = tt::Model::Instance()->CreateShape(*dev, type, layout, prim_type);
        }
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
        case GPF_R16:
            tf = ur::TextureFormat::R16;
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

        size_t buf_sz = ur::TextureUtility::RequiredSizeInBytes(img->width, img->height, tf, 1);
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
        } else if (strcmp(format, "rgb16f") == 0) {
            tf = ur::TextureFormat::RGB16F;
        } else if (strcmp(format, "rg16f") == 0) {
            tf = ur::TextureFormat::RG16F;
        } else if (strcmp(format, "r16") == 0) {
            tf = ur::TextureFormat::R16;
        } else if (strcmp(format, "r16f") == 0) {
            tf = ur::TextureFormat::R16F;
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

void w_Texture2D_get_width()
{
    auto tex = ((tt::Proxy<ur::Texture>*)ves_toforeign(0))->obj;
    ves_set_number(0, (double)tex->GetWidth());
}

void w_Texture2D_get_height()
{
    auto tex = ((tt::Proxy<ur::Texture>*)ves_toforeign(0))->obj;
    ves_set_number(0, (double)tex->GetHeight());
}

template<typename T>
void texture2d_upload(ur::Texture& tex, int num, int x, int y, int w, int h)
{
    std::vector<T> data;
    for (int i = 0; i < num; ++i) 
    {
        ves_geti(1, i);
        if (std::is_same<T, unsigned int>::value) {
            data.push_back((unsigned int)ves_tonumber(-1));
        } else if (std::is_same<T, double>::value) {
            data.push_back(ves_tonumber(-1));
        } else if (std::is_same<T, float>::value) {
            data.push_back((float)ves_tonumber(-1));
        } else {
            short f16 = FloatToFloat16((float)ves_tonumber(-1));
            data.push_back(f16);
        }
        ves_pop(1);
    }

    auto dev = tt::Render::Instance()->Device();
    auto pbuf = dev->CreateWritePixelBuffer(ur::BufferUsageHint::DynamicDraw, num * sizeof(T));
    pbuf->ReadFromMemory(data.data(), data.size() * sizeof(T), 0);
    pbuf->Bind();
    tex.Upload(reinterpret_cast<void*>(0), x, y, w, h);
}

void w_Texture2D_upload()
{
    auto tex = ((tt::Proxy<ur::Texture>*)ves_toforeign(0))->obj;
    auto fmt = tex->GetFormat();

    GD_ASSERT(ves_type(1) == VES_TYPE_LIST, "pixels should be list");

    const int num = ves_len(1);

    const float x = ves_tonumber(2);
    const float y = ves_tonumber(3);
    const float w = ves_tonumber(4);
    const float h = ves_tonumber(5);

    switch (fmt)
    {
    case ur::TextureFormat::RGBA8:
        texture2d_upload<unsigned int>(*tex, num, x, y, w, h);
        break;
    case ur::TextureFormat::RGBA16F:
    case ur::TextureFormat::RGB16F:
    case ur::TextureFormat::RG16F:
    case ur::TextureFormat::R16:
    case ur::TextureFormat::R16F:
        texture2d_upload<unsigned short>(*tex, num, x, y, w, h);
        break;
    case ur::TextureFormat::RGBA32F:
    case ur::TextureFormat::RGB32F:
    case ur::TextureFormat::RGB32I:
        texture2d_upload<float>(*tex, num, x, y, w, h);
        break;
    default:
        texture2d_upload<unsigned char>(*tex, num, x, y, w, h);
    }
}

void w_Texture2D_download()
{
    auto tex = ((tt::Proxy<ur::Texture>*)ves_toforeign(0))->obj;
    auto img = (tt::ImageData*)ves_toforeign(1);

    img->width = tex->GetWidth();
    img->height = tex->GetHeight();

    switch (tex->GetFormat())
    {
    case ur::TextureFormat::RGB:
        img->format = GPF_RGB;
        break;
    case ur::TextureFormat::RGBA8:
        img->format = GPF_RGBA8;
        break;
    case ur::TextureFormat::R16F:
        img->format = GPF_R16;
        break;
    default:
        GD_REPORT_ASSERT("unknown type.");
        return;
    }

    size_t sz = ur::TextureUtility::RequiredSizeInBytes(tex->GetWidth(), tex->GetHeight(), tex->GetFormat(), 4);
    img->pixels = (uint8_t*)tex->WriteToMemory(sz);
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
        bool fail = false;
        for (int i = 0; i < 6 && !fail; ++i)
        {
            ves_geti(1, i);
            auto f = ves_toforeign(-1);
            if (!f) {
                fail = true;
            } else {
                textures[i] = ((tt::Proxy<ur::Texture>*)f)->obj;
            }
            ves_pop(1);
        }

        if (!fail) {
            tex = tt::Render::Instance()->Device()->CreateTextureCubeMap(textures);
        }
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
        } else if (strcmp(format, "r16f") == 0) {
            tf = ur::TextureFormat::R16F;
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

void w_TextureSampler_allocate()
{
    std::shared_ptr<ur::TextureSampler> sampler = nullptr;

    auto dev = tt::Render::Instance()->Device();
    const char* type = ves_tostring(1);
    if (strcmp(type, "nearest_clamp") == 0) {
        sampler = dev->GetTextureSampler(ur::Device::TextureSamplerType::NearestClamp);
    } else if (strcmp(type, "linear_clamp") == 0) {
        sampler = dev->GetTextureSampler(ur::Device::TextureSamplerType::LinearClamp);
    } else if (strcmp(type, "linear_clamp_mipmap") == 0) {
        sampler = dev->GetTextureSampler(ur::Device::TextureSamplerType::LinearClampMipmap);
    } else if (strcmp(type, "nearest_repeat") == 0) {
        sampler = dev->GetTextureSampler(ur::Device::TextureSamplerType::NearestRepeat);
    } else if (strcmp(type, "linear_repeat") == 0) {
        sampler = dev->GetTextureSampler(ur::Device::TextureSamplerType::LinearRepeat);
    }

    auto proxy = (tt::Proxy<ur::TextureSampler>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<ur::TextureSampler>));
    proxy->obj = sampler;
}

int w_TextureSampler_finalize(void* data)
{
    auto proxy = (tt::Proxy<ur::TextureSampler>*)(data);
    proxy->~Proxy();
    return sizeof(tt::Proxy<ur::TextureSampler>);
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

void w_ComputeBuffer_allocate()
{
    const char* type = ves_tostring(1);
    const int num = ves_len(2);
    const int id = (int)ves_tonumber(3);
    std::shared_ptr<ur::ComputeBuffer> buf = nullptr;
    if (strcmp(type, "int") == 0)
    {
        std::vector<int> data(num);
        for (int i = 0; i < num; ++i) {
            ves_geti(2, i);
            data[i] = (int)ves_tonumber(-1);
            ves_pop(1);
        }
        buf = tt::Render::Instance()->Device()->CreateComputeBuffer(data.data(), sizeof(int) * data.size(), id);
    }
    else if (strcmp(type, "float") == 0)
    {
        std::vector<float> data(num);
        for (int i = 0; i < num; ++i) {
            ves_geti(2, i);
            data[i] = ves_tonumber(-1);
            ves_pop(1);
        }
        buf = tt::Render::Instance()->Device()->CreateComputeBuffer(data.data(), sizeof(float) * data.size(), id);
    }
    auto proxy = (tt::Proxy<ur::ComputeBuffer>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<ur::ComputeBuffer>));
    proxy->obj = buf;
}

int w_ComputeBuffer_finalize(void* data)
{
    auto proxy = (tt::Proxy<ur::ComputeBuffer>*)(data);
    proxy->~Proxy();
    return sizeof(tt::Proxy<ur::ComputeBuffer>);
}

ur::AttachmentType string2attachment(const std::string& str)
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
    auto atta_type = string2attachment(ves_tostring(2));

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
    auto atta_type = string2attachment(ves_tostring(2));
    fbo->SetAttachment(atta_type, ur::TextureTarget::Texture2D, nullptr, rbo);
}

void w_ComputeBuffer_download()
{
    auto buf = ((tt::Proxy<ur::ComputeBuffer>*)ves_toforeign(0))->obj;
    const char* type = ves_tostring(1);
    const size_t size = (size_t)ves_tonumber(2);
    if (strcmp(type, "int") == 0)
    {
        std::vector<int> data(size);
        buf->GetComputeBufferData(data.data(), sizeof(data) * size);
        tt::return_list(data);
    }
    else if (strcmp(type, "float") == 0)
    {
        std::vector<float> data(size);
        buf->GetComputeBufferData(data.data(), sizeof(float) * size);
        tt::return_list(data);
    }
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

void w_RenderState_allocate()
{
    auto rs = std::make_shared<ur::RenderState>();
    auto proxy = (tt::Proxy<ur::RenderState>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<ur::RenderState>));
    proxy->obj = rs;
}

int w_RenderState_finalize(void* data)
{
    auto proxy = (tt::Proxy<ur::RenderState>*)(data);
    proxy->~Proxy();
    return sizeof(tt::Proxy<ur::RenderState>);
}

ur::StencilOperation string2stencilop(const std::string& str)
{
    ur::StencilOperation op = ur::StencilOperation::Keep;
    if (str == "zero") {
        op = ur::StencilOperation::Zero;
    } else if (str == "invert") {
        op = ur::StencilOperation::Invert;
    } else if (str == "keep") {
        op = ur::StencilOperation::Keep;
    } else if (str == "replace") {
        op = ur::StencilOperation::Replace;
    } else if (str == "increment") {
        op = ur::StencilOperation::Increment;
    } else if (str == "decrement") {
        op = ur::StencilOperation::Decrement;
    } else if (str == "increment_wrap") {
        op = ur::StencilOperation::IncrementWrap;
    } else if (str == "decrement_wrap") {
        op = ur::StencilOperation::DecrementWrap;
    } else {
        GD_REPORT_ASSERT("unknown stencil op.");
    }
    return op;
}

void w_RenderState_stencil_test()
{
    auto rs = ((tt::Proxy<ur::RenderState>*)ves_toforeign(0))->obj;

    rs->stencil_test.enabled = true;

    auto func = ur::StencilTestFunction::Always;
    const char* s_func = ves_tostring(2);
    if (strcmp(s_func, "never") == 0) {
        func = ur::StencilTestFunction::Never;
    } else if (strcmp(s_func, "less") == 0) {
        func = ur::StencilTestFunction::Less;
    } else if (strcmp(s_func, "equal") == 0) {
        func = ur::StencilTestFunction::Equal;
    } else if (strcmp(s_func, "lequal") == 0) {
        func = ur::StencilTestFunction::LessThanOrEqual;
    } else if (strcmp(s_func, "greater") == 0) {
        func = ur::StencilTestFunction::Greater;
    } else if (strcmp(s_func, "notequal") == 0) {
        func = ur::StencilTestFunction::NotEqual;
    } else if (strcmp(s_func, "gequal") == 0) {
        func = ur::StencilTestFunction::GreaterThanOrEqual;
    } else if (strcmp(s_func, "always") == 0) {
        func = ur::StencilTestFunction::Always;
    } else {
        GD_REPORT_ASSERT("unknown stencil func.");
    }

    int ref_val = static_cast<int>(ves_tonumber(3));

    auto fail_op = string2stencilop(ves_tostring(4));
    auto pass_op = string2stencilop(ves_tostring(5));

    const char* s_face = ves_tostring(1);
    if (strcmp(s_face, "front") == 0 || strcmp(s_face, "front_and_back") == 0) {
        rs->stencil_test.front_face.function = func;
        rs->stencil_test.front_face.reference_value = ref_val;
        rs->stencil_test.front_face.depth_fail_stencil_pass_op = fail_op;
        rs->stencil_test.front_face.depth_pass_stencil_pass_op = pass_op;
    }
    if (strcmp(s_face, "back") == 0 || strcmp(s_face, "front_and_back") == 0) {
        rs->stencil_test.back_face.function = func;
        rs->stencil_test.back_face.reference_value = ref_val;
        rs->stencil_test.back_face.depth_fail_stencil_pass_op = fail_op;
        rs->stencil_test.back_face.depth_pass_stencil_pass_op = pass_op;
    }
}

void w_RenderState_z_write()
{
    auto rs = ((tt::Proxy<ur::RenderState>*)ves_toforeign(0))->obj;
    rs->depth_mask = ves_toboolean(1); 
}

void w_RenderState_z_test()
{
    auto rs = ((tt::Proxy<ur::RenderState>*)ves_toforeign(0))->obj;
    rs->depth_test.enabled = ves_toboolean(1);

    auto func = ur::DepthTestFunc::Less;
    const char* s_func = ves_tostring(2);
    if (s_func)
    {
        if (strcmp(s_func, "never") == 0) {
            func = ur::DepthTestFunc::Never;
        } else if (strcmp(s_func, "less") == 0) {
            func = ur::DepthTestFunc::Less;
        } else if (strcmp(s_func, "equal") == 0) {
            func = ur::DepthTestFunc::Equal;
        } else if (strcmp(s_func, "lequal") == 0) {
            func = ur::DepthTestFunc::LessThanOrEqual;
        } else if (strcmp(s_func, "greater") == 0) {
            func = ur::DepthTestFunc::Greater;
        } else if (strcmp(s_func, "notequal") == 0) {
            func = ur::DepthTestFunc::NotEqual;
        } else if (strcmp(s_func, "gequal") == 0) {
            func = ur::DepthTestFunc::GreaterThanOrEqual;
        } else if (strcmp(s_func, "always") == 0) {
            func = ur::DepthTestFunc::Always;
        } else {
            GD_REPORT_ASSERT("unknown stencil func.");
        }
    }
    rs->depth_test.function = func;
}

void w_RenderState_face_culling()
{
    auto rs = ((tt::Proxy<ur::RenderState>*)ves_toforeign(0))->obj;

    const char* s_type = ves_tostring(1);
    if (strcmp(s_type, "disable") == 0) {
        rs->facet_culling.enabled = false;
    } else if (strcmp(s_type, "front") == 0) {
        rs->facet_culling.enabled = true;
        rs->facet_culling.face = ur::CullFace::Front;
    } else if (strcmp(s_type, "back") == 0) {
        rs->facet_culling.enabled = true;
        rs->facet_culling.face = ur::CullFace::Back;
    } else if (strcmp(s_type, "front_and_back") == 0) {
        rs->facet_culling.enabled = true;
        rs->facet_culling.face = ur::CullFace::FrontAndBack;
    }
}

void w_RenderState_rasterization_mode()
{
    auto rs = ((tt::Proxy<ur::RenderState>*)ves_toforeign(0))->obj;

    const char* s_mode = ves_tostring(1);
    if (strcmp(s_mode, "point") == 0) {
        rs->rasterization_mode = ur::RasterizationMode::Point;
    } else if (strcmp(s_mode, "line") == 0) {
        rs->rasterization_mode = ur::RasterizationMode::Line;
    } else if (strcmp(s_mode, "fill") == 0) {
        rs->rasterization_mode = ur::RasterizationMode::Fill;
    }
}

void w_RenderState_clip_plane()
{
    auto rs = ((tt::Proxy<ur::RenderState>*)ves_toforeign(0))->obj;
    rs->clip_plane = ves_toboolean(1);
}

void w_RenderState_blending()
{
    auto rs = ((tt::Proxy<ur::RenderState>*)ves_toforeign(0))->obj;
    rs->blending.enabled = ves_toboolean(1);

    if (rs->blending.enabled)
    {
        rs->blending.enabled = true;
        rs->blending.separately = false;
        rs->blending.src = ur::BlendingFactor::SrcAlpha;
        rs->blending.dst = ur::BlendingFactor::OneMinusSrcAlpha;
        rs->blending.equation = ur::BlendEquation::Add;
    }
}

void w_RenderState_prim_restart()
{
    auto rs = ((tt::Proxy<ur::RenderState>*)ves_toforeign(0))->obj;
    rs->prim_restart.enabled = ves_toboolean(1);
    if (rs->prim_restart.enabled) {
        rs->prim_restart.index = 0xffff;
    }
}

void w_RenderState_depth_clamp()
{
    auto rs = ((tt::Proxy<ur::RenderState>*)ves_toforeign(0))->obj;
    rs->depth_clamp = ves_toboolean(1);
}

ur::PrimitiveType get_prim_type(const char* str)
{
    ur::PrimitiveType ret;
    if (strcmp(str, "points") == 0) {
        ret = ur::PrimitiveType::Points;
    } else if (strcmp(str, "lines") == 0) {
        ret = ur::PrimitiveType::Lines;
    } else if (strcmp(str, "line_strip") == 0) {
        ret = ur::PrimitiveType::LineStrip;
    } else if (strcmp(str, "triangles") == 0) {
        ret = ur::PrimitiveType::Triangles;
    } else if (strcmp(str, "tri_strip") == 0) {
        ret = ur::PrimitiveType::TriangleStrip;
    } else if (strcmp(str, "tri_adjacency") == 0) {
        ret = ur::PrimitiveType::TrianglesAdjacency;
    } else if (strcmp(str, "patches") == 0) {
        ret = ur::PrimitiveType::Patches;
    } else {
        GD_REPORT_ASSERT("unknown prim type.");
    }
    return ret;
}

void w_Render_draw()
{
    ur::DrawState ds;

    const char* prim_type_str = ves_tostring(1);
    auto prim_type = get_prim_type(prim_type_str);

    ds.program = ((tt::Proxy<ur::ShaderProgram>*)ves_toforeign(2))->obj;
    ds.vertex_array = ((tt::Proxy<ur::VertexArray>*)ves_toforeign(3))->obj;
    ds.render_state = *((tt::Proxy<ur::RenderState>*)ves_toforeign(4))->obj;

    tt::Render::Instance()->Context()->Draw(prim_type, ds, nullptr);
}

void w_Render_draw_instanced()
{
    ur::DrawState ds;

    const char* prim_type_str = ves_tostring(1);
    auto prim_type = get_prim_type(prim_type_str);

    ds.program = ((tt::Proxy<ur::ShaderProgram>*)ves_toforeign(2))->obj;
    ds.vertex_array = ((tt::Proxy<ur::VertexArray>*)ves_toforeign(3))->obj;

    std::vector<sm::mat4> mats;
    tt::list_to_foreigns(5, mats);
    ds.num_instances = mats.size();

    ds.render_state = *((tt::Proxy<ur::RenderState>*)ves_toforeign(4))->obj;

    tt::Render::Instance()->Context()->Draw(prim_type, ds, nullptr);
}

void draw_mesh(ur::DrawState& ds, const model::Model& model, const model::Model::Mesh& mesh)
{
    auto ctx = tt::Render::Instance()->Context();

    auto& geo = mesh.geometry;
    ds.vertex_array = geo.vertex_array;
    GD_ASSERT(geo.sub_geometries.size() == geo.sub_geometry_materials.size(), "error size.");
    for (int i = 0, n = geo.sub_geometries.size(); i < n; ++i)
    {
        auto& mat = model.materials[geo.sub_geometry_materials[i]];
        if (ds.program)
        {
            if (mat->diffuse_tex >= 0) {
                int slot = ds.program->QueryTexSlot("texture_diffuse1");
                if (slot >= 0) {
                    ctx->SetTexture(slot, model.textures[mat->diffuse_tex].second);
                    ctx->SetTextureSampler(slot, nullptr);
                }
            }
            if (mat->normal_tex >= 0) {
                int slot = ds.program->QueryTexSlot("tex_normal");
                if (slot >= 0) {
                    ctx->SetTexture(slot, model.textures[mat->normal_tex].second);
                    ctx->SetTextureSampler(slot, nullptr);
                }
            }
            if (mat->occlusion_tex >= 0) {
                int slot = ds.program->QueryTexSlot("tex_occlusion");
                if (slot >= 0) {
                    ctx->SetTexture(slot, model.textures[mat->occlusion_tex].second);
                    ctx->SetTextureSampler(slot, nullptr);
                }
            }
            if (mat->emissive_tex >= 0) {
                int slot = ds.program->QueryTexSlot("tex_emissive");
                if (slot >= 0) {
                    ctx->SetTexture(slot, model.textures[mat->emissive_tex].second);
                    ctx->SetTextureSampler(slot, nullptr);
                }
            }
            if (mat->metallic_roughness_tex >= 0) {
                int slot = ds.program->QueryTexSlot("tex_metallic_roughness");
                if (slot >= 0) {
                    ctx->SetTexture(slot, model.textures[mat->metallic_roughness_tex].second);
                    ctx->SetTextureSampler(slot, nullptr);
                }
            }
        }
        ds.offset = geo.sub_geometries[i].offset;
        ds.count = geo.sub_geometries[i].count;
        ctx->Draw(ur::PrimitiveType::Triangles, ds, nullptr);
    }
}

void w_Render_draw_model()
{
    auto prog = ((tt::Proxy<ur::ShaderProgram>*)ves_toforeign(1))->obj;
    if (!prog) {
        return;
    }

    ur::DrawState ds;
    ds.program = prog;
    ds.render_state = *((tt::Proxy<ur::RenderState>*)ves_toforeign(3))->obj;

    auto model = ((tt::Proxy<model::Model>*)ves_toforeign(2))->obj;
    if (!model->nodes.empty()) 
    {
        for (auto& node : model->nodes) {
            draw_mesh(ds, *model, *model->meshes[node->mesh]);
        }
    } 
    else 
    {
        for (auto& mesh : model->meshes) {
            draw_mesh(ds, *model, *mesh);
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
    ves_import_class("render", "Framebuffer");
    std::shared_ptr<ur::Framebuffer>* fbo = (std::shared_ptr<ur::Framebuffer>*)ves_set_newforeign(0, 1, sizeof(std::shared_ptr<ur::Framebuffer>));
    ves_pop(1);
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
    tt::return_list(std::vector<int>{ x, y, w, h });
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

void w_Render_push_debug_group()
{
    const char* msg = ves_tostring(1);
    tt::Render::Instance()->Device()->PushDebugGroup(msg);
}

void w_Render_pop_debug_group()
{
    tt::Render::Instance()->Device()->PopDebugGroup();
}

std::vector<unsigned int> shader_string_to_spirv(const char* stage_str, const char* shader_str, const char* inc_dir, const char* entry_point, const char* lang_str, bool no_link)
{
    std::vector<unsigned int> spirv;

    shadertrans::ShaderStage stage;
    if (strcmp(stage_str, "vertex") == 0) {
        stage = shadertrans::ShaderStage::VertexShader;
    } else if (strcmp(stage_str, "tess_ctrl") == 0) {
        stage = shadertrans::ShaderStage::TessCtrlShader;
    } else if (strcmp(stage_str, "tess_eval") == 0) {
        stage = shadertrans::ShaderStage::TessEvalShader;
    } else if (strcmp(stage_str, "geometry") == 0) {
        stage = shadertrans::ShaderStage::GeometryShader;
    } else if (strcmp(stage_str, "pixel") == 0) {
        stage = shadertrans::ShaderStage::PixelShader;
    } else if (strcmp(stage_str, "compute") == 0) {
        stage = shadertrans::ShaderStage::ComputeShader;
    } else {
        return spirv;
    }

    if (strcmp(lang_str, "glsl") == 0) {
        auto code = shadertrans::ShaderPreprocess::PrepareGLSL(stage, shader_str);
        shadertrans::ShaderTrans::GLSL2SpirV(stage, code, inc_dir, spirv, no_link);
    }  else if (strcmp(lang_str, "hlsl") == 0) {
        shadertrans::ShaderTrans::HLSL2SpirV(stage, shader_str, entry_point, spirv);
    }

    return spirv;
}

void get_shader_uniforms(const char* stage_str, const char* shader_str, const char* inc_dir, const char* entry_point, const char* lang_str,
                         std::vector<shadertrans::ShaderReflection::Variable>& uniforms)
{
    auto spirv = shader_string_to_spirv(stage_str, shader_str, inc_dir, entry_point, lang_str, true);
    if (!spirv.empty()) {
        shadertrans::ShaderReflection::GetUniforms(spirv, uniforms);
    }
}

int count_uniform_num(const std::vector<shadertrans::ShaderReflection::Variable>& uniforms)
{
    int num = 0;
    for (auto& unif : uniforms)
    {
        if (unif.type == shadertrans::ShaderReflection::VarType::Array || 
            unif.type == shadertrans::ShaderReflection::VarType::Struct) {
            const int n = count_uniform_num(unif.children);
            num += n;
        } else {
            assert(unif.children.empty());
            ++num;
        }
    }
    return num;
}

void push_variants(const std::vector<shadertrans::ShaderReflection::Variable>& variants)
{
    ves_newlist(variants.size());

    for (int i = 0, n = variants.size(); i < n; ++i)
    {
        // name + type + value[]
        ves_newlist(3);

        auto& var = variants[i];
        if (var.type == shadertrans::ShaderReflection::VarType::Array || 
            var.type == shadertrans::ShaderReflection::VarType::Struct)
        {
            // name
            ves_pushstring(var.name.c_str());
            ves_seti(-2, 0);
            ves_pop(1);

            // type
            ves_pushstring(unif_type_to_string(var.type));
            ves_seti(-2, 1);
            ves_pop(1);

            // value
            push_variants(var.children);
            ves_seti(-2, 2);
            ves_pop(1);
        }
        else 
        {
            assert(var.children.empty());

            // name
            ves_pushstring(var.name.c_str());
            ves_seti(-2, 0);
            ves_pop(1);

            // type
            ves_pushstring(unif_type_to_string(var.type));
            ves_seti(-2, 1);
            ves_pop(1);

            // value
            if (var.type == shadertrans::ShaderReflection::VarType::Sampler ||
                var.type == shadertrans::ShaderReflection::VarType::Image)
            {
                ves_newlist(0);
            }
            else if (var.type == shadertrans::ShaderReflection::VarType::Mat2 || 
                     var.type == shadertrans::ShaderReflection::VarType::Mat3 || 
                     var.type == shadertrans::ShaderReflection::VarType::Mat4)
            {
                ves_newlist(0);
            }
            else
            {
                const int num = get_value_number_size(var.type);
                ves_newlist(num);
                for (int i = 0; i < num; ++i) {
                    ves_pushnumber(0);
                    ves_seti(-2, i);
                    ves_pop(1);
                }
            }
            ves_seti(-2, 2);
            ves_pop(1);
        }
        ves_seti(-2, i);
        ves_pop(1);
    }
}

void w_Render_get_shader_uniforms()
{
    const char* stage       = ves_tostring(1);
    const char* code        = ves_tostring(2);
    const char* inc_dir     = ves_tostring(3);
    const char* lang        = ves_tostring(4);
    const char* entry_point = ves_tostring(5);

    std::vector<shadertrans::ShaderReflection::Variable> uniforms;
    if (strlen(code) != 0) {
        get_shader_uniforms(stage, code, inc_dir, entry_point, lang, uniforms);
    }

    ves_pop(ves_argnum());

    push_variants(uniforms);
} 

void w_Render_get_shader_func_argus()
{
    const char* stage   = ves_tostring(1);
    const char* code    = ves_tostring(2);
    const char* inc_dir = ves_tostring(3);
    const char* lang    = ves_tostring(4);
    const char* name    = ves_tostring(5);

    auto spirv = shader_string_to_spirv(stage, code, inc_dir, name, lang, true);
    if (spirv.empty()) {
        ves_set_nil(0);
        return;
    }

    shadertrans::ShaderReflection::Function func;
    bool success = shadertrans::ShaderReflection::GetFunction(spirv, name, func);
    if (!success) {
        ves_set_nil(0);
        return;
    }
    
    ves_pop(ves_argnum());

    auto vars = func.arguments;
    vars.push_back(func.ret_type);
    push_variants(vars);
}

}

namespace tt
{

VesselForeignMethodFn RenderBindMethod(const char* signature)
{
    if (strcmp(signature, "Shader.set_uniform_value(_)") == 0) return w_Shader_set_uniform_value;

    if (strcmp(signature, "Texture2D.get_width()") == 0) return w_Texture2D_get_width;
    if (strcmp(signature, "Texture2D.get_height()") == 0) return w_Texture2D_get_height;
    if (strcmp(signature, "Texture2D.upload(_,_,_,_,_)") == 0) return w_Texture2D_upload;
    if (strcmp(signature, "Texture2D.download(_)") == 0) return w_Texture2D_download;

    if (strcmp(signature, "Cubemap.get_width()") == 0) return w_Cubemap_get_width;
    if (strcmp(signature, "Cubemap.get_height()") == 0) return w_Cubemap_get_height;

    if (strcmp(signature, "Framebuffer.attach_tex(_,_,_,_)") == 0) return w_Framebuffer_attach_tex;
    if (strcmp(signature, "Framebuffer.attach_rbo(_,_)") == 0) return w_Framebuffer_attach_rbo;

    if (strcmp(signature, "ComputeBuffer.download(_,_)") == 0) return w_ComputeBuffer_download;

    if (strcmp(signature, "RenderState.stencil_test(_,_,_,_,_)") == 0) return w_RenderState_stencil_test;
    if (strcmp(signature, "RenderState.z_write(_)") == 0) return w_RenderState_z_write;
    if (strcmp(signature, "RenderState.z_test(_,_)") == 0) return w_RenderState_z_test;
    if (strcmp(signature, "RenderState.face_culling(_)") == 0) return w_RenderState_face_culling;
    if (strcmp(signature, "RenderState.rasterization_mode(_)") == 0) return w_RenderState_rasterization_mode;
    if (strcmp(signature, "RenderState.clip_plane(_)") == 0) return w_RenderState_clip_plane;
    if (strcmp(signature, "RenderState.blending(_)") == 0) return w_RenderState_blending;
    if (strcmp(signature, "RenderState.prim_restart(_)") == 0) return w_RenderState_prim_restart;
    if (strcmp(signature, "RenderState.depth_clamp(_)") == 0) return w_RenderState_depth_clamp;

    if (strcmp(signature, "static Render.draw(_,_,_,_)") == 0) return w_Render_draw;
    if (strcmp(signature, "static Render.draw_instanced(_,_,_,_,_)") == 0) return w_Render_draw_instanced;
    if (strcmp(signature, "static Render.draw_model(_,_,_)") == 0) return w_Render_draw_model;
    if (strcmp(signature, "static Render.compute(_,_,_,_)") == 0) return w_Render_compute;
    if (strcmp(signature, "static Render.clear(_,_)") == 0) return w_Render_clear;
    if (strcmp(signature, "static Render.get_shader_uniforms(_,_,_,_,_)") == 0) return w_Render_get_shader_uniforms;
    if (strcmp(signature, "static Render.get_shader_func_argus(_,_,_,_,_)") == 0) return w_Render_get_shader_func_argus;
    if (strcmp(signature, "static Render.get_fbo()") == 0) return w_Render_get_fbo;
    if (strcmp(signature, "static Render.set_fbo(_)") == 0) return w_Render_set_fbo;
    if (strcmp(signature, "static Render.get_viewport()") == 0) return w_Render_get_viewport;
    if (strcmp(signature, "static Render.set_viewport(_)") == 0) return w_Render_set_viewport;
    if (strcmp(signature, "static Render.push_debug_group(_)") == 0) return w_Render_push_debug_group;
    if (strcmp(signature, "static Render.pop_debug_group()") == 0) return w_Render_pop_debug_group;

    return NULL;
}

void RenderBindClass(const char* class_name, VesselForeignClassMethods* methods)
{
    if (strcmp(class_name, "Shader") == 0)
    {
        methods->allocate = w_Shader_allocate;
        methods->finalize = w_Shader_finalize;
        return;
    }

    if (strcmp(class_name, "VertexArray") == 0)
    {
        methods->allocate = w_VertexArray_allocate;
        methods->finalize = w_VertexArray_finalize;
        return;
    }

    if (strcmp(class_name, "Texture2D") == 0)
    {
        methods->allocate = w_Texture2D_allocate;
        methods->finalize = w_Texture2D_finalize;
        return;
    }

    if (strcmp(class_name, "Cubemap") == 0)
    {
        methods->allocate = w_Cubemap_allocate;
        methods->finalize = w_Cubemap_finalize;
        return;
    }

    if (strcmp(class_name, "TextureSampler") == 0)
    {
        methods->allocate = w_TextureSampler_allocate;
        methods->finalize = w_TextureSampler_finalize;
        return;
    }

    if (strcmp(class_name, "Framebuffer") == 0)
    {
        methods->allocate = w_Framebuffer_allocate;
        methods->finalize = w_Framebuffer_finalize;
        return;
    }

    if (strcmp(class_name, "ComputeBuffer") == 0)
    {
        methods->allocate = w_ComputeBuffer_allocate;
        methods->finalize = w_ComputeBuffer_finalize;
        return;
    }

    if (strcmp(class_name, "RenderBuffer") == 0)
    {
        methods->allocate = w_RenderBuffer_allocate;
        methods->finalize = w_RenderBuffer_finalize;
        return;
    }

    if (strcmp(class_name, "RenderState") == 0)
    {
        methods->allocate = w_RenderState_allocate;
        methods->finalize = w_RenderState_finalize;
        return;
    }
}

}