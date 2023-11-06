#define QUOTE(...) #__VA_ARGS__
static const char* omModuleSource = QUOTE(

foreign class Label
{
	init() {}

	foreign set_shape(shape)
	foreign get_shape()

	foreign set_render_obj(va)
	foreign get_render_obj()

	foreign set_color(rgb)
	foreign get_color()

	foreign get_children()

	foreign build_vao()
}

foreign class TopoShape
{
}

foreign class TopoVertex is TopoShape
{
	init() {}
}

foreign class TopoEdge is TopoShape
{
	init() {}
}

foreign class TopoLoop is TopoShape
{
	init() {}
}

foreign class TopoFace is TopoShape
{
	init() {}
}

foreign class TopoShell is TopoShape
{
	init() {}
}

foreign class TopoBody is TopoShape
{
	init() {}
}

foreign class BRepTools
{
	foreign static poly2shape(poly)
	foreign static shape2vao(shape, color)	
}

foreign class BRepAlgos
{
	foreign static clip(shape, plane, keep, seam)
}

foreign class PrimBuilder
{
	foreign static box(center, size)
}

);