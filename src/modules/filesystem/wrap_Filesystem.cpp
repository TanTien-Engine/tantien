#include "modules/filesystem/wrap_Filesystem.h"
#include "modules/filesystem/Filesystem.h"

#include <filesystem>

#include <string.h>

namespace
{

void w_Filesystem_get_file_dir()
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

    ves_set_lstring(0, directory.c_str(), directory.size());
}

void w_Filesystem_setAssetBaseDir()
{
    const char* directory = ves_tostring(1);
    tt::Filesystem::Instance()->SetAssetBaseDir(directory);
}

void w_Filesystem_get_asset_base_dir()
{
    auto& dir = tt::Filesystem::Instance()->GetAssetBaseDir();
    ves_set_lstring(0, dir.c_str(), dir.size());
}

void w_Filesystem_get_absolute_path()
{
    const char* path = ves_tostring(1);
    if (std::filesystem::exists(std::filesystem::path(path))) {
        auto absolute = std::filesystem::canonical(path).string();
        ves_set_lstring(0, absolute.c_str(), absolute.size());
    } else {
        const auto& dir = std::filesystem::path(tt::Filesystem::Instance()->GetAssetBaseDir());
        auto absolute = std::filesystem::canonical(std::filesystem::path(dir) / path).string();
        ves_set_lstring(0, absolute.c_str(), absolute.size());
    }
}

void w_Filesystem_get_filename()
{
    const char* path = ves_tostring(1);
    auto filename = std::filesystem::path(path).stem().string();
    ves_set_lstring(0, filename.c_str(), filename.size());
}

}

namespace tt
{

VesselForeignMethodFn FilesystemBindMethod(const char* signature)
{
    if (strcmp(signature, "static Filesystem.get_file_dir(_)") == 0) return w_Filesystem_get_file_dir;
    if (strcmp(signature, "static Filesystem.setAssetBaseDir(_)") == 0) return w_Filesystem_setAssetBaseDir;
    if (strcmp(signature, "static Filesystem.get_asset_base_dir()") == 0) return w_Filesystem_get_asset_base_dir;
    if (strcmp(signature, "static Filesystem.get_absolute_path(_)") == 0) return w_Filesystem_get_absolute_path;
    if (strcmp(signature, "static Filesystem.get_filename(_)") == 0) return w_Filesystem_get_filename;

    return NULL;
}

void FilesystemBindClass(const char* className, VesselForeignClassMethods* methods)
{
}

}