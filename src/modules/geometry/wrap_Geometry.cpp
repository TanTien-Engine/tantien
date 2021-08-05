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
#include <guard/check.h>
#include <constraints2/Scene.h>
#include <constraints2/Constraint.h>
#include <halfedge/Polygon.h>

#include <string>
#include <iterator>

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

    ves_pop(1);
    auto proxy = (tt::Proxy<gs::Line2D>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<gs::Line2D>));
    proxy->obj = dst;
}

void w_Line_get()
{
    auto l = ((tt::Proxy<gs::Line2D>*)ves_toforeign(0))->obj;

    ves_pop(1);
    ves_newlist(4);

    ves_pushnumber(l->GetStart().x);
    ves_seti(-2, 0);
    ves_pop(1);

    ves_pushnumber(l->GetStart().y);
    ves_seti(-2, 1);
    ves_pop(1);

    ves_pushnumber(l->GetEnd().x);
    ves_seti(-2, 2);
    ves_pop(1);

    ves_pushnumber(l->GetEnd().y);
    ves_seti(-2, 3);
    ves_pop(1);
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

    ves_pop(1);
    auto proxy = (tt::Proxy<gs::Rect>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<gs::Rect>));
    proxy->obj = dst;
}

void w_Rect_get()
{
    auto r = ((tt::Proxy<gs::Rect>*)ves_toforeign(0))->obj;

    ves_pop(1);
    ves_newlist(4);

    auto& rect = r->GetRect();

    ves_pushnumber(rect.xmin);
    ves_seti(-2, 0);
    ves_pop(1);

    ves_pushnumber(rect.ymin);
    ves_seti(-2, 1);
    ves_pop(1);

    ves_pushnumber(rect.xmax - rect.xmin);
    ves_seti(-2, 2);
    ves_pop(1);

    ves_pushnumber(rect.ymax - rect.ymin);
    ves_seti(-2, 3);
    ves_pop(1);
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

    ves_pop(1);
    auto proxy = (tt::Proxy<gs::Circle>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<gs::Circle>));
    proxy->obj = dst;
}

void w_Circle_get()
{
    auto c = ((tt::Proxy<gs::Circle>*)ves_toforeign(0))->obj;

    ves_pop(1);
    ves_newlist(3);

    ves_pushnumber(c->GetCenter().x);
    ves_seti(-2, 0);
    ves_pop(1);

    ves_pushnumber(c->GetCenter().y);
    ves_seti(-2, 1);
    ves_pop(1);

    ves_pushnumber(c->GetRadius());
    ves_seti(-2, 2);
    ves_pop(1);
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

    ves_pop(1);
    auto proxy = (tt::Proxy<gs::Polyline2D>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<gs::Polyline2D>));
    proxy->obj = dst;
}

void w_Polyline_get_vertices()
{
    auto pl = ((tt::Proxy<gs::Polyline2D>*)ves_toforeign(0))->obj;
    auto& vertices = pl->GetVertices();

    ves_pop(1);
    ves_newlist(int(vertices.size() * 2));
    for (int i = 0, n = (int)(vertices.size()); i < n; ++i)
    {
        ves_pushnumber(vertices[i].x);
        ves_seti(-2, i * 2);
        ves_pop(1);

        ves_pushnumber(vertices[i].y);
        ves_seti(-2, i * 2 + 1);
        ves_pop(1);
    }
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

    ves_pop(2);
    ves_newlist((int)(pts.size()) * 2);
    for (int i = 0, n = int(pts.size()); i < n; ++i)
    {
        ves_pushnumber(pts[i].x);
        ves_seti(-2, i * 2);
        ves_pop(1);

        ves_pushnumber(pts[i].y);
        ves_seti(-2, i * 2 + 1);
        ves_pop(1);
    }
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

    ves_pop(1);
    auto proxy = (tt::Proxy<gs::Polygon2D>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<gs::Polygon2D>));
    proxy->obj = dst;
}

void w_Polygon_get_vertices()
{
    auto pl = ((tt::Proxy<gs::Polygon2D>*)ves_toforeign(0))->obj;
    auto& vertices = pl->GetVertices();

    ves_pop(1);
    ves_newlist(int(vertices.size()) * 2);
    for (int i = 0, n = int(vertices.size()); i < n; ++i)
    {
        ves_pushnumber(vertices[i].x);
        ves_seti(-2, i * 2);
        ves_pop(1);

        ves_pushnumber(vertices[i].y);
        ves_seti(-2, i * 2 + 1);
        ves_pop(1);
    }
}

void w_Polygon_set_vertices()
{
    auto polyline = ((tt::Proxy<gs::Polygon2D>*)ves_toforeign(0))->obj;
    auto vertices = tt::list_to_vec2_array(1);
    polyline->SetVertices(vertices);
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

    ves_pop(1);
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

    ves_pop(1);
    ves_newlist(3);

    for (int i = 0; i < 3; ++i) {
        ves_pushnumber(p0.xyz[i]);
        ves_seti(-2, i);
        ves_pop(1);
    }
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

    ves_pop(1);
    ves_newlist(3);

    for (int i = 0; i < 3; ++i) {
        ves_pushnumber(p1.xyz[i]);
        ves_seti(-2, i);
        ves_pop(1);
    }
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

    ves_pop(1);
    ves_newlist(3);

    for (int i = 0; i < 3; ++i) {
        ves_pushnumber(min[i]);
        ves_seti(-2, i);
        ves_pop(1);
    }
}

void w_Box_get_max()
{
    auto b = ((tt::Proxy<gs::Box>*)ves_toforeign(0))->obj;

    auto max = b->GetCube().Max();

    ves_pop(1);
    ves_newlist(3);

    for (int i = 0; i < 3; ++i) {
        ves_pushnumber(max[i]);
        ves_seti(-2, i);
        ves_pop(1);
    }
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

    ves_pop(1);
    ves_newlist(int(vertices.size() * 3));
    for (int i = 0, n = (int)(vertices.size()); i < n; ++i)
    {
        ves_pushnumber(vertices[i].x);
        ves_seti(-2, i * 3);
        ves_pop(1);

        ves_pushnumber(vertices[i].y);
        ves_seti(-2, i * 3 + 1);
        ves_pop(1);

        ves_pushnumber(vertices[i].z);
        ves_seti(-2, i * 3 + 2);
        ves_pop(1);
    }
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

    ves_pop(1);
    ves_newlist(int(vertices.size() * 3));
    for (int i = 0, n = (int)(vertices.size()); i < n; ++i)
    {
        ves_pushnumber(vertices[i].x);
        ves_seti(-2, i * 3);
        ves_pop(1);

        ves_pushnumber(vertices[i].y);
        ves_seti(-2, i * 3 + 1);
        ves_pop(1);

        ves_pushnumber(vertices[i].z);
        ves_seti(-2, i * 3 + 2);
        ves_pop(1);
    }
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
        auto border = tt::list_to_int_array(1);
        std::copy(border.begin(), border.end(), std::back_inserter(face->border));

        std::vector<std::vector<size_t>> holes;
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

    ves_pop(1);
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

    auto& vertices = poly->GetTopoPoly()->GetVerts();
    auto vert = vertices.Head();
    do {
        vert->position = *mt * vert->position;
        vert = vert->linked_next;
    } while (vert != vertices.Head());

    poly->BuildFromTopo();
}

void w_Polytope_boolean()
{
    auto op = ves_tostring(1);
    auto a = ((tt::Proxy<pm3::Polytope>*)ves_toforeign(2))->obj;
    auto b = ((tt::Proxy<pm3::Polytope>*)ves_toforeign(3))->obj;

    auto topo_a = a->GetTopoPoly();
    auto topo_b = b->GetTopoPoly();
    if (!topo_a || !topo_b) {
        ves_set_nil(0);
        return;
    }

    std::vector<std::shared_ptr<pm3::Polytope>> polytopes;

    if (strcmp(op, "union") == 0) 
    {
    }
    else if (strcmp(op, "intersect") == 0)
    {
        auto poly = topo_a->Intersect(*topo_b);
        if (poly && poly->GetLoops().Size() > 0) {
            polytopes.push_back(std::make_shared<pm3::Polytope>(poly));
        }
    }
    else if (strcmp(op, "subtract") == 0)
    {
        auto polys = topo_a->Subtract(*topo_b);
        for (auto& poly : polys) {
            if (poly->GetLoops().Size() > 0) {
                polytopes.push_back(std::make_shared<pm3::Polytope>(poly));
            }
        }
    }

    ves_pop(ves_argnum());

    ves_newlist(polytopes.size());
    for (int i = 0, n = polytopes.size(); i < n; ++i)
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

    if (strcmp(signature, "Polytope.clone()") == 0) return w_Polytope_clone;
    if (strcmp(signature, "Polytope.extrude(_)") == 0) return w_Polytope_extrude;
    if (strcmp(signature, "Polytope.offset(_,_)") == 0) return w_Polytope_offset;
    if (strcmp(signature, "Polytope.transform(_)") == 0) return w_Polytope_transform;
    if (strcmp(signature, "static Polytope.boolean(_,_,_)") == 0) return w_Polytope_boolean;

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