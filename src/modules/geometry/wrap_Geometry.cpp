#include "modules/geometry/wrap_Geometry.h"
#include "modules/script/TransHelper.h"
#include "modules/script/Proxy.h"

#include <geoshape/Point2D.h>
#include <geoshape/Line2D.h>
#include <geoshape/Rect.h>
#include <geoshape/Circle.h>
#include <geoshape/Polyline2D.h>
#include <geoshape/Bezier.h>
#include <guard/check.h>
#include <constraints2/Scene.h>

#include <string>

namespace
{

void w_Line_allocate()
{
    float x0 = (float)ves_tonumber(1);
    float y0 = (float)ves_tonumber(2);
    float x1 = (float)ves_tonumber(3);
    float y1 = (float)ves_tonumber(4);

    auto proxy = (tt::Proxy<gs::Line2D>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<gs::Line2D>));
    proxy->obj = std::make_shared<gs::Line2D>(sm::vec2(x0, y0), sm::vec2(x1, y1));
}

int w_Line_finalize(void* data)
{
    auto proxy = (tt::Proxy<gs::Line2D>*)(data);
    proxy->~Proxy();
    return sizeof(tt::Proxy<gs::Line2D>);
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

void w_Rect_allocate()
{
    float x = (float)ves_tonumber(1);
    float y = (float)ves_tonumber(2);
    float w = (float)ves_tonumber(3);
    float h = (float)ves_tonumber(4);

    auto proxy = (tt::Proxy<gs::Rect>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<gs::Rect>));
    proxy->obj = std::make_shared<gs::Rect>(sm::rect(x, y, x + w, y + h));
}

int w_Rect_finalize(void* data)
{
    auto proxy = (tt::Proxy<gs::Rect>*)(data);
    proxy->~Proxy();
    return sizeof(tt::Proxy<gs::Rect>);
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

void w_Circle_allocate()
{
    float cx = (float)ves_tonumber(1);
    float cy = (float)ves_tonumber(2);
    float r  = (float)ves_tonumber(3);

    auto proxy = (tt::Proxy<gs::Circle>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<gs::Circle>));
    proxy->obj = std::make_shared<gs::Circle>(sm::vec2(cx, cy), r);
}

int w_Circle_finalize(void* data)
{
    auto proxy = (tt::Proxy<gs::Circle>*)(data);
    proxy->~Proxy();
    return sizeof(tt::Proxy<gs::Circle>);
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

void w_Polyline_set_vertices()
{
    auto polyline = ((tt::Proxy<gs::Polyline2D>*)ves_toforeign(0))->obj;
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

void w_Bezier_set_ctrl_pos()
{
    auto b = ((tt::Proxy<gs::Bezier>*)ves_toforeign(0))->obj;
    auto vertices = tt::list_to_vec2_array(1);
    GD_ASSERT(vertices.size() == 4, "error number");
    b->SetCtrlPos({ vertices[0], vertices[1], vertices[2], vertices[3] });
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

void w_ConstraintSolver_add_distance()
{
    auto scene = ((tt::Proxy<ct2::Scene>*)ves_toforeign(0))->obj;
    auto line = ((tt::Proxy<gs::Line2D>*)ves_toforeign(1))->obj;
    double dist = ves_tonumber(2);

    auto p0 = std::make_shared<gs::Point2D>(sm::vec2(line->GetStart()));
    auto p0_id = scene->AddPoint(p0);

    auto p1 = std::make_shared<gs::Point2D>(sm::vec2(line->GetEnd()));
    auto p1_id = scene->AddPoint(p1);

    auto cons = scene->AddDistanceConstraint(p0_id, ct2::Scene::PointPos::Start,
        p1_id, ct2::Scene::PointPos::Start, &dist);

    scene->Solve();

    line->SetStart(p0->GetPos());
    line->SetEnd(p1->GetPos());
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
    scene->ClearConstraints();
}

}

namespace tt
{

VesselForeignMethodFn GeometryBindMethod(const char* signature)
{
    if (strcmp(signature, "Line.get()") == 0) return w_Line_get;

    if (strcmp(signature, "Rect.get()") == 0) return w_Rect_get;

    if (strcmp(signature, "Circle.get()") == 0) return w_Circle_get;

    if (strcmp(signature, "Polyline.set_vertices(_)") == 0) return w_Polyline_set_vertices;

    if (strcmp(signature, "Bezier.set_ctrl_pos(_)") == 0) return w_Bezier_set_ctrl_pos;

    if (strcmp(signature, "ConstraintSolver.add_distance(_,_)") == 0) return w_ConstraintSolver_add_distance;
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

    if (strcmp(class_name, "ConstraintSolver") == 0)
    {
        methods->allocate = w_ConstraintSolver_allocate;
        methods->finalize = w_ConstraintSolver_finalize;
        return;
    }
}

}