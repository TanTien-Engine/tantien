#define QUOTE(...) #__VA_ARGS__
static const char* geometryModuleSource = QUOTE(

foreign class Line
{
	init() {}

	// [x0, y0, x1, y1]
	foreign get()
	foreign set(x0, y0, x1, y1)
}

foreign class Rect
{
	init() {}

	// [x, y, w, h]
	foreign get()
	foreign set(x, y, w, h)
}

foreign class Circle
{
	init() {}

	// [x, y, radius]
	foreign get()
	foreign set(x, y, radius)
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

foreign class Constraint
{
	init(type, geo0, geo1, value) {}

	foreign set_value(v)
}

foreign class ConstraintSolver
{
	init() {}

	foreign add_geo(shape)
	foreign add_cons(cons)

	foreign solve()
	foreign clear()
}

);