#pragma once

#include "core/macro.h"

#include <map>
#include <memory>

namespace pm3 { class Polytope; }

namespace tt
{

class BRepKey;

class DB
{
public:
	auto& GetPoly2KeyMap() { return m_poly2key; }

private:
	std::map<std::shared_ptr<pm3::Polytope>, std::shared_ptr<tt::BRepKey>> m_poly2key;

	TT_SINGLETON_DECLARATION(DB)

}; // DB

}