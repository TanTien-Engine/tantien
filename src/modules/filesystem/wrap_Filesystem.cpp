#include "modules/filesystem/wrap_Filesystem.h"
#include "modules/filesystem/Filesystem.h"

#include <string.h>

namespace
{

void w_Filesystem_setAssetBaseDir()
{
    const char* path = ves_tostring(1);
    std::string filename(path);

    std::string directory;
    size_t last_slash_idx = filename.rfind('/');
    if (std::string::npos != last_slash_idx) {
        directory = filename.substr(0, last_slash_idx);
    } else {
        last_slash_idx = filename.rfind('\\');
        if (std::string::npos != last_slash_idx) {
            directory = filename.substr(0, last_slash_idx);
        }
    }

    tt::Filesystem::Instance()->SetAssetBaseDir(directory);
}

}

namespace tt
{

VesselForeignMethodFn FilesystemBindMethod(const char* signature)
{
    if (strcmp(signature, "static Filesystem.setAssetBaseDir(_)") == 0) return w_Filesystem_setAssetBaseDir;

    return NULL;
}

void FilesystemBindClass(const char* className, VesselForeignClassMethods* methods)
{
}

}