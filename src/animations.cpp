#include "animations.h"

#include <algorithm>
#include <ranges>

using namespace anm2ed::document;
using namespace anm2ed::settings;
using namespace anm2ed::resources;
using namespace anm2ed::types;

namespace anm2ed::animations
{
  void Animations::update(Document& document, int& documentIndex, Settings& settings, Resources& resources)
  {
    auto& anm2 = document.anm2;
    auto& reference = document.reference;
    auto& selection = document.selectedAnimations;
    storage.user_data_set(&selection);

    if (ImGui::Begin("Animations", &settings.windowIsAnimations))
    {
      auto childSize = imgui::size_without_footer_get();

      if (ImGui::BeginChild("##Animations Child", childSize, ImGuiChildFlags_Borders))
      {
        storage.begin(anm2.animations.items.size());

        for (auto [i, animation] : std::views::enumerate(anm2.animations.items))
        {
          ImGui::PushID(i);

          auto isDefault = anm2.animations.defaultAnimation == animation.name;
          auto isReferenced = reference.animationIndex == i;
          auto isSelected = selection.contains(i);

          auto font = isDefault && isReferenced ? font::BOLD_ITALICS
                      : isDefault               ? font::BOLD
                      : isReferenced            ? font::ITALICS
                                                : font::REGULAR;

          ImGui::PushFont(resources.fonts[font].get(), font::SIZE);
          ImGui::SetNextItemSelectionUserData(i);
          if (imgui::selectable_input_text(animation.name,
                                           std::format("###Document #{} Animation #{}", documentIndex, i),
                                           animation.name, isSelected))
            if (!isReferenced) reference = {(int)i};
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
            std::vector<int> sorted = {};
            ImGui::SetDragDropPayload("Animation Drag Drop", sorted.data(), sorted.size() * sizeof(int));
            for (auto& index : sorted)
              ImGui::TextUnformatted(anm2.animations.items[index].name.c_str());
            ImGui::EndDragDropSource();
          }

          if (ImGui::BeginDragDropTarget())
          {
            if (auto payload = ImGui::AcceptDragDropPayload("Animation Drag Drop"))
            {
              auto count = payload->DataSize / sizeof(int);
              auto data = (int*)(payload->Data);
              std::vector<int> indices(data, data + count);
              //std::vector<int> destinationIndices = vector::indices_move(anm2.animations.items, indices, i);

              selection.clear();
              /*
              for (const auto& index : destinationIndices)
                selection.insert((int)index);
                */
            }
            ImGui::EndDragDropTarget();
          }

          ImGui::PopID();
        }

        storage.end();
      }
      ImGui::EndChild();

      auto widgetSize = imgui::widget_size_with_row_get(5);

      imgui::shortcut(settings.shortcutAdd);
      if (ImGui::Button("Add", widgetSize))
      {
        anm2::Animation animation;
        if (anm2::Animation* referenceAnimation = document.animation_get())
        {
          for (auto [id, layerAnimation] : referenceAnimation->layerAnimations)
            animation.layerAnimations[id] = anm2::Item();
          animation.layerOrder = referenceAnimation->layerOrder;
          for (auto [id, nullAnimation] : referenceAnimation->nullAnimations)
            animation.nullAnimations[id] = anm2::Item();
        }
        animation.rootAnimation.frames.emplace_back(anm2::Frame());

        auto index = selection.empty() ? (int)anm2.animations.items.size() - 1 : (int)std::ranges::max(selection) + 1;
        anm2.animations.items.insert(anm2.animations.items.begin() + index, animation);
        selection = {index};
        reference = {index};
        document.change(change::ANIMATIONS);
      }
      imgui::set_item_tooltip_shortcut("Add a new animation.", settings.shortcutAdd);

      ImGui::SameLine();

      ImGui::BeginDisabled(selection.empty());
      {

        imgui::shortcut(settings.shortcutDuplicate);
        if (ImGui::Button("Duplicate", widgetSize))
        {
          auto duplicated = selection;
          auto duplicatedEnd = std::ranges::max(duplicated);
          for (auto& id : duplicated)
          {
            anm2.animations.items.insert(anm2.animations.items.begin() + duplicatedEnd, anm2.animations.items[id]);
            selection.insert(++duplicatedEnd);
            selection.erase(id);
          }
          document.change(change::ANIMATIONS);
        }
        imgui::set_item_tooltip_shortcut("Duplicate the selected animation(s).", settings.shortcutDuplicate);

        ImGui::SameLine();

        ImGui::BeginDisabled(selection.size() != 1);
        {
          if (ImGui::Button("Merge", widgetSize))
          {
            mergePopup.open();
            mergeSelection.clear();
            mergeTarget = *selection.begin();
          }
        }
        ImGui::EndDisabled();

        imgui::set_item_tooltip_shortcut(
            "Open the merge popup.\nUsing the shortcut will merge the animations with\nthe last "
            "configured merge settings.",
            settings.shortcutMerge);

        ImGui::SameLine();

        imgui::shortcut(settings.shortcutRemove);
        if (ImGui::Button("Remove", widgetSize))
        {
          /*
          auto selectionErase = set::to_size_t(selection);
          if (selectionErase.contains(document.reference.animationIndex)) document.reference.animationIndex = -1;
          vector::range_erase(anm2.animations.items, selectionErase);
          */
          document.change(change::ANIMATIONS);
          selection.clear();
        }
        imgui::set_item_tooltip_shortcut("Remove the selected animation(s).", settings.shortcutDuplicate);

        ImGui::SameLine();

        ImGui::BeginDisabled(selection.size() != 1);
        {
          imgui::shortcut(settings.shortcutDefault);
          if (ImGui::Button("Default", widgetSize))
            anm2.animations.defaultAnimation = anm2.animations.items[*selection.begin()].name;
          imgui::set_item_tooltip_shortcut("Set the selected animation as the default.", settings.shortcutDuplicate);
        }
        ImGui::EndDisabled();
      }
      ImGui::EndDisabled();

      mergePopup.trigger();

      if (ImGui::BeginPopupModal(mergePopup.label, &mergePopup.isOpen, ImGuiWindowFlags_NoResize))
      {
        auto merge_close = [&]()
        {
          mergeSelection.clear();
          mergePopup.close();
        };

        auto& type = settings.mergeType;
        auto& isDeleteAnimationsAfter = settings.mergeIsDeleteAnimationsAfter;

        mergeStorage.user_data_set(&mergeSelection);

        auto footerSize = imgui::footer_size_get();
        auto optionsSize = imgui::child_size_get(2);
        auto deleteAfterSize = imgui::child_size_get();
        auto animationsSize =
            ImVec2(0, ImGui::GetContentRegionAvail().y -
                          (optionsSize.y + deleteAfterSize.y + footerSize.y + ImGui::GetStyle().ItemSpacing.y * 3));

        if (ImGui::BeginChild("Animations", animationsSize, ImGuiChildFlags_Borders))
        {
          mergeSelection.begin();

          for (auto [i, animation] : std::views::enumerate(anm2.animations.items))
          {
            auto isSelected = mergeSelection.contains(i);

            ImGui::PushID(i);

            ImGui::SetNextItemSelectionUserData(i);
            ImGui::Selectable(animation.name.c_str(), isSelected);

            ImGui::PopID();
          }

          mergeSelection.end();
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

        auto widgetSize = imgui::widget_size_with_row_get(2);

        if (ImGui::Button("Merge", widgetSize))
        {
          /*
          std::set<int> sources = set::to_set(mergeSelection);
          const auto merged = anm2.animations.merge(mergeTarget, sources, (MergeType)type, isDeleteAnimationsAfter);
          selection = {merged};
          reference = {merged};
          */
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