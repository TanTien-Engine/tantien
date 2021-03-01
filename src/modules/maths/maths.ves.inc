#define QUOTE(...) #__VA_ARGS__
static const char* mathsModuleSource = QUOTE(

foreign class Float2
{
	init() {}
	init(v) {
		this.setX(v.x())
		this.setY(v.y())
	}
	init(x, y) {
		this.setX(x)
		this.setY(y)
	}

	//foreign x=(x)
	//foreign x
	foreign setX(x)
	foreign x()
	foreign setY(y)
	foreign y()

	// mt: Matrix2D
	foreign transform(mt)
}

foreign class Matrix2D
{
	init() {}

	foreign transform(x, y, angle, sx, sy, ox, oy, kx, ky)
}

foreign class Matrix44
{
	init() {}

	foreign translate(x, y, z)
	foreign rotateAxis(angle, x, y, z)

	foreign perspective(fovy, aspect, znear, zfar)
	foreign lookat(eye, center, up)

	foreign fromRotateMat(mat)
}

foreign class Maths
{
	// conv0, conv1: float list
	foreign static isConvexIntersectConvex(conv0, conv1)
}

);