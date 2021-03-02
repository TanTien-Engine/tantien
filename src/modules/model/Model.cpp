#include "modules/model/Model.h"

#include <unirender/Device.h>
#include <unirender/IndexBuffer.h>
#include <unirender/VertexBuffer.h>
#include <unirender/VertexArray.h>
#include <unirender/ComponentDataType.h>
#include <unirender/VertexInputAttribute.h>
#include <guard/check.h>
#include <model/ParametricEquations.h>
#include <model/typedef.h>

namespace
{

std::shared_ptr<ur::VertexArray> build_vertex_array(const ur::Device& dev, const model::ParametricSurface& surface, ur::VertexLayoutType layout)
{
    int vertex_type = 0;
    int stride = 3;
    switch (layout)
    {
    case ur::VertexLayoutType::Pos:
        vertex_type = 0;
        stride = 3;
        break;
    case ur::VertexLayoutType::PosTex:
        vertex_type = model::VERTEX_FLAG_TEXCOORDS;
        stride = 3 + 2;
        break;
    case ur::VertexLayoutType::PosNorm:
        vertex_type = model::VERTEX_FLAG_NORMALS;
        stride = 3 + 3;
        break;
    case ur::VertexLayoutType::PosNormTex:
        vertex_type = model::VERTEX_FLAG_NORMALS | model::VERTEX_FLAG_TEXCOORDS;
        stride = 3 + 3 + 2;
        break;
    default:
        return nullptr;
    }
    std::vector<unsigned short> indices;
    surface.GenerateTriangleIndices(indices);

	std::vector<float> vertices;
	surface.GenerateVertices(vertex_type, vertices);

    auto va = dev.CreateVertexArray();

    auto usage = ur::BufferUsageHint::StaticDraw;

    auto ibuf_sz = sizeof(unsigned short) * indices.size();
    auto ibuf = dev.CreateIndexBuffer(usage, ibuf_sz);
    ibuf->SetCount(indices.size());
    ibuf->ReadFromMemory(indices.data(), ibuf_sz, 0);
    va->SetIndexBuffer(ibuf);

    auto vbuf_sz = sizeof(float) * vertices.size();
    auto vbuf = dev.CreateVertexBuffer(ur::BufferUsageHint::StaticDraw, vbuf_sz);
    vbuf->ReadFromMemory(vertices.data(), vbuf_sz, 0);
    va->SetVertexBuffer(vbuf);

    std::vector<std::shared_ptr<ur::VertexInputAttribute>> vbuf_attrs;
    int ptr = 0;
    const int size = stride * 4;
    vbuf_attrs.push_back(std::make_shared<ur::VertexInputAttribute>(
        0, ur::ComponentDataType::Float, 3, ptr, size
    ));
    ptr += 3 * 4;
    if (vertex_type & model::VERTEX_FLAG_NORMALS) 
    {
        vbuf_attrs.push_back(std::make_shared<ur::VertexInputAttribute>(
            vbuf_attrs.size(), ur::ComponentDataType::Float, 3, ptr, size
        ));
        ptr += 3 * 4;
    }
    if (vertex_type & model::VERTEX_FLAG_TEXCOORDS)
    {
        vbuf_attrs.push_back(std::make_shared<ur::VertexInputAttribute>(
            vbuf_attrs.size(), ur::ComponentDataType::Float, 2, ptr, size
        ));
        ptr += 2 * 4;
    }
    va->SetVertexBufferAttrs(vbuf_attrs);

    return va;
}

struct Vertex
{
    Vertex() {}
    Vertex(size_t ix, size_t iz, size_t size_x, size_t size_z)
    {
        //position = sm::vec3(
        //    ix / static_cast<float>(size_x),
        //    0,
        //    iz / static_cast<float>(size_z)
        //);
        position = sm::vec3(
            ix / static_cast<float>(size_x),
            iz / static_cast<float>(size_z),
            0
        );
        texcoords = sm::vec2(
            ix / static_cast<float>(size_x),
            iz / static_cast<float>(size_z)
        );
    }

    sm::vec3 position;
    sm::vec2 texcoords;
};

const int HEIGHT_FIELD_SIZE = 512;

std::shared_ptr<ur::VertexArray> build_grids_va(const ur::Device& dev, ur::VertexLayoutType layout)
{
   auto va = dev.CreateVertexArray();

    auto usage = ur::BufferUsageHint::StaticDraw;

    const size_t num_vert = HEIGHT_FIELD_SIZE * HEIGHT_FIELD_SIZE;
    Vertex* vertices = new Vertex[num_vert];
    auto vert_ptr = vertices;
    for (size_t y = 0; y < HEIGHT_FIELD_SIZE; ++y) {
        for (size_t x = 0; x < HEIGHT_FIELD_SIZE; ++x) {
            *vert_ptr++ = Vertex(x, y, HEIGHT_FIELD_SIZE, HEIGHT_FIELD_SIZE);
        }
    }

    auto vbuf = dev.CreateVertexBuffer(usage, 0);
    auto vbuf_sz = sizeof(Vertex) * num_vert;
    vbuf->Reserve(vbuf_sz);
    vbuf->ReadFromMemory(vertices, vbuf_sz, 0);
    va->SetVertexBuffer(vbuf);

    const size_t num_idx = (HEIGHT_FIELD_SIZE - 1) * (HEIGHT_FIELD_SIZE - 1) * 6;
    unsigned int* indices = new unsigned int[num_idx];
    auto index_ptr = indices;
    for (size_t y = 0; y < HEIGHT_FIELD_SIZE - 1; ++y) {
        for (size_t x = 0; x < HEIGHT_FIELD_SIZE - 1; ++x) {
            size_t ll = y * HEIGHT_FIELD_SIZE + x;
            size_t rl = ll + 1;
            size_t lh = ll + HEIGHT_FIELD_SIZE;
            size_t rh = lh + 1;
            *index_ptr++ = ll;
            *index_ptr++ = lh;
            *index_ptr++ = rh;
            *index_ptr++ = ll;
            *index_ptr++ = rh;
            *index_ptr++ = rl;
        }
    }

    auto ibuf = dev.CreateIndexBuffer(usage, 0);
    auto ibuf_sz = sizeof(unsigned int) * num_idx;
    ibuf->SetCount(num_idx);
    ibuf->Reserve(ibuf_sz);
    ibuf->ReadFromMemory(indices, ibuf_sz, 0);
    ibuf->SetDataType(ur::IndexBufferDataType::UnsignedInt);
    va->SetIndexBuffer(ibuf);

    va->SetVertexBufferAttrs({
        // pos
        std::make_shared<ur::VertexInputAttribute>(0, ur::ComponentDataType::Float, 3, 0, 20),
        // uv
        std::make_shared<ur::VertexInputAttribute>(1, ur::ComponentDataType::Float, 2, 12, 20)
    });

    return va;
}

}

namespace tt
{

TT_SINGLETON_DEFINITION(Model)

Model::Model()
{
}

Model::~Model()
{
}

std::shared_ptr<ur::VertexArray> 
Model::CreateShape(const ur::Device& dev, ShapeType type, ur::VertexLayoutType layout, ur::PrimitiveType& prim_type)
{
    std::shared_ptr<ur::VertexArray> va = nullptr;
    switch (type)
    {
    case ShapeType::Quad:
        prim_type = ur::PrimitiveType::TriangleStrip;
        va = dev.GetVertexArray(ur::Device::PrimitiveType::Quad, layout);
        break;
    case ShapeType::Cube:
        prim_type = ur::PrimitiveType::Triangles;
        va = dev.GetVertexArray(ur::Device::PrimitiveType::Cube, layout);
        break;
    case ShapeType::Sphere:
    {
        auto sphere = std::make_unique<model::Sphere>(1.0f);
        prim_type = ur::PrimitiveType::Triangles;
        va = build_vertex_array(dev, *sphere, layout);
    }
        break;
    case ShapeType::Grids:
        prim_type = ur::PrimitiveType::Triangles;
        va = build_grids_va(dev, layout);
        break;
    default:
        GD_REPORT_ASSERT("unknown type.");
    }
    return va;
}

}