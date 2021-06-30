#include "modules/graphics/DTex.h"

#include <unirender/Device.h>
#include <unirender/Context.h>
#include <unirender/IndexBuffer.h>
#include <unirender/VertexBuffer.h>
#include <unirender/VertexArray.h>
#include <unirender/VertexInputAttribute.h>
#include <unirender/ComponentDataType.h>
#include <unirender/DrawState.h>
#include <unirender/Factory.h>
#include <shadertrans/ShaderTrans.h>
#include <dtex/TextureBuffer.h>
#include <dtex/PixelBuffer.h>
#include <dtex/TexRenderer.h>

namespace
{

const ur::Device* UR_DEV = nullptr;
std::shared_ptr<ur::Context> UR_CTX = nullptr;

const char* vs = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

void main()
{
	gl_Position = vec4(aPos, 0.0, 1.0);
	TexCoord = vec2(aTexCoord.x, aTexCoord.y);
}
)";

const char* fs = R"(
#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D texture1;

void main()
{
	FragColor = texture(texture1, TexCoord);
}
)";

}

namespace tt
{

TT_SINGLETON_DEFINITION(DTex);

DTex::DTex()
{
}

void DTex::InitHook(void(*draw_begin)(), void(*draw_end)(), void(*error_reload)())
{
}

void DTex::Init(const ur::Device& dev)
{
    if (m_inited) {
        return;
    }

    UR_DEV = &dev;

    m_glyph_buffer = std::make_unique<dtex::PixelBuffer>(*UR_DEV, 1024, 512);
    m_texture_buffer = std::make_unique<dtex::TextureBuffer>(*UR_DEV, 1024, 1024);

    m_tex_renderer = std::make_unique<dtex::TexRenderer>(*UR_DEV);

    std::vector<unsigned int> _vs, _fs;
    shadertrans::ShaderTrans::GLSL2SpirV(shadertrans::ShaderStage::VertexShader, vs, nullptr, _vs);
    shadertrans::ShaderTrans::GLSL2SpirV(shadertrans::ShaderStage::PixelShader, fs, nullptr, _fs);
    m_debug_shader = dev.CreateShaderProgram(_vs, _fs);

    m_debug_va = dev.CreateVertexArray();

    auto usage = ur::BufferUsageHint::StaticDraw;

    auto ibuf = dev.CreateIndexBuffer(usage, 0);
    auto ibuf_sz = sizeof(unsigned short) * 6;
    const unsigned short indices[] = { 0, 1, 2, 0, 2, 3 };
    ibuf->SetCount(6);
    ibuf->Reserve(ibuf_sz);
    ibuf->ReadFromMemory(indices, ibuf_sz, 0);
    m_debug_va->SetIndexBuffer(ibuf);

    auto vbuf = dev.CreateVertexBuffer(ur::BufferUsageHint::StaticDraw, 0);
    auto vbuf_sz = sizeof(float) * 4 * 4;
    vbuf->Reserve(vbuf_sz);
    sm::vec2 min(-1, -1);
    sm::vec2 max(1, 1);
    float vertices[] = {
        // positions  // texture Coords
        min.x, min.y, 0.0f, 0.0f,
        max.x, min.y, 1.0f, 0.0f,
        max.x, max.y, 1.0f, 1.0f,
        min.x, max.y, 0.0f, 1.0f,
    };
    m_debug_va->SetVertexBuffer(vbuf);
    m_debug_va->SetVertexBufferAttrs({
        std::make_shared<ur::VertexInputAttribute>(0, ur::ComponentDataType::Float, 2, 0, 16),
        std::make_shared<ur::VertexInputAttribute>(1, ur::ComponentDataType::Float, 2, 8, 16)
    });

    vbuf->ReadFromMemory(vertices, vbuf_sz, 0);

    m_inited = true;
}

void DTex::LoadSymStart()
{
    m_texture_buffer->LoadStart();
}

void DTex::LoadSymbol(uint64_t sym_id, const ur::TexturePtr& tex, const sm::irect& region,
	                  int padding, int extrude, int src_extrude)
{
	dtex::Rect r;
	r.xmin = region.xmin;
	r.ymin = region.ymin;
	r.xmax = region.xmax;
	r.ymax = region.ymax;
    m_texture_buffer->Load(tex, r, sym_id, padding, extrude, src_extrude);
}

void DTex::LoadSymFinish(ur::Context& ctx)
{
    m_texture_buffer->LoadFinish(ctx, *m_tex_renderer);
}

const float* DTex::QuerySymbol(uint64_t sym_id, ur::TexturePtr& texture, int& block_id) const
{
	if (!m_c2_enable) {
		return nullptr;
	}

	int b_id;
	auto node = m_texture_buffer->Query(sym_id, b_id);
	if (node) {
        texture = m_texture_buffer->GetTexture();
		block_id = b_id;
		return node->GetTexcoords();
	}
	else {
		return nullptr;
	}

    return nullptr;
}

void DTex::LoadGlyph(ur::Context& ctx, uint32_t* bitmap, int width, int height, uint64_t key)
{
	m_glyph_buffer->Load(*UR_DEV, ctx, bitmap, width, height, key);
}

bool DTex::QueryGlyph(uint64_t key, float* texcoords, ur::TexturePtr& texture) const
{
	return m_glyph_buffer->QueryAndInsert(key, texcoords, texture);
}

bool DTex::ExistGlyph(uint64_t key) const
{
	return m_glyph_buffer->Exist(key);
}

ur::TexturePtr DTex::GetGlyphFirstPageTex() const
{
    return m_glyph_buffer->GetFirstPageTex();
}

void DTex::Clear()
{
}

bool DTex::Flush(ur::Context& ctx)
{
	return m_glyph_buffer->Flush(ctx, *m_texture_buffer, *m_tex_renderer);
}

void DTex::DebugDraw(ur::Context& ctx) const
{
    ur::DrawState ds;
    ds.program = m_debug_shader;
    ds.render_state = ur::DefaultRenderState2D();
    ds.vertex_array = m_debug_va;

    auto tex = m_texture_buffer->GetTexture();
    ctx.SetTexture(0, tex);
    ctx.SetTextureSampler(0, nullptr);

    ctx.Draw(ur::PrimitiveType::Triangles, ds, nullptr);
}

}