#include "src/limbo/menu_node.hpp"

MenuNode::MenuNode(const std::string &label, const std::optional<std::string> &selection_hint, bool is_selected) {
    this->label = label;
    this->selection_hint = selection_hint;
    this->is_selected = is_selected;
}
