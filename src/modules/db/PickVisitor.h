#pragma once

#include <spatialdb/SpatialIndex.h>
#include <SM_Vector.h>
#include <SM_Ray.h>

#include <memory>

namespace spatialdb { class Region; }
namespace pm3 { class Polytope; }

namespace tt
{

class PickVisitor : public spatialdb::IVisitor
{
public:
    PickVisitor(const sm::vec3& pos, const sm::vec3& dir) 
        : m_ray(pos, dir) {}

    virtual spatialdb::VisitorStatus VisitNode(const spatialdb::INode& n) override;
    virtual void VisitData(const spatialdb::IData& d) override {}
    virtual void VisitData(std::vector<const spatialdb::IData*>& v) override {}

    auto& GetRegions() const { return m_regions; }
    auto& GetPolys() const { return m_polys; }

private:
    void PickPoly(const spatialdb::INode& n);

private:
    sm::Ray m_ray;

    std::vector<spatialdb::Region> m_regions;
    std::vector<std::shared_ptr<pm3::Polytope>> m_polys;

}; // PickVisitor

}