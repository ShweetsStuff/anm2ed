#include "animations.h"

#include <cstddef>
#include <ranges>

#include "toast.h"
#include "vector_.h"

using namespace anm2ed::util;
using namespace anm2ed::resource;
using namespace anm2ed::types;

namespace anm2ed::imgui
{
  void Animations::update(Manager& manager, Settings& settings, Resources& resources, Clipboard& clipboard)
  {
    auto& document = *manager.get();
    auto& anm2 = document.anm2;
    auto& reference = document.reference;
    auto& hovered = document.animation.hovered;
    auto& selection = document.animation.selection;
    auto& mergeSelection = document.merge.selection;
    auto& mergeReference = document.merge.reference;
    auto& overlayIndex = document.overlayIndex;

    hovered = -1;

    auto animations_remove = [&]()
    {
      if (!selection.empty())
      {
        for (auto it = selection.rbegin(); it != selection.rend(); ++it)
        {
          auto i = *it;
          if (overlayIndex == i) overlayIndex = -1;
          if (reference.animationIndex == i) reference.animationIndex = -1;
          anm2.animations.items.erase(anm2.animations.items.begin() + i);
        }
        selection.clear();
      }
    };

    if (ImGui::Begin("Animations", &settings.windowIsAnimations))
    {
      auto childSize = size_without_footer_get();

      if (ImGui::BeginChild("##Animations Child", childSize, ImGuiChildFlags_Borders))
      {
        selection.start(anm2.animations.items.size());

        for (auto [i, animation] : std::views::enumerate(anm2.animations.items))
        {
          ImGui::PushID((int)i);

          auto isDefault = anm2.animations.defaultAnimation == animation.name;
          auto isReferenced = reference.animationIndex == (int)i;
          auto isNewAnimation = newAnimationSelectedIndex == (int)i;

          auto font = isDefault && isReferenced ? font::BOLD_ITALICS
                      : isDefault               ? font::BOLD
                      : isReferenced            ? font::ITALICS
                                                : font::REGULAR;

          ImGui::PushFont(resources.fonts[font].get(), font::SIZE);
          ImGui::SetNextItemSelectionUserData((int)i);

          if (isNewAnimation) renameState = RENAME_FORCE_EDIT;
          if (selectable_input_text(animation.name, std::format("###Document #{} Animation #{}", manager.selected, i),
                                    animation.name, selection.contains((int)i), ImGuiSelectableFlags_None, renameState))
          {
            reference = {(int)i};
            document.frames.clear();

            if (renameState == RENAME_BEGIN)
              document.snapshot("Rename Animation");
            else if (renameState == RENAME_FINISHED)
              document.change(Document::ANIMATIONS);
          }
          if (ImGui::IsItemHovered()) hovered = (int)i;

          if (isNewAnimation)
          {
            ImGui::SetScrollHereY(0.5f);
            newAnimationSelectedIndex = -1;
          }
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
            static std::vector<int> dragDropSelection{};
            dragDropSelection.assign(selection.begin(), selection.end());
            ImGui::SetDragDropPayload("Animation Drag Drop", dragDropSelection.data(),
                                      dragDropSelection.size() * sizeof(int));
            for (auto& i : dragDropSelection)
              ImGui::Text("%s", anm2.animations.items[(int)i].name.c_str());
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
              DOCUMENT_EDIT(document, "Move Animation(s)", Document::ANIMATIONS,
                            selection = vector::move_indices(anm2.animations.items, indices, i));
            }
            ImGui::EndDragDropTarget();
          }

          ImGui::PopID();
        }

        selection.finish();

        auto copy = [&]()
        {
          if (!selection.empty())
          {
            std::string clipboardText{};
            for (auto& i : selection)
              clipboardText += anm2.animations.items[i].to_string();
            clipboard.set(clipboardText);
          }
          else if (hovered > -1)
            clipboard.set(anm2.animations.items[hovered].to_string());
        };

        auto cut = [&]()
        {
          copy();
          DOCUMENT_EDIT(document, "Cut Animation(s)", Document::ANIMATIONS, animations_remove());
        };

        auto paste = [&]()
        {
          auto clipboardText = clipboard.get();

          auto deserialize = [&]()
          {
            auto start = selection.empty() ? anm2.animations.items.size() : *selection.rbegin() + 1;
            std::set<int> indices{};
            std::string errorString{};
            if (anm2.animations_deserialize(clipboardText, start, indices, &errorString))
              selection = indices;
            else
              toasts.error(std::format("Failed to deserialize animation(s): {}", errorString));
          };

          DOCUMENT_EDIT(document, "Paste Animation(s)", Document::ANIMATIONS, deserialize());
        };

        if (shortcut(manager.chords[SHORTCUT_CUT], shortcut::FOCUSED)) cut();
        if (shortcut(manager.chords[SHORTCUT_COPY], shortcut::FOCUSED)) copy();
        if (shortcut(manager.chords[SHORTCUT_PASTE], shortcut::FOCUSED)) paste();

        if (ImGui::BeginPopupContextWindow("##Context Menu", ImGuiPopupFlags_MouseButtonRight))
        {
          if (ImGui::MenuItem("Cut", settings.shortcutCut.c_str(), false, !selection.empty() || hovered > -1)) cut();
          if (ImGui::MenuItem("Copy", settings.shortcutCopy.c_str(), false, !selection.empty() || hovered > -1)) copy();
          if (ImGui::MenuItem("Paste", settings.shortcutPaste.c_str(), false, !clipboard.is_empty())) paste();
          ImGui::EndPopup();
        }
      }
      ImGui::EndChild();

      auto widgetSize = widget_size_with_row_get(5);

      shortcut(manager.chords[SHORTCUT_ADD]);
      if (ImGui::Button("Add", widgetSize))
      {
        auto add = [&]()
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

          auto index = (int)anm2.animations.items.size();
          if (!selection.empty())
          {
            index = *selection.rbegin() + 1;
            index = std::min(index, (int)anm2.animations.items.size());
          }

          anm2.animations.items.insert(anm2.animations.items.begin() + index, animation);
          selection = {index};
          reference = {index};
          newAnimationSelectedIndex = index;
        };

        DOCUMENT_EDIT(document, "Add Animation", Document::ANIMATIONS, add());
      }
      set_item_tooltip_shortcut("Add a new animation.", settings.shortcutAdd);

      ImGui::SameLine();

      ImGui::BeginDisabled(selection.empty());
      {
        shortcut(manager.chords[SHORTCUT_DUPLICATE]);
        if (ImGui::Button("Duplicate", widgetSize))
        {
          auto duplicate = [&]()
          {
            auto duplicated = selection;
            auto end = std::ranges::max(duplicated);
            for (auto& id : duplicated)
            {
              anm2.animations.items.insert(anm2.animations.items.begin() + end, anm2.animations.items[id]);
              selection.insert(++end);
              selection.erase(id);
            }
          };

          DOCUMENT_EDIT(document, "Duplicate Animation(s)", Document::ANIMATIONS, duplicate());
        }
        set_item_tooltip_shortcut("Duplicate the selected animation(s).", settings.shortcutDuplicate);

        ImGui::SameLine();

        if (shortcut(manager.chords[SHORTCUT_MERGE], shortcut::FOCUSED) && !selection.empty())
        {
          auto merge_quick = [&]()
          {
            int merged{};
            if (selection.contains(overlayIndex)) overlayIndex = -1;

            if (selection.size() > 1)
              merged = anm2.animations_merge(*selection.begin(), selection);
            else if (selection.size() == 1 && *selection.begin() != (int)anm2.animations.items.size() - 1)
            {
              auto start = *selection.begin();
              auto next = *selection.begin() + 1;
              std::set<int> animationSet{};
              animationSet.insert(start);
              animationSet.insert(next);

              merged = anm2.animations_merge(start, animationSet);
            }
            else
              return;

            selection = {merged};
            reference = {merged};
          };

          DOCUMENT_EDIT(document, "Merge Animations", Document::ANIMATIONS, merge_quick())
        }

        ImGui::BeginDisabled(selection.size() != 1);
        {
          if (ImGui::Button("Merge", widgetSize))
          {
            mergePopup.open();
            mergeSelection.clear();
            mergeReference = *selection.begin();
          }
        }
        ImGui::EndDisabled();
        set_item_tooltip_shortcut("Open the merge popup.\nUsing the shortcut will merge the animations with\nthe last "
                                  "configured merge settings.",
                                  settings.shortcutMerge);

        ImGui::SameLine();

        shortcut(manager.chords[SHORTCUT_REMOVE]);
        if (ImGui::Button("Remove", widgetSize))
          DOCUMENT_EDIT(document, "Remove Animation(s)", Document::ANIMATIONS, animations_remove());
        set_item_tooltip_shortcut("Remove the selected animation(s).", settings.shortcutRemove);

        ImGui::SameLine();

        shortcut(manager.chords[SHORTCUT_DEFAULT]);
        ImGui::BeginDisabled(selection.size() != 1);
        if (ImGui::Button("Default", widgetSize))
        {
          DOCUMENT_EDIT(document, "Default Animation", Document::ANIMATIONS,
                        anm2.animations.defaultAnimation = anm2.animations.items[*selection.begin()].name);
        }
        ImGui::EndDisabled();
        set_item_tooltip_shortcut("Set the selected animation as the default.", settings.shortcutDefault);
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

        auto footerSize = footer_size_get();
        auto optionsSize = child_size_get(2);
        auto deleteAfterSize = child_size_get();
        auto animationsSize =
            ImVec2(0, ImGui::GetContentRegionAvail().y -
                          (optionsSize.y + deleteAfterSize.y + footerSize.y + ImGui::GetStyle().ItemSpacing.y * 3));

        if (ImGui::BeginChild("Animations", animationsSize, ImGuiChildFlags_Borders))
        {
          mergeSelection.start(anm2.animations.items.size());

          for (std::size_t index = 0; index < anm2.animations.items.size(); ++index)
          {
            if ((int)index == mergeReference) continue;

            auto& animation = anm2.animations.items[index];

            ImGui::PushID((int)index);

            ImGui::SetNextItemSelectionUserData((int)index);
            ImGui::Selectable(animation.name.c_str(), mergeSelection.contains((int)index));

            ImGui::PopID();
          }

          mergeSelection.finish();
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

        ImGui::BeginDisabled(mergeSelection.empty());
        {
          if (ImGui::Button("Merge", widgetSize))
          {
            auto merge = [&]()
            {
              if (mergeSelection.contains(overlayIndex)) overlayIndex = -1;
              auto merged =
                  anm2.animations_merge(mergeReference, mergeSelection, (merge::Type)type, isDeleteAnimationsAfter);

              selection = {merged};
              reference = {merged};
            };

            DOCUMENT_EDIT(document, "Merge Animations", Document::ANIMATIONS, merge());
            merge_close();
          }
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
        if (ImGui::Button("Close", widgetSize)) merge_close();

        ImGui::EndPopup();
      }
    }
    ImGui::End();
  }
}
