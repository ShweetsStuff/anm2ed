#pragma once

#include <string>

#include <imgui/imgui.h>

#include "ui/selectable.h"

  struct TreeNodeInputTextResult
  {
    bool isOpen = false;
    bool isActivated = false;
    bool isClicked = false;
    bool isRenameStarted = false;
    bool isRenameFinished = false;
  };

  TreeNodeInputTextResult tree_node_input_text(const std::string&, const std::string&, std::string&, bool,
                                               ImGuiTreeNodeFlags, RenameState&);
