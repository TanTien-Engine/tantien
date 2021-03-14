#define QUOTE(...) #__VA_ARGS__
static const char* imageModuleSource = QUOTE(

foreign class ImageData
{
	init(filepath) {}
	init(texture) {}
	// format: <"rgb", "rgba">
	init(width, height, format) {}

	foreign get_width()
	foreign get_height()

	foreign to_rgb565()

	foreign store_to_file(filepath)

	foreign get_pixel(x, y)
	foreign set_pixel(x, y, r, g, b)
	
	foreign cropping(x, y, w, h)
	foreign copy_from(sub_img, x, y, debug_border)

	foreign is_empty()
}

);