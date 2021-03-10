#define QUOTE(...) #__VA_ARGS__
static const char* filesystemModuleSource = QUOTE(

foreign class Filesystem
{
	foreign static setAssetBaseDir(path)
	foreign static get_absolute_path(path)
	foreign static get_filename(path)
}

);