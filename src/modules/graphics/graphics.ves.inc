#define QUOTE(...) #__VA_ARGS__
static const char* graphicsModuleSource = QUOTE(

foreign class Viewport
{
	init() {}

	foreign set_size(width, height)
}

// color: list [r, g, b, a]
foreign class Painter
{
	init() {}

	// line: list [x0, x1, y0, y1]
	foreign add_line(mat, line, color, line_width)

	// rect: list [x, y, width, height]	
	foreign add_rect(mat, rect, color, line_width)
	foreign add_rect_filled(mat, rect, color)
	foreign add_capsule_filled(mat, rect, color, hori)

	// vertices: list [x0, y0, x1, y1 ...]
	foreign add_polygon(mat, vertices, color, line_width)
	foreign add_polygon_filled(mat, vertices, color)
	foreign add_polyline(mat, vertices, color, line_width)

	foreign add_triangles_filled(mat, tris, color)

	foreign add_circle(mat, x, y, radius, color, line_width, num_segments)
	foreign add_circle_filled(mat, x, y, radius, color, num_segments)

	foreign add_arc(mat, x, y, radius, start_angle, end_angle, color, line_width, num_segments)

	foreign add_bezier(mat, bezier, color, line_width)

	foreign add_point3d(mat, p, color, size)

	foreign add_line3d(mat, p0, p1, color, line_width)

	foreign add_cube(mat, min, max, color, line_width)

	foreign add_polyline3d(mat, vertices, color, line_width)

	foreign add_polygon3d_filled(mat, vertices, color)
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
	foreign static draw_texture(texture, pos, angle, scale, offset)
	foreign static draw_texture2(texture, mat)
	foreign static flush()
	foreign static dtex_debug_draw()
	foreign static get_width()
	foreign static get_height()

	// for debug draw
	foreign static set_intense(intense)

	// fixme
	foreign static set_cam_ortho(ortho)
}

);