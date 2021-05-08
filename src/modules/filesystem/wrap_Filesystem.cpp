#include "modules/filesystem/wrap_Filesystem.h"
#include "modules/filesystem/Filesystem.h"

#include <filesystem>
#include <cstdio>

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

void w_Filesystem_get_directory_files()
{
    std::vector<std::string> files;

    const char* dir_path = ves_tostring(1);
    for (auto& p : std::filesystem::recursive_directory_iterator(dir_path)) {
        files.push_back(std::filesystem::absolute(p).string());
    }

    ves_pop(2);
    ves_newlist(files.size());
    for (int i = 0, n = files.size(); i < n; ++i)
    {
        ves_pushstring(files[i].c_str());
        ves_seti(-2, i);
        ves_pop(1);
    }
}

void w_Filesystem_remove_file()
{
    const char* path = ves_tostring(1);
    int ret = remove(path);
    ves_set_boolean(0, ret == 0);
}

}

namespace tt
{

VesselForeignMethodFn FilesystemBindMethod(const char* signature)
{
    if (strcmp(signature, "static Filesystem.get_file_dir(_)") == 0) return w_Filesystem_get_file_dir;
    if (strcmp(signature, "static Filesystem.set_asset_base_dir(_)") == 0) return w_Filesystem_setAssetBaseDir;
    if (strcmp(signature, "static Filesystem.get_asset_base_dir()") == 0) return w_Filesystem_get_asset_base_dir;
    if (strcmp(signature, "static Filesystem.get_absolute_path(_)") == 0) return w_Filesystem_get_absolute_path;
    if (strcmp(signature, "static Filesystem.get_filename(_)") == 0) return w_Filesystem_get_filename;
    if (strcmp(signature, "static Filesystem.get_directory_files(_)") == 0) return w_Filesystem_get_directory_files;
    if (strcmp(signature, "static Filesystem.remove_file(_)") == 0) return w_Filesystem_remove_file;

    return NULL;
}

void FilesystemBindClass(const char* class_name, VesselForeignClassMethods* methods)
{
}

}