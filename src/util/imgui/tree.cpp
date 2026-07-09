#include "tree.hpp"

namespace anm2ed::imgui
{
  TreeNodeInputTextResult tree_node_input_text(const std::string& label, const std::string& id, std::string& text,
                                               bool isSelected, ImGuiTreeNodeFlags flags, RenameState& state)
  {
    TreeNodeInputTextResult result{};
    auto& editID = selectable_input_text_id();
    auto isRename = editID == id;
    auto treeFlags = flags | ImGuiTreeNodeFlags_FramePadding | (isSelected ? ImGuiTreeNodeFlags_Selected : 0);

    if (!isRename)
    {
      result.isOpen = ImGui::TreeNodeEx(id.c_str(), treeFlags, "%s", label.c_str());
      result.isClicked =
          ImGui::IsItemClicked(ImGuiMouseButton_Left) || ImGui::IsItemClicked(ImGuiMouseButton_Right);
      result.isActivated = result.isClicked;

      if (state == RENAME_FORCE_EDIT || (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)))
      {
        state = RENAME_BEGIN;
        editID = id;
        result.isActivated = true;
        result.isRenameStarted = true;
      }

      return result;
    }

    result.isOpen = ImGui::TreeNodeEx(id.c_str(), treeFlags, "%s", "");
    auto itemMin = ImGui::GetItemRectMin();
    auto itemMax = ImGui::GetItemRectMax();
    auto labelX = itemMin.x + ImGui::GetTreeNodeToLabelSpacing();
    ImGui::SetCursorScreenPos(ImVec2(labelX, itemMin.y));
    auto width = itemMax.x > labelX ? itemMax.x - labelX : 1.0f;
    result.isActivated = input_text_rename_update(text, state, width);
    result.isRenameFinished = state == RENAME_FINISHED;
    return result;
  }
}
