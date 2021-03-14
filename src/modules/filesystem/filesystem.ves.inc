#define QUOTE(...) #__VA_ARGS__
static const char* filesystemModuleSource = QUOTE(

foreign class Filesystem
{
	foreign static get_file_dir(filepath)	
	foreign static set_asset_base_dir(path)
	foreign static get_asset_base_dir()
	foreign static get_absolute_path(path)
	foreign static get_filename(path)
	foreign static get_directory_files(path)
}

);