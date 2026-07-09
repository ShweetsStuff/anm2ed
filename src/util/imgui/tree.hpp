#pragma once

#include <string>

#include <imgui/imgui.h>

#include "selectable.hpp"

namespace anm2ed::imgui
{
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
}
