#define QUOTE(...) #__VA_ARGS__
static const char* graphModuleSource = QUOTE(

foreign class Graph
{
	init() {}

	foreign get_nodes()
	foreign get_edges()

	foreign is_directed()
}

foreign class Node
{
	init() {}

	foreign is_valid()

	foreign get_title()
	foreign has_name()

	foreign get_pos()
	foreign set_pos(x, y)

	foreign get_component(key)
}

foreign class GraphTools
{
	foreign static load_graph(desc)
	foreign static layout(graph, method)
}

);