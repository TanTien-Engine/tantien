#include "modules/filesystem/wrap_Filesystem.h"
#include "modules/filesystem/Filesystem.h"

#include <filesystem>

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

void w_Filesystem_get_absolute_path()
{
    const char* path = ves_tostring(1);
    const auto& dir = std::filesystem::path(tt::Filesystem::Instance()->GetAssetBaseDir());
    auto absolute = std::filesystem::canonical(std::filesystem::path(dir) / path).string();

    ves_set_lstring(0, absolute.c_str(), absolute.size());
}

}

namespace tt
{

VesselForeignMethodFn FilesystemBindMethod(const char* signature)
{
    if (strcmp(signature, "static Filesystem.setAssetBaseDir(_)") == 0) return w_Filesystem_setAssetBaseDir;
    if (strcmp(signature, "static Filesystem.get_absolute_path(_)") == 0) return w_Filesystem_get_absolute_path;

    return NULL;
}

void FilesystemBindClass(const char* className, VesselForeignClassMethods* methods)
{
}

}