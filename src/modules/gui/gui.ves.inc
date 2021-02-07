#define QUOTE(...) #__VA_ARGS__
static const char* guiModuleSource = QUOTE(

foreign class GUI
{
	foreign static begin()
	foreign static end()
	foreign static update()

	foreign static button(label, x, y, width, height)
}

);