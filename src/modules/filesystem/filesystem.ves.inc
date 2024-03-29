#define QUOTE(...) #__VA_ARGS__
static const char* filesystemModuleSource = QUOTE(

foreign class Filesystem
{
	foreign static get_file_dir(filepath)	
	foreign static set_asset_base_dir(path)
	foreign static get_asset_base_dir()
	foreign static get_absolute_path(path)
	foreign static get_relative_path(path)
	foreign static get_filename(path)
	foreign static get_directory(path)	
	foreign static get_directory_files(path)
	foreign static remove_file(path)
	foreign static is_file_exists(path)	
	foreign static get_file_last_write_time(path)
}

);