#define QUOTE(...) #__VA_ARGS__
static const char* geometryModuleSource = QUOTE(

foreign class Shape2D
{
	foreign is_contain(x, y)
	foreign is_intersect(x, y, w, h)
}

foreign class Line is Shape2D
{
	init() {}
	init(x0, y0, x1, y1) { this.set(x0, y0, x1, y1) }

	toString() {
		var params = this.get()
		return "::geometry::Line.init(%(params[0]), %(params[1]), %(params[2]), %(params[3]))"
	}

	foreign clone()

	// [x0, y0, x1, y1]
	foreign get()
	foreign set(x0, y0, x1, y1)
}

foreign class Rect is Shape2D
{
	init() {}
	init(x, y, w, h) { this.set(x, y, w, h) }

	toString() {
		var params = this.get()
		return "::geometry::Rect.init(%(params[0]), %(params[1]), %(params[2]), %(params[3]))"
	}

	foreign clone()

	// [x, y, w, h]
	foreign get()
	foreign set(x, y, w, h)
}

foreign class Circle is Shape2D
{
	init() {}
	init(x, y, radius) { this.set(x, y, radius) }

	toString() {
		var params = this.get()
		return "::geometry::Circle.init(%(params[0]), %(params[1]), %(params[2]))"
	}

	foreign clone()

	// [x, y, radius]
	foreign get()
	foreign set(x, y, radius)
}

foreign class Polyline is Shape2D
{
	init() {}

	foreign clone()

	// vertices: list
	foreign set_vertices(vertices)
}

foreign class Bezier is Shape2D
{
	init() {}

	foreign clone()
	
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