#include "PolytopeAlgos.h"
#include "TopoPolyAdapter.h"

#include <halfedge/Polyhedron.h>

#include <iterator>

namespace tt
{

std::vector<pm3::PolytopePtr>
PolytopeAlgos::Intersect(const std::vector<pm3::PolytopePtr>& a,
	                     const std::vector<pm3::PolytopePtr>& b)
{
    std::vector<pm3::PolytopePtr> ret;
    for (auto& pa : a)
    {
        auto topo_a = pa->GetTopoPoly();
        if (!topo_a) {
            continue;
        }
        for (auto& pb : b)
        {
            auto topo_b = pb->GetTopoPoly();
            if (!topo_b) {
                continue;
            }

            auto poly = topo_a->Intersect(*topo_b);
            if (poly && poly->GetLoops().Size() > 0) {
                ret.push_back(std::make_shared<pm3::Polytope>(poly));
            }
        }
    }
    return ret;
}

std::vector<pm3::PolytopePtr>
PolytopeAlgos::Subtract(const std::vector<pm3::PolytopePtr>& a, 
                        const std::vector<pm3::PolytopePtr>& b)
{
    std::vector<pm3::PolytopePtr> polytopes = a;
    for (auto& cb : b)
    {
        auto intersect = tt::PolytopeAlgos::Intersect(polytopes, { cb });
        if (!intersect.empty()) {
            polytopes = tt::PolytopeAlgos::SubtractImpl(polytopes, intersect);
        }
    }
    return polytopes;
}

std::vector<pm3::PolytopePtr>
PolytopeAlgos::Union(const std::vector<pm3::PolytopePtr>& a,
                     const std::vector<pm3::PolytopePtr>& b)
{
    std::vector<pm3::PolytopePtr> polytopes;

    auto intersect = tt::PolytopeAlgos::Intersect(a, b);
    if (intersect.empty())
    {
        std::copy(a.begin(), a.end(), std::back_inserter(polytopes));
        std::copy(b.begin(), b.end(), std::back_inserter(polytopes));
    }
    else
    {
        auto a_left = tt::PolytopeAlgos::Subtract(a, intersect);
        auto b_left = tt::PolytopeAlgos::Subtract(b, intersect);
        polytopes = intersect;
        std::copy(a_left.begin(), a_left.end(), std::back_inserter(polytopes));
        std::copy(b_left.begin(), b_left.end(), std::back_inserter(polytopes));
    }

    return polytopes;
}

void PolytopeAlgos::Extrude(pm3::PolytopePtr& poly, float dist)
{
    auto topo_poly = poly->GetTopoPoly();
    if (!topo_poly) {
        return;
    }

    bool create_face[he::Polyhedron::ExtrudeMaxCount];
    create_face[he::Polyhedron::ExtrudeFront] = true;
    create_face[he::Polyhedron::ExtrudeBack] = true;
    create_face[he::Polyhedron::ExtrudeSide] = true;

    std::vector<he::TopoID> face_ids;
    auto& faces = topo_poly->GetLoops();
    face_ids.reserve(faces.Size());
    auto first_f = faces.Head();
    auto curr_f = first_f;
    do {
        face_ids.push_back(curr_f->ids);
        curr_f = curr_f->linked_next;
    } while (curr_f != first_f);

    if (!topo_poly->Extrude(dist, face_ids, create_face)) {
        return;
    }

    poly->BuildFromTopo();
}

pm3::PolytopePtr PolytopeAlgos::Offset(const pm3::PolytopePtr& poly,
                                       he::Polygon::KeepType keep, float dist)
{
    std::vector<pm3::Polytope::PointPtr> dst_pts;
    std::vector<pm3::Polytope::FacePtr>  dst_faces;

    auto& src_points = poly->Points();
    auto& src_faces = poly->Faces();
    for (auto& src_f : src_faces)
    {
        std::vector<sm::vec3> src_poly;
        src_poly.reserve(src_f->border.size());
        for (auto& p : src_f->border) {
            src_poly.push_back(src_points[p]->pos);
        }

        tt::TopoPolyAdapter topo_poly(src_poly);
        topo_poly.GetPoly()->Offset(dist, keep);
        topo_poly.TransToPolymesh(dst_pts, dst_faces);
    }

    return std::make_shared<pm3::Polytope>(dst_pts, dst_faces);
}

std::vector<pm3::PolytopePtr>
PolytopeAlgos::SubtractImpl(const std::vector<pm3::PolytopePtr>& a,
                            const std::vector<pm3::PolytopePtr>& b)
{
    std::vector<pm3::PolytopePtr> ret = a;
    for (auto& pa : a)
    {
        auto topo_a = pa->GetTopoPoly();
        if (!topo_a) {
            continue;
        }
        for (auto& pb : b)
        {
            auto topo_b = pb->GetTopoPoly();
            if (!topo_b) {
                continue;
            }

            auto intersect = topo_a->Intersect(*topo_b);
            if (intersect && intersect->GetLoops().Size() > 0)
            {
                auto poly = std::make_shared<pm3::Polytope>(intersect);

                auto a_left = a;
                for (auto itr = a_left.begin(); itr != a_left.end(); ++itr) {
                    if (*itr == pa) {
                        a_left.erase(itr);
                        break;
                    }
                }
                auto a_sub = topo_a->Subtract(*topo_b);
                for (auto& p : a_sub) {
                    a_left.push_back(std::make_shared<pm3::Polytope>(p));
                }

                auto b_left = b;
                for (auto itr = b_left.begin(); itr != b_left.end(); ++itr) {
                    if (*itr == pb) {
                        b_left.erase(itr);
                        break;
                    }
                }
                auto b_sub = topo_b->Subtract(*topo_b);
                for (auto& p : b_sub) {
                    b_left.push_back(std::make_shared<pm3::Polytope>(p));
                }

                return Subtract(a_left, b_left);
            }
        }
    }
    return ret;
}

}