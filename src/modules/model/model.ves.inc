#define QUOTE(...) #__VA_ARGS__
static const char* modelModuleSource = QUOTE(

foreign class Model
{
	init(filepath) {}

	foreign static create_from_polytope(poly)

	foreign static calc_brush_size(poly)
}

foreign class glTF
{
	init(filepath) {}

	foreign get_desc()

	foreign static create_from_polytope(poly, material, offset)
	foreign static create_from_surface(surface)
}

);