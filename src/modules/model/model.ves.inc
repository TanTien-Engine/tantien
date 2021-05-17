#define QUOTE(...) #__VA_ARGS__
static const char* modelModuleSource = QUOTE(

foreign class Model
{
	init(filepath) {}
}

foreign class glTF
{
	init(filepath) {}

	foreign get_desc()
}

);