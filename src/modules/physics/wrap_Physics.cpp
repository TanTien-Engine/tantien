#include "modules/physics/wrap_Physics.h"
#include "modules/physics/Physics.h"
#include "modules/script/Proxy.h"
#include "modules/graphics/Graphics.h"
#include "modules/script/TransHelper.h"

#include <uniphysics/rigid/DebugDraw.h>
#include <uniphysics/rigid/box2d/World.h>
#include <uniphysics/rigid/box2d/Body.h>
#include <uniphysics/rigid/box2d/Shape.h>
#include <uniphysics/rigid/box2d/Joint.h>
#include <uniphysics/rigid/box2d/config.h>
#include <geoshape/Line2D.h>
#include <geoshape/Rect.h>
#include <geoshape/Circle.h>
#include <geoshape/Polyline2D.h>
#include <geoshape/Polygon2D.h>
#include <sm/SM_Calc.h>

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

void w_World_remove_body()
{
    auto world = ((tt::Proxy<up::rigid::box2d::World>*)ves_toforeign(0))->obj;
    auto body = ((tt::Proxy<up::rigid::box2d::Body>*)ves_toforeign(1))->obj;
    world->RemoveBody(body);
}

void w_World_add_joint()
{
    auto world = ((tt::Proxy<up::rigid::box2d::World>*)ves_toforeign(0))->obj;
    auto joint = ((tt::Proxy<up::rigid::box2d::Joint>*)ves_toforeign(1))->obj;
    world->AddJoint(joint);
}

void w_World_remove_joint()
{
    auto world = ((tt::Proxy<up::rigid::box2d::World>*)ves_toforeign(0))->obj;
    auto joint = ((tt::Proxy<up::rigid::box2d::Joint>*)ves_toforeign(1))->obj;
    world->RemoveJoint(joint);
}

void w_World_query_by_pos()
{
    auto world = ((tt::Proxy<up::rigid::box2d::World>*)ves_toforeign(0))->obj;
    auto pos = tt::list_to_vec2(1);
    auto body = world->QueryByPos(pos);
    if (body) 
    {
        ves_pop(ves_argnum());

        ves_pushnil();
        ves_import_class("physics", "Body");
        auto proxy = (tt::Proxy<up::rigid::box2d::Body>*)ves_set_newforeign(0, 1, sizeof(tt::Proxy<up::rigid::box2d::Body>));
        proxy->obj = body;
        ves_pop(1);
    } 
    else 
    {
        ves_set_nil(0);
    }
}

void w_Body_allocate()
{
    const char* type = ves_tostring(1);
    int flag = (int)ves_optnumber(2, -1);
    auto body = std::make_shared<up::rigid::box2d::Body>(type, flag);

    auto proxy = (tt::Proxy<up::rigid::box2d::Body>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<up::rigid::box2d::Body>));
    proxy->obj = body;
}

int w_Body_finalize(void* data)
{
    auto proxy = (tt::Proxy<up::rigid::box2d::Body>*)(data);
    proxy->~Proxy();
    return sizeof(tt::Proxy<up::rigid::box2d::Body>);
}

void w_Body_add_fixture()
{
    auto body = ((tt::Proxy<up::rigid::box2d::Body>*)ves_toforeign(0))->obj;
    auto shape = ((tt::Proxy<gs::Shape2D>*)ves_toforeign(1))->obj;
    auto filled = ves_optboolean(2, false);
    auto mt = (sm::Matrix2D*)ves_toforeign(3);
    auto filter = tt::list_to_array<int>(4);

    auto phy_shape = std::make_shared<up::rigid::box2d::Shape>();

    switch (shape->GetType())
    {
    case gs::ShapeType2D::Line:
    {
        auto line = std::static_pointer_cast<gs::Line2D>(shape);
        if (mt) {
            phy_shape->InitEdgeShape(*mt * line->GetStart(), *mt * line->GetEnd());
        } else {
            phy_shape->InitEdgeShape(line->GetStart(), line->GetEnd());
        }
    }
        break;
    case gs::ShapeType2D::Rect:
    {
        auto rect = std::static_pointer_cast<gs::Rect>(shape);

        std::vector<sm::vec2> vertices;
        auto& r = rect->GetRect();
        vertices.push_back({ r.xmin, r.ymin });
        vertices.push_back({ r.xmin, r.ymax });
        vertices.push_back({ r.xmax, r.ymax });
        vertices.push_back({ r.xmax, r.ymin });
        if (mt) {
            for (auto& v : vertices) {
                v = *mt * v;
            }
        }

        if (filled) {
            phy_shape->InitPolygonShape(vertices);
        } else {
            phy_shape->InitChainShape(vertices, true);
        }
    }
        break;
    case gs::ShapeType2D::Circle:
    {
        auto circle = std::static_pointer_cast<gs::Circle>(shape);
        if (mt) {
            auto p0 = *mt * circle->GetCenter();
            auto p1 = *mt * sm::vec2(circle->GetCenter().x + circle->GetRadius(), circle->GetCenter().y);
            phy_shape->InitCircleShape(p0, sm::dis_pos_to_pos(p0, p1));
        } else {
            phy_shape->InitCircleShape(circle->GetCenter(), circle->GetRadius());
        }
    }
        break;
    case gs::ShapeType2D::Polyline: 
    {
        auto polyline = std::dynamic_pointer_cast<gs::Polyline2D>(shape);

        auto vertices = polyline->GetVertices();
        if (mt) {
            for (auto& v : vertices) {
                v = *mt * v;
            }
        }

        if (vertices.size() == 2) {
            phy_shape->InitEdgeShape(vertices[0], vertices[1]);
        } else if (vertices.size() > 2) {
            phy_shape->InitChainShape(vertices, polyline->GetClosed());
        }
    }
        break;
    case gs::ShapeType2D::Polygon:
    {
        auto polygon = std::dynamic_pointer_cast<gs::Polygon2D>(shape);

        auto vertices = polygon->GetVertices();
        if (mt) {
            for (auto& v : vertices) {
                v = *mt * v;
            }
        }

        phy_shape->InitPolygonShape(vertices);
    }
        break;
    default:
        assert(0);
    }

    int category = 0;
    std::vector<int> not_collide;
    if (!filter.empty())
    {
        category = filter[0];
        for (size_t i = 1, n = filter.size(); i < n; ++i) {
            not_collide.push_back(filter[i]);
        }
    }
    body->AddFixture(phy_shape, category, not_collide);
}

void w_Body_set_gravity_scale()
{
    auto body = ((tt::Proxy<up::rigid::box2d::Body>*)ves_toforeign(0))->obj;
    float gravity = (float)ves_tonumber(1);
    body->SetGravityScale(gravity);
}

void w_Body_set_density()
{
    auto body = ((tt::Proxy<up::rigid::box2d::Body>*)ves_toforeign(0))->obj;
    float density = (float)ves_tonumber(1);
    body->SetDensity(density);
}

void w_Body_set_restitution()
{
    auto body = ((tt::Proxy<up::rigid::box2d::Body>*)ves_toforeign(0))->obj;
    float restitution = (float)ves_tonumber(1);
    body->SetRestitution(restitution);
}

void w_Body_set_friction()
{
    auto body = ((tt::Proxy<up::rigid::box2d::Body>*)ves_toforeign(0))->obj;
    float friction = (float)ves_tonumber(1);
    body->SetFriction(friction);
}

void w_Body_apply_force()
{
    auto body = ((tt::Proxy<up::rigid::box2d::Body>*)ves_toforeign(0))->obj;
    auto force = tt::list_to_vec2(1);
    body->ApplyForce(force);
}

void w_Body_apply_torque()
{
    auto body = ((tt::Proxy<up::rigid::box2d::Body>*)ves_toforeign(0))->obj;
    float torque = (float)ves_tonumber(1);
    body->ApplyTorque(torque);
}

void w_Body_apply_linear_impulse()
{
    auto body = ((tt::Proxy<up::rigid::box2d::Body>*)ves_toforeign(0))->obj;
    auto impulse = tt::list_to_vec2(1);
    body->ApplyLinearImpulse(impulse);
}

void w_Body_apply_angular_impulse()
{
    auto body = ((tt::Proxy<up::rigid::box2d::Body>*)ves_toforeign(0))->obj;
    float impulse = (float)ves_tonumber(1);
    body->ApplyAngularImpulse(impulse);
}

void w_Body_get_pos()
{
    auto body = ((tt::Proxy<up::rigid::box2d::Body>*)ves_toforeign(0))->obj;
    auto pos = body->GetPosition();

    tt::return_list(std::vector<float>{
        pos.x * up::rigid::box2d::SCALE_FACTOR,
        pos.y * up::rigid::box2d::SCALE_FACTOR
    });
}

void w_Body_get_angle()
{
    auto body = ((tt::Proxy<up::rigid::box2d::Body>*)ves_toforeign(0))->obj;
    ves_set_number(0, body->GetAngle());
}

void w_Body_set_transform()
{
    auto body  = ((tt::Proxy<up::rigid::box2d::Body>*)ves_toforeign(0))->obj;
    auto pos   = tt::list_to_vec2(1) / up::rigid::box2d::SCALE_FACTOR;
    auto angle = (float)ves_tonumber(2);

    body->SetTransform(pos, angle);
}

void w_Body_get_type()
{
    auto body = ((tt::Proxy<up::rigid::box2d::Body>*)ves_toforeign(0))->obj;
    auto& type = body->GetType();
    ves_set_lstring(0, type.c_str(), type.size());
}

void w_Body_get_flag()
{
    auto body = ((tt::Proxy<up::rigid::box2d::Body>*)ves_toforeign(0))->obj;
    ves_set_number(0, body->GetFlag());
}

void w_Body_set_linear_velocity()
{
    auto body = ((tt::Proxy<up::rigid::box2d::Body>*)ves_toforeign(0))->obj;
    float x = (float)ves_tonumber(1);
    float y = (float)ves_tonumber(2);
    body->SetLinearVelocity({ x, y });
}

void w_Body_get_linear_velocity()
{
    auto body = ((tt::Proxy<up::rigid::box2d::Body>*)ves_toforeign(0))->obj;
    auto velocity = body->GetLinearVelocity();
    tt::return_list(std::vector<float>{ velocity.x, velocity.y });
}

void w_Body_is_valid()
{
    auto body = ((tt::Proxy<up::rigid::box2d::Body>*)ves_toforeign(0))->obj;
    ves_set_boolean(0, body->GetImpl() != nullptr);
}

void w_Body_get_mass()
{
    auto body = ((tt::Proxy<up::rigid::box2d::Body>*)ves_toforeign(0))->obj;
    ves_set_number(0, body->GetMass());
}

void w_RevoluteJoint_allocate()
{
    auto body_a = ((tt::Proxy<up::rigid::box2d::Body>*)ves_toforeign(1))->obj;
    auto body_b = ((tt::Proxy<up::rigid::box2d::Body>*)ves_toforeign(2))->obj;
    auto anchor = tt::list_to_vec2(3) / up::rigid::box2d::SCALE_FACTOR;

    auto joint = std::make_shared<up::rigid::box2d::RevoluteJoint>(body_a, body_b, anchor);
    auto proxy = (tt::Proxy<up::rigid::box2d::RevoluteJoint>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<up::rigid::box2d::RevoluteJoint>));
    proxy->obj = joint;
}

int w_RevoluteJoint_finalize(void* data)
{
    auto proxy = (tt::Proxy<up::rigid::box2d::RevoluteJoint>*)(data);
    proxy->~Proxy();
    return sizeof(tt::Proxy<up::rigid::box2d::RevoluteJoint>);
}

void w_RevoluteJoint_set_angle_limit()
{
    auto joint = ((tt::Proxy<up::rigid::box2d::RevoluteJoint>*)ves_toforeign(0))->obj;
    auto enable_limit = ves_toboolean(1);
    auto lower = (float)ves_tonumber(2);
    auto upper = (float)ves_tonumber(3);
    joint->SetAngleLimit(enable_limit, lower, upper);
}

void w_RevoluteJoint_set_motor()
{
    auto joint = ((tt::Proxy<up::rigid::box2d::RevoluteJoint>*)ves_toforeign(0))->obj;
    auto enable_motor = ves_toboolean(1);
    auto max_torque = (float)ves_tonumber(2);
    auto speed = (float)ves_tonumber(3);
    joint->SetMotor(enable_motor, max_torque, speed);
}

void w_PrismaticJoint_allocate()
{
    auto body_a = ((tt::Proxy<up::rigid::box2d::Body>*)ves_toforeign(1))->obj;
    auto body_b = ((tt::Proxy<up::rigid::box2d::Body>*)ves_toforeign(2))->obj;
    auto anchor = tt::list_to_vec2(3) / up::rigid::box2d::SCALE_FACTOR;
    auto axis   = tt::list_to_vec2(4);

    auto joint = std::make_shared<up::rigid::box2d::PrismaticJoint>(body_a, body_b, anchor, axis);
    auto proxy = (tt::Proxy<up::rigid::box2d::PrismaticJoint>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<up::rigid::box2d::PrismaticJoint>));
    proxy->obj = joint;
}

int w_PrismaticJoint_finalize(void* data)
{
    auto proxy = (tt::Proxy<up::rigid::box2d::PrismaticJoint>*)(data);
    proxy->~Proxy();
    return sizeof(tt::Proxy<up::rigid::box2d::PrismaticJoint>);
}

void w_PrismaticJoint_set_translate_limit()
{
    auto joint = ((tt::Proxy<up::rigid::box2d::PrismaticJoint>*)ves_toforeign(0))->obj;
    auto enable_limit = ves_toboolean(1);
    auto lower = (float)ves_tonumber(2);
    auto upper = (float)ves_tonumber(3);
    joint->SetTranslateLimit(enable_limit, lower, upper);
}

void w_PrismaticJoint_set_motor()
{
    auto joint = ((tt::Proxy<up::rigid::box2d::PrismaticJoint>*)ves_toforeign(0))->obj;
    auto enable_motor = ves_toboolean(1);
    auto max_torque = (float)ves_tonumber(2);
    auto speed = (float)ves_tonumber(3);
    joint->SetMotor(enable_motor, max_torque, speed);
}

void w_DistanceJoint_allocate()
{
    auto body_a = ((tt::Proxy<up::rigid::box2d::Body>*)ves_toforeign(1))->obj;
    auto body_b = ((tt::Proxy<up::rigid::box2d::Body>*)ves_toforeign(2))->obj;
    auto anchor_a = tt::list_to_vec2(3) / up::rigid::box2d::SCALE_FACTOR;
    auto anchor_b = tt::list_to_vec2(4) / up::rigid::box2d::SCALE_FACTOR;

    auto joint = std::make_shared<up::rigid::box2d::DistanceJoint>(body_a, body_b, anchor_a, anchor_b);
    auto proxy = (tt::Proxy<up::rigid::box2d::DistanceJoint>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<up::rigid::box2d::DistanceJoint>));
    proxy->obj = joint;
}

int w_DistanceJoint_finalize(void* data)
{
    auto proxy = (tt::Proxy<up::rigid::box2d::DistanceJoint>*)(data);
    proxy->~Proxy();
    return sizeof(tt::Proxy<up::rigid::box2d::DistanceJoint>);
}

void w_DistanceJoint_set_length()
{
    auto joint = ((tt::Proxy<up::rigid::box2d::DistanceJoint>*)ves_toforeign(0))->obj;
    auto min = (float)ves_tonumber(1) / up::rigid::box2d::SCALE_FACTOR;
    auto max = (float)ves_tonumber(2) / up::rigid::box2d::SCALE_FACTOR;
    joint->SetLength(min, max);
}

void w_MouseJoint_allocate()
{
    auto body_a = ((tt::Proxy<up::rigid::box2d::Body>*)ves_toforeign(1))->obj;
    auto body_b = ((tt::Proxy<up::rigid::box2d::Body>*)ves_toforeign(2))->obj;
    auto target = tt::list_to_vec2(3);
    auto max_force = (float)ves_tonumber(4);

    auto joint = std::make_shared<up::rigid::box2d::MouseJoint>(body_a, body_b, target, max_force);
    auto proxy = (tt::Proxy<up::rigid::box2d::MouseJoint>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<up::rigid::box2d::MouseJoint>));
    proxy->obj = joint;
}

int w_MouseJoint_finalize(void* data)
{
    auto proxy = (tt::Proxy<up::rigid::box2d::MouseJoint>*)(data);
    proxy->~Proxy();
    return sizeof(tt::Proxy<up::rigid::box2d::MouseJoint>);
}

void w_MouseJoint_set_target()
{
    auto joint = ((tt::Proxy<up::rigid::box2d::MouseJoint>*)ves_toforeign(0))->obj;
    auto target = tt::list_to_vec2(1);
    joint->SetTarget(target);
}

void w_WheelJoint_allocate()
{
    auto body_a = ((tt::Proxy<up::rigid::box2d::Body>*)ves_toforeign(1))->obj;
    auto body_b = ((tt::Proxy<up::rigid::box2d::Body>*)ves_toforeign(2))->obj;
    auto anchor = tt::list_to_vec2(3) / up::rigid::box2d::SCALE_FACTOR;
    auto axis   = tt::list_to_vec2(4);

    auto joint = std::make_shared<up::rigid::box2d::WheelJoint>(body_a, body_b, anchor, axis);
    auto proxy = (tt::Proxy<up::rigid::box2d::WheelJoint>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<up::rigid::box2d::WheelJoint>));
    proxy->obj = joint;
}

int w_WheelJoint_finalize(void* data)
{
    auto proxy = (tt::Proxy<up::rigid::box2d::WheelJoint>*)(data);
    proxy->~Proxy();
    return sizeof(tt::Proxy<up::rigid::box2d::WheelJoint>);
}

void w_WheelJoint_set_translate_limit()
{
    auto joint = ((tt::Proxy<up::rigid::box2d::WheelJoint>*)ves_toforeign(0))->obj;
    auto enable_limit = ves_toboolean(1);
    auto lower = (float)ves_tonumber(2);
    auto upper = (float)ves_tonumber(3);
    joint->SetTranslateLimit(enable_limit, lower, upper);
}

void w_WheelJoint_set_motor()
{
    auto joint = ((tt::Proxy<up::rigid::box2d::WheelJoint>*)ves_toforeign(0))->obj;
    auto enable_motor = ves_toboolean(1);
    auto max_torque = (float)ves_tonumber(2);
    auto speed = (float)ves_tonumber(3);
    joint->SetMotor(enable_motor, max_torque, speed);
}

void w_WheelJoint_set_suspension()
{
    auto joint = ((tt::Proxy<up::rigid::box2d::WheelJoint>*)ves_toforeign(0))->obj;
    auto stiffness = (float)ves_tonumber(1);
    auto damping = (float)ves_tonumber(2);
    joint->SetSuspension(stiffness, damping);
}

void w_WheelJoint_set_motor_speed()
{
    auto joint = ((tt::Proxy<up::rigid::box2d::WheelJoint>*)ves_toforeign(0))->obj;
    auto speed = (float)ves_tonumber(1);
    joint->SetMotorSpeed(speed);
}

}

namespace tt
{

VesselForeignMethodFn PhysicsBindMethod(const char* signature)
{
    if (strcmp(signature, "World.update()") == 0) return w_World_update;
    if (strcmp(signature, "World.debug_draw()") == 0) return w_World_debug_draw;
    if (strcmp(signature, "World.add_body(_)") == 0) return w_World_add_body;
    if (strcmp(signature, "World.remove_body(_)") == 0) return w_World_remove_body;
    if (strcmp(signature, "World.add_joint(_)") == 0) return w_World_add_joint;
    if (strcmp(signature, "World.remove_joint(_)") == 0) return w_World_remove_joint;
    if (strcmp(signature, "World.query_by_pos(_)") == 0) return w_World_query_by_pos;

    if (strcmp(signature, "Body.add_fixture(_,_,_,_)") == 0) return w_Body_add_fixture;
    if (strcmp(signature, "Body.set_gravity_scale(_)") == 0) return w_Body_set_gravity_scale;
    if (strcmp(signature, "Body.set_density(_)") == 0) return w_Body_set_density;
    if (strcmp(signature, "Body.set_restitution(_)") == 0) return w_Body_set_restitution;
    if (strcmp(signature, "Body.set_friction(_)") == 0) return w_Body_set_friction;
    if (strcmp(signature, "Body.apply_force(_)") == 0) return w_Body_apply_force;
    if (strcmp(signature, "Body.apply_torque(_)") == 0) return w_Body_apply_torque;
    if (strcmp(signature, "Body.apply_linear_impulse(_)") == 0) return w_Body_apply_linear_impulse;
    if (strcmp(signature, "Body.apply_angular_impulse(_)") == 0) return w_Body_apply_angular_impulse;
    if (strcmp(signature, "Body.set_transform(_,_)") == 0) return w_Body_set_transform;
    if (strcmp(signature, "Body.get_pos()") == 0) return w_Body_get_pos;
    if (strcmp(signature, "Body.get_angle()") == 0) return w_Body_get_angle;
    if (strcmp(signature, "Body.get_type()") == 0) return w_Body_get_type;
    if (strcmp(signature, "Body.get_flag()") == 0) return w_Body_get_flag;
    if (strcmp(signature, "Body.set_linear_velocity(_,_)") == 0) return w_Body_set_linear_velocity;
    if (strcmp(signature, "Body.get_linear_velocity()") == 0) return w_Body_get_linear_velocity;
    if (strcmp(signature, "Body.is_valid()") == 0) return w_Body_is_valid;
    if (strcmp(signature, "Body.get_mass()") == 0) return w_Body_get_mass;

    if (strcmp(signature, "RevoluteJoint.set_angle_limit(_,_,_)") == 0) return w_RevoluteJoint_set_angle_limit;
    if (strcmp(signature, "RevoluteJoint.set_motor(_,_,_)") == 0) return w_RevoluteJoint_set_motor;
    if (strcmp(signature, "PrismaticJoint.set_translate_limit(_,_,_)") == 0) return w_PrismaticJoint_set_translate_limit;
    if (strcmp(signature, "PrismaticJoint.set_motor(_,_,_)") == 0) return w_PrismaticJoint_set_motor;
    if (strcmp(signature, "DistanceJoint.set_length(_,_)") == 0) return w_DistanceJoint_set_length;
    if (strcmp(signature, "MouseJoint.set_target(_)") == 0) return w_MouseJoint_set_target;
    if (strcmp(signature, "WheelJoint.set_translate_limit(_,_,_)") == 0) return w_WheelJoint_set_translate_limit;
    if (strcmp(signature, "WheelJoint.set_motor(_,_,_)") == 0) return w_WheelJoint_set_motor;
    if (strcmp(signature, "WheelJoint.set_suspension(_,_)") == 0) return w_WheelJoint_set_suspension;
    if (strcmp(signature, "WheelJoint.set_motor_speed(_)") == 0) return w_WheelJoint_set_motor_speed;

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

    if (strcmp(class_name, "RevoluteJoint") == 0)
    {
        methods->allocate = w_RevoluteJoint_allocate;
        methods->finalize = w_RevoluteJoint_finalize;
        return;
    }

    if (strcmp(class_name, "PrismaticJoint") == 0)
    {
        methods->allocate = w_PrismaticJoint_allocate;
        methods->finalize = w_PrismaticJoint_finalize;
        return;
    }

    if (strcmp(class_name, "DistanceJoint") == 0)
    {
        methods->allocate = w_DistanceJoint_allocate;
        methods->finalize = w_DistanceJoint_finalize;
        return;
    }

    if (strcmp(class_name, "MouseJoint") == 0)
    {
        methods->allocate = w_MouseJoint_allocate;
        methods->finalize = w_MouseJoint_finalize;
        return;
    }

    if (strcmp(class_name, "WheelJoint") == 0)
    {
        methods->allocate = w_WheelJoint_allocate;
        methods->finalize = w_WheelJoint_finalize;
        return;
    }
}

}