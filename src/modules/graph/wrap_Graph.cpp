#include "modules/graph/wrap_Graph.h"
#include "modules/graph/Graph.h"

#include <graph/Graph.h>
#include <graph/Node.h>
#include <graph/Edge.h>
#include <graph/GraphTools.h>
#include <graph/GraphLayout.h>
#include <graph/NodePos.h>
#include <graph/NodeColor.h>
#include <graph/EdgeStyle.h>
#include <wrapper/TransHelper.h>

#include <set>

namespace
{

void w_Graph_allocate()
{
    auto proxy = (wrapper::Proxy<graph::Graph>*)ves_set_newforeign(0, 0, sizeof(wrapper::Proxy<graph::Graph>));
    proxy->obj = std::make_shared<graph::Graph>();
}

int w_Graph_finalize(void* data)
{
    auto proxy = (wrapper::Proxy<graph::Graph>*)(data);
    proxy->~Proxy();
    return sizeof(wrapper::Proxy<graph::Graph>);
}

void w_Graph_get_nodes()
{
    auto graph = ((wrapper::Proxy<graph::Graph>*)ves_toforeign(0))->obj;

    ves_pop(ves_argnum());

    const int num = (int)(graph->GetNodesNum());
    ves_newlist(num);
    for (int i = 0; i < num; ++i)
    {
        ves_pushnil();
        ves_import_class("graph", "Node");
        auto proxy = (wrapper::Proxy<graph::Node>*)ves_set_newforeign(1, 2, sizeof(wrapper::Proxy<graph::Node>));
        proxy->obj = graph->GetNode(i);
        ves_pop(1);
        ves_seti(-2, i);
        ves_pop(1);
    }
}

void w_Graph_get_edges()
{
    auto graph = ((wrapper::Proxy<graph::Graph>*)ves_toforeign(0))->obj;

    ves_pop(ves_argnum());

    auto& edges = graph->GetEdges();
    ves_newlist(edges.size());
    int i = 0;
    for (auto& pair : edges)
    {
        ves_pushnil();
        ves_import_class("graph", "Edge");
        auto proxy = (wrapper::Proxy<graph::Edge>*)ves_set_newforeign(1, 2, sizeof(wrapper::Proxy<graph::Edge>));
        proxy->obj = pair.second;
        ves_pop(1);
        ves_seti(-2, i);
        ves_pop(1);
        ++i;
    }
}

void w_Graph_is_directed()
{
    auto graph = ((wrapper::Proxy<graph::Graph>*)ves_toforeign(0))->obj;
    ves_set_boolean(0, graph->IsDirected());
}

void w_Graph_clear_edges()
{
    auto graph = ((wrapper::Proxy<graph::Graph>*)ves_toforeign(0))->obj;
    int node_idx = (int)ves_tonumber(1);
    graph->ClearEdges(node_idx);
}

void w_Node_allocate()
{
    auto proxy = (wrapper::Proxy<graph::Node>*)ves_set_newforeign(0, 0, sizeof(wrapper::Proxy<graph::Node>));
    proxy->obj = std::make_shared<graph::Node>();
}

int w_Node_finalize(void* data)
{
    auto proxy = (wrapper::Proxy<graph::Node>*)(data);
    proxy->~Proxy();
    return sizeof(wrapper::Proxy<graph::Node>);
}

void w_Node_is_valid()
{
    auto node = ((wrapper::Proxy<graph::Node>*)ves_toforeign(0))->obj;
    ves_set_boolean(0, node != nullptr);
}

void w_Node_get_title()
{
    auto node = ((wrapper::Proxy<graph::Node>*)ves_toforeign(0))->obj;

    auto& name = node->GetName();
    if (name.empty())
    {
        auto val = node->GetValue();
        auto str = std::to_string(val);
        ves_set_lstring(0, str.c_str(), str.size());
    }
    else
    {
        ves_set_lstring(0, name.c_str(), name.size());
    }
}

void w_Node_has_name()
{
    auto node = ((wrapper::Proxy<graph::Node>*)ves_toforeign(0))->obj;
    ves_set_boolean(0, !node->GetName().empty());
}

void w_Node_get_pos()
{
    auto node = ((wrapper::Proxy<graph::Node>*)ves_toforeign(0))->obj;
    wrapper::return_vec(node->GetComponent<graph::NodePos>().GetPos());
}

void w_Node_set_pos()
{
    auto node = ((wrapper::Proxy<graph::Node>*)ves_toforeign(0))->obj;

    float x = (float)ves_tonumber(1);
    float y = (float)ves_tonumber(2);

    node->GetComponent<graph::NodePos>().SetPos({ x, y });
}

void w_Node_get_color()
{
    auto node = ((wrapper::Proxy<graph::Node>*)ves_toforeign(0))->obj;
    if (node->HasComponent<graph::NodeColor>())
        wrapper::return_vec(node->GetComponent<graph::NodeColor>().GetColor());
    else
        ves_set_nil(0);
}

void w_Node_get_component()
{
    auto node = ((wrapper::Proxy<graph::Node>*)ves_toforeign(0))->obj;

    std::string key = ves_tostring(1);

    auto func = tt::Graph::Instance()->GetRegNodeGetCompCB(key);
    if (func) {
        func(*node);
    }
}

void w_Edge_allocate()
{
    auto proxy = (wrapper::Proxy<graph::Edge>*)ves_set_newforeign(0, 0, sizeof(wrapper::Proxy<graph::Edge>));
    proxy->obj = std::make_shared<graph::Edge>();
}

int w_Edge_finalize(void* data)
{
    auto proxy = (wrapper::Proxy<graph::Edge>*)(data);
    proxy->~Proxy();
    return sizeof(wrapper::Proxy<graph::Edge>);
}

void w_Edge_get_fpos()
{
    auto edge = ((wrapper::Proxy<graph::Edge>*)ves_toforeign(0))->obj;
    auto node = edge->GetFromNode();
    wrapper::return_vec(node->GetComponent<graph::NodePos>().GetPos());
}

void w_Edge_get_tpos()
{
    auto edge = ((wrapper::Proxy<graph::Edge>*)ves_toforeign(0))->obj;
    auto node = edge->GetToNode();
    wrapper::return_vec(node->GetComponent<graph::NodePos>().GetPos());
}

void w_Edge_get_color()
{
    auto edge = ((wrapper::Proxy<graph::Edge>*)ves_toforeign(0))->obj;
    if (edge->HasComponent<graph::EdgeStyle>())
        wrapper::return_vec(edge->GetComponent<graph::EdgeStyle>().GetColor());
    else
        ves_set_nil(0);
}

void w_GraphTools_load_graph()
{
    const char* desc = ves_tostring(1);

    auto graph = std::make_shared<graph::Graph>();

    for (int i = 0; i < 5; ++i)
    {
        auto node = std::make_shared<graph::Node>();
        node->SetValue(i);
        graph->AddNode(node);
    }

    graph->AddEdge(0, 1);
    graph->AddEdge(1, 2);
    graph->AddEdge(1, 3);
    graph->AddEdge(2, 4);
    graph->AddEdge(3, 4);

    ves_pop(ves_argnum());

    ves_pushnil();
    ves_import_class("graph", "Graph");
    auto proxy = (wrapper::Proxy<graph::Graph>*)ves_set_newforeign(0, 1, sizeof(wrapper::Proxy<graph::Graph>));
    proxy->obj = graph;
    ves_pop(1);
}

void w_GraphTools_layout()
{
    auto graph = ((wrapper::Proxy<graph::Graph>*)ves_toforeign(1))->obj;
    std::string method = ves_tostring(2);

    if (method == "stress_mini") {
        graph::GraphLayout::StressMinimization(*graph);
    } else if (method == "hierarchy") {
        graph::GraphLayout::OptimalHierarchy(*graph);
    } else if (method == "hierarchy_rank") {
        graph::GraphLayout::HierarchyRanking(*graph);
    }
}

}

namespace tt
{

VesselForeignMethodFn GraphBindMethod(const char* signature)
{
    if (strcmp(signature, "Graph.get_nodes()") == 0) return w_Graph_get_nodes;
    if (strcmp(signature, "Graph.get_edges()") == 0) return w_Graph_get_edges;
    if (strcmp(signature, "Graph.is_directed()") == 0) return w_Graph_is_directed;
    if (strcmp(signature, "Graph.clear_edges(_)") == 0) return w_Graph_clear_edges;

    if (strcmp(signature, "Node.is_valid()") == 0) return w_Node_is_valid;
    if (strcmp(signature, "Node.get_title()") == 0) return w_Node_get_title;
    if (strcmp(signature, "Node.has_name()") == 0) return w_Node_has_name;
    if (strcmp(signature, "Node.get_pos()") == 0) return w_Node_get_pos;
    if (strcmp(signature, "Node.set_pos(_,_)") == 0) return w_Node_set_pos;
    if (strcmp(signature, "Node.get_color()") == 0) return w_Node_get_color;
    if (strcmp(signature, "Node.get_component(_)") == 0) return w_Node_get_component;

    if (strcmp(signature, "Edge.get_fpos()") == 0) return w_Edge_get_fpos;
    if (strcmp(signature, "Edge.get_tpos()") == 0) return w_Edge_get_tpos;
    if (strcmp(signature, "Edge.get_color()") == 0) return w_Edge_get_color;

    if (strcmp(signature, "static GraphTools.load_graph(_)") == 0) return w_GraphTools_load_graph;
    if (strcmp(signature, "static GraphTools.layout(_,_)") == 0) return w_GraphTools_layout;

    return nullptr;
}

void GraphBindClass(const char* class_name, VesselForeignClassMethods* methods)
{
    if (strcmp(class_name, "Graph") == 0)
    {
        methods->allocate = w_Graph_allocate;
        methods->finalize = w_Graph_finalize;
        return;
    }

    if (strcmp(class_name, "Node") == 0)
    {
        methods->allocate = w_Node_allocate;
        methods->finalize = w_Node_finalize;
        return;
    }

    if (strcmp(class_name, "Edge") == 0)
    {
        methods->allocate = w_Edge_allocate;
        methods->finalize = w_Edge_finalize;
        return;
    }
}

}