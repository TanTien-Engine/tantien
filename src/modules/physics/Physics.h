#pragma once

#include "core/macro.h"

#include <memory>

namespace up::rigid { class DebugDraw; }

namespace tt
{

class Physics
{
public:
	auto GetDebugDraw() { return m_debug_draw; }

private:
	std::shared_ptr<up::rigid::DebugDraw> m_debug_draw = nullptr;

	TT_SINGLETON_DECLARATION(Physics)

}; // Physics

}