#include "modules/scene/SceneTree.h"
#include "modules/db/BrepSerialize.h"
#include "modules/render/Render.h"

#include <brepdb/SpatialIndex.h>
#include <brepdb/Node.h>
#include <polymesh3/Polytope.h>
#include <model/BrushBuilder.h>
#include <unirender/Device.h>
#include <unirender/IndexBuffer.h>
#include <unirender/VertexBuffer.h>
#include <unirender/VertexInputAttribute.h>
#include <unirender/VertexArray.h>

#include <map>

#include <assert.h>

namespace
{

struct Vertex
{
    sm::vec3 pos;
    sm::vec3 normal;
    sm::vec2 texcoord, texcoord2;
    sm::vec3 color;
    unsigned char mat_id = 0;
    float offset = 0;
};

class BuildTreeVisitor : public brepdb::IVisitor
{
public:
    BuildTreeVisitor() {}

    virtual brepdb::VisitorStatus VisitNode(const brepdb::INode& src) override
    {
        auto dst = std::make_shared<tt::SceneNode>();

        dst->id = src.GetIdentifier();

        if (!m_root)
        {
            m_root = dst;
        }
        else
        {
            auto itr = m_child2parent.find(dst->id);
            assert(itr != m_child2parent.end());

            dst->parent = itr->second;
            itr->second->children.push_back(dst);
        }

        if (src.IsIndex())
        {
            for (int i = 0, n = src.GetChildrenCount(); i < n; ++i)
            {
                auto c_idx = src.GetChildIdentifier(i);
                m_child2parent.insert({ c_idx, dst });
            }
        }
        else
        {
            std::vector<Vertex> vertices;
            std::vector<unsigned short> indices;

            size_t start_idx = 0;
            for (int i = 0, n = src.GetChildrenCount(); i < n; ++i)
            {
                uint32_t len = 0;
                uint8_t* data = nullptr;
                src.GetChildData(i, len, &data);

                auto poly = tt::BrepSerialize::BRepFromByteArray(data);
                Triangulate(poly, start_idx, vertices, indices);

                start_idx += poly->Points().size();
            }

            dst->vao = CreateVAO(vertices, indices);
        }

        return brepdb::VisitorStatus::Continue;
    }

    virtual void VisitData(const brepdb::IData& d) override {}
    virtual void VisitData(std::vector<const brepdb::IData*>& v) override {}

    auto GetRoot() const { return m_root; }

private:
    static void Triangulate(const std::shared_ptr<pm3::Polytope>& poly, size_t start_idx,
        std::vector<Vertex>& vertices, std::vector<unsigned short>& indices)
    {
        auto& points = poly->Points();
        for (auto& p : points)
        {
            Vertex v;
            v.pos = p->pos;
            v.color.Set(1, 1, 1);
            vertices.push_back(v);
        }

        auto& faces = poly->Faces();
        for (auto& f : faces)
        {
            if (f->border.size() == 3 && f->holes.empty())
            {
                for (auto idx : f->border) {
                    indices.push_back(static_cast<unsigned short>(start_idx + idx));
                }
            }
            else
            {
                auto tris_idx = model::BrushBuilder::Triangulation(points, f->border, f->holes);
                for (auto& idx : tris_idx) {
                    indices.push_back(static_cast<unsigned short>(start_idx + idx));
                }
            }
        }
    }

    static std::shared_ptr<ur::VertexArray> 
        CreateVAO(const std::vector<Vertex>& vertices, const std::vector<unsigned short>& indices)
    {
        auto dev = tt::Render::Instance()->Device();

        auto va = dev->CreateVertexArray();

        std::vector<float> buf;
        buf.reserve(vertices.size() * 8);
        for (auto& p : vertices)
        {
            for (int i = 0; i < 3; ++i) {
                buf.push_back(p.pos.xyz[i]);
            }
            for (int i = 0; i < 3; ++i) {
                buf.push_back(p.color.xyz[i]);
            }
            buf.push_back(p.mat_id);
            buf.push_back(p.offset);
        }

        int vbuf_sz = static_cast<int>(sizeof(float) * buf.size());
        auto vbuf = dev->CreateVertexBuffer(ur::BufferUsageHint::StaticDraw, vbuf_sz);
        vbuf->ReadFromMemory(buf.data(), vbuf_sz, 0);
        va->SetVertexBuffer(vbuf);

        auto ibuf = dev->CreateIndexBuffer(ur::BufferUsageHint::StaticDraw, 0);
        int ibuf_sz = static_cast<int>(sizeof(unsigned short) * indices.size());
        ibuf->SetCount(static_cast<int>(indices.size()));
        ibuf->Reserve(ibuf_sz);
        ibuf->ReadFromMemory(indices.data(), ibuf_sz, 0);
        ibuf->SetDataType(ur::IndexBufferDataType::UnsignedShort);
        va->SetIndexBuffer(ibuf);

        std::vector<std::shared_ptr<ur::VertexInputAttribute>> vbuf_attrs;
        vbuf_attrs.resize(4);
        // pos
        vbuf_attrs[0] = std::make_shared<ur::VertexInputAttribute>(
            0, ur::ComponentDataType::Float, 3, 0, 32
            );
        // col
        vbuf_attrs[1] = std::make_shared<ur::VertexInputAttribute>(
            1, ur::ComponentDataType::Float, 3, 12, 32
            );
        // mat_id
        vbuf_attrs[2] = std::make_shared<ur::VertexInputAttribute>(
            2, ur::ComponentDataType::Float, 1, 24, 32
            );
        // offset
        vbuf_attrs[3] = std::make_shared<ur::VertexInputAttribute>(
            3, ur::ComponentDataType::Float, 1, 28, 32
            );
        va->SetVertexBufferAttrs(vbuf_attrs);

        return va;
    }

private:
    std::shared_ptr<tt::SceneNode> m_root = nullptr;

    std::map<brepdb::id_type, std::shared_ptr<tt::SceneNode>> m_child2parent;

}; // BuildTreeVisitor

}

namespace tt
{

void SceneTree::Build(brepdb::ISpatialIndex& si)
{
    BuildTreeVisitor visitor;
    si.LevelTraversal(visitor);
    m_root = visitor.GetRoot();
}

}