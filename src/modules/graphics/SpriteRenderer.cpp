#include "modules/graphics/SpriteRenderer.h"
#include "modules/render/Render.h"

#include <unirender/Device.h>
#include <unirender/Context.h>
#include <unirender/Texture.h>
#include <unirender/VertexArray.h>
#include <unirender/IndexBuffer.h>
#include <unirender/VertexBuffer.h>
#include <unirender/DrawState.h>
#include <unirender/Factory.h>
#include <unirender/UniformBuffer.h>
#include <unirender/ShaderProgram.h>
#include <unirender/Descriptor.h>
#include <unirender/ComponentDataType.h>
#include <unirender/VertexInputAttribute.h>
#include <unirender/UniformUpdater.h>
#include <unirender/Uniform.h>
#include <tessellation/Palette.h>
#include <tessellation/Painter.h>
#include <shadertrans/ShaderTrans.h>

namespace
{

const char* vs = R"(

struct VS_INPUT
{
	float2 position : POSITION;
	float2 texcoord : TEXCOORD;
	float4 color    : COLOR;
};

cbuffer u_mvp
{
	float4x4 model;
	float4x4 view;
	float4x4 projection;
}

struct VS_OUTPUT
{
	float4 position : SV_Position;
	float4 color    : COLOR;
	float2 texcoord : TEXCOORD;
};

VS_OUTPUT main(VS_INPUT IN)
{
	VS_OUTPUT OUT = (VS_OUTPUT)0;

	OUT.color = IN.color;
	OUT.texcoord = IN.texcoord;

    // for vulkan
	//OUT.position = mul(mul(mul(float4(IN.position.x, -IN.position.y, 0.0, 1.0), model), view), projection);
	OUT.position = mul(mul(mul(float4(IN.position.x, IN.position.y, 0.0, 1.0), model), view), projection);

	return OUT;
}

)";

const char* fs = R"(

Texture2D ColorTexture : register(t1);
SamplerState ColorSampler : register(s1);

struct VS_OUTPUT
{
	float4 position : SV_Position;
	float4 color    : COLOR;
	float2 texcoord : TEXCOORD;
};

float4 main(VS_OUTPUT IN) : SV_TARGET
{
	float4 color = ColorTexture.Sample(ColorSampler, IN.texcoord);
	return color * IN.color;
}

)";

struct {
	sm::mat4 model;
	sm::mat4 view;
	sm::mat4 projection;
} ubo_vs;

std::shared_ptr<ur::VertexArray>
CreateVertexArray(const ur::Device& dev)
{
    auto va = dev.CreateVertexArray();

    auto usage = ur::BufferUsageHint::StaticDraw;

    auto ibuf = dev.CreateIndexBuffer(usage, 0);
    va->SetIndexBuffer(ibuf);

    auto vbuf = dev.CreateVertexBuffer(ur::BufferUsageHint::StaticDraw, 0);
    va->SetVertexBuffer(vbuf);

    return va;
}


class ModelMatUpdater : public ur::UniformUpdater
{
public:
	ModelMatUpdater(const ur::ShaderProgram& shader,
		const std::string& name, sm::mat4* mat = nullptr)
		: m_ret_mat(mat) {
		m_uniform = shader.QueryUniform(name);
	}

	virtual ur::UpdaterID UpdaterTypeID() const override {
		return ur::GetUpdaterTypeID<ModelMatUpdater>();
	}

	virtual void Update(const ur::Context& ctx,
		const ur::DrawState& draw, const void* scene = nullptr) override {
		if (m_uniform) {
			m_uniform->SetValue(m_mat.x, 4 * 4);
		}
	}

private:
	std::shared_ptr<ur::Uniform> m_uniform = nullptr;

	sm::mat4 m_mat;

	sm::mat4* m_ret_mat = nullptr;

}; // ModelMatUpdater

class ViewMatUpdater : public ur::UniformUpdater
{
public:
	ViewMatUpdater(const ur::ShaderProgram& shader,
		const std::string& name, sm::mat4* mat = nullptr)
		: m_ret_mat(mat) {
		m_uniform = shader.QueryUniform(name);
		m_view_offset.MakeInvalid();
	}

    virtual ur::UpdaterID UpdaterTypeID() const override {
        return ur::GetUpdaterTypeID<ViewMatUpdater>();
    }

	virtual void Update(const ur::Context& ctx,
		const ur::DrawState& draw, const void* scene = nullptr) override {
	}

	void Update(const sm::vec2& offset, float scale) {
		// for vulkan
		//sm::vec2 revert_y_offset(offset.x, -offset.y);
		sm::vec2 revert_y_offset(offset.x, offset.y);

		if (revert_y_offset == m_view_offset &&
			scale == m_view_scale) {
			return;
		}

		m_view_offset = revert_y_offset;
		m_view_scale  = scale;

		auto s_mat = sm::mat4::Scaled(m_view_scale, m_view_scale, 1);
		auto t_mat = sm::mat4::Translated(m_view_offset.x, m_view_offset.y, 0);
		auto mat = s_mat * t_mat;

		if (m_uniform) {
			m_uniform->SetValue(mat.x, 4 * 4);
		}

		if (m_ret_mat) {
			*m_ret_mat = mat;
		}
	}

private:
    std::shared_ptr<ur::Uniform> m_uniform = nullptr;

    sm::vec2 m_view_offset;
    float    m_view_scale = 0;

    sm::mat4* m_ret_mat = nullptr;

}; // ViewMatUpdater

class ProjectMatUpdater : public ur::UniformUpdater
{
public:
    ProjectMatUpdater(const ur::ShaderProgram& shader,
        const std::string& name, sm::mat4* mat = nullptr)
		: m_ret_mat(mat) {
		m_uniform = shader.QueryUniform(name);
	}

    virtual ur::UpdaterID UpdaterTypeID() const override {
        return ur::GetUpdaterTypeID<ProjectMatUpdater>();
    }

	virtual void Update(const ur::Context& ctx,
		const ur::DrawState& draw, const void* scene = nullptr) override {
	}

	void Update(float width, float height) {
		if (width == m_width &&
			height == m_height) {
			return;
		}

		m_width  = width;
		m_height = height;

		float hw = m_width * 0.5f;
		float hh = m_height * 0.5f;
		auto mat = sm::mat4::Orthographic(-hw, hw, -hh, hh, 1, -1);

		if (m_uniform) {
			m_uniform->SetValue(mat.x, 4 * 4);
		}

		if (m_ret_mat) {
			*m_ret_mat = mat;
		}
	}

private:
    std::shared_ptr<ur::Uniform> m_uniform = nullptr;

    float m_width  = 0;
    float m_height = 0;

    sm::mat4* m_ret_mat = nullptr;

}; // ProjectMatUpdater

}

namespace tt
{

SpriteRenderer::SpriteRenderer()
{
	auto dev = Render::Instance()->Device();
	auto ctx = Render::Instance()->Context();

	m_palette = std::make_shared<tess::Palette>(*dev);
	m_va = CreateVertexArray(*Render::Instance()->Device());
	m_rs = ur::DefaultRenderState2D();

	InitShader(*dev);
	InitRenderer(*dev, *ctx);

	OnCameraUpdate(sm::vec2(0, 0), 1.0f);
}

SpriteRenderer::~SpriteRenderer()
{
}

void SpriteRenderer::OnSize(float width, float height)
{
	auto proj_updater = m_shader->QueryUniformUpdater(ur::GetUpdaterTypeID<ProjectMatUpdater>());
	if (proj_updater) {
		std::static_pointer_cast<ProjectMatUpdater>(proj_updater)->Update(width, height);
	}
}

void SpriteRenderer::OnCameraUpdate(const sm::vec2& offset, float scale)
{
	auto view_updater = m_shader->QueryUniformUpdater(ur::GetUpdaterTypeID<ViewMatUpdater>());
	if (view_updater) {
		std::static_pointer_cast<ViewMatUpdater>(view_updater)->Update(offset, scale);
	}
}

void SpriteRenderer::DrawPainter(ur::Context& ctx, const ur::RenderState& rs,
	                             const tess::Painter& pt, const sm::mat4& mat)
{
	if (pt.IsEmpty()) {
		return;
	}

    if (m_buf.vertices.empty())
    {
        m_rs = rs;
    }
    else
    {
        if (m_rs != rs) {
            Flush(ctx);
            m_rs = rs;
        }
    }

	if (auto palette = pt.GetPalette())
	{
		auto relocated_tex = palette->GetRelocatedTex();
		if (relocated_tex) {
			if (m_tex != relocated_tex) {
				Flush(ctx);
				m_tex = relocated_tex;
			}
			CopyVertexBuffer(mat, pt.GetBuffer());
		}
	}
}

void SpriteRenderer::DrawQuad(ur::Context& ctx, const ur::RenderState& rs, const float* positions,
	                          const float* texcoords, const ur::TexturePtr& tex, uint32_t color)
{
    if (m_buf.vertices.empty())
    {
        m_tex = tex;
        m_rs  = rs;
        m_fbo = ctx.GetFramebuffer();
    }
    else
    {
        if (m_tex != tex || m_rs != rs || m_fbo != ctx.GetFramebuffer())
        {
            Flush(ctx);

            m_tex = tex;
            m_rs  = rs;
            m_fbo = ctx.GetFramebuffer();
        }

        if (m_buf.vertices.size() + 4 >= RenderBuffer<SpriteVertex, unsigned short>::MAX_VERTEX_NUM) {
            Flush(ctx);
        }
    }

	m_buf.Reserve(6, 4);

	m_buf.index_ptr[0] = m_buf.curr_index;
	m_buf.index_ptr[1] = m_buf.curr_index + 1;
	m_buf.index_ptr[2] = m_buf.curr_index + 2;
	m_buf.index_ptr[3] = m_buf.curr_index;
	m_buf.index_ptr[4] = m_buf.curr_index + 2;
	m_buf.index_ptr[5] = m_buf.curr_index + 3;
	m_buf.index_ptr += 6;

	int ptr = 0;
	for (int i = 0; i < 4; ++i)
	{
		auto& v = m_buf.vert_ptr[i];
		v.pos.x = positions[ptr];
		v.pos.y = positions[ptr + 1];
		v.uv.x = texcoords[ptr];
		v.uv.y = texcoords[ptr + 1];
		v.col = color;
		ptr += 2;
	}
	m_buf.vert_ptr += 4;

	m_buf.curr_index += 4;
}

void SpriteRenderer::Flush(ur::Context& ctx)
{
	// ubo
	if (m_uniform_buf) {
		m_uniform_buf->Update(&ubo_vs, sizeof(ubo_vs));
	}

    ctx.SetTexture(0, m_tex);
	ctx.SetTextureSampler(0, nullptr);

    auto fbo = ctx.GetFramebuffer();
    ctx.SetFramebuffer(m_fbo);

	if (m_buf.indices.empty()) {
		return;
	}

    auto ibuf = m_va->GetIndexBuffer();
    if (ibuf)
    {
        auto ibuf_sz = sizeof(unsigned short) * m_buf.indices.size();
		ibuf->SetCount(m_buf.indices.size());
        ibuf->Reserve(ibuf_sz);
        ibuf->ReadFromMemory(m_buf.indices.data(), ibuf_sz, 0);
    }

    auto vbuf_sz = sizeof(SpriteVertex) * m_buf.vertices.size();
    auto vbuf = m_va->GetVertexBuffer();
    vbuf->Reserve(vbuf_sz);
    vbuf->ReadFromMemory(m_buf.vertices.data(), vbuf_sz, 0);

    // todo
    //rc.BindTexture(m_tex_id, 0);
    //shader->SetMat4(MODEL_MAT_NAME, sm::mat4().x);

    ur::DrawState draw;
    draw.render_state    = m_rs;
    draw.program         = m_shader;
    draw.vertex_array    = m_va;
    draw.desc_set        = m_desc_set;
    draw.pipeline_layout = m_pipeline_layout;
    draw.pipeline        = m_pipeline;
    ctx.Draw(ur::PrimitiveType::Triangles, draw, nullptr);

	m_buf.Clear();

    ctx.SetFramebuffer(fbo);
}

void SpriteRenderer::InitShader(const ur::Device& dev)
{
    std::vector<std::string> attr_names = {
        "position",
        "texcoord",
        "color",
    };

	std::vector<unsigned int> _vs, _fs;
	try {
		shadertrans::ShaderTrans::HLSL2SpirV(shadertrans::ShaderStage::VertexShader, vs, "main", _vs);
		shadertrans::ShaderTrans::HLSL2SpirV(shadertrans::ShaderStage::PixelShader, fs, "main", _fs);
	} catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
		return;
	}

	if (_vs.empty() || _fs.empty())
	{
		m_shader = nullptr;
		return;
	}

    auto shader = dev.CreateShaderProgram(_vs, _fs);

    shader->AddUniformUpdater(std::make_shared<ModelMatUpdater>(*shader, "u_mvp.model", &ubo_vs.model));
    shader->AddUniformUpdater(std::make_shared<ViewMatUpdater>(*shader, "u_mvp.view", &ubo_vs.view));
    shader->AddUniformUpdater(std::make_shared<ProjectMatUpdater>(*shader, "u_mvp.projection", &ubo_vs.projection));

	m_shader = shader;
}

void SpriteRenderer::InitRenderer(const ur::Device& dev, const ur::Context& ctx)
{
	m_uniform_buf = dev.CreateUniformBuffer(&ubo_vs, sizeof(ubo_vs));

	std::vector<std::shared_ptr<ur::DescriptorSetLayout>> desc_layouts = { dev.GetDescriptorSetLayout("single_ubo_single_img") };
	std::vector<ur::Descriptor> desc_descriptors = { { ur::DescriptorType::UniformBuffer, m_uniform_buf } };
	auto tex = m_palette->GetTexture();
	if (tex) {
		desc_descriptors.push_back(ur::Descriptor(ur::DescriptorType::CombinedImageSampler, tex));
	}
	m_desc_set = dev.CreateDescriptorSet(*dev.GetDescriptorPool(), desc_layouts, desc_descriptors);

	auto vert_buf = dev.CreateVertexBuffer(ur::BufferUsageHint::DynamicDraw, 0);
	m_va->SetVertexBuffer(vert_buf);
	m_va->SetVertexBufferAttrs({
        std::make_shared<ur::VertexInputAttribute>(0, ur::ComponentDataType::Float,        2,  0, 20),		// position
        std::make_shared<ur::VertexInputAttribute>(1, ur::ComponentDataType::Float,        2,  8, 20),		// texcoords
		std::make_shared<ur::VertexInputAttribute>(2, ur::ComponentDataType::UnsignedByte, 4, 16, 20)		// color
    });

	m_pipeline_layout = dev.GetPipelineLayout("single_ubo_single_img");
	m_pipeline = ctx.CreatePipeline(false, true, *m_pipeline_layout, *vert_buf, *m_shader);
}

void SpriteRenderer::CopyVertexBuffer(const sm::mat4& mat, const tess::Painter::Buffer& src)
{
	m_buf.Reserve(src.indices.size(), src.vertices.size());
	for (auto& i : src.indices) {
		*m_buf.index_ptr++ = m_buf.curr_index + i;
	}
	for (auto& v : src.vertices)
	{
		m_buf.vert_ptr->pos = mat * v.pos;
		m_buf.vert_ptr->uv  = v.uv;
		m_buf.vert_ptr->col = v.col;
		++m_buf.vert_ptr;
	}
	m_buf.curr_index += static_cast<unsigned short>(src.vertices.size());
}

}