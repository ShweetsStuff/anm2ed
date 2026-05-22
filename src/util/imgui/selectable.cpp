#include "selectable.hpp"

#include <cfloat>

#include "input.hpp"

namespace anm2ed::imgui
{
  bool isSelectableInputTextRenaming = false;

  std::string& selectable_input_text_id()
  {
    static std::string editID{};
    return editID;
  }

  bool selectable_input_text(const std::string& label, const std::string& id, std::string& text, bool isSelected,
                             ImGuiSelectableFlags flags, RenameState& state)
  {
    auto& editID = selectable_input_text_id();
    auto isRename = editID == id;
    bool isActivated{};

    if (isRename)
    {
      auto finish = [&]()
      {
        editID.clear();
        isActivated = true;
        state = RENAME_FINISHED;
        isSelectableInputTextRenaming = false;
      };

      if (state == RENAME_BEGIN)
      {
        ImGui::SetKeyboardFocusHere();
        state = RENAME_EDITING;
      }

      ImGui::SetNextItemWidth(-FLT_MIN);
      if (input_text_string("##Edit", &text, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
        finish();
      if (ImGui::IsItemDeactivatedAfterEdit() || ImGui::IsKeyPressed(ImGuiKey_Escape)) finish();
    }
    else
    {
      if (ImGui::Selectable(label.c_str(), isSelected, flags)) isActivated = true;

      if (state == RENAME_FORCE_EDIT || (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)))
      {
        state = RENAME_BEGIN;
        editID = id;
        isActivated = true;
        isSelectableInputTextRenaming = true;
      }
    }

    return isActivated;
  }
}
