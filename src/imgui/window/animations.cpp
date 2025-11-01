#include "animations.h"

#include <ranges>

using namespace anm2ed::resource;
using namespace anm2ed::types;

namespace anm2ed::imgui
{
  void Animations::update(Manager& manager, Settings& settings, Resources& resources, Clipboard& clipboard)
  {
    auto& document = *manager.get();
    auto& anm2 = document.anm2;
    auto& reference = document.reference;
    auto& hovered = document.hoveredAnimation;
    auto& multiSelect = document.animationMultiSelect;
    auto& mergeMultiSelect = document.animationMergeMultiSelect;
    auto& mergeTarget = document.mergeTarget;

    hovered = -1;

    if (ImGui::Begin("Animations", &settings.windowIsAnimations))
    {
      auto childSize = size_without_footer_get();

      if (ImGui::BeginChild("##Animations Child", childSize, ImGuiChildFlags_Borders))
      {
        multiSelect.start(anm2.animations.items.size());

        for (auto [i, animation] : std::views::enumerate(anm2.animations.items))
        {
          ImGui::PushID(i);

          auto isDefault = anm2.animations.defaultAnimation == animation.name;
          auto isReferenced = reference.animationIndex == i;

          auto font = isDefault && isReferenced ? font::BOLD_ITALICS
                      : isDefault               ? font::BOLD
                      : isReferenced            ? font::ITALICS
                                                : font::REGULAR;

          ImGui::PushFont(resources.fonts[font].get(), font::SIZE);
          ImGui::SetNextItemSelectionUserData(i);
          if (selectable_input_text(animation.name, std::format("###Document #{} Animation #{}", manager.selected, i),
                                    animation.name, multiSelect.contains(i)))
            document.animation_set(i);
          if (ImGui::IsItemHovered()) hovered = i;
          ImGui::PopFont();

          if (ImGui::BeginItemTooltip())
          {
            ImGui::PushFont(resources.fonts[font::BOLD].get(), font::SIZE);
            ImGui::TextUnformatted(animation.name.c_str());
            ImGui::PopFont();

            if (isDefault)
            {

              ImGui::PushFont(resources.fonts[font::ITALICS].get(), font::SIZE);
              ImGui::TextUnformatted("(Default Animation)");
              ImGui::PopFont();
            }

            ImGui::Text("Length: %d", animation.frameNum);
            ImGui::Text("Loop: %s", animation.isLoop ? "true" : "false");

            ImGui::EndTooltip();
          }

          if (ImGui::BeginDragDropSource())
          {
            static std::vector<int> selection;
            selection.assign(multiSelect.begin(), multiSelect.end());
            ImGui::SetDragDropPayload("Animation Drag Drop", selection.data(), selection.size() * sizeof(int));
            for (auto& i : selection)
              ImGui::TextUnformatted(anm2.animations.items[i].name.c_str());
            ImGui::EndDragDropSource();
          }

          if (ImGui::BeginDragDropTarget())
          {
            if (auto payload = ImGui::AcceptDragDropPayload("Animation Drag Drop"))
            {
              auto payloadIndices = (int*)(payload->Data);
              auto payloadCount = payload->DataSize / sizeof(int);
              std::vector<int> indices(payloadIndices, payloadIndices + payloadCount);
              std::sort(indices.begin(), indices.end());
              document.animations_move(indices, i);
            }
            ImGui::EndDragDropTarget();
          }

          ImGui::PopID();
        }

        multiSelect.finish();

        auto copy = [&]()
        {
          if (!multiSelect.empty())
          {
            std::string clipboardText{};
            for (auto& i : multiSelect)
              clipboardText += anm2.animations.items[i].to_string();
            clipboard.set(clipboardText);
          }
          else if (hovered > -1)
            clipboard.set(anm2.animations.items[hovered].to_string());
        };

        auto cut = [&]()
        {
          copy();
          document.animations_remove();
        };

        auto paste = [&]()
        {
          auto clipboardText = clipboard.get();
          document.animations_deserialize(clipboardText);
        };

        if (shortcut(settings.shortcutCut, shortcut::FOCUSED)) cut();
        if (shortcut(settings.shortcutCopy, shortcut::FOCUSED)) copy();
        if (shortcut(settings.shortcutPaste, shortcut::FOCUSED)) paste();

        if (ImGui::BeginPopupContextWindow("##Context Menu", ImGuiPopupFlags_MouseButtonRight))
        {
          ImGui::BeginDisabled(multiSelect.empty() && hovered == -1);
          if (ImGui::MenuItem("Cut", settings.shortcutCut.c_str())) cut();
          if (ImGui::MenuItem("Copy", settings.shortcutCopy.c_str())) copy();
          ImGui::EndDisabled();

          ImGui::BeginDisabled(clipboard.is_empty());
          if (ImGui::MenuItem("Paste", settings.shortcutPaste.c_str())) paste();
          ImGui::EndDisabled();

          ImGui::EndPopup();
        }
      }
      ImGui::EndChild();

      auto widgetSize = widget_size_with_row_get(5);

      shortcut(settings.shortcutAdd);
      if (ImGui::Button("Add", widgetSize)) document.animation_add();
      set_item_tooltip_shortcut("Add a new animation.", settings.shortcutAdd);

      ImGui::SameLine();

      ImGui::BeginDisabled(multiSelect.empty());
      {
        shortcut(settings.shortcutDuplicate);
        if (ImGui::Button("Duplicate", widgetSize)) document.animation_duplicate();
        set_item_tooltip_shortcut("Duplicate the selected animation(s).", settings.shortcutDuplicate);

        ImGui::SameLine();

        if (shortcut(settings.shortcutMerge, shortcut::FOCUSED))
          if (multiSelect.size() > 0) document.animations_merge_quick();

        ImGui::BeginDisabled(multiSelect.size() != 1);
        {
          if (ImGui::Button("Merge", widgetSize))
          {
            mergePopup.open();
            mergeMultiSelect.clear();
            mergeTarget = *multiSelect.begin();
          }
        }
        ImGui::EndDisabled();

        set_item_tooltip_shortcut("Open the merge popup.\nUsing the shortcut will merge the animations with\nthe last "
                                  "configured merge settings.",
                                  settings.shortcutMerge);

        ImGui::SameLine();

        shortcut(settings.shortcutRemove);
        if (ImGui::Button("Remove", widgetSize)) document.animations_remove();
        set_item_tooltip_shortcut("Remove the selected animation(s).", settings.shortcutDuplicate);

        ImGui::SameLine();

        shortcut(settings.shortcutDefault);
        ImGui::BeginDisabled(multiSelect.size() != 1);
        if (ImGui::Button("Default", widgetSize)) document.animation_default();
        ImGui::EndDisabled();
        set_item_tooltip_shortcut("Set the selected animation as the default.", settings.shortcutDefault);
      }
      ImGui::EndDisabled();

      mergePopup.trigger();

      if (ImGui::BeginPopupModal(mergePopup.label, &mergePopup.isOpen, ImGuiWindowFlags_NoResize))
      {
        auto merge_close = [&]()
        {
          mergeMultiSelect.clear();
          mergePopup.close();
        };

        auto& type = settings.mergeType;
        auto& isDeleteAnimationsAfter = settings.mergeIsDeleteAnimationsAfter;

        auto footerSize = footer_size_get();
        auto optionsSize = child_size_get(2);
        auto deleteAfterSize = child_size_get();
        auto animationsSize =
            ImVec2(0, ImGui::GetContentRegionAvail().y -
                          (optionsSize.y + deleteAfterSize.y + footerSize.y + ImGui::GetStyle().ItemSpacing.y * 3));

        if (ImGui::BeginChild("Animations", animationsSize, ImGuiChildFlags_Borders))
        {
          mergeMultiSelect.start(anm2.animations.items.size());

          for (auto [i, animation] : std::views::enumerate(anm2.animations.items))
          {
            ImGui::PushID(i);

            ImGui::SetNextItemSelectionUserData(i);
            ImGui::Selectable(animation.name.c_str(), mergeMultiSelect.contains(i));

            ImGui::PopID();
          }

          mergeMultiSelect.finish();
        }
        ImGui::EndChild();

        if (ImGui::BeginChild("Merge Options", optionsSize, ImGuiChildFlags_Borders))
        {
          auto size = ImVec2(optionsSize.x * 0.5f, optionsSize.y - ImGui::GetStyle().WindowPadding.y * 2);

          if (ImGui::BeginChild("Merge Options 1", size))
          {
            ImGui::RadioButton("Append Frames", &type, merge::APPEND);
            ImGui::RadioButton("Prepend Frames", &type, merge::PREPEND);
          }
          ImGui::EndChild();

          ImGui::SameLine();

          if (ImGui::BeginChild("Merge Options 2", size))
          {
            ImGui::RadioButton("Replace Frames", &type, merge::REPLACE);
            ImGui::RadioButton("Ignore Frames", &type, merge::IGNORE);
          }
          ImGui::EndChild();
        }
        ImGui::EndChild();

        if (ImGui::BeginChild("Merge Delete After", deleteAfterSize, ImGuiChildFlags_Borders))
          ImGui::Checkbox("Delete Animations After", &isDeleteAnimationsAfter);
        ImGui::EndChild();

        auto widgetSize = widget_size_with_row_get(2);

        if (ImGui::Button("Merge", widgetSize))
        {
          document.animations_merge((merge::Type)type, isDeleteAnimationsAfter);
          merge_close();
        }
        ImGui::SameLine();
        if (ImGui::Button("Close", widgetSize)) merge_close();

        ImGui::EndPopup();
      }
    }
    ImGui::End();
  }
}