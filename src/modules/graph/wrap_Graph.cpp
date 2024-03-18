#include "modules/graph/wrap_Graph.h"
#include "modules/graph/Graph.h"
#include "modules/script/TransHelper.h"

#include <graph/Graph.h>
#include <graph/Node.h>
#include <graph/GraphTools.h>
#include <graph/GraphLayout.h>

#include <set>

namespace
{

void w_Graph_allocate()
{
    auto proxy = (tt::Proxy<graph::Graph>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<graph::Graph>));
    proxy->obj = std::make_shared<graph::Graph>();
}

int w_Graph_finalize(void* data)
{
    auto proxy = (tt::Proxy<graph::Graph>*)(data);
    proxy->~Proxy();
    return sizeof(tt::Proxy<graph::Graph>);
}

void w_Graph_get_nodes()
{
    auto graph = ((tt::Proxy<graph::Graph>*)ves_toforeign(0))->obj;

    auto& nodes = graph->GetNodes();

    ves_pop(ves_argnum());

    const int num = (int)(nodes.size());
    ves_newlist(num);
    for (int i = 0; i < num; ++i)
    {
        ves_pushnil();
        ves_import_class("graph", "Node");
        auto proxy = (tt::Proxy<graph::Node>*)ves_set_newforeign(1, 2, sizeof(tt::Proxy<graph::Node>));
        proxy->obj = std::static_pointer_cast<graph::Node>(nodes[i]);
        ves_pop(1);
        ves_seti(-2, i);
        ves_pop(1);
    }
}

void w_Graph_get_edges()
{
    auto graph = ((tt::Proxy<graph::Graph>*)ves_toforeign(0))->obj;

    auto& nodes = graph->GetNodes();

    std::map<std::shared_ptr<graph::Node>, size_t> node2idx;
    for (size_t i = 0, n = nodes.size(); i < n; ++i) {
        node2idx.insert({ nodes[i], i });
    }

    std::set<std::pair<size_t, size_t>> edges;
    for (auto& node : nodes)
    {
        size_t i0 = node2idx.find(node)->second;
        for (auto& conn : node->GetConnects())
        {
            size_t i1 = node2idx.find(conn)->second;
            if (i0 < i1)
                edges.insert({ i0, i1 });
            else
                edges.insert({ i1, i0 });
        }
    }

    std::vector<sm::ivec2> list;
    for (auto& edge : edges)
    {
        int n0 = static_cast<int>(edge.first);
        int n1 = static_cast<int>(edge.second);
        list.push_back({ n0, n1 });
    }
    tt::return_list(list);
}

void w_Graph_is_directed()
{
    auto graph = ((tt::Proxy<graph::Graph>*)ves_toforeign(0))->obj;
    ves_set_boolean(0, graph->IsDirected());
}

void w_Node_allocate()
{
    auto proxy = (tt::Proxy<graph::Node>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<graph::Node>));
    proxy->obj = std::make_shared<graph::Node>();
}

int w_Node_finalize(void* data)
{
    auto proxy = (tt::Proxy<graph::Node>*)(data);
    proxy->~Proxy();
    return sizeof(tt::Proxy<graph::Node>);
}

void w_Node_get_id()
{
    auto node = ((tt::Proxy<graph::Node>*)ves_toforeign(0))->obj;
    ves_set_number(0, node->GetId());
}

void w_Node_get_pos()
{
    auto node = ((tt::Proxy<graph::Node>*)ves_toforeign(0))->obj;
    tt::return_vec(node->GetPos());
}

void w_Node_set_pos()
{
    auto node = ((tt::Proxy<graph::Node>*)ves_toforeign(0))->obj;

    float x = (float)ves_tonumber(1);
    float y = (float)ves_tonumber(2);

    node->SetPos({ x, y });
}

void w_Node_get_component()
{
    auto node = ((tt::Proxy<graph::Node>*)ves_toforeign(0))->obj;

    std::string key = ves_tostring(1);

    if (key == "topo_shape")
    {
        auto func = tt::Graph::Instance()->GetRegNodeGetCompCB("topo_shape");
        if (func) {
            func(*node);
        }
    }
}

void w_GraphTools_load_graph()
{
    const char* desc = ves_tostring(1);

    auto graph = std::make_shared<graph::Graph>();

    graph->AddNode(std::make_shared<graph::Node>(0));
    graph->AddNode(std::make_shared<graph::Node>(1));
    graph->AddNode(std::make_shared<graph::Node>(2));
    graph->AddNode(std::make_shared<graph::Node>(3));
    graph->AddNode(std::make_shared<graph::Node>(4));

    graph->AddEdge(0, 1);
    graph->AddEdge(1, 2);
    graph->AddEdge(1, 3);
    graph->AddEdge(2, 4);
    graph->AddEdge(3, 4);

    ves_pop(ves_argnum());

    ves_pushnil();
    ves_import_class("graph", "Graph");
    auto proxy = (tt::Proxy<graph::Graph>*)ves_set_newforeign(0, 1, sizeof(tt::Proxy<graph::Graph>));
    proxy->obj = graph;
    ves_pop(1);
}

void w_GraphTools_layout()
{
    auto graph = ((tt::Proxy<graph::Graph>*)ves_toforeign(1))->obj;
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

    if (strcmp(signature, "Node.get_id()") == 0) return w_Node_get_id;
    if (strcmp(signature, "Node.get_pos()") == 0) return w_Node_get_pos;
    if (strcmp(signature, "Node.set_pos(_,_)") == 0) return w_Node_set_pos;
    if (strcmp(signature, "Node.get_component(_)") == 0) return w_Node_get_component;

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
}

}