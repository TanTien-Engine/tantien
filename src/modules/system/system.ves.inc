#define QUOTE(...) #__VA_ARGS__
static const char* systemModuleSource = QUOTE(

foreign class System
{
	foreign static set_clipboard_text(text)
	foreign static get_clipboard_text()
}

);