#define QUOTE(...) #__VA_ARGS__
static const char* geometryModuleSource = QUOTE(

foreign class Rect
{
	init(x, y, w, h) {}

	// [x, y, w, h]
	foreign get()
}

foreign class Circle
{
	init(x, y, radius) {}

	// [x, y, radius]
	foreign get()
}

foreign class Polyline
{
	init() {}

	// vertices: list
	foreign set_vertices(vertices)
}

foreign class Bezier
{
	init() {}
	
	// vertices: list [x0, y0, x1, y1, x2, y2, x3, y3]
	foreign set_ctrl_pos(vertices)
}

);