#pragma once

#include <brepdb/SpatialIndex.h>
#include <brepdb/Region.h>
#include <brepdb/Node.h>

namespace tt
{

class RegionVisitor : public brepdb::IVisitor
{
public:
    RegionVisitor() {}

    virtual void VisitNode(const brepdb::INode& n) override {
        if (n.IsLeaf() && n.GetChildrenCount() > 0) {
            auto& node = dynamic_cast<const brepdb::Node&>(n);
            m_regions.push_back(node.GetRegion());
        }
    }
    virtual void VisitData(const brepdb::IData& d) override {}
    virtual void VisitData(std::vector<const brepdb::IData*>& v) override {}

    auto& GetRegions() const { return m_regions; }
    
private:
    std::vector<brepdb::Region> m_regions;

}; // RegionVisitor

}