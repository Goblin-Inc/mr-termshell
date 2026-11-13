#include "fs_utils.hpp"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <array>
#include <cstdio>
#include <memory>
#include <iomanip>
#include <chrono>

std::string format_size(uintmax_t size) {
    static const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int idx = 0;
    double count = static_cast<double>(size);
    while (count >= 1024.0 && idx < 4) {
        count /= 1024.0;
        idx++;
    }
    std::ostringstream out;
    out << std::fixed << std::setprecision(idx == 0 ? 0 : 1) << count << " " << units[idx];
    return out.str();
}

std::string format_permissions(fs::perms p) {
    std::string res = "---------";
    if ((p & fs::perms::owner_read) != fs::perms::none) res[0] = 'r';
    if ((p & fs::perms::owner_write) != fs::perms::none) res[1] = 'w';
    if ((p & fs::perms::owner_exec) != fs::perms::none) res[2] = 'x';
    if ((p & fs::perms::group_read) != fs::perms::none) res[3] = 'r';
    if ((p & fs::perms::group_write) != fs::perms::none) res[4] = 'w';
    if ((p & fs::perms::group_exec) != fs::perms::none) res[5] = 'x';
    if ((p & fs::perms::others_read) != fs::perms::none) res[6] = 'r';
    if ((p & fs::perms::others_write) != fs::perms::none) res[7] = 'w';
    if ((p & fs::perms::others_exec) != fs::perms::none) res[8] = 'x';
    return res;
}

std::string format_time(fs::file_time_type ftime) {
    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
    );
    std::time_t tt = std::chrono::system_clock::to_time_t(sctp);
    std::tm* tm_info = std::localtime(&tt);
    char buf[20];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", tm_info);
    return std::string(buf);
}

bool is_image_file(const fs::path& path) {
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".webp" || ext == ".bmp");
}

std::vector<FileEntry> get_directory_contents(const fs::path& dir_path, SortMode sort_mode, bool reverse_sort) {
    std::vector<FileEntry> entries;
    
    if (dir_path.has_parent_path() && dir_path != dir_path.root_path()) {
        entries.push_back({"..", dir_path.parent_path(), true, 0, fs::file_time_type{}, fs::perms::none});
    }

    try {
        for (const auto& entry : fs::directory_iterator(dir_path, fs::directory_options::skip_permission_denied)) {
            bool is_dir = entry.is_directory();
            uintmax_t size = 0;
            if (!is_dir && entry.is_regular_file()) {
                size = entry.file_size();
            }

            entries.push_back({
                entry.path().filename().string(),
                entry.path(),
                is_dir,
                size,
                entry.last_write_time(),
                entry.status().permissions()
            });
        }
    } catch (...) {}

    std::sort(entries.begin(), entries.end(), [sort_mode, reverse_sort](const FileEntry& a, const FileEntry& b) {
        if (a.name == "..") return true;
        if (b.name == "..") return false;

        bool result = false;
        if (a.is_dir != b.is_dir) {
            return a.is_dir > b.is_dir;
        }

        if (sort_mode == SortMode::SIZE) {
            result = a.size < b.size;
        } else if (sort_mode == SortMode::DATE) {
            result = a.last_write_time < b.last_write_time;
        } else {
            result = a.name < b.name;
        }

        return reverse_sort ? !result : result;
    });

    return entries;
}

std::vector<std::string> read_file_lines(const fs::path& file_path) {
    std::vector<std::string> lines;
    std::ifstream file(file_path);
    if (!file.is_open()) {
        lines.push_back("[Cannot open file / Access denied]");
        return lines;
    }

    std::string line;
    int count = 0;
    while (std::getline(file, line) && count < 500) {
        lines.push_back(line);
        count++;
    }
    if (count >= 500) {
        lines.push_back("... [Truncated after 500 lines] ...");
    }
    return lines;
}

bool save_file_content(const fs::path& file_path, const std::string& content) {
    std::ofstream file(file_path, std::ios::trunc);
    if (!file.is_open()) return false;
    file << content;
    return true;
}

std::vector<std::string> execute_command(const std::string& command, const fs::path& work_dir) {
    std::vector<std::string> output;
    std::string full_cmd = "cd " + work_dir.string() + " && (" + command + ") 2>&1";
    
    std::array<char, 256> buffer;
    auto pipe_closer = [](FILE* f) { if (f) pclose(f); };
    std::unique_ptr<FILE, decltype(pipe_closer)> pipe(popen(full_cmd.c_str(), "r"), pipe_closer);

    if (!pipe) {
        output.push_back("[Failed to execute command]");
        return output;
    }

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        std::string line = buffer.data();
        if (!line.empty() && line.back() == '\n') line.pop_back();
        output.push_back(line);
    }

    if (output.empty()) {
        output.push_back("[Command executed with no output]");
    }

    return output;
}

std::vector<std::string> get_image_info(const fs::path& file_path) {
    std::vector<std::string> info;
    info.push_back("🖼️  IMAGE METADATA");
    info.push_back("--------------------------------");

    std::string cmd = "file \"" + file_path.string() + "\" 2>/dev/null";
    
    std::array<char, 512> buffer;
    auto pipe_closer = [](FILE* f) { if (f) pclose(f); };
    std::unique_ptr<FILE, decltype(pipe_closer)> pipe(popen(cmd.c_str(), "r"), pipe_closer);

    if (pipe && fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        std::string raw_info = buffer.data();
        size_t pos = raw_info.find(":");
        if (pos != std::string::npos && pos + 2 < raw_info.length()) {
            std::string details = raw_info.substr(pos + 2);
            
            std::stringstream ss(details);
            std::string token;
            while (std::getline(ss, token, ',')) {
                if (!token.empty() && token[0] == ' ') token.erase(0, 1);
                info.push_back("• " + token);
            }
        }
    } else {
        info.push_back("[Unable to extract image metadata]");
    }

    info.push_back("");
    info.push_back("👉 Press [ENTER] or [O] to open image");
    return info;
}
