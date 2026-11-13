#include "types.hpp"
#include "fs_utils.hpp"
#include "ui_components.hpp"

#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/screen/terminal.hpp>
#include <fstream>
#include <algorithm>

int main() {
    fs::path current_path = fs::current_path();
    SortMode current_sort = SortMode::NAME;
    std::string filter_query = "";

    std::vector<FileEntry> all_entries = get_directory_contents(current_path, current_sort);
    std::vector<FileEntry> entries = all_entries;

    int selected = 0;
    int scroll_offset = 0;
    bool full_screen_view = false;
    ModalType current_modal = ModalType::NONE;
    
    ClipboardItem clipboard;
    std::string input_buffer = "";
    std::string status_message = "Ready";
    std::vector<std::string> command_output;

    std::vector<std::string> entry_names;

    auto apply_filter_and_refresh = [&]() {
        entries.clear();
        for (const auto& e : all_entries) {
            if (filter_query.empty() || e.name == ".." || e.name.find(filter_query) != std::string::npos) {
                entries.push_back(e);
            }
        }

        entry_names.clear();
        for (const auto& e : entries) {
            std::string prefix = e.is_dir ? "📁 " : "📄 ";
            if (e.name == "..") prefix = "⬆️  ";
            else if (is_image_file(e.path)) prefix = "🖼️  ";
            entry_names.push_back(prefix + e.name);
        }

        if (selected >= static_cast<int>(entries.size())) {
            selected = std::max(0, static_cast<int>(entries.size()) - 1);
        }
        scroll_offset = 0;
    };

    auto reload_directory = [&]() {
        all_entries = get_directory_contents(current_path, current_sort);
        apply_filter_and_refresh();
    };

    apply_filter_and_refresh();

    MenuOption option;
    option.on_change = [&]() { scroll_offset = 0; };
    option.on_enter = [&]() {
        if (current_modal != ModalType::NONE) return;
        if (selected >= 0 && selected < static_cast<int>(entries.size())) {
           if (entries[selected].is_dir) {
               current_path = fs::canonical(entries[selected].path);
               filter_query = "";
               selected = 0;
               reload_directory();
           } else if (is_image_file(entries[selected].path)) {
               std::string open_cmd = "xdg-open \"" + entries[selected].path.string() + "\" >/dev/null 2>&1 &";
               std::system(open_cmd.c_str());
               status_message = "Opened in default viewer";
           } else {
               full_screen_view = true;
               scroll_offset = 0;
           }
        }
    };

    auto menu = Menu(&entry_names, &selected, option);

    InputOption input_opt;
    auto input_field = Input(&input_buffer, "Type here...", input_opt);

    auto main_container = Container::Vertical({
        menu,
        input_field
    });

    auto open_modal = [&](ModalType type) {
        current_modal = type;
        if (type == ModalType::CREATE || type == ModalType::RENAME || type == ModalType::SHELL_INPUT || type == ModalType::SEARCH_FILTER || type == ModalType::EDITOR) {
            input_field->TakeFocus();
        }
    };

    auto container = CatchEvent(main_container, [&](Event event) {
        if (current_modal != ModalType::NONE) {
            if (event == Event::Escape) {
                current_modal = ModalType::NONE;
                input_buffer = "";
                return true;
            }

            if (event == Event::Return) {
                if (current_modal == ModalType::COMMAND_OUTPUT) {
                    current_modal = ModalType::NONE;
                    return true;
                }

                try {
                    if (current_modal == ModalType::SHELL_INPUT && !input_buffer.empty()) {
                        std::string cmd = input_buffer;
                        input_buffer = "";
                        command_output = execute_command(cmd, current_path);
                        current_modal = ModalType::COMMAND_OUTPUT;
                        status_message = "Executed: " + cmd;
                        reload_directory();
                        return true;
                    }
                    else if (current_modal == ModalType::SEARCH_FILTER) {
                        filter_query = input_buffer;
                        apply_filter_and_refresh();
                        status_message = filter_query.empty() ? "Filter cleared" : "Filtered by: " + filter_query;
                    }
                    else if (selected >= 0 && selected < static_cast<int>(entries.size())) {
                        auto target = entries[selected];
                        if (current_modal == ModalType::EDITOR) {
                            if (save_file_content(target.path, input_buffer)) {
                                status_message = "Saved: " + target.name;
                            } else {
                                status_message = "Failed to save file!";
                            }
                        }
                        else if (current_modal == ModalType::CREATE && !input_buffer.empty()) {
                            fs::path new_p = current_path / input_buffer;
                            if (input_buffer.back() == '/' || input_buffer.find('.') == std::string::npos) {
                                fs::create_directory(new_p);
                            } else {
                                std::ofstream(new_p).close();
                            }
                            status_message = "Created: " + input_buffer;
                        } 
                        else if (current_modal == ModalType::RENAME && !input_buffer.empty()) {
                            fs::rename(target.path, current_path / input_buffer);
                            status_message = "Renamed to: " + input_buffer;
                        } 
                        else if (current_modal == ModalType::DELETE_CONFIRM) {
                            if (target.name != "..") {
                                fs::remove_all(target.path);
                                status_message = "Deleted: " + target.name;
                            }
                        }
                    }
                } catch (const std::exception& e) {
                    status_message = "Error: " + std::string(e.what());
                }

                current_modal = ModalType::NONE;
                input_buffer = "";
                reload_directory();
                return true;
            }
            return false;
        }

        if (full_screen_view) {
            if (event == Event::Escape || event == Event::Backspace) {
                full_screen_view = false;
                return true;
            }
            if (event == Event::ArrowDown || event == Event::Character('j')) {
                scroll_offset++;
                return true;
            }
            if (event == Event::ArrowUp || event == Event::Character('k')) {
                if (scroll_offset > 0) scroll_offset--;
                return true;
            }
            if (event == Event::PageDown) {
                scroll_offset += 15;
                return true;
            }
            if (event == Event::PageUp) {
                scroll_offset = std::max(0, scroll_offset - 15);
                return true;
            }
            return false;
        }

        // Встроенный редактор файлов по клавише 'e'
        if (event == Event::Character('e')) {
            if (selected >= 0 && selected < static_cast<int>(entries.size()) && !entries[selected].is_dir && entries[selected].name != "..") {
                auto lines = read_file_lines(entries[selected].path);
                input_buffer = "";
                for (size_t i = 0; i < lines.size(); ++i) {
                    input_buffer += lines[i] + (i + 1 < lines.size() ? "\n" : "");
                }
                open_modal(ModalType::EDITOR);
                return true;
            }
        }

        if (event == Event::Character('s')) {
            if (current_sort == SortMode::NAME) current_sort = SortMode::SIZE;
            else if (current_sort == SortMode::SIZE) current_sort = SortMode::DATE;
            else current_sort = SortMode::NAME;
            reload_directory();
            return true;
        }

        if (event == Event::Character('/')) {
            input_buffer = filter_query;
            open_modal(ModalType::SEARCH_FILTER);
            return true;
        }

        if (event == Event::Character(':')) {
            input_buffer = "";
            open_modal(ModalType::SHELL_INPUT);
            return true;
        }

        if (event == Event::F5 || event == Event::Character('c')) {
            if (selected >= 0 && entries[selected].name != "..") {
                clipboard = { entries[selected].path, ClipboardAction::COPY };
                status_message = "Copied to buffer: " + entries[selected].name;
                return true;
            }
        }
        if (event == Event::Character('x')) {
            if (selected >= 0 && entries[selected].name != "..") {
                clipboard = { entries[selected].path, ClipboardAction::CUT };
                status_message = "Cut to buffer: " + entries[selected].name;
                return true;
            }
        }
        if (event == Event::Character('v')) {
            if (clipboard.action != ClipboardAction::NONE && fs::exists(clipboard.path)) {
                try {
                    fs::path dest = current_path / clipboard.path.filename();
                    if (clipboard.action == ClipboardAction::COPY) {
                        fs::copy(clipboard.path, dest, fs::copy_options::recursive | fs::copy_options::overwrite_existing);
                        status_message = "Pasted: " + dest.filename().string();
                    } else if (clipboard.action == ClipboardAction::CUT) {
                        fs::rename(clipboard.path, dest);
                        status_message = "Moved: " + dest.filename().string();
                        clipboard = { "", ClipboardAction::NONE };
                    }
                    reload_directory();
                } catch (const std::exception& e) {
                    status_message = "Paste Error: " + std::string(e.what());
                }
                return true;
            }
        }
        if (event == Event::Character('o')) {
            if (selected >= 0 && selected < static_cast<int>(entries.size()) && !entries[selected].is_dir) {
                std::string open_cmd = "xdg-open \"" + entries[selected].path.string() + "\" >/dev/null 2>&1 &";
                std::system(open_cmd.c_str());
                status_message = "Opened file externally";
                return true;
            }
        }
        if (event == Event::F6 || event == Event::Character('r')) {
            if (selected >= 0 && entries[selected].name != "..") {
                input_buffer = entries[selected].name;
                open_modal(ModalType::RENAME);
                return true;
            }
        }
        if (event == Event::F7 || event == Event::Character('n')) {
            input_buffer = "";
            open_modal(ModalType::CREATE);
            return true;
        }
        if (event == Event::F8 || event == Event::Delete || event == Event::Character('d')) {
            if (selected >= 0 && entries[selected].name != "..") {
                open_modal(ModalType::DELETE_CONFIRM);
                return true;
            }
        }

        return false;
    });

    auto component = Renderer(container, [&] {
        auto term_size = Terminal::Size();
        int max_y = term_size.dimx > 0 ? term_size.dimy : 24;

        std::vector<std::string> raw_lines;
        if (selected >= 0 && selected < static_cast<int>(entries.size())) {
            if (!entries[selected].is_dir) {
                if (is_image_file(entries[selected].path)) {
                    raw_lines = get_image_info(entries[selected].path);
                } else {
                    raw_lines = read_file_lines(entries[selected].path);
                }
            }
        }

        if (full_screen_view) {
            Elements visible_lines;
            int view_height = max_y - 6;
            if (view_height < 1) view_height = 1;

            int start = std::min((int)raw_lines.size(), scroll_offset);
            int end = std::min((int)raw_lines.size(), start + view_height);

            for (int i = start; i < end; ++i) {
                visible_lines.push_back(paragraph(raw_lines[i]));
            }

            return vbox({
                text(" FULLSCREEN VIEW | [Esc] - Back | [↑/↓/PgUp/PgDn] - Scroll ") | bold | color(Color::Black) | bgcolor(Color::Green),
                separator(),
                text(" File: " + entries[selected].path.string() + " (Line " + std::to_string(scroll_offset + 1) + "/" + std::to_string(raw_lines.size()) + ")") | color(Color::Yellow),
                separator(),
                vbox(std::move(visible_lines)) | flex
            }) | border;
        }

        Elements preview_elements;
        if (selected >= 0 && selected < static_cast<int>(entries.size())) {
            auto current_entry = entries[selected];
            preview_elements.push_back(text("Path: " + current_entry.path.string()) | color(Color::Yellow));
            preview_elements.push_back(text("Type: " + std::string(current_entry.is_dir ? "Directory" : "File")) | color(Color::Cyan));
            
            if (current_entry.name != "..") {
                preview_elements.push_back(text("Size: " + (current_entry.is_dir ? "<DIR>" : format_size(current_entry.size))) | color(Color::Green));
                preview_elements.push_back(text("Perms: " + format_permissions(current_entry.permissions)) | color(Color::Magenta));
                preview_elements.push_back(text("Modified: " + format_time(current_entry.last_write_time)) | color(Color::GrayLight));
            }

            preview_elements.push_back(separator());

            if (current_entry.is_dir) {
                preview_elements.push_back(text("[ Directory Contents Preview ]") | bold);
            } else {
                preview_elements.push_back(text("[ Press ENTER for Fullscreen ]") | color(Color::GrayDark));
                preview_elements.push_back(separator());

                int preview_height = max_y - 14;
                if (preview_height < 1) preview_height = 1;

                int end = std::min((int)raw_lines.size(), preview_height);
                for (int i = 0; i < end; ++i) {
                    preview_elements.push_back(paragraph(raw_lines[i]));
                }
            }
        }

        int menu_height = max_y - 10;
        if (menu_height < 1) menu_height = 1;

        auto main_layout = vbox({
            hbox({
                vbox({
                    text(" FSOCIETY TERM-SHELL ") | bold | color(Color::Red) | bgcolor(Color::Black),
                    separator(),
                    text(" Path: " + current_path.string()) | color(Color::GrayDark),
                    separator(),
                    menu->Render() | vscroll_indicator | frame | size(HEIGHT, LESS_THAN, menu_height) | flex
                }) | border | flex_grow,

                vbox({
                    text(" PREVIEW & METADATA ") | bold | color(Color::Green),
                    separator(),
                    vbox(std::move(preview_elements)) | flex
                }) | border | flex_grow
            }) | flex,

            render_status_bar(clipboard, status_message, current_sort)
        });

        if (current_modal != ModalType::NONE) {
            std::string target_name = (selected >= 0 && selected < static_cast<int>(entries.size())) ? entries[selected].name : "";
            return dbox({
                main_layout,
                render_modal(current_modal, input_field, target_name, command_output) | clear_under | center
            });
        }

        return main_layout;
    });

    auto screen = ScreenInteractive::TerminalOutput();
    screen.Loop(component);

    return 0;
}
