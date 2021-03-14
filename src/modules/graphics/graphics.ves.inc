#define QUOTE(...) #__VA_ARGS__
static const char* graphicsModuleSource = QUOTE(

foreign class Painter
{
	init() {}

	// rect: list [x, y, width, height]
	// color: list [r, g, b, a]
	foreign add_rect(rect, color, line_width)
	foreign add_rect_filled(rect, color)

	// vertices: list [x0, y0, x1, y1 ...]
	// color: list [r, g, b, a]
	foreign add_polygon(vertices, color, line_width)
	foreign add_polyline(vertices, color, line_width)

	// color: list [r, g, b, a]
	foreign add_circle(x, y, radius, color)
	foreign add_circle_filled(x, y, radius, color)

	// 
	foreign add_bezier(bezier, color, line_width)
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
	foreign static on_size(w, h)
	foreign static on_cam_update(dx, dy, scale)
	foreign static draw_painter(pt)
	foreign static draw_text(text, x, y, scale, textbox)
	foreign static draw_texture(texture, x, y, scale)
	foreign static flush()
	foreign static dtex_debug_draw()
	foreign static get_width()
	foreign static get_height()
}

);