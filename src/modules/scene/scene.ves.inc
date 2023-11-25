#define QUOTE(...) #__VA_ARGS__
static const char* sceneModuleSource = QUOTE(

foreign class SceneTree
{
	init() {}

	foreign build(rtree)

	foreign get_all_vao()
}

);