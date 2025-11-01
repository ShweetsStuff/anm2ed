#include "nulls.h"

#include <ranges>

using namespace anm2ed::resource;
using namespace anm2ed::types;

namespace anm2ed::imgui
{
  void Nulls::update(Manager& manager, Settings& settings, Resources& resources, Clipboard& clipboard)
  {
    auto& document = *manager.get();
    auto& anm2 = document.anm2;
    auto& reference = document.referenceNull;
    auto& unused = document.unusedNullIDs;
    auto& hovered = document.hoveredNull;
    auto& multiSelect = document.nullMultiSelect;
    auto& propertiesPopup = manager.nullPropertiesPopup;

    hovered = -1;

    if (ImGui::Begin("Nulls", &settings.windowIsNulls))
    {
      auto childSize = size_without_footer_get();

      if (ImGui::BeginChild("##Nulls Child", childSize, true))
      {
        multiSelect.start(anm2.content.nulls.size());

        for (auto& [id, null] : anm2.content.nulls)
        {
          auto isSelected = multiSelect.contains(id);
          auto isReferenced = reference == id;

          ImGui::PushID(id);
          ImGui::SetNextItemSelectionUserData(id);
          if (isReferenced) ImGui::PushFont(resources.fonts[font::ITALICS].get(), font::SIZE);
          ImGui::Selectable(std::format(anm2::NULL_FORMAT, id, null.name).c_str(), isSelected);
          if (ImGui::IsItemHovered())
          {
            hovered = id;
            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) manager.null_properties_open(id);
          }

          if (isReferenced) ImGui::PopFont();

          if (ImGui::BeginItemTooltip())
          {
            ImGui::PushFont(resources.fonts[font::BOLD].get(), font::SIZE);
            ImGui::TextUnformatted(null.name.c_str());
            ImGui::PopFont();
            ImGui::Text("ID: %d", id);
            ImGui::EndTooltip();
          }
          ImGui::PopID();
        }

        multiSelect.finish();

        auto copy = [&]()
        {
          if (!multiSelect.empty())
          {
            std::string clipboardText{};
            for (auto& id : multiSelect)
              clipboardText += anm2.content.nulls[id].to_string(id);
            clipboard.set(clipboardText);
          }
          else if (hovered > -1)
            clipboard.set(anm2.content.nulls[hovered].to_string(hovered));
        };

        auto paste = [&](merge::Type type)
        {
          auto clipboardText = clipboard.get();
          document.nulls_deserialize(clipboardText, type);
        };

        if (shortcut(settings.shortcutCopy, shortcut::FOCUSED)) copy();
        if (shortcut(settings.shortcutPaste, shortcut::FOCUSED)) paste(merge::APPEND);

        if (ImGui::BeginPopupContextWindow("##Context Menu", ImGuiPopupFlags_MouseButtonRight))
        {
          ImGui::BeginDisabled();
          ImGui::MenuItem("Cut", settings.shortcutCut.c_str());
          ImGui::EndDisabled();

          ImGui::BeginDisabled(multiSelect.empty() && hovered == -1);
          if (ImGui::MenuItem("Copy", settings.shortcutCopy.c_str())) copy();
          ImGui::EndDisabled();

          ImGui::BeginDisabled(clipboard.is_empty());
          {
            if (ImGui::BeginMenu("Paste"))
            {
              if (ImGui::MenuItem("Append", settings.shortcutPaste.c_str())) paste(merge::APPEND);
              if (ImGui::MenuItem("Replace")) paste(merge::REPLACE);

              ImGui::EndMenu();
            }
          }
          ImGui::EndDisabled();

          ImGui::EndPopup();
        }
      }
      ImGui::EndChild();

      auto widgetSize = widget_size_with_row_get(2);

      shortcut(settings.shortcutAdd);
      if (ImGui::Button("Add", widgetSize)) manager.null_properties_open();
      set_item_tooltip_shortcut("Add a null.", settings.shortcutAdd);
      ImGui::SameLine();

      shortcut(settings.shortcutRemove);
      ImGui::BeginDisabled(unused.empty());
      if (ImGui::Button("Remove Unused", widgetSize)) document.nulls_remove_unused();
      ImGui::EndDisabled();
      set_item_tooltip_shortcut("Remove unused nulls (i.e., ones not used in any animation.)", settings.shortcutRemove);
    }
    ImGui::End();

    manager.null_properties_trigger();

    if (ImGui::BeginPopupModal(propertiesPopup.label, &propertiesPopup.isOpen, ImGuiWindowFlags_NoResize))
    {
      auto childSize = child_size_get(2);
      auto& null = manager.editNull;

      if (ImGui::BeginChild("Child", childSize, ImGuiChildFlags_Borders))
      {
        if (propertiesPopup.isJustOpened) ImGui::SetKeyboardFocusHere();
        input_text_string("Name", &null.name);
        ImGui::SetItemTooltip("Set the null's name.");

        ImGui::Checkbox("Rect", &null.isShowRect);
        ImGui::SetItemTooltip("The null will have a rectangle show around it.");
      }
      ImGui::EndChild();

      auto widgetSize = widget_size_with_row_get(2);

      if (ImGui::Button(reference == -1 ? "Add" : "Confirm", widgetSize))
      {
        document.null_set(null);
        manager.null_properties_close();
      }

      ImGui::SameLine();

      if (ImGui::Button("Cancel", widgetSize)) manager.null_properties_close();

      ImGui::EndPopup();
    }

    manager.null_properties_end();
  }
}