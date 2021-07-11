#define QUOTE(...) #__VA_ARGS__
static const char* keyboardModuleSource = QUOTE(

foreign class Keyboard
{
	foreign static is_ctrl_pressed()
	foreign static is_shift_pressed()
}

);