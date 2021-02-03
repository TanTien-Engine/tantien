#define QUOTE(...) #__VA_ARGS__
static const char* graphicsModuleSource = QUOTE(

foreign class Painter
{
	init() {}

	// rect: list [x, y, width, height]
	// color: list [r, g, b, a]
	foreign addRect(rect, color, line_width)

	// rect: list [x, y, width, height]
	// color: list [r, g, b, a]
	foreign addRectFilled(rect, color)

	// vertices: list [x0, y0, x1, y1 ...]
	// color: list [r, g, b, a]
	foreign addPolygon(vertices, color, line_width)
}

foreign class Graphics
{
	foreign static onSize(w, h)
	foreign static onCamUpdate(dx, dy, scale)
	foreign static drawPainter(pt)
	foreign static drawText(text)
	foreign static flush()
}

);