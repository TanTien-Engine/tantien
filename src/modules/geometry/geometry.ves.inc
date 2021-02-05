#define QUOTE(...) #__VA_ARGS__
static const char* geometryModuleSource = QUOTE(

foreign class Bezier
{
	init() {}
	
	// vertices: list [x0, y0, x1, y1, x2, y2, x3, y3]
	foreign setCtrlPos(vertices)
}

);