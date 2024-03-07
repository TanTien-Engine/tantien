#define QUOTE(...) #__VA_ARGS__
static const char* graphModuleSource = QUOTE(

foreign class Graph
{
	init() {}

	foreign get_nodes()
	foreign get_edges()
}

foreign class Node
{
	init() {}

	foreign get_pos()
	foreign set_pos(x, y)
}

foreign class GraphTools
{
	foreign static load_graph(desc)
	foreign static layout(graph, method)
}

);