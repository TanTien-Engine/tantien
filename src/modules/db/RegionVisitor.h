#pragma once

#include <brepdb/SpatialIndex.h>

namespace brepdb { class Region; }

namespace tt
{

class RegionVisitor : public brepdb::IVisitor
{
public:
    RegionVisitor() {}

    virtual brepdb::VisitorStatus VisitNode(const brepdb::INode& n) override;
    virtual void VisitData(const brepdb::IData& d) override {}
    virtual void VisitData(std::vector<const brepdb::IData*>& v) override {}

    auto& GetRegions() const { return m_regions; }
    
private:
    std::vector<brepdb::Region> m_regions;

}; // RegionVisitor

}