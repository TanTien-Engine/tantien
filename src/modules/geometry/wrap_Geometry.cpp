#include "modules/geometry/wrap_Geometry.h"
#include "modules/geometry/TopoPolyAdapter.h"
#include "modules/script/TransHelper.h"
#include "modules/script/Proxy.h"

#include <geoshape/Point2D.h>
#include <geoshape/Line2D.h>
#include <geoshape/Rect.h>
#include <geoshape/Circle.h>
#include <geoshape/Polyline2D.h>
#include <geoshape/Polygon2D.h>
#include <geoshape/Bezier.h>
#include <geoshape/Box.h>
#include <geoshape/Line3D.h>
#include <geoshape/Polyline3D.h>
#include <geoshape/Polygon3D.h>
#include <polymesh3/Polytope.h>
#include <model/ParametricEquations.h>
#include <guard/check.h>
#include <constraints2/Scene.h>
#include <constraints2/Constraint.h>
#include <halfedge/Polygon.h>
#include <SM_Calc.h>

#include <string>
#include <iterator>
#include <set>

namespace
{

void w_Shape2D_is_contain()
{
    auto l = ((tt::Proxy<gs::Shape2D>*)ves_toforeign(0))->obj;

    float x = (float)ves_tonumber(1);
    float y = (float)ves_tonumber(2);

    ves_set_boolean(0, l->IsContain(sm::vec2(x, y)));
}

void w_Shape2D_is_intersect()
{
    auto l = ((tt::Proxy<gs::Shape2D>*)ves_toforeign(0))->obj;

    float x = (float)ves_tonumber(1);
    float y = (float)ves_tonumber(2);
    float w = (float)ves_tonumber(3);
    float h = (float)ves_tonumber(4);

    ves_set_boolean(0, l->IsIntersect(sm::rect(x, y, x + w, y + h)));
}

void w_Shape2D_translate()
{
    auto l = ((tt::Proxy<gs::Shape2D>*)ves_toforeign(0))->obj;

    float dx = (float)ves_tonumber(1);
    float dy = (float)ves_tonumber(2);

    l->Translate(dx, dy);
}

void w_Line_allocate()
{
    auto proxy = (tt::Proxy<gs::Line2D>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<gs::Line2D>));
    proxy->obj = std::make_shared<gs::Line2D>();
}

int w_Line_finalize(void* data)
{
    auto proxy = (tt::Proxy<gs::Line2D>*)(data);
    proxy->~Proxy();
    return sizeof(tt::Proxy<gs::Line2D>);
}

void w_Line_clone()
{
    auto src = ((tt::Proxy<gs::Line2D>*)ves_toforeign(0))->obj;
    auto dst = std::make_shared<gs::Line2D>(src->GetStart(), src->GetEnd());

    ves_pop(ves_argnum());
    auto proxy = (tt::Proxy<gs::Line2D>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<gs::Line2D>));
    proxy->obj = dst;
}

void w_Line_get()
{
    auto l = ((tt::Proxy<gs::Line2D>*)ves_toforeign(0))->obj;
    tt::return_list(std::vector<float>{ 
        l->GetStart().x, 
        l->GetStart().y,
        l->GetEnd().x,
        l->GetEnd().y
    });
}

void w_Line_set()
{
    auto l = ((tt::Proxy<gs::Line2D>*)ves_toforeign(0))->obj;

    float x0 = (float)ves_tonumber(1);
    float y0 = (float)ves_tonumber(2);
    float x1 = (float)ves_tonumber(3);
    float y1 = (float)ves_tonumber(4);

    l->SetStart({ x0, y0 });
    l->SetEnd({ x1, y1 });
}

void w_Rect_allocate()
{
    auto proxy = (tt::Proxy<gs::Rect>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<gs::Rect>));
    proxy->obj = std::make_shared<gs::Rect>();
}

int w_Rect_finalize(void* data)
{
    auto proxy = (tt::Proxy<gs::Rect>*)(data);
    proxy->~Proxy();
    return sizeof(tt::Proxy<gs::Rect>);
}

void w_Rect_clone()
{
    auto src = ((tt::Proxy<gs::Rect>*)ves_toforeign(0))->obj;
    auto dst = std::make_shared<gs::Rect>(src->GetRect());

    ves_pop(ves_argnum());
    auto proxy = (tt::Proxy<gs::Rect>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<gs::Rect>));
    proxy->obj = dst;
}

void w_Rect_get()
{
    auto r = ((tt::Proxy<gs::Rect>*)ves_toforeign(0))->obj;

    auto& rect = r->GetRect();
    tt::return_list(std::vector<float>{
        rect.xmin,
        rect.ymin,
        rect.Width(),
        rect.Height()
    });
}

void w_Rect_set()
{
    auto r = ((tt::Proxy<gs::Rect>*)ves_toforeign(0))->obj;

    float x = (float)ves_tonumber(1);
    float y = (float)ves_tonumber(2);
    float w = (float)ves_tonumber(3);
    float h = (float)ves_tonumber(4);

    r->SetRect(sm::rect(x, y, x + w, y + h));
}

void w_Circle_allocate()
{
    auto proxy = (tt::Proxy<gs::Circle>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<gs::Circle>));
    proxy->obj = std::make_shared<gs::Circle>();
}

int w_Circle_finalize(void* data)
{
    auto proxy = (tt::Proxy<gs::Circle>*)(data);
    proxy->~Proxy();
    return sizeof(tt::Proxy<gs::Circle>);
}

void w_Circle_clone()
{
    auto src = ((tt::Proxy<gs::Circle>*)ves_toforeign(0))->obj;
    auto dst = std::make_shared<gs::Circle>(src->GetCenter(), src->GetRadius());

    ves_pop(ves_argnum());
    auto proxy = (tt::Proxy<gs::Circle>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<gs::Circle>));
    proxy->obj = dst;
}

void w_Circle_get()
{
    auto c = ((tt::Proxy<gs::Circle>*)ves_toforeign(0))->obj;
    tt::return_list(std::vector<float>{
        c->GetCenter().x,
        c->GetCenter().y,
        c->GetRadius()
    });
}

void w_Circle_set()
{
    auto c = ((tt::Proxy<gs::Circle>*)ves_toforeign(0))->obj;

    float cx = (float)ves_tonumber(1);
    float cy = (float)ves_tonumber(2);
    float r  = (float)ves_tonumber(3);

    c->SetCenter({ cx, cy });
    c->SetRadius(r);
}

void w_Polyline_allocate()
{
    auto proxy = (tt::Proxy<gs::Polyline2D>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<gs::Polyline2D>));
    proxy->obj = std::make_shared<gs::Polyline2D>();
}

int w_Polyline_finalize(void* data)
{
    auto proxy = (tt::Proxy<gs::Polyline2D>*)(data);
    proxy->~Proxy();
    return sizeof(tt::Proxy<gs::Polyline2D>);
}

void w_Polyline_clone()
{
    auto src = ((tt::Proxy<gs::Polyline2D>*)ves_toforeign(0))->obj;
    auto dst = std::make_shared<gs::Polyline2D>(src->GetVertices());

    ves_pop(ves_argnum());
    auto proxy = (tt::Proxy<gs::Polyline2D>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<gs::Polyline2D>));
    proxy->obj = dst;
}

void w_Polyline_get_vertices()
{
    auto pl = ((tt::Proxy<gs::Polyline2D>*)ves_toforeign(0))->obj;
    auto& vertices = pl->GetVertices();
    tt::return_list(vertices);
}

void w_Polyline_set_vertices()
{
    auto polyline = ((tt::Proxy<gs::Polyline2D>*)ves_toforeign(0))->obj;
    auto vertices = tt::list_to_vec2_array(1);
    polyline->SetVertices(vertices);
}

void w_Polyline_get_closed()
{
    auto polyline = ((tt::Proxy<gs::Polyline2D>*)ves_toforeign(0))->obj;
    ves_set_boolean(0, polyline->GetClosed());
}

void w_Polyline_set_closed()
{
    auto polyline = ((tt::Proxy<gs::Polyline2D>*)ves_toforeign(0))->obj;
    bool is_closed = ves_toboolean(1);
    polyline->SetClosed(is_closed);
}

void w_Polyline_resample()
{
    auto polyline = ((tt::Proxy<gs::Polyline2D>*)ves_toforeign(0))->obj;
    float length = (float)ves_tonumber(1);
    auto pts = polyline->Resample(length);
    tt::return_list(pts);
}

void w_Polygon_allocate()
{
    auto proxy = (tt::Proxy<gs::Polygon2D>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<gs::Polygon2D>));
    proxy->obj = std::make_shared<gs::Polygon2D>();
}

int w_Polygon_finalize(void* data)
{
    auto proxy = (tt::Proxy<gs::Polygon2D>*)(data);
    proxy->~Proxy();
    return sizeof(tt::Proxy<gs::Polygon2D>);
}

void w_Polygon_clone()
{
    auto src = ((tt::Proxy<gs::Polygon2D>*)ves_toforeign(0))->obj;
    auto dst = std::make_shared<gs::Polygon2D>(src->GetVertices());

    ves_pop(ves_argnum());
    auto proxy = (tt::Proxy<gs::Polygon2D>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<gs::Polygon2D>));
    proxy->obj = dst;
}

void w_Polygon_get_vertices()
{
    auto poly = ((tt::Proxy<gs::Polygon2D>*)ves_toforeign(0))->obj;
    auto& vertices = poly->GetVertices();
    tt::return_list(vertices);
}

void w_Polygon_set_vertices()
{
    auto poly = ((tt::Proxy<gs::Polygon2D>*)ves_toforeign(0))->obj;
    auto vertices = tt::list_to_vec2_array(1);
    poly->SetVertices(vertices);
}

void w_Polygon_add_hole()
{
    auto poly = ((tt::Proxy<gs::Polygon2D>*)ves_toforeign(0))->obj;
    auto hole = tt::list_to_vec2_array(1);
    poly->AddHole(hole);
}

void w_Polygon_get_holes()
{
    auto poly = ((tt::Proxy<gs::Polygon2D>*)ves_toforeign(0))->obj;
    auto& holes = poly->GetHoles();
    tt::return_list(holes);
}

void w_Polygon_get_tris()
{
    auto poly = ((tt::Proxy<gs::Polygon2D>*)ves_toforeign(0))->obj;
    auto& tris = poly->GetTris();
    tt::return_list(tris);
}

void w_Polygon_calc_area()
{
    auto poly = ((tt::Proxy<gs::Polygon2D>*)ves_toforeign(0))->obj;
    auto area = sm::get_polygon_area(poly->GetVertices());
    ves_set_number(0, area);
}

void w_Bezier_allocate()
{
    auto proxy = (tt::Proxy<gs::Bezier>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<gs::Bezier>));
    proxy->obj = std::make_shared<gs::Bezier>();
}

int w_Bezier_finalize(void* data)
{
    auto proxy = (tt::Proxy<gs::Bezier>*)(data);
    proxy->~Proxy();
    return sizeof(tt::Proxy<gs::Bezier>);
}

void w_Bezier_clone()
{
    auto src = ((tt::Proxy<gs::Bezier>*)ves_toforeign(0))->obj;
    auto dst = std::make_shared<gs::Bezier>(src->GetCtrlPos());

    ves_pop(ves_argnum());
    auto proxy = (tt::Proxy<gs::Bezier>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<gs::Bezier>));
    proxy->obj = dst;
}

void w_Bezier_set_ctrl_pos()
{
    auto b = ((tt::Proxy<gs::Bezier>*)ves_toforeign(0))->obj;
    auto vertices = tt::list_to_vec2_array(1);
    GD_ASSERT(vertices.size() == 4, "error number");
    b->SetCtrlPos({ vertices[0], vertices[1], vertices[2], vertices[3] });
}

void w_Line3D_allocate()
{
    auto proxy = (tt::Proxy<gs::Line3D>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<gs::Line3D>));
    proxy->obj = std::make_shared<gs::Line3D>();
}

int w_Line3D_finalize(void* data)
{
    auto proxy = (tt::Proxy<gs::Line3D>*)(data);
    proxy->~Proxy();
    return sizeof(tt::Proxy<gs::Line3D>);
}

void w_Line3D_get_p0()
{
    auto l = ((tt::Proxy<gs::Line3D>*)ves_toforeign(0))->obj;

    auto p0 = l->GetStart();
    tt::return_list(std::vector<float>{ p0.x, p0.y, p0.z });
}

void w_Line3D_set_p0()
{
    auto l = ((tt::Proxy<gs::Line3D>*)ves_toforeign(0))->obj;
    auto p0 = tt::list_to_vec3(1);

    l->SetStart(p0);
}

void w_Line3D_get_p1()
{
    auto l = ((tt::Proxy<gs::Line3D>*)ves_toforeign(0))->obj;

    auto p1 = l->GetEnd();
    tt::return_list(std::vector<float>{ p1.x, p1.y, p1.z });
}

void w_Line3D_set_p1()
{
    auto l = ((tt::Proxy<gs::Line3D>*)ves_toforeign(0))->obj;
    auto p1 = tt::list_to_vec3(1);

    l->SetEnd(p1);
}

void w_Box_allocate()
{
    auto proxy = (tt::Proxy<gs::Box>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<gs::Box>));
    proxy->obj = std::make_shared<gs::Box>();
}

int w_Box_finalize(void* data)
{
    auto proxy = (tt::Proxy<gs::Box>*)(data);
    proxy->~Proxy();
    return sizeof(tt::Proxy<gs::Box>);
}

void w_Box_get_min()
{
    auto b = ((tt::Proxy<gs::Box>*)ves_toforeign(0))->obj;

    auto min = b->GetCube().Min();
    tt::return_list(std::vector<float>{ min[0], min[1], min[2] });
}

void w_Box_get_max()
{
    auto b = ((tt::Proxy<gs::Box>*)ves_toforeign(0))->obj;

    auto max = b->GetCube().Max();
    tt::return_list(std::vector<float>{ max[0], max[1], max[2] });
}

void w_Box_set_size()
{
    auto b = ((tt::Proxy<gs::Box>*)ves_toforeign(0))->obj;
    auto min = tt::list_to_vec3(1);
    auto max = tt::list_to_vec3(2);

    b->SetCube(sm::cube(min, max));
}

void w_Polyline3D_allocate()
{
    auto proxy = (tt::Proxy<gs::Polyline3D>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<gs::Polyline3D>));
    proxy->obj = std::make_shared<gs::Polyline3D>();
}

int w_Polyline3D_finalize(void* data)
{
    auto proxy = (tt::Proxy<gs::Polyline3D>*)(data);
    proxy->~Proxy();
    return sizeof(tt::Proxy<gs::Polyline3D>);
}

void w_Polyline3D_get_vertices()
{
    auto pl = ((tt::Proxy<gs::Polyline3D>*)ves_toforeign(0))->obj;
    auto& vertices = pl->GetVertices();
    tt::return_list(vertices);
}

void w_Polyline3D_set_vertices()
{
    auto polyline = ((tt::Proxy<gs::Polyline3D>*)ves_toforeign(0))->obj;
    auto vertices = tt::list_to_vec3_array(1);
    polyline->SetVertices(vertices);
}

void w_Polyline3D_get_closed()
{
    auto polyline = ((tt::Proxy<gs::Polyline3D>*)ves_toforeign(0))->obj;
    ves_set_boolean(0, polyline->GetClosed());
}

void w_Polyline3D_set_closed()
{
    auto polyline = ((tt::Proxy<gs::Polyline3D>*)ves_toforeign(0))->obj;
    bool is_closed = ves_toboolean(1);
    polyline->SetClosed(is_closed);
}

void w_Polygon3D_allocate()
{
    auto proxy = (tt::Proxy<gs::Polygon3D>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<gs::Polygon3D>));
    proxy->obj = std::make_shared<gs::Polygon3D>();
}

int w_Polygon3D_finalize(void* data)
{
    auto proxy = (tt::Proxy<gs::Polygon3D>*)(data);
    proxy->~Proxy();
    return sizeof(tt::Proxy<gs::Polygon3D>);
}

void w_Polygon3D_get_vertices()
{
    auto pl = ((tt::Proxy<gs::Polygon3D>*)ves_toforeign(0))->obj;
    auto& vertices = pl->GetVertices();
    tt::return_list(vertices);
}

void w_Polygon3D_set_vertices()
{
    auto polyline = ((tt::Proxy<gs::Polygon3D>*)ves_toforeign(0))->obj;
    auto vertices = tt::list_to_vec3_array(1);
    polyline->SetVertices(vertices);
}

void w_PolyPoint_allocate()
{
    auto pos = tt::list_to_vec3(1);
    auto p = std::make_shared<pm3::Polytope::Point>(pos);

    auto proxy = (tt::Proxy<pm3::Polytope::Point>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<pm3::Polytope::Point>));
    proxy->obj = p;
}

int w_PolyPoint_finalize(void* data)
{
    auto proxy = (tt::Proxy<pm3::Polytope::Point>*)(data);
    proxy->~Proxy();
    return sizeof(tt::Proxy<pm3::Polytope::Point>);
}

void w_PolyPoint_clone()
{
    auto src = ((tt::Proxy<pm3::Polytope::Point>*)ves_toforeign(0))->obj;
    auto dst = std::make_shared<pm3::Polytope::Point>(*src);

    ves_pop(ves_argnum());
    auto proxy = (tt::Proxy<pm3::Polytope::Point>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<pm3::Polytope::Point>));
    proxy->obj = dst;
}

void w_PolyPoint_get_pos()
{
    auto p = ((tt::Proxy<pm3::Polytope::Point>*)ves_toforeign(0))->obj;
    tt::return_list(std::vector<float>{ p->pos.x, p->pos.y, p->pos.z });
}

void w_PolyPoint_set_pos()
{
    auto p = ((tt::Proxy<pm3::Polytope::Point>*)ves_toforeign(0))->obj;
    auto pos = tt::list_to_vec3(1);
    p->pos = pos;
}

void w_PolyFace_allocate()
{
    auto face = std::make_shared<pm3::Polytope::Face>();

    auto num = ves_argnum();
    if (num == 2)
    {
        face->plane = *(sm::Plane*)ves_toforeign(1);
    }
    else if (num == 3)
    {
        auto border = tt::list_to_array<int>(1);
        std::copy(border.begin(), border.end(), std::back_inserter(face->border));

        face->holes = tt::list_to_array2<size_t>(2);
    }

    auto proxy = (tt::Proxy<pm3::Polytope::Face>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<pm3::Polytope::Face>));
    proxy->obj = face;
}

int w_PolyFace_finalize(void* data)
{
    auto proxy = (tt::Proxy<pm3::Polytope::Face>*)(data);
    proxy->~Proxy();
    return sizeof(tt::Proxy<pm3::Polytope::Face>);
}

void w_PolyFace_clone()
{
    auto src = ((tt::Proxy<pm3::Polytope::Face>*)ves_toforeign(0))->obj;
    auto dst = std::make_shared<pm3::Polytope::Face>(*src);

    ves_pop(ves_argnum());
    auto proxy = (tt::Proxy<pm3::Polytope::Face>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<pm3::Polytope::Face>));
    proxy->obj = dst;
}

void w_PolyFace_get_normal()
{
    auto f = ((tt::Proxy<pm3::Polytope::Face>*)ves_toforeign(0))->obj;
    tt::return_list(std::vector<float>{ f->plane.normal.x, f->plane.normal.y, f->plane.normal.z });
}

void w_PolyFace_flip_normal()
{
    auto f = ((tt::Proxy<pm3::Polytope::Face>*)ves_toforeign(0))->obj;
    std::reverse(f->border.begin(), f->border.end());
    f->plane.Flip();
}

void w_PolyFace_get_border()
{
    auto f = ((tt::Proxy<pm3::Polytope::Face>*)ves_toforeign(0))->obj;
    tt::return_list(f->border);
}

void w_PolyFace_get_holes()
{
    auto f = ((tt::Proxy<pm3::Polytope::Face>*)ves_toforeign(0))->obj;
    tt::return_list2(f->holes);
}

void w_Polytope_allocate()
{
    std::shared_ptr<pm3::Polytope> poly = nullptr;

    auto argnum = ves_argnum();
    if (argnum == 2)
    {
        std::vector<pm3::Polytope::FacePtr> faces;
        tt::list_to_foreigns(1, faces);

        poly = std::make_shared<pm3::Polytope>(faces);
    }
    else if (argnum == 3)
    {
        std::vector<pm3::Polytope::PointPtr> points;
        tt::list_to_foreigns(1, points);

        std::vector<pm3::Polytope::FacePtr> faces;
        tt::list_to_foreigns(2, faces);

        poly = std::make_shared<pm3::Polytope>(points, faces);
    }

    auto proxy = (tt::Proxy<pm3::Polytope>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<pm3::Polytope>));
    proxy->obj = poly;
}

int w_Polytope_finalize(void* data)
{
    auto proxy = (tt::Proxy<pm3::Polytope>*)(data);
    proxy->~Proxy();
    return sizeof(tt::Proxy<pm3::Polytope>);
}

void w_Polytope_clone()
{
    auto src = ((tt::Proxy<pm3::Polytope>*)ves_toforeign(0))->obj;
    auto dst = std::make_shared<pm3::Polytope>();
    *dst = *src;

    ves_pop(ves_argnum());
    auto proxy = (tt::Proxy<pm3::Polytope>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<pm3::Polytope>));
    proxy->obj = dst;
}

void w_Polytope_extrude()
{
    auto poly = ((tt::Proxy<pm3::Polytope>*)ves_toforeign(0))->obj;
    auto dist = (float)ves_tonumber(1);

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

void w_Polytope_offset()
{
    auto poly = ((tt::Proxy<pm3::Polytope>*)ves_toforeign(0))->obj;
    auto selector = ves_tostring(1);
    auto dist = (float)ves_tonumber(2);

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

        he::Polygon::KeepType keep = he::Polygon::KeepType::KeepInside;
        if (strcmp(selector, "all") == 0) {
            keep = he::Polygon::KeepType::KeepAll;
        } else if (strcmp(selector, "inside") == 0) {
            keep = he::Polygon::KeepType::KeepInside;
        } else if (strcmp(selector, "border") == 0) {
            keep = he::Polygon::KeepType::KeepBorder;
        }
        topo_poly.GetPoly()->Offset(dist, keep);

        topo_poly.TransToPolymesh(dst_pts, dst_faces);
    }

    auto proxy = (tt::Proxy<pm3::Polytope>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<pm3::Polytope>));
    proxy->obj = std::make_shared<pm3::Polytope>(dst_pts, dst_faces);
}

void w_Polytope_transform()
{
    auto poly = ((tt::Proxy<pm3::Polytope>*)ves_toforeign(0))->obj;

    sm::mat4* mt = (sm::mat4*)ves_toforeign(1);

    auto& pts = poly->Points();
    for (auto& p : pts) {
        p->pos = *mt * p->pos;
    }

    poly->SetTopoDirty();
}

void w_Polytope_mirror()
{
    auto poly = ((tt::Proxy<pm3::Polytope>*)ves_toforeign(0))->obj;
    sm::Plane* plane = (sm::Plane*)ves_toforeign(1);

    float len_s = plane->normal.LengthSquared();

    // mirror points
    for (auto& p : poly->Points()) {
        p->pos = sm::calc_plane_mirror(*plane, p->pos);
    }

    // mirror faces
    for (auto& f : poly->Faces())
    {
        auto p0 = poly->Points()[f->border.front()]->pos;
        auto p1 = p0 + f->plane.normal;
        p0 = sm::calc_plane_mirror(*plane, p0);
        p1 = sm::calc_plane_mirror(*plane, p1);
        f->plane.normal = p1 - p0;
        f->plane.dist = -p0.Dot(f->plane.normal);
    }

    poly->SortVertices();

    poly->SetTopoDirty();
}

void w_Polytope_clip()
{
    auto poly = ((tt::Proxy<pm3::Polytope>*)ves_toforeign(0))->obj;
    sm::Plane* plane = (sm::Plane*)ves_toforeign(1);

    auto keep_str = ves_tostring(2);
    auto keep = he::Polyhedron::KeepType::KeepAbove;
    if (strcmp(keep_str, "above") == 0) {
        keep = he::Polyhedron::KeepType::KeepAbove;
    } else if (strcmp(keep_str, "below") == 0) {
        keep = he::Polyhedron::KeepType::KeepBelow;
    } else if (strcmp(keep_str, "all") == 0) {
        keep = he::Polyhedron::KeepType::KeepAll;
    }

    auto seam_face = ves_toboolean(3);
    if (poly->GetTopoPoly()->Clip(*plane, keep, seam_face)) {
        poly->BuildFromTopo();
        ves_set_boolean(0, true);
    } else {
        ves_set_boolean(0, false);
    }
}

void w_Polytope_get_points()
{
    auto poly = ((tt::Proxy<pm3::Polytope>*)ves_toforeign(0))->obj;

    auto& points = poly->Points();
    
    ves_pop(ves_argnum());

    const int num = (int)(points.size());
    ves_newlist(num);
    for (int i = 0; i < num; ++i)
    {
        ves_pushnil();
        ves_import_class("geometry", "PolyPoint");
        auto proxy = (tt::Proxy<pm3::Polytope::Point>*)ves_set_newforeign(1, 2, sizeof(tt::Proxy<pm3::Polytope::Point>));
        proxy->obj = points[i];
        ves_pop(1);
        ves_seti(-2, i);
        ves_pop(1);
    }
}

void w_Polytope_get_faces()
{
    auto poly = ((tt::Proxy<pm3::Polytope>*)ves_toforeign(0))->obj;

    auto& faces = poly->Faces();
    
    ves_pop(ves_argnum());

    const int num = (int)(faces.size());
    ves_newlist(num);
    for (int i = 0; i < num; ++i)
    {
        ves_pushnil();
        ves_import_class("geometry", "PolyFace");
        auto proxy = (tt::Proxy<pm3::Polytope::Face>*)ves_set_newforeign(1, 2, sizeof(tt::Proxy<pm3::Polytope::Face>));
        proxy->obj = faces[i];
        ves_pop(1);
        ves_seti(-2, i);
        ves_pop(1);
    }
}

void w_Polytope_set_topo_dirty()
{
    auto poly = ((tt::Proxy<pm3::Polytope>*)ves_toforeign(0))->obj;
    poly->SetTopoDirty();
}

void w_Polytope_is_contain()
{
    auto poly = ((tt::Proxy<pm3::Polytope>*)ves_toforeign(0))->obj;
    auto pos = tt::list_to_vec3(1);

    bool is_contain = true;

    auto& faces = poly->Faces();
    for (auto& f : faces)
    {
        sm::vec3 normal;
        if (!poly->CalcFaceNormal(*f, normal)) {
            continue;
        }

        sm::Plane plane;
        plane.Build(normal, poly->Points()[f->border[0]]->pos);

        if (plane.GetDistance(pos) < -SM_LARGE_EPSILON) {
            is_contain = false;
            break;
        }
    }

    ves_set_boolean(0, is_contain);
}

void w_Polytope_is_face_inside()
{
    auto poly = ((tt::Proxy<pm3::Polytope>*)ves_toforeign(0))->obj;
    auto face = ((tt::Proxy<pm3::Polytope::Face>*)ves_toforeign(1))->obj;

    sm::vec3 normal;
    if (!poly->CalcFaceNormal(*face, normal)) {
        ves_set_nil(0);
        return;
    }

    auto& pts = poly->Points();

    sm::Plane plane;
    plane.Build(normal, pts[face->border[0]]->pos);

    bool is_inside = false;
    for (auto& p : pts) {
        if (plane.GetDistance(p->pos) > SM_LARGE_EPSILON) {
            is_inside = true;
            break;
        }
    }

    ves_set_boolean(0, is_inside);
}

static std::vector<pm3::PolytopePtr> 
intersect_poly_list(const std::vector<pm3::PolytopePtr>& a, const std::vector<pm3::PolytopePtr>& b)
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

static std::vector<pm3::PolytopePtr>
subtract_poly_list(const std::vector<pm3::PolytopePtr>& a, const std::vector<pm3::PolytopePtr>& b)
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

                return subtract_poly_list(a_left, b_left);
            }
        }
    }
    return ret;
}

void w_Polytope_boolean()
{
    auto op = ves_tostring(1);

    std::vector<std::shared_ptr<pm3::Polytope>> a, b, polytopes;
    tt::list_to_foreigns(2, a);
    tt::list_to_foreigns(3, b);

    if (strcmp(op, "union") == 0) 
    {
        auto intersect = intersect_poly_list(a, b);
        if (intersect.empty())
        {
            std::copy(a.begin(), a.end(), std::back_inserter(polytopes));
            std::copy(b.begin(), b.end(), std::back_inserter(polytopes));
        }
        else
        {
            auto a_left = subtract_poly_list(a, intersect);
            auto b_left = subtract_poly_list(b, intersect);
            polytopes = intersect;
            std::copy(a_left.begin(), a_left.end(), std::back_inserter(polytopes));
            std::copy(b_left.begin(), b_left.end(), std::back_inserter(polytopes));
        }
    }
    else if (strcmp(op, "intersect") == 0)
    {
        polytopes = intersect_poly_list(a, b);
    }
    else if (strcmp(op, "subtract") == 0)
    {
        polytopes = a;
        for (auto& cb : b)
        {
            auto intersect = intersect_poly_list(polytopes, { cb });
            if (!intersect.empty()) {
                polytopes = subtract_poly_list(polytopes, intersect);
            }
        }
    }

    ves_pop(ves_argnum());

    const int num = (int)polytopes.size();
    ves_newlist(num);
    for (int i = 0; i < num; ++i)
    {
        ves_pushnil();
        ves_import_class("geometry", "Polytope");
        auto proxy = (tt::Proxy<pm3::Polytope>*)ves_set_newforeign(1, 2, sizeof(tt::Proxy<pm3::Polytope>));
        proxy->obj = polytopes[i];
        ves_pop(1);
        ves_seti(-2, i);
        ves_pop(1);
    }
}

void w_Sphere_allocate()
{
    auto radius = ves_tonumber(1);
    auto proxy = (tt::Proxy<model::Sphere>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<model::Sphere>));
    proxy->obj = std::make_shared<model::Sphere>((float)radius);
}

int w_Sphere_finalize(void* data)
{
    auto proxy = (tt::Proxy<model::Sphere>*)(data);
    proxy->~Proxy();
    return sizeof(tt::Proxy<model::Sphere>);
}

void w_Sphere_clone()
{
    auto src = ((tt::Proxy<model::Sphere>*)ves_toforeign(0))->obj;
    auto dst = std::make_shared<model::Sphere>(src->GetRadius());

    ves_pop(ves_argnum());
    auto proxy = (tt::Proxy<model::Sphere>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<model::Sphere>));
    proxy->obj = dst;
}

void w_Ellipsoid_allocate()
{
    auto radius = tt::list_to_vec3(1);
    auto proxy = (tt::Proxy<model::Ellipsoid>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<model::Ellipsoid>));
    proxy->obj = std::make_shared<model::Ellipsoid>(radius);
}

int w_Ellipsoid_finalize(void* data)
{
    auto proxy = (tt::Proxy<model::Ellipsoid>*)(data);
    proxy->~Proxy();
    return sizeof(tt::Proxy<model::Ellipsoid>);
}

void w_Ellipsoid_clone()
{
    auto src = ((tt::Proxy<model::Ellipsoid>*)ves_toforeign(0))->obj;
    auto dst = std::make_shared<model::Ellipsoid>(src->GetRadius());

    ves_pop(ves_argnum());
    auto proxy = (tt::Proxy<model::Ellipsoid>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<model::Ellipsoid>));
    proxy->obj = dst;
}

void w_Constraint_allocate()
{
    const char* type_str = ves_tostring(1);
    int geo0 = (int)ves_optnumber(2, -1);
    int geo1 = (int)ves_optnumber(3, -1);
    double value = ves_tonumber(4);

    ct2::ConstraintType type = ct2::ConstraintType::None;
    if (strcmp(type_str, "distance") == 0) {
        type = ct2::ConstraintType::Distance;
    }

    auto proxy = (tt::Proxy<ct2::Constraint>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<ct2::Constraint>));
    proxy->obj = std::make_shared<ct2::Constraint>(type, geo0, geo1, value);
}

int w_Constraint_finalize(void* data)
{
    auto proxy = (tt::Proxy<ct2::Constraint>*)(data);
    proxy->~Proxy();
    return sizeof(tt::Proxy<ct2::Constraint>);
}

void w_Constraint_set_value()
{
    auto cons = ((tt::Proxy<ct2::Constraint>*)ves_toforeign(0))->obj;
    double value = ves_tonumber(1);
    cons->SetValue(value);
}

void w_ConstraintSolver_allocate()
{
    auto proxy = (tt::Proxy<ct2::Scene>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<ct2::Scene>));
    proxy->obj = std::make_shared<ct2::Scene>();
}

int w_ConstraintSolver_finalize(void* data)
{
    auto proxy = (tt::Proxy<ct2::Scene>*)(data);
    proxy->~Proxy();
    return sizeof(tt::Proxy<ct2::Scene>);
}

void w_ConstraintSolver_add_geo()
{
    auto scene = ((tt::Proxy<ct2::Scene>*)ves_toforeign(0))->obj;
    auto shape = ((tt::Proxy<gs::Shape2D>*)ves_toforeign(1))->obj;

    int id = scene->AddGeometry(shape);
    ves_set_number(0, id);
}

void w_ConstraintSolver_add_cons()
{
    auto scene = ((tt::Proxy<ct2::Scene>*)ves_toforeign(0))->obj;
    auto cons = ((tt::Proxy<ct2::Constraint>*)ves_toforeign(1))->obj;

    int id = scene->AddConstraint(cons);
    ves_set_number(0, id);
}

void w_ConstraintSolver_solve()
{
    auto scene = ((tt::Proxy<ct2::Scene>*)ves_toforeign(0))->obj;
    scene->Solve();
}

void w_ConstraintSolver_clear()
{
    auto scene = ((tt::Proxy<ct2::Scene>*)ves_toforeign(0))->obj;
    scene->Clear();
}

}

namespace tt
{

VesselForeignMethodFn GeometryBindMethod(const char* signature)
{
    if (strcmp(signature, "Shape2D.is_contain(_,_)") == 0) return w_Shape2D_is_contain;
    if (strcmp(signature, "Shape2D.is_intersect(_,_,_,_)") == 0) return w_Shape2D_is_intersect;
    if (strcmp(signature, "Shape2D.translate(_,_)") == 0) return w_Shape2D_translate;

    if (strcmp(signature, "Line.clone()") == 0) return w_Line_clone;
    if (strcmp(signature, "Line.get()") == 0) return w_Line_get;
    if (strcmp(signature, "Line.set(_,_,_,_)") == 0) return w_Line_set;

    if (strcmp(signature, "Rect.clone()") == 0) return w_Rect_clone;
    if (strcmp(signature, "Rect.get()") == 0) return w_Rect_get;
    if (strcmp(signature, "Rect.set(_,_,_,_)") == 0) return w_Rect_set;

    if (strcmp(signature, "Circle.clone()") == 0) return w_Circle_clone;
    if (strcmp(signature, "Circle.get()") == 0) return w_Circle_get;
    if (strcmp(signature, "Circle.set(_,_,_)") == 0) return w_Circle_set;

    if (strcmp(signature, "Polyline.clone()") == 0) return w_Polyline_clone;
    if (strcmp(signature, "Polyline.get_vertices()") == 0) return w_Polyline_get_vertices;
    if (strcmp(signature, "Polyline.set_vertices(_)") == 0) return w_Polyline_set_vertices;
    if (strcmp(signature, "Polyline.get_closed()") == 0) return w_Polyline_get_closed;
    if (strcmp(signature, "Polyline.set_closed(_)") == 0) return w_Polyline_set_closed;
    if (strcmp(signature, "Polyline.resample(_)") == 0) return w_Polyline_resample;

    if (strcmp(signature, "Polygon.clone()") == 0) return w_Polygon_clone;
    if (strcmp(signature, "Polygon.get_vertices()") == 0) return w_Polygon_get_vertices;
    if (strcmp(signature, "Polygon.set_vertices(_)") == 0) return w_Polygon_set_vertices;
    if (strcmp(signature, "Polygon.add_hole(_)") == 0) return w_Polygon_add_hole;
    if (strcmp(signature, "Polygon.get_holes()") == 0) return w_Polygon_get_holes;
    if (strcmp(signature, "Polygon.get_tris()") == 0) return w_Polygon_get_tris;
    if (strcmp(signature, "Polygon.calc_area()") == 0) return w_Polygon_calc_area;

    if (strcmp(signature, "Bezier.clone()") == 0) return w_Bezier_clone;
    if (strcmp(signature, "Bezier.set_ctrl_pos(_)") == 0) return w_Bezier_set_ctrl_pos;

    if (strcmp(signature, "Line3D.get_p0()") == 0) return w_Line3D_get_p0;
    if (strcmp(signature, "Line3D.set_p0(_)") == 0) return w_Line3D_set_p0;
    if (strcmp(signature, "Line3D.get_p1()") == 0) return w_Line3D_get_p1;
    if (strcmp(signature, "Line3D.set_p1(_)") == 0) return w_Line3D_set_p1;

    if (strcmp(signature, "Box.get_min()") == 0) return w_Box_get_min;
    if (strcmp(signature, "Box.get_max()") == 0) return w_Box_get_max;
    if (strcmp(signature, "Box.set_size(_,_)") == 0) return w_Box_set_size;

    if (strcmp(signature, "Polyline3D.get_vertices()") == 0) return w_Polyline3D_get_vertices;
    if (strcmp(signature, "Polyline3D.set_vertices(_)") == 0) return w_Polyline3D_set_vertices;
    if (strcmp(signature, "Polyline3D.get_closed()") == 0) return w_Polyline3D_get_closed;
    if (strcmp(signature, "Polyline3D.set_closed(_)") == 0) return w_Polyline3D_set_closed;

    if (strcmp(signature, "Polygon3D.get_vertices()") == 0) return w_Polygon3D_get_vertices;
    if (strcmp(signature, "Polygon3D.set_vertices(_)") == 0) return w_Polygon3D_set_vertices;

    if (strcmp(signature, "PolyPoint.clone()") == 0) return w_PolyPoint_clone;
    if (strcmp(signature, "PolyPoint.get_pos()") == 0) return w_PolyPoint_get_pos;
    if (strcmp(signature, "PolyPoint.set_pos(_)") == 0) return w_PolyPoint_set_pos;
    if (strcmp(signature, "PolyFace.clone()") == 0) return w_PolyFace_clone;
    if (strcmp(signature, "PolyFace.get_normal()") == 0) return w_PolyFace_get_normal;
    if (strcmp(signature, "PolyFace.flip_normal()") == 0) return w_PolyFace_flip_normal;
    if (strcmp(signature, "PolyFace.get_border()") == 0) return w_PolyFace_get_border;
    if (strcmp(signature, "PolyFace.get_holes()") == 0) return w_PolyFace_get_holes;

    if (strcmp(signature, "Polytope.clone()") == 0) return w_Polytope_clone;
    if (strcmp(signature, "Polytope.extrude(_)") == 0) return w_Polytope_extrude;
    if (strcmp(signature, "Polytope.offset(_,_)") == 0) return w_Polytope_offset;
    if (strcmp(signature, "Polytope.transform(_)") == 0) return w_Polytope_transform;
    if (strcmp(signature, "Polytope.mirror(_)") == 0) return w_Polytope_mirror;
    if (strcmp(signature, "Polytope.clip(_,_,_)") == 0) return w_Polytope_clip;
    if (strcmp(signature, "Polytope.get_points()") == 0) return w_Polytope_get_points;
    if (strcmp(signature, "Polytope.get_faces()") == 0) return w_Polytope_get_faces;
    if (strcmp(signature, "Polytope.set_topo_dirty()") == 0) return w_Polytope_set_topo_dirty;
    if (strcmp(signature, "Polytope.is_contain(_)") == 0) return w_Polytope_is_contain;
    if (strcmp(signature, "Polytope.is_face_inside(_)") == 0) return w_Polytope_is_face_inside;
    if (strcmp(signature, "static Polytope.boolean(_,_,_)") == 0) return w_Polytope_boolean;

    if (strcmp(signature, "Sphere.clone()") == 0) return w_Sphere_clone;

    if (strcmp(signature, "Ellipsoid.clone()") == 0) return w_Ellipsoid_clone;

    if (strcmp(signature, "Constraint.set_value(_)") == 0) return w_Constraint_set_value;

    if (strcmp(signature, "ConstraintSolver.add_geo(_)") == 0) return w_ConstraintSolver_add_geo;
    if (strcmp(signature, "ConstraintSolver.add_cons(_)") == 0) return w_ConstraintSolver_add_cons;
    if (strcmp(signature, "ConstraintSolver.solve()") == 0) return w_ConstraintSolver_solve;
    if (strcmp(signature, "ConstraintSolver.clear()") == 0) return w_ConstraintSolver_clear;

    return nullptr;
}

void GeometryBindClass(const char* class_name, VesselForeignClassMethods* methods)
{
    if (strcmp(class_name, "Line") == 0)
    {
        methods->allocate = w_Line_allocate;
        methods->finalize = w_Line_finalize;
        return;
    }

    if (strcmp(class_name, "Rect") == 0)
    {
        methods->allocate = w_Rect_allocate;
        methods->finalize = w_Rect_finalize;
        return;
    }

    if (strcmp(class_name, "Circle") == 0)
    {
        methods->allocate = w_Circle_allocate;
        methods->finalize = w_Circle_finalize;
        return;
    }

    if (strcmp(class_name, "Polyline") == 0)
    {
        methods->allocate = w_Polyline_allocate;
        methods->finalize = w_Polyline_finalize;
        return;
    }

    if (strcmp(class_name, "Polygon") == 0)
    {
        methods->allocate = w_Polygon_allocate;
        methods->finalize = w_Polygon_finalize;
        return;
    }

    if (strcmp(class_name, "Bezier") == 0)
    {
        methods->allocate = w_Bezier_allocate;
        methods->finalize = w_Bezier_finalize;
        return;
    }

    if (strcmp(class_name, "Bezier") == 0)
    {
        methods->allocate = w_Bezier_allocate;
        methods->finalize = w_Bezier_finalize;
        return;
    }

    if (strcmp(class_name, "Line3D") == 0)
    {
        methods->allocate = w_Line3D_allocate;
        methods->finalize = w_Line3D_finalize;
        return;
    }

    if (strcmp(class_name, "Box") == 0)
    {
        methods->allocate = w_Box_allocate;
        methods->finalize = w_Box_finalize;
        return;
    }

    if (strcmp(class_name, "Polyline3D") == 0)
    {
        methods->allocate = w_Polyline3D_allocate;
        methods->finalize = w_Polyline3D_finalize;
        return;
    }

    if (strcmp(class_name, "Polygon3D") == 0)
    {
        methods->allocate = w_Polygon3D_allocate;
        methods->finalize = w_Polygon3D_finalize;
        return;
    }

    if (strcmp(class_name, "PolyPoint") == 0)
    {
        methods->allocate = w_PolyPoint_allocate;
        methods->finalize = w_PolyPoint_finalize;
        return;
    }

    if (strcmp(class_name, "PolyFace") == 0)
    {
        methods->allocate = w_PolyFace_allocate;
        methods->finalize = w_PolyFace_finalize;
        return;
    }

    if (strcmp(class_name, "Polytope") == 0)
    {
        methods->allocate = w_Polytope_allocate;
        methods->finalize = w_Polytope_finalize;
        return;
    }

    if (strcmp(class_name, "Sphere") == 0)
    {
        methods->allocate = w_Sphere_allocate;
        methods->finalize = w_Sphere_finalize;
        return;
    }

    if (strcmp(class_name, "Ellipsoid") == 0)
    {
        methods->allocate = w_Ellipsoid_allocate;
        methods->finalize = w_Ellipsoid_finalize;
        return;
    }

    if (strcmp(class_name, "Constraint") == 0)
    {
        methods->allocate = w_Constraint_allocate;
        methods->finalize = w_Constraint_finalize;
        return;
    }

    if (strcmp(class_name, "ConstraintSolver") == 0)
    {
        methods->allocate = w_ConstraintSolver_allocate;
        methods->finalize = w_ConstraintSolver_finalize;
        return;
    }
}

}