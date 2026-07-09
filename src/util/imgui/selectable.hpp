#pragma once

#include <cfloat>
#include <string>

#include <imgui/imgui.h>

namespace anm2ed::imgui
{
  enum RenameState
  {
    RENAME_SELECTABLE,
    RENAME_BEGIN,
    RENAME_EDITING,
    RENAME_FINISHED,
    RENAME_FORCE_EDIT
  };

  std::string& selectable_input_text_id();
  bool input_text_rename_update(std::string&, RenameState&, float = -FLT_MIN);
  bool selectable_input_text(const std::string&, const std::string&, std::string&, bool, ImGuiSelectableFlags,
                             RenameState&);
}
