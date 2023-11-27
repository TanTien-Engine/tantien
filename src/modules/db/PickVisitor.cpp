#include "PickVisitor.h"

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
    }

    if (is_intersect) {
        return brepdb::VisitorStatus::Continue;
    } else {
        return brepdb::VisitorStatus::Skip;
    }
}

}