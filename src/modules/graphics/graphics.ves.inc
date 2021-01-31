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
}

foreign class Graphics
{
	foreign static drawPainter(pt)
	foreign static flush()
}

);