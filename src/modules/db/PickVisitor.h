#pragma once

#include <brepdb/SpatialIndex.h>
#include <SM_Vector.h>
#include <SM_Ray.h>

namespace brepdb { class Region; }

namespace tt
{

class PickVisitor : public brepdb::IVisitor
{
public:
    PickVisitor(const sm::vec3& pos, const sm::vec3& dir) 
        : m_ray(pos, dir) {}

    virtual brepdb::VisitorStatus VisitNode(const brepdb::INode& n) override;
    virtual void VisitData(const brepdb::IData& d) override {}
    virtual void VisitData(std::vector<const brepdb::IData*>& v) override {}

    auto& GetRegions() const { return m_regions; }
    
private:
    sm::Ray m_ray;

    std::vector<brepdb::Region> m_regions;

}; // PickVisitor

}