#include "ui_components.hpp"

Element render_status_bar(const ClipboardItem& clipboard, const std::string& status_message, SortMode sort_mode) {
    std::string clip_info = clipboard.action == ClipboardAction::NONE ? "Empty" : 
                           (clipboard.action == ClipboardAction::COPY ? "[Copy] " : "[Cut] ") + clipboard.path.filename().string();

    std::string sort_str = "Name";
    if (sort_mode == SortMode::SIZE) sort_str = "Size";
    else if (sort_mode == SortMode::DATE) sort_str = "Date";

    return hbox({
        text(" C ") | bold | bgcolor(Color::Blue) | color(Color::White),
        text(" Copy "),
        text(" X ") | bold | bgcolor(Color::Magenta) | color(Color::White),
        text(" Cut "),
        text(" V ") | bold | bgcolor(Color::DarkGreen) | color(Color::White),
        text(" Paste "),
        text(" E ") | bold | bgcolor(Color::Yellow) | color(Color::Black),
        text(" Edit "),
        text(" R ") | bold | bgcolor(Color::Cyan) | color(Color::Black),
        text(" Rename "),
        text(" N ") | bold | bgcolor(Color::Green) | color(Color::Black),
        text(" New "),
        text(" D ") | bold | bgcolor(Color::Red) | color(Color::White),
        text(" Del "),
        text(" : ") | bold | bgcolor(Color::Yellow) | color(Color::Black),
        text(" Exec "),
        text(" S ") | bold | bgcolor(Color::White) | color(Color::Black),
        text(" Sort "),
        text(" / ") | bold | bgcolor(Color::BlueLight) | color(Color::Black),
        text(" Filter "),
        text(" | Sort: " + sort_str) | color(Color::Cyan),
        text(" | Buf: " + clip_info) | color(Color::Yellow),
        text(" | " + status_message) | color(Color::GrayLight)
    });
}

Element render_modal(ModalType current_modal, Component input_field, const std::string& target_name, const std::vector<std::string>& cmd_output) {
    std::string modal_title = "";
    if (current_modal == ModalType::CREATE) modal_title = " Create New (Folder or File) ";
    else if (current_modal == ModalType::RENAME) modal_title = " Rename Target ";
    else if (current_modal == ModalType::DELETE_CONFIRM) modal_title = " Confirm Delete ";
    else if (current_modal == ModalType::SHELL_INPUT) modal_title = " Run Shell Command ";
    else if (current_modal == ModalType::COMMAND_OUTPUT) modal_title = " Command Output ";
    else if (current_modal == ModalType::SEARCH_FILTER) modal_title = " Filter Entries ";
    else if (current_modal == ModalType::EDITOR) modal_title = " Edit File: " + target_name;

    Element modal_content;
    if (current_modal == ModalType::DELETE_CONFIRM) {
        modal_content = vbox({
            text("Are you sure you want to delete:") | color(Color::Yellow),
            text(target_name) | bold | color(Color::Red),
            separator(),
            text("[Enter] - Confirm  |  [Esc] - Cancel") | color(Color::GrayDark)
        });
    } else if (current_modal == ModalType::COMMAND_OUTPUT) {
        Elements lines;
        for (const auto& l : cmd_output) {
            lines.push_back(paragraph(l));
        }
        modal_content = vbox({
            vbox(std::move(lines)) | vscroll_indicator | frame | size(HEIGHT, LESS_THAN, 15),
            separator(),
            text("[Esc / Enter] - Close") | color(Color::GrayDark)
        });
    } else if (current_modal == ModalType::EDITOR) {
        modal_content = vbox({
            text("Edit contents:") | color(Color::GrayLight),
            input_field->Render() | border | size(HEIGHT, LESS_THAN, 12),
            separator(),
            text("[Enter] - Save Changes  |  [Esc] - Cancel") | color(Color::GrayDark)
        });
    } else {
        std::string label = "Enter target name:";
        if (current_modal == ModalType::SHELL_INPUT) label = "Enter shell command:";
        else if (current_modal == ModalType::SEARCH_FILTER) label = "Filter by name pattern:";

        modal_content = vbox({
            text(label) | color(Color::GrayLight),
            input_field->Render() | border,
            separator(),
            text("[Enter] - Apply  |  [Esc] - Cancel") | color(Color::GrayDark)
        });
    }

    return vbox({
        text(modal_title) | bold | color(Color::White) | bgcolor(Color::Red),
        separator(),
        modal_content
    }) | border | bgcolor(Color::Black);
}
