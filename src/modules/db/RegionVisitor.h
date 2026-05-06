#pragma once

#include <spatialdb/SpatialIndex.h>

namespace spatialdb { class Region; }

namespace tt
{

class RegionVisitor : public spatialdb::IVisitor
{
public:
    RegionVisitor() {}

    virtual spatialdb::VisitorStatus VisitNode(const spatialdb::INode& n) override;
    virtual void VisitData(const spatialdb::IData& d) override {}
    virtual void VisitData(std::vector<const spatialdb::IData*>& v) override {}

    auto& GetRegions() const { return m_regions; }
    
private:
    std::vector<spatialdb::Region> m_regions;

}; // RegionVisitor

}