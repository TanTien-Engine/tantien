#pragma once

#include "modules/graphics/RenderBuffer.h"

#include <SM_Vector.h>
#include <SM_Matrix.h>
#include <unirender/typedef.h>
#include <unirender/RenderState.h>
#include <tessellation/Painter.h>

#include <memory>

namespace ur {
    class Device; class Context; struct RenderState; class ShaderProgram;
    class UniformBuffer; class DescriptorSet; class VertexArray;
    class Framebuffer; class PipelineLayout; class Pipeline;
}
namespace tess { class Painter; class Palette; }

namespace tt
{

struct SpriteVertex
{
	sm::vec2 pos;
	sm::vec2 uv;
	uint32_t col = 0;
};

class SpriteRenderer
{
public:
    SpriteRenderer();
    ~SpriteRenderer();

    void OnSize(float width, float height);
    void OnCameraUpdate(const sm::vec2& offset, float scale);

    void DrawPainter(ur::Context& ctx, const ur::RenderState& rs, const tess::Painter& pt,
        const sm::mat4& mat = sm::mat4());
    void DrawQuad(ur::Context& ctx, const ur::RenderState& rs, const float* positions, 
        const float* texcoords, const ur::TexturePtr& tex, uint32_t color);

    void Flush(ur::Context& ctx);

    auto GetPalette() const { return m_palette; }

    auto GetShader() const { return m_shader; }
    auto GetTexture() const { return m_tex; }

    void SetIntense(float intense);

private:
    void InitShader(const ur::Device& dev);
    void InitRenderer(const ur::Device& dev, const ur::Context& ctx);

    void CopyVertexBuffer(const sm::mat4& mat, const tess::Painter::Buffer& src);

private:
    std::shared_ptr<ur::ShaderProgram> m_shader = nullptr;

    RenderBuffer<SpriteVertex, unsigned short> m_buf;
    std::shared_ptr<ur::VertexArray> m_va = nullptr;

    std::shared_ptr<tess::Palette> m_palette = nullptr;

    ur::TexturePtr m_tex = nullptr;

    ur::RenderState m_rs;
    std::shared_ptr<ur::Framebuffer> m_fbo = nullptr;

    std::shared_ptr<ur::UniformBuffer> m_uniform_buf = nullptr;
    std::shared_ptr<ur::DescriptorSet> m_desc_set = nullptr;

    std::shared_ptr<ur::PipelineLayout> m_pipeline_layout = nullptr;
    std::shared_ptr<ur::Pipeline>       m_pipeline = nullptr;

}; // SpriteRenderer

}