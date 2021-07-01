#include "modules/physics/wrap_Physics.h"
#include "modules/physics/Physics.h"
#include "modules/script/Proxy.h"
#include "modules/graphics/Graphics.h"

#include <uniphysics/rigid/DebugDraw.h>
#include <uniphysics/rigid/box2d/World.h>
#include <uniphysics/rigid/box2d/Body.h>
#include <uniphysics/rigid/box2d/Shape.h>
#include <geoshape/Line2D.h>
#include <geoshape/Rect.h>
#include <geoshape/Circle.h>
#include <geoshape/Polyline2D.h>

#include <string>

namespace
{

void w_World_allocate()
{
    auto world = std::make_shared<up::rigid::box2d::World>();

    auto draw = tt::Physics::Instance()->GetDebugDraw();
    world->SetDebugDraw(*draw);

    auto proxy = (tt::Proxy<up::rigid::box2d::World>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<up::rigid::box2d::World>));
    proxy->obj = world;
}

int w_World_finalize(void* data)
{
    auto proxy = (tt::Proxy<up::rigid::box2d::World>*)(data);
    proxy->~Proxy();
    return sizeof(tt::Proxy<up::rigid::box2d::World>);
}

void w_World_update()
{
    auto world = ((tt::Proxy<up::rigid::box2d::World>*)ves_toforeign(0))->obj;
    world->StepSimulation(1.0f / 60);
}

void w_World_debug_draw()
{
    auto world = ((tt::Proxy<up::rigid::box2d::World>*)ves_toforeign(0))->obj;
    world->DebugDraw();

    auto draw = tt::Physics::Instance()->GetDebugDraw();
    auto& pt = draw->GetPainter();
    tt::Graphics::Instance()->DrawPainter(pt);
    pt.Clear();
}

void w_World_add_body()
{
    auto world = ((tt::Proxy<up::rigid::box2d::World>*)ves_toforeign(0))->obj;
    auto body = ((tt::Proxy<up::rigid::box2d::Body>*)ves_toforeign(1))->obj;
    world->AddBody(body);
}

void w_Body_add_shape()
{
    auto body = ((tt::Proxy<up::rigid::box2d::Body>*)ves_toforeign(0))->obj;
    auto shape = ((tt::Proxy<gs::Shape2D>*)ves_toforeign(1))->obj;

    bool inited = false;
    auto phy_shape = std::make_shared<up::rigid::box2d::Shape>();

    if (!inited)
    {
        auto line = std::dynamic_pointer_cast<gs::Line2D>(shape);
        if (line) {
            inited = true;
            phy_shape->InitEdgeShape(line->GetStart(), line->GetEnd());
        }
    }

    if (!inited)
    {
        auto rect = std::dynamic_pointer_cast<gs::Rect>(shape);
        if (rect) {
            inited = true;
            phy_shape->InitRectShape(rect->GetRect());
        }
    }

    if (!inited)
    {
        auto circle = std::dynamic_pointer_cast<gs::Circle>(shape);
        if (circle) {
            inited = true;
            phy_shape->InitCircleShape(circle->GetCenter(), circle->GetRadius());
        }
    }

    if (!inited)
    {
        auto polyline = std::dynamic_pointer_cast<gs::Polyline2D>(shape);
        if (polyline) 
        {
            inited = true;

            auto& vertices = polyline->GetVertices();
            if (vertices.size() == 2) {
                phy_shape->InitEdgeShape(vertices[0], vertices[1]);
            } else if (vertices.size() > 2) {
                phy_shape->InitChainShape(vertices, polyline->GetClosed());
            }
        }
    }

    if (inited) {
        body->AddShape(phy_shape);
    }
}

void w_Body_get_pos()
{
    auto body = ((tt::Proxy<up::rigid::box2d::Body>*)ves_toforeign(0))->obj;
    auto pos = body->GetPosition();

    ves_pop(1);
    ves_newlist(2);

    ves_pushnumber(pos.x);
    ves_seti(-2, 0);
    ves_pop(1);

    ves_pushnumber(pos.y);
    ves_seti(-2, 1);
    ves_pop(1);
}

void w_Body_allocate()
{
    auto world = std::make_shared<up::rigid::box2d::Body>();

    auto proxy = (tt::Proxy<up::rigid::box2d::Body>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<up::rigid::box2d::Body>));
    proxy->obj = world;
}

int w_Body_finalize(void* data)
{
    auto proxy = (tt::Proxy<up::rigid::box2d::Body>*)(data);
    proxy->~Proxy();
    return sizeof(tt::Proxy<up::rigid::box2d::Body>);
}

}

namespace tt
{

VesselForeignMethodFn PhysicsBindMethod(const char* signature)
{
    if (strcmp(signature, "World.update()") == 0) return w_World_update;
    if (strcmp(signature, "World.debug_draw()") == 0) return w_World_debug_draw;
    if (strcmp(signature, "World.add_body(_)") == 0) return w_World_add_body;

    if (strcmp(signature, "Body.add_shape(_)") == 0) return w_Body_add_shape;
    if (strcmp(signature, "Body.get_pos()") == 0) return w_Body_get_pos;

	return nullptr;
}

void PhysicsBindClass(const char* class_name, VesselForeignClassMethods* methods)
{
    if (strcmp(class_name, "World") == 0)
    {
        methods->allocate = w_World_allocate;
        methods->finalize = w_World_finalize;
        return;
    }

    if (strcmp(class_name, "Body") == 0)
    {
        methods->allocate = w_Body_allocate;
        methods->finalize = w_Body_finalize;
        return;
    }
}

}