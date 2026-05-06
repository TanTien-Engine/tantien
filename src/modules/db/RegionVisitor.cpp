#include "RegionVisitor.h"

#include <spatialdb/Region.h>
#include <spatialdb/Node.h>

namespace tt
{

spatialdb::VisitorStatus RegionVisitor::VisitNode(const spatialdb::INode& n)
{
    if (n.IsLeaf() && n.GetChildrenCount() > 0) 
    {
        auto& node = dynamic_cast<const spatialdb::Node&>(n);
        m_regions.push_back(node.GetRegion());
    }

    return spatialdb::VisitorStatus::Continue;
}

}