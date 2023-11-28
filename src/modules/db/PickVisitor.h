#pragma once

#include <brepdb/SpatialIndex.h>
#include <SM_Vector.h>
#include <SM_Ray.h>

#include <memory>

namespace brepdb { class Region; }
namespace pm3 { class Polytope; }

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
    auto& GetPolys() const { return m_polys; }

private:
    void PickPoly(const brepdb::INode& n);

private:
    sm::Ray m_ray;

    std::vector<brepdb::Region> m_regions;
    std::vector<std::shared_ptr<pm3::Polytope>> m_polys;

}; // PickVisitor

}