#ifndef UI_COMPONENTS_HPP
#define UI_COMPONENTS_HPP

#include "types.hpp"
#include <ftxui/dom/elements.hpp>
#include <ftxui/component/component.hpp>
#include <vector>

using namespace ftxui;

Element render_status_bar(const ClipboardItem& clipboard, const std::string& status_message, SortMode sort_mode);
Element render_modal(ModalType current_modal, Component input_field, const std::string& target_name, const std::vector<std::string>& cmd_output);

#endif
