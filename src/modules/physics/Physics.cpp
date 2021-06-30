#include "modules/physics/Physics.h"
#include "modules/graphics/Graphics.h"
#include "modules/graphics/SpriteRenderer.h"

#include <uniphysics/rigid/Factory.h>
#include <uniphysics/rigid/DebugDraw.h>

namespace tt
{

TT_SINGLETON_DEFINITION(Physics)

Physics::Physics()
{
	m_debug_draw = up::rigid::CreateDebugDraw(up::rigid::Type::Box2D);

	auto& pt = m_debug_draw->GetPainter();
	pt.SetPalette(tt::Graphics::Instance()->GetSpriteRenderer()->GetPalette());
}

Physics::~Physics()
{
}

}