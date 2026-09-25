#ifndef OBLIVION_MENU_NODE_HPP
#define OBLIVION_MENU_NODE_HPP

#include <vector>

#include "src/common/session.hpp"

class MenuNode {
public:
    MenuNode *parent = nullptr;
    bool is_selected = false;
    bool is_enabled = true;
    std::string label;
    std::vector<MenuNode *> children;
    SharedArray<std::optional<std::string>> detail_lines;
    std::optional<std::string> selection_hint;

    MenuNode(const std::string &label, const std::optional<std::string> &selection_hint, bool is_selected);
};

#endif
