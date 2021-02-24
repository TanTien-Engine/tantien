#include "modules/filesystem/Filesystem.h"

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
	struct stat buffer;
	return (stat(filepath, &buffer) == 0);
}

}