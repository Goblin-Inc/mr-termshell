#ifndef FS_UTILS_HPP
#define FS_UTILS_HPP

#include "types.hpp"
#include <vector>
#include <string>

bool is_image_file(const fs::path& path);
std::vector<FileEntry> get_directory_contents(const fs::path& dir_path, SortMode sort_mode = SortMode::NAME, bool reverse_sort = false);
std::vector<std::string> read_file_lines(const fs::path& file_path);
bool save_file_content(const fs::path& file_path, const std::string& content);
std::vector<std::string> execute_command(const std::string& command, const fs::path& work_dir);
std::vector<std::string> get_image_info(const fs::path& file_path);

std::string format_size(uintmax_t size);
std::string format_permissions(fs::perms p);
std::string format_time(fs::file_time_type ftime);

#endif
