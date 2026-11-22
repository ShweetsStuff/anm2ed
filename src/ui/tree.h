#pragma once

#include <string>

#include <imgui/imgui.h>

#include "ui/selectable.h"

  struct TreeNodeInputTextResult
  {
    bool isOpen{};
    bool isActivated{};
    bool isClicked{};
    bool isRenameStarted{};
    bool isRenameFinished{};
  };

  TreeNodeInputTextResult tree_node_input_text(const std::string&, const std::string&, std::string&, bool,
                                               ImGuiTreeNodeFlags, RenameState&);
