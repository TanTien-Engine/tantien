#include "modules/filesystem/Filesystem.h"

#include <filesystem>

namespace tt
{

TT_SINGLETON_DEFINITION(Filesystem)

Filesystem::Filesystem()
{
}

Filesystem::~Filesystem()
{
}

bool Filesystem::IsExists(const char* filepath)
{
//	struct stat buffer;
//	return (stat(filepath, &buffer) == 0);

	return std::filesystem::exists(filepath) 
		&& std::filesystem::is_regular_file(filepath);
}

}