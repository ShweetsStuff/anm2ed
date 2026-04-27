#include "widgets.hpp"

namespace anm2ed::util::imgui
{
  int input_text_callback(ImGuiInputTextCallbackData* data)
  {
    if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
    {
      auto* string = static_cast<std::string*>(data->UserData);
      string->resize(data->BufTextLen);
      data->Buf = string->data();
    }
    return 0;
  }

  bool input_text_string(const char* label, std::string* string, ImGuiInputTextFlags flags = 0)
  {
    if (!string) return false;

    flags |= ImGuiInputTextFlags_CallbackResize;
    return ImGui::InputText(label, string->data(), string->capacity() + 1, flags, input_text_callback, string);
  }

  std::string& selectable_input_text_edit_id()
  {
    static std::string editId{};
    return editId;
  }

  bool selectable_input_text(const std::string& displayText, const std::string& id, std::string& buffer,
                             bool isSelected, ImGuiSelectableFlags flags, RenameState& state)
  {
    auto& editId = selectable_input_text_edit_id();
    auto isRename = editId == id;
    bool isActivated{};

    if (isRename)
    {
      auto finish = [&]()
      {
        editId.clear();
        isActivated = true;
        state = RENAME_FINISHED;
      };

      if (state == RENAME_BEGIN)
      {
        ImGui::SetKeyboardFocusHere();
        state = RENAME_EDITING;
      }

      ImGui::SetNextItemWidth(-FLT_MIN);
      if (input_text_string("##Edit", &buffer,
                            ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
        finish();
      if (ImGui::IsItemDeactivatedAfterEdit() || ImGui::IsKeyPressed(ImGuiKey_Escape)) finish();
    }
    else
    {
      if (ImGui::Selectable(displayText.c_str(), isSelected, flags)) isActivated = true;

      if (state == RENAME_FORCE_EDIT || (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)))
      {
        state = RENAME_BEGIN;
        editId = id;
        isActivated = true;
      }
    }

    return isActivated;
  }
}
