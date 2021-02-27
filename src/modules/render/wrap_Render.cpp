#include "modules/render/wrap_Render.h"
#include "modules/render/Render.h"
#include "modules/image/ImageData.h"

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
#include <shadertrans/ShaderTrans.h>
#include <SM_Matrix.h>
#include <gimg_typedef.h>

#include <glslang/glslang/Public/ShaderLang.h>

#include <spirv-cross/spirv.hpp>
#include <spirv-cross/spirv_glsl.hpp>

#include <assert.h>

//#define SHADER_DEBUG_PRINT

namespace
{

void w_Shader_allocate()
{
    const char* vs_str = ves_tostring(1);
    const char* fs_str = ves_tostring(2);

    if (strlen(fs_str) == 0)
    {
        std::vector<unsigned int> cs;
        shadertrans::ShaderTrans::GLSL2SpirV(shadertrans::ShaderStage::ComputeShader, vs_str, cs);

#ifdef SHADER_DEBUG_PRINT
        std::string cs_glsl;
        shadertrans::ShaderTrans::SpirV2GLSL(shadertrans::ShaderStage::ComputeShader, cs, cs_glsl);
        printf("cs:\n%s\nfs:\n%s\n", cs_glsl.c_str(), cs_glsl.c_str());
#endif // SHADER_DEBUG_PRINT

        std::shared_ptr<ur::ShaderProgram>* prog = (std::shared_ptr<ur::ShaderProgram>*)ves_set_newforeign(0, 0, sizeof(std::shared_ptr<ur::ShaderProgram>));
        *prog = tt::Render::Instance()->Device()->CreateShaderProgram(cs);
    }
    else
    {
        std::vector<unsigned int> vs, fs;
        shadertrans::ShaderTrans::GLSL2SpirV(shadertrans::ShaderStage::VertexShader, vs_str, vs);
        shadertrans::ShaderTrans::GLSL2SpirV(shadertrans::ShaderStage::PixelShader, fs_str, fs);

#ifdef SHADER_DEBUG_PRINT
        std::string vs_glsl, fs_glsl;
        shadertrans::ShaderTrans::SpirV2GLSL(shadertrans::ShaderStage::VertexShader, vs, vs_glsl);
        shadertrans::ShaderTrans::SpirV2GLSL(shadertrans::ShaderStage::PixelShader, fs, fs_glsl);
        printf("vs:\n%s\nfs:\n%s\n", vs_glsl.c_str(), fs_glsl.c_str());
#endif // SHADER_DEBUG_PRINT

        std::shared_ptr<ur::ShaderProgram>* prog = (std::shared_ptr<ur::ShaderProgram>*)ves_set_newforeign(0, 0, sizeof(std::shared_ptr<ur::ShaderProgram>));
        *prog = tt::Render::Instance()->Device()->CreateShaderProgram(vs, fs);
    }
}

static int w_Shader_finalize(void* data)
{
    std::shared_ptr<ur::ShaderProgram>* prog = static_cast<std::shared_ptr<ur::ShaderProgram>*>(data);
    (*prog)->~ShaderProgram();
    return sizeof(std::shared_ptr<ur::ShaderProgram>);
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
    std::shared_ptr<ur::ShaderProgram> prog = *(std::shared_ptr<ur::ShaderProgram>*)ves_toforeign(0);

    assert(ves_type(1) == VES_TYPE_LIST);

    ves_geti(1, 0);
    const char* name = ves_tostring(-1);
    ves_pop(1);

    ves_geti(1, 1);
    const char* type = ves_tostring(-1);
    ves_pop(1);
    assert(strcmp(type, "unknown") != 0);

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
                ur::TexturePtr* tex = static_cast<ur::TexturePtr*>(ves_toforeign(-1));
                if (slot >= 0) {
                    ctx->SetTexture(slot, *tex);
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
                ur::TexturePtr* tex = static_cast<ur::TexturePtr*>(ves_toforeign(-1));
                if (slot >= 0) {
                    ctx->SetImage(slot, *tex, ur::AccessType::WriteOnly);
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
                assert(num <= 4);
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
                assert(num <= 16);
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

    std::shared_ptr<ur::VertexArray>* va = (std::shared_ptr<ur::VertexArray>*)ves_set_newforeign(0, 0, sizeof(std::shared_ptr<ur::VertexArray>));
    *va = dev->CreateVertexArray();

    int vbuf_sz = sizeof(float) * data_num;
    auto vbuf = dev->CreateVertexBuffer(ur::BufferUsageHint::StaticDraw, vbuf_sz);
    vbuf->ReadFromMemory(data.data(), vbuf_sz, 0);
    (*va)->SetVertexBuffer(vbuf);

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
    (*va)->SetVertexBufferAttrs(vbuf_attrs);

    if (!index_data.empty())
    {
        auto ibuf_sz = sizeof(unsigned short) * index_data.size();
        auto ibuf = dev->CreateIndexBuffer(ur::BufferUsageHint::StaticDraw, static_cast<int>(ibuf_sz));
        ibuf->ReadFromMemory(index_data.data(), static_cast<int>(ibuf_sz), 0);
        ibuf->SetCount(static_cast<int>(index_data.size()));
        (*va)->SetIndexBuffer(ibuf);
    }
}

static int w_VertexArray_finalize(void* data)
{
    std::shared_ptr<ur::VertexArray>* va = static_cast<std::shared_ptr<ur::VertexArray>*>(data);
    (*va)->~VertexArray();
    return sizeof(std::shared_ptr<ur::VertexArray>);
}

void w_Texture_allocate()
{
    if (ves_type(1) == VES_TYPE_FOREIGN)
    {
        tt::ImageData* img = (tt::ImageData*)ves_toforeign(1);

        ur::TextureFormat tf;
        size_t bpp = 0; // bytes per pixel
	    switch (img->format)
	    {
	    case GPF_ALPHA: case GPF_LUMINANCE: case GPF_LUMINANCE_ALPHA:
		    tf =  ur::TextureFormat::A8;
            bpp = 1;
		    break;
        case GPF_RED:
            tf =  ur::TextureFormat::RED;
            bpp = 1;
            break;
	    case GPF_RGB:
		    tf =  ur::TextureFormat::RGB;
            bpp = 3;
		    break;
        case GPF_RGB565:
            tf = ur::TextureFormat::RGB565;
            bpp = 2;
            break;
	    case GPF_RGBA8:
		    tf =  ur::TextureFormat::RGBA8;
            bpp = 4;
		    break;
	    case GPF_BGRA_EXT:
		    tf =  ur::TextureFormat::BGRA_EXT;
            bpp = 4;
		    break;
	    case GPF_BGR_EXT:
		    tf =  ur::TextureFormat::BGR_EXT;
            bpp = 3;
		    break;
        case GPF_RGBA16F:
            tf =  ur::TextureFormat::RGBA16F;
            bpp = 4 * 4;
            break;
        case GPF_RGB16F:
            tf =  ur::TextureFormat::RGB16F;
            bpp = 3 * 4;
            break;
        case GPF_RGB32F:
            tf =  ur::TextureFormat::RGB32F;
            bpp = 3 * 4;
            break;
	    case GPF_COMPRESSED_RGBA_S3TC_DXT1_EXT:
		    tf =  ur::TextureFormat::COMPRESSED_RGBA_S3TC_DXT1_EXT;
            bpp = 4;
		    break;
	    case GPF_COMPRESSED_RGBA_S3TC_DXT3_EXT:
		    tf =  ur::TextureFormat::COMPRESSED_RGBA_S3TC_DXT3_EXT;
            bpp = 4;
		    break;
	    case GPF_COMPRESSED_RGBA_S3TC_DXT5_EXT:
		    tf =  ur::TextureFormat::COMPRESSED_RGBA_S3TC_DXT5_EXT;
            bpp = 4;
		    break;
	    default:
		    assert(0);
	    }

        size_t buf_sz = img->width * img->height * bpp;
        ur::TexturePtr* tex = (ur::TexturePtr*)ves_set_newforeign(0, 0, sizeof(ur::TexturePtr));
        *tex = tt::Render::Instance()->Device()->CreateTexture(img->width, img->height, tf, img->pixels, buf_sz);
    }
    else
    {
        int width  = (int)ves_tonumber(1);
        int height = (int)ves_tonumber(2);
        const char* format = ves_tostring(3);
        ur::TextureFormat tf;
        size_t bpp = 0; // bytes per pixel
        if (strcmp(format, "rgb") == 0) {
            tf = ur::TextureFormat::RGB;
            bpp = 3;
        } else if (strcmp(format, "rgba") == 0) {
            tf = ur::TextureFormat::RGBA8;
            bpp = 4;
        } else {
            assert(0);
        }

        size_t buf_sz = width * height * bpp;
        ur::TexturePtr* tex = (ur::TexturePtr*)ves_set_newforeign(0, 0, sizeof(ur::TexturePtr));
        *tex = tt::Render::Instance()->Device()->CreateTexture(width, height, tf, nullptr, buf_sz);
    }
}

static int w_Texture_finalize(void* data)
{
    ur::TexturePtr* tex = static_cast<ur::TexturePtr*>(data);
    (*tex)->~Texture();
    return sizeof(ur::TexturePtr);
}

void w_Texture_getWidth()
{
    ur::TexturePtr* tex = static_cast<ur::TexturePtr*>(ves_toforeign(0));
    ves_set_number(0, (double)(*tex)->GetWidth());
}

void w_Texture_getHeight()
{
    ur::TexturePtr* tex = static_cast<ur::TexturePtr*>(ves_toforeign(0));
    ves_set_number(0, (double)(*tex)->GetHeight());
}

void w_Render_draw()
{
    ur::DrawState ds;

    ur::PrimitiveType prim_type = ur::PrimitiveType::Triangles;

    const char* prim_type_str = ves_tostring(1);
    if (strcmp(prim_type_str, "triangles") == 0) {
        prim_type = ur::PrimitiveType::Triangles;
    }

    ds.program = *static_cast<std::shared_ptr<ur::ShaderProgram>*>(ves_toforeign(2));
    ds.vertex_array = *static_cast<std::shared_ptr<ur::VertexArray>*>(ves_toforeign(3));

    if (ves_getfield(4, "depth_test") == VES_TYPE_BOOL) {
        ds.render_state.depth_test.enabled = ves_toboolean(-1);
    }
    ves_pop(1);

    if (ves_getfield(4, "cull") == VES_TYPE_STRING) 
    {
        const char* mode = ves_tostring(-1);
        if (strcmp(mode, "disable") == 0) {
            ds.render_state.facet_culling.enabled = false;
        } else if (strcmp(mode, "front") == 0) {
            ds.render_state.facet_culling.enabled = true;
            ds.render_state.facet_culling.face = ur::CullFace::Front;
        } else if (strcmp(mode, "back") == 0) {
            ds.render_state.facet_culling.enabled = true;
            ds.render_state.facet_culling.face = ur::CullFace::Back;
        } else if (strcmp(mode, "front_and_back") == 0) {
            ds.render_state.facet_culling.enabled = true;
            ds.render_state.facet_culling.face = ur::CullFace::FrontAndBack;
        }
    }
    ves_pop(1);

    tt::Render::Instance()->Context()->Draw(prim_type, ds, nullptr);
}

void w_Render_compute()
{
    ur::DrawState ds;

    ds.program = *static_cast<std::shared_ptr<ur::ShaderProgram>*>(ves_toforeign(1));

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
            assert(0);
        }
    }
    clear.buffers = static_cast<ur::ClearBuffers>(clear_mask);

    int type = ves_getfield(2, "color");
    if (type == VES_TYPE_LIST)
    {
        bool is01 = true;
        double rgba[4];
        assert(ves_len(-1) == 4);
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
    assert(ret != "unknown");
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

                    size_t size = compiler.get_declared_struct_member_size(type, j);
                    size_t offset = compiler.type_struct_member_offset(type, j);

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
            else
            {
                std::pair<std::string, std::string> unif;

                unif.first = name;
                unif.second = parse_spir_type(sub_type);

                size_t size = compiler.get_declared_struct_member_size(type, i);
                size_t offset = compiler.type_struct_member_offset(type, i);

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
    get_shader_uniforms(stage, code, lang, uniforms);

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

    if (strcmp(signature, "Texture.getWidth()") == 0) return w_Texture_getWidth;
    if (strcmp(signature, "Texture.getHeight()") == 0) return w_Texture_getHeight;

    if (strcmp(signature, "static Render.draw(_,_,_,_)") == 0) return w_Render_draw;
    if (strcmp(signature, "static Render.compute(_,_,_,_)") == 0) return w_Render_compute;
    if (strcmp(signature, "static Render.clear(_,_)") == 0) return w_Render_clear;
    if (strcmp(signature, "static Render.getShaderUniforms(_,_,_)") == 0) return w_Render_getShaderUniforms;

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

    if (strcmp(className, "Texture") == 0)
    {
        methods->allocate = w_Texture_allocate;
        methods->finalize = w_Texture_finalize;
        return;
    }
}

}