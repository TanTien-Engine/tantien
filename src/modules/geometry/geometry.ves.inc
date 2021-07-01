#define QUOTE(...) #__VA_ARGS__
static const char* geometryModuleSource = QUOTE(

foreign class Line
{
	init(x0, y0, x1, y1) {}

	// [x0, y0, x1, y1]
	foreign get()
}

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

foreign class ConstraintSolver
{
	init() {}

	foreign add_distance(line, dist)

	foreign solve()
	foreign clear()
}

);