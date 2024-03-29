#define QUOTE(...) #__VA_ARGS__
static const char* mathsModuleSource = QUOTE(

foreign class Float2
{
	init() {}
	init(v) {
		this.set_x(v.x())
		this.set_y(v.y())
	}
	init(x, y) {
		this.set_x(x)
		this.set_y(y)
	}

	//foreign x=(x)
	//foreign x
	foreign set_x(x)
	foreign x()
	foreign set_y(y)
	foreign y()

	// mt: Matrix2D
	foreign transform(mt)
}

foreign class Float3
{
	init() {}
	init(v) {
		this.set_x(v.x())
		this.set_y(v.y())
		this.set_z(v.z())
	}
	init(x, y, z) {
		this.set_x(x)
		this.set_y(y)
		this.set_z(z)
	}

	//foreign x=(x)
	//foreign x
	foreign set_x(x)
	foreign x()
	foreign set_y(y)
	foreign y()
	foreign set_z(z)
	foreign z()

	// mt: mat4
	foreign transform(mt)
}

foreign class Float4
{
	init() {}
	init(v) {
		this.set_x(v.x())
		this.set_y(v.y())
		this.set_z(v.z())
		this.set_w(v.w())
	}
	init(x, y, z, w) {
		this.set_x(x)
		this.set_y(y)
		this.set_z(z)
		this.set_w(w)
	}

	//foreign x=(x)
	//foreign x
	foreign set_x(x)
	foreign x()
	foreign set_y(y)
	foreign y()
	foreign set_z(z)
	foreign z()
	foreign set_w(w)
	foreign w()
}

foreign class Matrix2D
{
	init() {}

	foreign transform(x, y, angle, sx, sy, ox, oy, kx, ky)
	foreign transform_mat(mat)

	foreign decompose()
}

foreign class Matrix44
{
	init() {}

	foreign clone()

	foreign translate(x, y, z)
	foreign rotate(x, y, z)
	foreign rotate_axis(angle, x, y, z)
	foreign scale(sx, sy, sz)	
	foreign skew(x, y, z)

	foreign perspective(fovy, aspect, znear, zfar)
	foreign orthographic(left, right, bottom, top, near, far)
	foreign lookat(eye, center, up)
	foreign viewport(width, height, near, far)

	foreign from_rotate_mat(mat)
	foreign from_quaternion(x, y, z, w)
	foreign from_vectors(from, to)

	foreign transform_mat2d(mt)
	foreign transform_mat4(mt)

	foreign get_scale()	

	foreign inverse()

	foreign decompose()
}

foreign class Plane
{
	init(p0, p1, p2) {}
	init(ori, dir) {}

	foreign clone()

	foreign transform(mt)
}

foreign class Cube
{
	init() {}

	foreign get_center()
	foreign get_size()
}

foreign class Maths
{
	// conv0, conv1: float list
	foreign static is_convex_intersect_convex(conv0, conv1)

	// line0, line1: [x0, y0, x1, y1]
	foreign static get_line_intersect_line(line0, line1, is_seg)

	// from, to: [x, y, z]
	foreign static calc_rot_mat(from, to)

	foreign static calc_angle(center, start, end)

	foreign static is_pos_above_plane(pos, plane)
}

);