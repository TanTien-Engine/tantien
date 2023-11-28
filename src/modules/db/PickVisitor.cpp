#include "PickVisitor.h"
#include "BRepSerialize.h"

#include <polymesh3/Polytope.h>
#include <brepdb/Node.h>
#include <SM_RayIntersect.h>

namespace tt
{

brepdb::VisitorStatus PickVisitor::VisitNode(const brepdb::INode& n)
{
    auto& r = dynamic_cast<const brepdb::Node&>(n).GetRegion();

    sm::cube aabb;
    for (int i = 0; i < 3; ++i)
    {
        aabb.min[i] = static_cast<float>(r.GetLow()[i]);
        aabb.max[i] = static_cast<float>(r.GetHigh()[i]);
    }

    bool is_intersect = sm::ray_aabb_intersect(aabb, m_ray, nullptr);

    if (is_intersect && n.IsLeaf() && n.GetChildrenCount() > 0)
    {
        auto& node = dynamic_cast<const brepdb::Node&>(n);
        m_regions.push_back(node.GetRegion());

        PickPoly(n);
    }

    if (is_intersect) {
        return brepdb::VisitorStatus::Continue;
    } else {
        return brepdb::VisitorStatus::Skip;
    }
}

void PickVisitor::PickPoly(const brepdb::INode& node)
{
    if (!node.IsLeaf() || node.GetChildrenCount() == 0) {
        return;
    }

    for (int i = 0, n = node.GetChildrenCount(); i < n; ++i)
    {
        uint32_t len = 0;
        uint8_t* data = nullptr;
        node.GetChildData(i, len, &data);

        auto poly = tt::BrepSerialize::BRepFromByteArray(data);
        
        sm::cube aabb;
        for (auto& p : poly->Points()) {
            aabb.Combine(p->pos);
        }

        bool is_intersect = sm::ray_aabb_intersect(aabb, m_ray, nullptr);
        if (is_intersect) {
            m_polys.push_back(poly);
        }
    }
}

}