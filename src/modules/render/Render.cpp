#include "modules/render/Render.h"

#include <unirender/Device.h>
#include <unirender/Context.h>
#include <unirender/Factory.h>
#include <unirender/VertexArray.h>
#include <unirender/VertexBuffer.h>
#include <unirender/ComponentDataType.h>
#include <unirender/VertexInputAttribute.h>
#include <unirender/DrawState.h>
#include <unirender/ClearState.h>
#include <shadertrans/ShaderTrans.h>

#include <assert.h>

namespace
{

// fixme: move to script
std::vector<std::shared_ptr<ur::ShaderProgram>> PROGRAMS;
std::vector<std::shared_ptr<ur::VertexArray>>   VERTEX_ARRAIES;

static void new_shader()
{
    const char* vs_str = ves_tostring(1);
    const char* fs_str = ves_tostring(2);

    std::vector<unsigned int> vs, fs;
    shadertrans::ShaderTrans::GLSL2SpirV(shadertrans::ShaderStage::VertexShader, vs_str, vs);
    shadertrans::ShaderTrans::GLSL2SpirV(shadertrans::ShaderStage::PixelShader, fs_str, fs);
    auto prog = tt::Render::Instance()->Device()->CreateShaderProgram(vs, fs);

    size_t ret = PROGRAMS.size();
    PROGRAMS.push_back(prog);
    ves_set_number(0, static_cast<double>(ret));
}

static void new_vertex_array()
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

    auto dev = tt::Render::Instance()->Device();
    auto va = dev->CreateVertexArray();

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

    size_t ret = VERTEX_ARRAIES.size();
    VERTEX_ARRAIES.push_back(va);
    ves_set_number(0, static_cast<double>(ret));
}

static void draw()
{
    ur::DrawState ds;

    ur::PrimitiveType prim_type = ur::PrimitiveType::Triangles;

    const char* prim_type_str = ves_tostring(1);
    if (strcmp(prim_type_str, "triangles") == 0) {
        prim_type = ur::PrimitiveType::Triangles;
    }

    int prog = static_cast<int>(ves_tonumber(2));
    int va = static_cast<int>(ves_tonumber(3));

    ds.program = PROGRAMS[prog];
    ds.vertex_array = VERTEX_ARRAIES[va];

    int type = ves_getfield(4, "depth_test");
    if (type == VES_TYPE_BOOL) {
        ds.render_state.depth_test.enabled = ves_toboolean(-1);
    }
    ves_pop(1);

    tt::Render::Instance()->Context()->Draw(prim_type, ds, nullptr);
}

static void clear()
{
    ur::ClearState clear;

    int clear_mask = 0;
    int mask_num = ves_len(1);
    for (int i = 0; i < mask_num; ++i)
    {
        ves_geti(1, i);
        auto mask_str = ves_tostring(-1);
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
    ves_pop(mask_num);
    clear.buffers = static_cast<ur::ClearBuffers>(clear_mask);

    int type = ves_getfield(2, "color");
    if (type == VES_TYPE_LIST)
    {
        assert(ves_len(-1) == 4);
        ves_geti(-1, 0);
        clear.color.r = static_cast<uint8_t>(ves_tonumber(-1));
        ves_pop(1);
        ves_geti(-1, 1);
        clear.color.g = static_cast<uint8_t>(ves_tonumber(-1));
        ves_pop(1);
        ves_geti(-1, 2);
        clear.color.b = static_cast<uint8_t>(ves_tonumber(-1));
        ves_pop(1);
        ves_geti(-1, 3);
        clear.color.a = static_cast<uint8_t>(ves_tonumber(-1));
        ves_pop(1);
    }

    tt::Render::Instance()->Context()->Clear(clear);
}

}

namespace tt
{

TT_SINGLETON_DEFINITION(Render)

Render::Render()
{
    m_dev = ur::CreateDevice(ur::APIType::OpenGL);
    m_dev->Init();

    m_ctx = ur::CreateContext(ur::APIType::OpenGL, *m_dev, nullptr);
}

Render::~Render()
{
}

VesselForeignMethodFn RenderBindMethod(const char* signature)
{
    if (strcmp(signature, "static Render.newShader(_,_)") == 0) return new_shader;
    if (strcmp(signature, "static Render.newVertexArray(_,_)") == 0) return new_vertex_array;
    if (strcmp(signature, "static Render.draw(_,_,_,_)") == 0) return draw;
    if (strcmp(signature, "static Render.clear(_,_)") == 0) return clear;

    return NULL;
}

void RenderBindClass(const char* className, VesselForeignClassMethods* methods)
{
}

}