#pragma once

#include <SM_Vector.h>
#include <SM_Matrix.h>
#include <polymesh3/Polytope.h>

#include <vector>
#include <memory>

namespace he { class Polygon; }

namespace tt
{

class TopoPolyAdapter
{
public:
    TopoPolyAdapter(const std::vector<sm::vec3>& border);

    auto& GetPoly() const { return m_poly; }

    void TransToPolymesh(std::vector<pm3::Polytope::PointPtr>& dst_pts,
        std::vector<pm3::Polytope::FacePtr>& dst_faces) const;

private:
    std::unique_ptr<he::Polygon> m_poly = nullptr;

    sm::mat4 m_inv_rot;

}; // GeoHelper

}