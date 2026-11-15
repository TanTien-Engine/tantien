#include "modules/filesystem/wrap_Filesystem.h"
#include "modules/filesystem/Filesystem.h"

#include <wrapper/TransHelper.h>

#include <filesystem>
#include <cstdio>

#include <string.h>
#include <sys/stat.h> // struct stat / stat() (not pulled in transitively on macOS)

namespace
{

// A non-string script arg makes ves_tostring return null; these are path
// utilities, so treat that as an empty path instead of crashing in
// std::string/std::filesystem::path construction.
const char* arg_str(int idx)
{
    const char* s = ves_tostring(idx);
    return s ? s : "";
}

void w_Filesystem_get_file_dir()
{
    const char* path = arg_str(1);
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

void w_Filesystem_set_asset_base_dir()
{
    const char* directory = arg_str(1);
    tt::Filesystem::Instance()->SetAssetBaseDir(directory);
}

void w_Filesystem_get_asset_base_dir()
{
    auto& dir = tt::Filesystem::Instance()->GetAssetBaseDir();
    ves_set_lstring(0, dir.c_str(), dir.size());
}

void w_Filesystem_get_absolute_path()
{
    // error_code overloads throughout: a throwing std::filesystem call would
    // unwind across the VM's C callback boundary (UB/abort).
    const char* path = arg_str(1);
    std::error_code ec;
    if (std::filesystem::exists(std::filesystem::path(path), ec)) {
        auto absolute = std::filesystem::weakly_canonical(path, ec).string();
        ves_set_lstring(0, absolute.c_str(), absolute.size());
    } else {
        auto& base_dir = tt::Filesystem::Instance()->GetAssetBaseDir();
        if (base_dir.empty()) {
            auto absolute = std::filesystem::weakly_canonical(path, ec).string();
            ves_set_lstring(0, absolute.c_str(), absolute.size());
        } else {
            const auto& dir_path = std::filesystem::path(base_dir);
            auto full_path = std::filesystem::path(dir_path) / path;
            auto absolute = std::filesystem::weakly_canonical(full_path, ec).string();
            ves_set_lstring(0, absolute.c_str(), absolute.size());
        }
    }
}

void w_Filesystem_get_relative_path()
{
    const char* path = arg_str(1);
    auto& dir = tt::Filesystem::Instance()->GetAssetBaseDir();
    std::error_code ec;
    auto relative = std::filesystem::relative(path, dir, ec);

    std::string formated = relative.string();
    std::replace(formated.begin(), formated.end(), '\\', '/');
    ves_set_lstring(0, formated.c_str(), formated.size());
}

void w_Filesystem_get_filename()
{
    const char* path = arg_str(1);
    auto filename = std::filesystem::path(path).stem().string();
    ves_set_lstring(0, filename.c_str(), filename.size());
}

void w_Filesystem_get_directory()
{
    const char* path = arg_str(1);
    auto directory = std::filesystem::path(path).parent_path().string();
    ves_set_lstring(0, directory.c_str(), directory.size());
}

void w_Filesystem_get_directory_files()
{
    std::vector<std::string> files;

    const char* dir_path = arg_str(1);
    // error_code overload: a missing/inaccessible dir yields an end iterator
    // instead of throwing a filesystem_error across the VM's C callback boundary
    // (an uncaught C++ exception there is undefined behaviour / abort).
    std::error_code ec;
    for (auto& p : std::filesystem::recursive_directory_iterator(dir_path, ec)) {
        std::error_code abs_ec;
        files.push_back(std::filesystem::absolute(p, abs_ec).string());
    }
    wrapper::return_list(files);
}

void w_Filesystem_remove_file()
{
    const char* path = arg_str(1);
    int ret = remove(path);
    ves_set_boolean(0, ret == 0);
}

void w_Filesystem_is_file_exists()
{
    const char* path = arg_str(1);
    auto exists = tt::Filesystem::IsExists(path);
    ves_set_boolean(0, exists);
}

void w_Filesystem_get_file_last_write_time()
{
    const char* path = arg_str(1);
    struct stat attr;
    if (stat(path, &attr) != 0) {
        // missing file: 0 instead of reading uninitialized attr.st_mtime
        ves_set_number(0, 0);
        return;
    }
    ves_set_number(0, attr.st_mtime);
}

}

namespace tt
{

VesselForeignMethodFn FilesystemBindMethod(const char* signature)
{
    if (strcmp(signature, "static Filesystem.get_file_dir(_)") == 0) return w_Filesystem_get_file_dir;
    if (strcmp(signature, "static Filesystem.set_asset_base_dir(_)") == 0) return w_Filesystem_set_asset_base_dir;
    if (strcmp(signature, "static Filesystem.get_asset_base_dir()") == 0) return w_Filesystem_get_asset_base_dir;
    if (strcmp(signature, "static Filesystem.get_absolute_path(_)") == 0) return w_Filesystem_get_absolute_path;
    if (strcmp(signature, "static Filesystem.get_relative_path(_)") == 0) return w_Filesystem_get_relative_path;
    if (strcmp(signature, "static Filesystem.get_filename(_)") == 0) return w_Filesystem_get_filename;
    if (strcmp(signature, "static Filesystem.get_directory(_)") == 0) return w_Filesystem_get_directory;
    if (strcmp(signature, "static Filesystem.get_directory_files(_)") == 0) return w_Filesystem_get_directory_files;
    if (strcmp(signature, "static Filesystem.remove_file(_)") == 0) return w_Filesystem_remove_file;
    if (strcmp(signature, "static Filesystem.is_file_exists(_)") == 0) return w_Filesystem_is_file_exists;
    if (strcmp(signature, "static Filesystem.get_file_last_write_time(_)") == 0) return w_Filesystem_get_file_last_write_time;

    return NULL;
}

void FilesystemBindClass(const char* class_name, VesselForeignClassMethods* methods)
{
}

}