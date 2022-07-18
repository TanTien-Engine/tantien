#include "modules/geometry/TopoPolyAdapter.h"

#include <SM_Calc.h>
#include <halfedge/Polygon.h>

namespace tt
{

TopoPolyAdapter::TopoPolyAdapter(const std::vector<sm::vec3>& border)
{
    auto norm = sm::calc_face_normal(border);
    auto rot  = sm::mat4(sm::Quaternion::CreateFromVectors(norm, sm::vec3(0, 1, 0)));
    m_inv_rot = rot.Inverted();

    std::vector<he::Polygon::in_vert> verts;
    std::vector<he::Polygon::in_face> faces;

    std::vector<size_t> border_idx;
    size_t idx = 0;
    border_idx.reserve(border.size());
    for (auto itr = border.rbegin(); itr != border.rend(); ++itr)
    {
        auto p = rot * *itr;
        verts.push_back({ he::TopoID(), sm::vec2(p.x, p.z) });
        border_idx.push_back(idx++);
    }
    faces.push_back({ he::TopoID(), border_idx, std::vector<he::Polygon::in_loop>() });

    m_poly = std::make_unique<he::Polygon>(verts, faces);
}

void TopoPolyAdapter::TransToPolymesh(std::vector<pm3::Polytope::PointPtr>& dst_pts,
                                      std::vector<pm3::Polytope::FacePtr>& dst_faces) const
{
    std::map<he::vert2*, size_t> vert2idx;

    auto& src_pts = m_poly->GetVerts();
    dst_pts.reserve(dst_pts.size() + src_pts.Size());
    auto first_p = src_pts.Head();
    auto curr_p = first_p;
    do {
        vert2idx.insert({ curr_p, dst_pts.size() });

        auto& p2 = curr_p->position;
        auto p3 = m_inv_rot * sm::vec3(p2.x, 0, p2.y);
        dst_pts.push_back(std::make_shared<pm3::Polytope::Point>(p3));

        curr_p = curr_p->linked_next;
    } while (curr_p != first_p);

    auto trans_loop = [](const std::map<he::vert2*, size_t>& vert2idx, const he::loop2& loop) -> std::vector<size_t>
    {
        std::vector<size_t> ret;

        auto first_e = loop.edge;
        auto curr_e = first_e;
        do {
            auto itr = vert2idx.find(curr_e->vert);
            assert(itr != vert2idx.end());
            ret.push_back(itr->second);

            curr_e = curr_e->next;
        } while (curr_e != first_e);

        return ret;
    };

    auto& src_faces = m_poly->GetFaces();
    dst_faces.reserve(dst_faces.size() + src_faces.size());
    for (auto& src_f : src_faces)
    {
        auto dst_f = std::make_shared<pm3::Polytope::Face>();

        dst_f->border = trans_loop(vert2idx, *src_f.border);

        dst_f->holes.reserve(src_f.holes.size());
        for (auto& src_h : src_f.holes) {
            dst_f->holes.push_back(trans_loop(vert2idx, *src_h));
        }

        dst_faces.push_back(dst_f);
    }
}

}