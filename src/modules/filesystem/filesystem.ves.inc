#define QUOTE(...) #__VA_ARGS__
static const char* filesystemModuleSource = QUOTE(

foreign class Filesystem
{
	foreign static setAssetBaseDir(path)
}

);