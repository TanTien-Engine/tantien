#define QUOTE(...) #__VA_ARGS__
static const char* guiModuleSource = QUOTE(

foreign class GUI
{
	foreign static begin()
	foreign static end()
	foreign static update()

	foreign static onSize(w, h)
	// btn: 1 left, 2 right
	// action: 0 down, 1 up
	foreign static mouseInput(btn, action, x, y)

	foreign static button(label, x, y, width, height)
}

);