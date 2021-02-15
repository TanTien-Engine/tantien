#define QUOTE(...) #__VA_ARGS__
static const char* imageModuleSource = QUOTE(

foreign class ImageData
{
	init(filepath) {}
	init(width,height,format) {}

	foreign getWidth()
	foreign getHeight()

	foreign toRGB565()

	foreign storeToFile(filepath)

	foreign getPixel(x, y)
	foreign setPixel(x, y, r, g, b)
	
	foreign cropping(x, y, w, h)
	foreign copyFrom(sub_img, x, y)

	foreign isEmpty()
}

);