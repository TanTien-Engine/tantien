#define QUOTE(...) #__VA_ARGS__
static const char* graphicsModuleSource = QUOTE(

foreign class Painter
{
	init() {}

	// rect: list [x, y, width, height]
	// color: list [r, g, b, a]
	foreign addRect(rect, color, line_width)
	foreign addRectFilled(rect, color)

	// vertices: list [x0, y0, x1, y1 ...]
	// color: list [r, g, b, a]
	foreign addPolygon(vertices, color, line_width)
	foreign addPolyline(vertices, color, line_width)

	// color: list [r, g, b, a]
	foreign addCircle(x, y, radius, color)
	foreign addCircleFilled(x, y, radius, color)

	// 
	foreign addBezier(bezier, color, line_width)
}

class Textbox
{
	init() 
	{
		this.width  = 100
		this.height = 20

		this.font_type = 0
		this.font_size = 16
		this.font_color = [255, 255, 255]

		// left, right, center, auto
		this.align_hori = 0
		// top, bottom, center, auto
		this.align_vert = 0
	}
}

foreign class Graphics
{
	foreign static onSize(w, h)
	foreign static onCamUpdate(dx, dy, scale)
	foreign static drawPainter(pt)
	foreign static drawText(text, x, y, scale, textbox)
	foreign static flush()
}

);