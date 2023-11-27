#include "RegionVisitor.h"

#include <brepdb/Region.h>
#include <brepdb/Node.h>

namespace tt
{

brepdb::VisitorStatus RegionVisitor::VisitNode(const brepdb::INode& n)
{
    if (n.IsLeaf() && n.GetChildrenCount() > 0) 
    {
        auto& node = dynamic_cast<const brepdb::Node&>(n);
        m_regions.push_back(node.GetRegion());
    }

    return brepdb::VisitorStatus::Continue;
}

}