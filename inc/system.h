#ifndef FF_SYSTEM_H
#define FF_SYSTEM_H

#include <string>
#include <filesystem>

namespace System {

bool is_file_exists(const std::string &path);
bool is_directory_exists(const std::string &path);
bool remove_directory(const std::filesystem::path path);

};

#endif
