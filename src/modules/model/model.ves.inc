#define QUOTE(...) #__VA_ARGS__
static const char* modelModuleSource = QUOTE(

foreign class Model
{
	init(filepath) {}

	foreign static create_from_polytope(poly)

	foreign static calc_brush_aabb(poly)
}

foreign class glTF
{
	init(filepath) {}

	foreign get_desc()

	foreign set_instanced_mats(mats)

	foreign static create_from_polytope(poly, material, offset, color, adjacencies)
	foreign static create_from_surface(surface)
}

);