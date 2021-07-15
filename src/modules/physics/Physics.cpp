#include "modules/physics/Physics.h"
#include "modules/graphics/Graphics.h"
#include "modules/graphics/SpriteRenderer.h"
#include "modules/script/Proxy.h"

#include <uniphysics/rigid/Factory.h>
#include <uniphysics/rigid/DebugDraw.h>
#include <uniphysics/rigid/box2d/World.h>
#include <uniphysics/rigid/box2d/Body.h>

#include <vessel.h>

namespace
{

auto begin_contact = [](const std::shared_ptr<up::rigid::box2d::Body>& a, const std::shared_ptr<up::rigid::box2d::Body>& b)
{
	auto offset = ves_argnum();

	ves_pushnil();
	ves_pushnil();
	ves_import_class("physics", "Body");

	auto proxy_a = (tt::Proxy<up::rigid::box2d::Body>*)ves_set_newforeign(offset + 0, offset + 2, sizeof(tt::Proxy<up::rigid::box2d::Body>));
	proxy_a->obj = a;
	auto proxy_b = (tt::Proxy<up::rigid::box2d::Body>*)ves_set_newforeign(offset + 1, offset + 2, sizeof(tt::Proxy<up::rigid::box2d::Body>));
	proxy_b->obj = b;

	ves_pop(1);

	ves_pushstring("begin_contact(_,_)");
	ves_call(2, 0);
};

}

namespace tt
{

TT_SINGLETON_DEFINITION(Physics)

Physics::Physics()
{
	m_debug_draw = up::rigid::CreateDebugDraw(up::rigid::Type::Box2D);

	auto& pt = m_debug_draw->GetPainter();
	pt.SetPalette(tt::Graphics::Instance()->GetSpriteRenderer()->GetPalette());

	up::rigid::box2d::World::Callback cb;
	cb.begin_contact = begin_contact;
	up::rigid::box2d::World::RegisterCallback(cb);
}

Physics::~Physics()
{
}

}