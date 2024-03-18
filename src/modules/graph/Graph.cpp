#include "modules/graph/Graph.h"

namespace tt
{

TT_SINGLETON_DEFINITION(Graph)

Graph::Graph()
{
}

Graph::~Graph()
{
}

void Graph::RegNodeGetCompCB(const std::string& name, std::function<void(const graph::Node&)> func)
{
	m_node_get_comp.insert({ name, func });
}

std::function<void(const graph::Node&)> Graph::GetRegNodeGetCompCB(const std::string& name)
{
	auto itr = m_node_get_comp.find(name);
	return itr == m_node_get_comp.end() ? nullptr : itr->second;
}

}