#define QUOTE(...) #__VA_ARGS__
static const char* geometryModuleSource = QUOTE(

foreign class Shape2D
{
	foreign is_contain(x, y)
	foreign is_intersect(x, y, w, h)

	foreign translate(dx, dy)
}

foreign class Point is Shape2D
{
	init() {}
	init(x, y) { this.set(x, y) }

	toString() {
		var params = this.get()
		return "::geometry::Point.init(%(params[0]), %(params[1]))"
	}

	foreign clone()

	// [x, y]
	foreign get()
	foreign set(x, y)
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

foreign class Arc is Shape2D
{
	init() {}
	init(x, y, radius, start_angle, end_angle) { this.set(x, y, radius, start_angle, end_angle) }

	toString() {
		var params = this.get()
		return "::geometry::Arc.init(%(params[0]), %(params[1]), %(params[2]), %(params[3]), %(params[4]))"
	}

	foreign clone()

	// [x, y, radius, start_angle, end_angle]
	foreign get()
	foreign set(x, y, radius, start_angle, end_angle)

	foreign get_vertices()
}

foreign class Ellipse is Shape2D
{
	init() {}
	init(x, y, rx, ry) { this.set(x, y, rx, ry) }

	toString() {
		var params = this.get()
		return "::geometry::Ellipse.init(%(params[0]), %(params[1]), %(params[2]), %(params[3]))"
	}

	foreign clone()

	// [x, y, rx, ry]
	foreign get()
	foreign set(x, y, rx, ry)

	foreign get_vertices()
}

foreign class Polyline is Shape2D
{
	init() {}
	init(vertices) { this.set_vertices(vertices) }

	toString() {
		var vertices = this.get_vertices()
		return "::geometry::Polyline.init(%(vertices))"
	}

	foreign clone()

	foreign get_vertices()
	// vertices: list
	foreign set_vertices(vertices)

	foreign set_closed(is_closed)
	foreign get_closed()

	foreign resample(length)
}

foreign class Polygon is Shape2D
{
	init() {}
	init(vertices) { this.set_vertices(vertices) }

	toString() {
		var vertices = this.get_vertices()
		return "::geometry::Polygon.init(%(vertices))"
	}

	foreign clone()

	foreign get_vertices()
	foreign set_vertices(vertices)

	foreign add_hole(hole)
	foreign get_holes()

	foreign get_tris()

	foreign calc_area()
}

foreign class Bezier is Shape2D
{
	init() {}

	foreign clone()
	
	// vertices: list [x0, y0, x1, y1, x2, y2, x3, y3]
	foreign set_ctrl_pos(vertices)
}

foreign class Shape3D
{
	init() {}
}

foreign class Line3D is Shape3D
{
	init() {}
	init(p0, p1) { 
		this.set_p0(p0)
		this.set_p1(p1)
	}

	toString() {
		return "::geometry::Line3D.init(%(this.get_p0()), %(this.get_p1()))"
	}

	foreign get_p0()
	foreign set_p0(p)
	foreign get_p1()
	foreign set_p1(p)
}

foreign class Box is Shape3D
{
	init() {}
	init(min, max) { this.set_size(min, max) }

	toString() {
		return "::geometry::Box.init(%(this.get_min()), %(this.get_max()))"
	}

	foreign get_min()
	foreign get_max()
	foreign set_size(min, max)
}

foreign class Polyline3D is Shape3D
{
	init() {}
	init(vertices) { this.set_vertices(vertices) }

	toString() {
		var vertices = this.get_vertices()
		return "::geometry::Polyline3D.init(%(vertices))"
	}

	foreign get_vertices()
	foreign set_vertices(vertices)

	foreign set_closed(is_closed)
	foreign get_closed()
}

foreign class Polygon3D is Shape3D
{
	init() {}
	init(vertices) { this.set_vertices(vertices) }

	toString() {
		var vertices = this.get_vertices()
		return "::geometry::Polygon3D.init(%(vertices))"
	}

	foreign get_vertices()
	foreign set_vertices(vertices)
}

foreign class PolyPoint
{
	init(pos) {}

	foreign clone()

	foreign get_pos()
	foreign set_pos(pos)
}

foreign class PolyFace
{
	init(plane) {}
	init(border, holes) {}

	foreign clone()

	foreign get_normal()
	foreign flip_normal()

	foreign get_border()
	foreign get_holes()
}

foreign class Polytope
{
	init(faces) {}
	init(points, faces) {}

	foreign clone()

	foreign extrude(dist)
	foreign offset(selector, dist)

	foreign transform(mat)
	foreign mirror(plane)

	foreign clip(plane, keep, seam)

	foreign get_points()
	foreign get_faces()

	foreign set_topo_dirty()

	foreign is_contain(pos)
	foreign is_face_inside(face)

	// a, b is list
	foreign static boolean(operator, a, b)
}

foreign class Sphere
{
	init(radius) {}

	foreign clone()	
}

foreign class Ellipsoid
{
	// [x, y, z]
	init(radius) {}

	foreign clone()
}

foreign class ShapeMaths
{
	foreign static scissor(shape, rect)
	foreign static expand(shape, dist)
	foreign static extrude(shape, dist)
	foreign static merge(shapes)
}

);