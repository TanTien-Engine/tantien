#define QUOTE(...) #__VA_ARGS__
static const char* modelModuleSource = QUOTE(

foreign class Model
{
	init(filepath) {}

	foreign get_pbr_textures()
}

foreign class glTF
{
	init(filepath) {}

	foreign get_desc()
}

);