#ifndef TYPES_HPP
#define TYPES_HPP

#include <string>
#include <filesystem>
#include <chrono>

namespace fs = std::filesystem;

struct FileEntry {
    std::string name;
    fs::path path;
    bool is_dir;
    uintmax_t size = 0;
    fs::file_time_type last_write_time;
    fs::perms permissions;
};

enum class SortMode { NAME, SIZE, DATE };
enum class ClipboardAction { NONE, COPY, CUT };

struct ClipboardItem {
    fs::path path;
    ClipboardAction action = ClipboardAction::NONE;
};

enum class ModalType { NONE, CREATE, RENAME, DELETE_CONFIRM, SHELL_INPUT, COMMAND_OUTPUT, SEARCH_FILTER, EDITOR };

#endif
