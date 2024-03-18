#pragma once

#include "core/macro.h"

#include <string>
#include <map>
#include <functional>

namespace graph { class Node; }

namespace tt
{

class Graph
{
public:
	void RegNodeGetCompCB(const std::string& name, std::function<void(const graph::Node&)> func);
	std::function<void(const graph::Node&)> GetRegNodeGetCompCB(const std::string& name);

private:
	std::map<std::string, std::function<void(const graph::Node&)>> m_node_get_comp;

	TT_SINGLETON_DECLARATION(Graph)
};

}