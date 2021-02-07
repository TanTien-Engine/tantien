#define QUOTE(...) #__VA_ARGS__
static const char* imageModuleSource = QUOTE(

foreign class ImageData
{
	init(filepath) {}
}

);