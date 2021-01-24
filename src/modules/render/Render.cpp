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
    const char* vs_str = vessel_get_slot_string(1);
    const char* fs_str = vessel_get_slot_string(2);

    std::vector<unsigned int> vs, fs;
    shadertrans::ShaderTrans::GLSL2SpirV(shadertrans::ShaderStage::VertexShader, vs_str, vs);
    shadertrans::ShaderTrans::GLSL2SpirV(shadertrans::ShaderStage::PixelShader, fs_str, fs);
    auto prog = tt::Render::Instance()->Device()->CreateShaderProgram(vs, fs);

    size_t ret = PROGRAMS.size();
    PROGRAMS.push_back(prog);
    vessel_set_slot_double(0, static_cast<double>(ret));
}

static void new_vertex_array()
{
    std::vector<float> data;
    int data_num = vessel_get_list_count(1);
    data.resize(data_num);
    for (int i = 0; i < data_num; ++i) {
        vessel_get_list_element(1, i, 0);
        data[i] = static_cast<float>(vessel_get_slot_double(0));
    }

    std::vector<int> attrs;
    int attr_num = vessel_get_list_count(2);
    attrs.resize(attr_num);
    for (int i = 0; i < attr_num; ++i) {
        vessel_get_list_element(2, i, 0);
        attrs[i] = static_cast<int>(vessel_get_slot_double(0));
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
    vessel_set_slot_double(0, static_cast<double>(ret));
}

static void draw()
{
    ur::DrawState ds;

    ur::PrimitiveType prim_type = ur::PrimitiveType::Triangles;

    const char* prim_type_str = vessel_get_slot_string(1);
    if (strcmp(prim_type_str, "triangles") == 0) {
        prim_type = ur::PrimitiveType::Triangles;
    }

    int prog = static_cast<int>(vessel_get_slot_double(2));
    int va = static_cast<int>(vessel_get_slot_double(3));

    ds.program = PROGRAMS[prog];
    ds.vertex_array = VERTEX_ARRAIES[va];

    vessel_get_map_value(4, "depth_test", 0);
    if (vessel_get_slot_type(0) == VESSEL_TYPE_BOOL) {
        ds.render_state.depth_test.enabled = vessel_get_slot_bool(0);
    }

    tt::Render::Instance()->Context()->Draw(prim_type, ds, nullptr);
}

static void clear()
{
    ur::ClearState clear;

    int clear_mask = 0;
    int mask_num = vessel_get_list_count(1);
    for (int i = 0; i < mask_num; ++i)
    {
        vessel_get_list_element(1, i, 0);
        auto mask_str = vessel_get_slot_string(0);
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

    vessel_get_map_value(2, "color", 0);
    if (vessel_get_slot_type(0) == VESSEL_TYPE_LIST)
    {
        assert(vessel_get_list_count(0) == 4);
        vessel_get_list_element(0, 0, 1);
        clear.color.r = static_cast<uint8_t>(vessel_get_slot_double(1));
        vessel_get_list_element(0, 1, 1);
        clear.color.g = static_cast<uint8_t>(vessel_get_slot_double(1));
        vessel_get_list_element(0, 2, 1);
        clear.color.b = static_cast<uint8_t>(vessel_get_slot_double(1));
        vessel_get_list_element(0, 3, 1);
        clear.color.a = static_cast<uint8_t>(vessel_get_slot_double(1));
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