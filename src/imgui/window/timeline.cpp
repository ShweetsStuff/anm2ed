#include "timeline.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <format>
#include <map>
#include <set>

#include <imgui_internal.h>

#include "actions.hpp"
#include "log.hpp"
#include "toast.hpp"
#include "util/imgui/draw.hpp"

#include "vector.hpp"

using namespace anm2ed::resource;
using namespace anm2ed::types;
using namespace anm2ed::util;
using namespace glm;

namespace anm2ed::imgui
{

  constexpr auto WIDTH_MULTIPLIER = 1.25f;

  constexpr auto COLOR_HIDDEN_MULTIPLIER = vec4(0.5f, 0.5f, 0.5f, 1.000f);
  constexpr auto FRAME_BORDER_COLOR_DARK = ImVec4(1.0f, 1.0f, 1.0f, 0.15f);
  constexpr auto FRAME_BORDER_COLOR_LIGHT = ImVec4(0.0f, 0.0f, 0.0f, 0.25f);
  constexpr auto FRAME_BORDER_COLOR_REFERENCED_DARK = ImVec4(1.0f, 1.0f, 1.0f, 0.50f);
  constexpr auto FRAME_BORDER_COLOR_REFERENCED_LIGHT = ImVec4(0.0f, 0.0f, 0.0f, 0.60f);
  constexpr auto FRAME_BORDER_THICKNESS = 1.0f;
  constexpr auto FRAME_BORDER_THICKNESS_REFERENCED = 2.0f;
  constexpr auto FRAME_MULTIPLE_OVERLAY_COLOR_DARK = ImVec4(1.0f, 1.0f, 1.0f, 0.05f);
  constexpr auto FRAME_MULTIPLE_OVERLAY_COLOR_LIGHT = ImVec4(0.0f, 0.0f, 0.0f, 0.1f);
  constexpr auto FRAME_ROUNDING = 3.0f;
  constexpr auto ICON_TINT_DEFAULT_DARK = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
  constexpr auto ICON_TINT_DEFAULT_LIGHT = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
  constexpr auto ITEM_TEXT_COLOR_DARK = to_imvec4(color::WHITE);
  constexpr auto ITEM_TEXT_COLOR_LIGHT = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
  constexpr auto PLAYHEAD_ICON_TINT_LIGHT = ImVec4(0.3922f, 0.7843f, 0.5882f, 1.0f);
  constexpr auto PLAYHEAD_LINE_COLOR_DARK = to_imvec4(color::WHITE);
  constexpr auto PLAYHEAD_LINE_COLOR_LIGHT = ImVec4(0.1961f, 0.5882f, 0.3922f, 1.0f);
  constexpr auto PLAYHEAD_LINE_THICKNESS = 4.0f;
  constexpr auto TEXT_MULTIPLE_COLOR_DARK = to_imvec4(color::WHITE);
  constexpr auto TEXT_MULTIPLE_COLOR_LIGHT = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
  constexpr auto TIMELINE_BACKGROUND_COLOR_LIGHT = ImVec4(0.5490f, 0.5490f, 0.5882f, 1.0f);
  constexpr auto TIMELINE_PLAYHEAD_RECT_COLOR_DARK = ImVec4(0.60f, 0.45f, 0.30f, 1.0f);
  constexpr auto TIMELINE_PLAYHEAD_RECT_COLOR_LIGHT = ImVec4(0.8353f, 0.8353f, 0.7294f, 1.0f);
  constexpr auto TIMELINE_TICK_COLOR_LIGHT = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
  constexpr auto TIMELINE_CHILD_BG_COLOR_LIGHT = ImVec4(0.5490f, 0.5490f, 0.5882f, 1.0f);
  constexpr auto TIMELINE_TEXT_COLOR_LIGHT = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);

  constexpr glm::vec4 FRAME_COLOR_LIGHT_BASE[] = {{0.80f, 0.80f, 0.80f, 1.0f},
                                                  {0.5216f, 0.7333f, 1.0f, 1.0f},
                                                  {1.0f, 0.9961f, 0.5882f, 1.0f},
                                                  {0.6157f, 1.0f, 0.5882f, 1.0f},
                                                  {1.0f, 0.5882f, 0.8314f, 1.0f}};
  constexpr glm::vec4 FRAME_COLOR_LIGHT_ACTIVE[] = {{0.74f, 0.74f, 0.74f, 1.0f},
                                                    {0.0980f, 0.3765f, 0.6431f, 1.0f},
                                                    {1.0f, 0.5255f, 0.3333f, 1.0f},
                                                    {0.3686f, 0.5765f, 0.2353f, 1.0f},
                                                    {0.6118f, 0.2039f, 0.2745f, 1.0f}};
  constexpr glm::vec4 FRAME_COLOR_LIGHT_HOVERED[] = {{0.84f, 0.84f, 0.84f, 1.0f},
                                                     {0.0752f, 0.2887f, 0.4931f, 1.0f},
                                                     {0.85f, 0.4467f, 0.2833f, 1.0f},
                                                     {0.2727f, 0.4265f, 0.1741f, 1.0f},
                                                     {0.4618f, 0.1539f, 0.2072f, 1.0f}};
  constexpr glm::vec4 ITEM_COLOR_LIGHT_BASE[] = {{0.3059f, 0.3255f, 0.5412f, 1.0f},
                                                 {0.3333f, 0.5725f, 0.8392f, 1.0f},
                                                 {1.0f, 0.5412f, 0.3412f, 1.0f},
                                                 {0.5255f, 0.8471f, 0.4588f, 1.0f},
                                                 {0.7961f, 0.3882f, 0.5412f, 1.0f}};
  constexpr glm::vec4 ITEM_COLOR_LIGHT_ACTIVE[] = {{0.3459f, 0.3655f, 0.5812f, 1.0f},
                                                   {0.3733f, 0.6125f, 0.8792f, 1.0f},
                                                   {0.9106f, 0.4949f, 0.2753f, 1.0f},
                                                   {0.5655f, 0.8871f, 0.4988f, 1.0f},
                                                   {0.8361f, 0.4282f, 0.5812f, 1.0f}};
  constexpr glm::vec4 ITEM_COLOR_LIGHT_SELECTED[] = {{0.74f, 0.74f, 0.74f, 1.0f},
                                                     {0.2039f, 0.4549f, 0.7176f, 1.0f},
                                                     {0.8745f, 0.4392f, 0.2275f, 1.0f},
                                                     {0.3765f, 0.6784f, 0.2980f, 1.0f},
                                                     {0.6353f, 0.2235f, 0.3647f, 1.0f}};

  constexpr auto FRAME_MULTIPLE = 5;
  constexpr auto FRAME_TOOLTIP_HOVER_DELAY = 0.75f;

#define ITEM_CHILD_WIDTH ImGui::GetTextLineHeightWithSpacing() * 12.5

  struct TimelineItemRow
  {
    int type{NONE};
    int id{-1};
    int index{-1};
    int groupId{-1};
    int depth{};
    bool isGroup{};
  };

  void Timeline::update(Manager& manager, Settings& settings, Resources& resources, Clipboard& clipboard)
  {
    auto& document = *manager.get();
    auto& anm2 = document.anm2;
    auto& playback = document.playback;
    auto& reference = document.reference;
    auto& frames = document.frames;
    auto& region = document.region;
    auto animation = anm2.element_get(ElementType::ANIMATION, reference.animationIndex);
    auto rowFrameChildHeight = ImGui::GetTextLineHeightWithSpacing() + ImGui::GetStyle().WindowPadding.y * 1.5f;

    style = ImGui::GetStyle();
    auto isLightTheme = settings.theme == theme::LIGHT;
    bool isTextPushed = false;
    if (isLightTheme)
    {
      ImGui::PushStyleColor(ImGuiCol_Text, TIMELINE_TEXT_COLOR_LIGHT);
      isTextPushed = true;
    }

    auto type_index = [](int type) { return std::clamp(type, 0, (int)TRIGGER); };
    auto item_type_get = [](int type) { return static_cast<ItemType>(type); };
    auto item_get = [&](int type, int id = -1)
    { return animation ? animation_item_get(*animation, item_type_get(type), id) : nullptr; };
    auto frame_get = [&]()
    {
      return anm2.element_get(reference.animationIndex, item_type_get(reference.itemType), reference.frameIndex,
                              reference.itemID);
    };
    auto selected_item_get = [&]()
    { return anm2.element_get(reference.animationIndex, item_type_get(reference.itemType), reference.itemID); };
    auto item_frames_count = [](const Element* item) { return item ? (int)item->children.size() : 0; };
    auto layer_get = [&](int id) { return anm2.element_get(ElementType::LAYER_ELEMENT, id); };
    auto null_get = [&](int id)
    {
      auto nulls = anm2.element_get(ElementType::NULLS);
      return nulls ? element_child_id_get(*nulls, ElementType::NULL_ELEMENT, id) : nullptr;
    };
    auto spritesheet_get = [&](int id) { return anm2.element_get(ElementType::SPRITESHEET, id); };
    auto info_get = [&]() { return element_first_get(anm2.root, ElementType::INFO); };
    auto container_type_get = [](int type)
    {
      if (type == LAYER) return ElementType::LAYER_ANIMATIONS;
      if (type == NULL_) return ElementType::NULL_ANIMATIONS;
      return ElementType::UNKNOWN;
    };
    auto track_type_get = [](int type)
    {
      if (type == LAYER) return ElementType::LAYER_ANIMATION;
      if (type == NULL_) return ElementType::NULL_ANIMATION;
      return ElementType::UNKNOWN;
    };
    auto track_id_get = [](const Element& track, int type)
    {
      if (type == LAYER) return track.layerId;
      if (type == NULL_) return track.nullId;
      return -1;
    };
    auto track_container_get = [&](int type)
    {
      return animation ? element_child_first_get(*animation, container_type_get(type)) : nullptr;
    };
    auto track_group_get = [&](int type, int groupId)
    {
      auto container = track_container_get(type);
      return container ? element_child_id_get(*container, ElementType::GROUP, groupId) : nullptr;
    };
    auto is_track_group_visible = [&](int type, int groupId)
    {
      if (groupId == -1) return true;
      auto group = track_group_get(type, groupId);
      return !group || group->isVisible;
    };
    auto row_group_get = [&](const TimelineItemRow& row) -> Element*
    {
      return row.isGroup ? track_group_get(row.type, row.id) : nullptr;
    };
    auto group_items_count_get = [&](int type, int groupId)
    {
      auto container = track_container_get(type);
      auto trackType = track_type_get(type);
      int count{};
      if (!container) return count;
      for (auto& item : container->children)
        if (item.type == trackType && item.groupId == groupId) ++count;
      return count;
    };

    auto command_animation_get = [](Document& document, int animationIndex)
    { return document.anm2.element_get(ElementType::ANIMATION, animationIndex); };
    auto command_item_get = [](Document& document, int animationIndex, int type, int id)
    {
      auto animation = document.anm2.element_get(ElementType::ANIMATION, animationIndex);
      return animation ? animation_item_get(*animation, static_cast<ItemType>(type), id) : nullptr;
    };
    auto command_frame_get = [](Document& document, const Reference& targetReference)
    {
      return document.anm2.element_get(targetReference.animationIndex, static_cast<ItemType>(targetReference.itemType),
                                       targetReference.frameIndex, targetReference.itemID);
    };
    auto command_layer_get = [](Document& document, int id)
    { return document.anm2.element_get(ElementType::LAYER_ELEMENT, id); };
    auto command_spritesheet_get = [](Document& document, int id)
    { return document.anm2.element_get(ElementType::SPRITESHEET, id); };
    auto command_info_get = [](Document& document)
    { return element_first_get(document.anm2.root, ElementType::INFO); };
    auto item_reference_get = [&](int type, int id) { return Reference{reference.animationIndex, type, id}; };
    auto item_reference_from_frame_get = [](Reference frameReference)
    {
      frameReference.frameIndex = -1;
      return frameReference;
    };
    auto is_same_item = [](const Reference& left, const Reference& right)
    {
      return left.animationIndex == right.animationIndex && left.itemType == right.itemType &&
             left.itemID == right.itemID;
    };
    auto group_selection_reset = [this]()
    {
      groupReferences.clear();
      isRowSelectionAnchorSet = false;
    };
    auto frame_references_for_current_get = [&]()
    {
      std::set<Reference> result = frames.references;
      if (result.empty())
        for (auto frameIndex : frames.selection)
          result.insert({reference.animationIndex, reference.itemType, reference.itemID, frameIndex});
      return result;
    };
    auto item_references_for_current_get = [&]()
    {
      std::set<Reference> result = document.items.references;
      if (result.empty() && reference.itemType != NONE)
        result.insert(item_reference_get(reference.itemType, reference.itemID));
      return result;
    };
    auto frames_selection_sync_for = [&](Document& targetDocument)
    {
      targetDocument.frames.selection.clear();
      for (const auto& frameReference : targetDocument.frames.references)
        if (is_same_item(frameReference, targetDocument.reference) && frameReference.frameIndex >= 0)
          targetDocument.frames.selection.insert(frameReference.frameIndex);
      frameSelectionSnapshot.assign(targetDocument.frames.selection.begin(), targetDocument.frames.selection.end());
      frameSelectionSnapshotReference = targetDocument.reference;
    };
    auto item_selection_set_for = [&](Document& targetDocument, Reference itemReference)
    {
      itemReference.frameIndex = -1;
      targetDocument.items.references = {itemReference};
    };
    auto frame_selection_set_for = [&](Document& targetDocument, Reference frameReference)
    {
      group_selection_reset();
      targetDocument.frames.references = {frameReference};
      targetDocument.reference = frameReference;
      item_selection_set_for(targetDocument, item_reference_from_frame_get(frameReference));
      frames_selection_sync_for(targetDocument);
    };
    auto frame_selection_toggle_for = [&](Document& targetDocument, Reference frameReference)
    {
      group_selection_reset();
      auto& selection = targetDocument.frames.references;
      auto itemReference = item_reference_from_frame_get(frameReference);
      if (selection.contains(frameReference))
      {
        if (selection.size() > 1) selection.erase(frameReference);
        bool isItemStillSelected{};
        for (const auto& selectedFrame : selection)
          if (is_same_item(selectedFrame, itemReference)) isItemStillSelected = true;
        if (!isItemStillSelected && targetDocument.items.references.size() > 1)
          targetDocument.items.references.erase(itemReference);
      }
      else
      {
        selection.insert(frameReference);
        targetDocument.items.references.insert(itemReference);
      }
      targetDocument.reference = frameReference;
      frames_selection_sync_for(targetDocument);
    };
    auto frame_selection_range_set_for = [&](Document& targetDocument, Reference firstReference,
                                             Reference lastReference, bool isAdditive) -> bool
    {
      group_selection_reset();
      if (!is_same_item(firstReference, lastReference) || firstReference.frameIndex < 0 || lastReference.frameIndex < 0)
        return false;

      auto item = command_item_get(targetDocument, lastReference.animationIndex, lastReference.itemType,
                                   lastReference.itemID);
      if (!item) return false;
      if (firstReference.frameIndex >= (int)item->children.size() ||
          lastReference.frameIndex >= (int)item->children.size())
        return false;

      auto firstIndex = firstReference.frameIndex;
      auto lastIndex = lastReference.frameIndex;
      if (firstIndex > lastIndex) std::swap(firstIndex, lastIndex);
      auto itemReference = item_reference_from_frame_get(lastReference);
      if (!isAdditive)
      {
        targetDocument.frames.references.clear();
        item_selection_set_for(targetDocument, itemReference);
      }
      else
        targetDocument.items.references.insert(itemReference);

      for (int i = firstIndex; i <= lastIndex; ++i)
        targetDocument.frames.references.insert(
            {lastReference.animationIndex, lastReference.itemType, lastReference.itemID, i});

      targetDocument.reference = lastReference;
      frames_selection_sync_for(targetDocument);
      return true;
    };
    auto all_frame_references_for_item_get = [&](const Reference& itemReference)
    {
      std::set<Reference> result{};
      auto item = item_get(itemReference.itemType, itemReference.itemID);
      if (!item) return result;
      for (int i = 0; i < (int)item->children.size(); ++i)
        result.insert({itemReference.animationIndex, itemReference.itemType, itemReference.itemID, i});
      return result;
    };
    auto all_frame_references_for_items_get = [&]()
    {
      std::set<Reference> result{};
      auto itemReferences = item_references_for_current_get();
      if (itemReferences.empty())
        itemReferences.insert(item_reference_get(reference.itemType, reference.itemID));
      for (auto itemReference : itemReferences)
      {
        auto itemFrames = all_frame_references_for_item_get(itemReference);
        result.insert(itemFrames.begin(), itemFrames.end());
      }
      return result;
    };
    auto frames_selection_reset_for = [this](Document& targetDocument)
    {
      targetDocument.frames.selection.clear();
      targetDocument.frames.references.clear();
      frameSelectionSnapshot.clear();
      frameSelectionLocked.clear();
      isFrameSelectionLocked = false;
      frameFocusRequested = false;
      frameFocusIndex = -1;
      frameSelectionSnapshotReference = targetDocument.reference;
    };
    auto frames_selection_set_reference_for = [this](Document& targetDocument)
    {
      auto& targetFrames = targetDocument.frames;
      auto& targetReference = targetDocument.reference;
      targetFrames.selection.clear();
      targetFrames.references.clear();
      if (targetReference.frameIndex >= 0)
      {
        targetFrames.selection.insert(targetReference.frameIndex);
        targetFrames.references.insert(targetReference);
      }
      frameSelectionSnapshot.assign(targetFrames.selection.begin(), targetFrames.selection.end());
      frameSelectionSnapshotReference = targetReference;
      frameSelectionLocked.clear();
      isFrameSelectionLocked = false;
      frameFocusIndex = targetReference.frameIndex;
      frameFocusRequested = targetReference.frameIndex >= 0;
    };
    auto reference_clear_for = [=](Document& targetDocument)
    {
      targetDocument.reference = {targetDocument.reference.animationIndex};
      frames_selection_reset_for(targetDocument);
      targetDocument.items.references.clear();
    };
    auto reference_set_item_for = [=](Document& targetDocument, int type, int id)
    {
      targetDocument.reference = {targetDocument.reference.animationIndex, type, id};
      frames_selection_reset_for(targetDocument);
      item_selection_set_for(targetDocument, targetDocument.reference);
    };
    auto reference_set_timeline_item_for = [=](Document& targetDocument, int type, int id)
    {
      if (type == LAYER)
        if (auto layer = command_layer_get(targetDocument, id))
          targetDocument.spritesheet.reference = layer->spritesheetId;
      reference_set_item_for(targetDocument, type, id);
    };
    auto command_push = [&](auto run)
    {
      manager.command_push({manager.selected,
                            [run](Manager& manager, Document& document) mutable { run(manager, document); }});
    };
    auto snapshot_command_push = [&](StringType messageType)
    {
      auto message = std::string(localize.get(messageType));
      manager.command_push(
          {manager.selected, [message](Manager&, Document& document) { document.snapshot(message); }});
    };
    auto edit_command_push = [&](StringType messageType, Document::ChangeType changeType, auto run)
    {
      auto message = std::string(localize.get(messageType));
      manager.command_push({manager.selected,
                            [=](Manager& manager, Document& document) mutable
                            {
                              document.snapshot(message);
                              run(manager, document);
                              document.anm2_change(changeType);
                            }});
    };
    auto type_color_base_vec = [&](int type)
    { return isLightTheme ? FRAME_COLOR_LIGHT_BASE[type_index(type)] : TYPE_COLOR[type]; };

    auto type_color_active_vec = [&](int type)
    {
      if (isLightTheme) return FRAME_COLOR_LIGHT_ACTIVE[type_index(type)];
      return TYPE_COLOR_ACTIVE[type];
    };

    auto type_color_hovered_vec = [&](int type)
    {
      if (isLightTheme) return FRAME_COLOR_LIGHT_HOVERED[type_index(type)];
      return TYPE_COLOR_HOVERED[type];
    };

    auto item_color_vec = [&](int type)
    {
      if (!isLightTheme) return TYPE_COLOR[type];
      return ITEM_COLOR_LIGHT_BASE[type_index(type)];
    };

    auto item_color_active_vec = [&](int type)
    {
      if (!isLightTheme) return TYPE_COLOR_ACTIVE[type];
      return ITEM_COLOR_LIGHT_ACTIVE[type_index(type)];
    };

    auto iconTintDefault = isLightTheme ? ICON_TINT_DEFAULT_LIGHT : ICON_TINT_DEFAULT_DARK;
    auto itemIconTint = isLightTheme ? ICON_TINT_DEFAULT_LIGHT : iconTintDefault;
    auto frameBorderColor = isLightTheme ? FRAME_BORDER_COLOR_LIGHT : FRAME_BORDER_COLOR_DARK;
    auto frameBorderColorReferenced =
        isLightTheme ? FRAME_BORDER_COLOR_REFERENCED_LIGHT : FRAME_BORDER_COLOR_REFERENCED_DARK;
    auto frameMultipleOverlayColor =
        isLightTheme ? FRAME_MULTIPLE_OVERLAY_COLOR_LIGHT : FRAME_MULTIPLE_OVERLAY_COLOR_DARK;
    auto textMultipleColor = isLightTheme ? TEXT_MULTIPLE_COLOR_LIGHT : TEXT_MULTIPLE_COLOR_DARK;
    auto playheadLineColor = isLightTheme ? PLAYHEAD_LINE_COLOR_LIGHT : PLAYHEAD_LINE_COLOR_DARK;
    auto playheadIconTint = isLightTheme ? PLAYHEAD_ICON_TINT_LIGHT : iconTintDefault;
    auto timelineBackgroundColor =
        isLightTheme ? TIMELINE_BACKGROUND_COLOR_LIGHT : ImGui::GetStyleColorVec4(ImGuiCol_Header);
    auto timelinePlayheadRectColor =
        isLightTheme ? TIMELINE_PLAYHEAD_RECT_COLOR_LIGHT : TIMELINE_PLAYHEAD_RECT_COLOR_DARK;
    auto timelineTickColor = isLightTheme ? TIMELINE_TICK_COLOR_LIGHT : frameBorderColor;
    auto itemTextColor = isLightTheme ? ITEM_TEXT_COLOR_LIGHT : ITEM_TEXT_COLOR_DARK;

    auto overlay_icon = [&](GLuint textureId, ImVec4 tint, bool isForced = false)
    {
      if (!isForced && !isLightTheme) return;
      auto min = ImGui::GetItemRectMin();
      auto max = ImGui::GetItemRectMax();
      ImGui::GetWindowDrawList()->AddImage((ImTextureID)(intptr_t)textureId, min, max, ImVec2(0, 0), ImVec2(1, 1),
                                           ImGui::GetColorU32(tint));
    };

    auto frames_selection_set_reference = [&]()
    {
      frames_selection_set_reference_for(document);
    };

    auto playback_stop = [&]()
    {
      playback.isPlaying = false;
      playback.isFinished = false;
      playback.timing_reset();
    };

    auto frame_insert = [&]()
    {
      auto targetReference = reference;
      auto targetFrameTime = document.frameTime;
      if (!animation || !command_item_get(document, targetReference.animationIndex, targetReference.itemType,
                                          targetReference.itemID))
        return;

      edit_command_push(EDIT_INSERT_FRAME, Document::FRAMES,
                        [=, this](Manager&, Document& document) mutable
                        {
                          auto animation = command_animation_get(document, targetReference.animationIndex);
                          auto item = command_item_get(document, targetReference.animationIndex,
                                                       targetReference.itemType, targetReference.itemID);
                          if (!animation || !item) return;

                          auto newReference = targetReference;

                          if (targetReference.itemType == TRIGGER)
                          {
                            for (auto& trigger : item->children)
                              if (targetFrameTime == trigger.atFrame) return;

                            auto addFrame = element_make(ElementType::TRIGGER);
                            addFrame.atFrame = targetFrameTime;
                            item->children.push_back(addFrame);
                            frames_sort_by_at_frame(*item);
                            newReference.frameIndex = frame_index_from_at_frame_get(*item, addFrame.atFrame);
                          }
                          else
                          {
                            auto frame = command_frame_get(document, targetReference);
                            if (frame)
                            {
                              auto addFrame = *frame;
                              auto insertIndex =
                                  std::clamp(targetReference.frameIndex + 1, 0, (int)item->children.size());
                              item->children.insert(item->children.begin() + insertIndex, addFrame);
                              newReference.frameIndex = insertIndex;
                            }
                            else if (!item->children.empty())
                            {
                              auto addFrame = item->children.back();
                              item->children.emplace_back(addFrame);
                              newReference.frameIndex = (int)item->children.size() - 1;
                            }
                            else
                            {
                              item->children.emplace_back(element_make(ElementType::FRAME));
                              newReference.frameIndex = 0;
                            }
                          }

                          document.reference = newReference;
                          frames_selection_set_reference_for(document);
                          if (newReference.itemType != TRIGGER)
                            document.frameTime = frame_time_from_index_get(*item, newReference.frameIndex);
                        });
    };

    auto frames_delete_for = [=](Document& document, std::set<Reference> selectedFrames) mutable
    {
      std::map<Reference, std::set<int>> groupedFrames{};
      for (auto frameReference : selectedFrames)
      {
        auto itemReference = item_reference_from_frame_get(frameReference);
        groupedFrames[itemReference].insert(frameReference.frameIndex);
      }

      for (auto& [itemReference, indices] : groupedFrames)
      {
        auto item = command_item_get(document, itemReference.animationIndex, itemReference.itemType,
                                     itemReference.itemID);
        if (!item) continue;

        for (auto it = indices.rbegin(); it != indices.rend(); ++it)
        {
          auto i = *it;
          if (i >= 0 && i < (int)item->children.size()) item->children.erase(item->children.begin() + i);
        }
      }

      auto item = command_item_get(document, document.reference.animationIndex, document.reference.itemType,
                                   document.reference.itemID);
      if (!item || item->children.empty())
      {
        document.reference.frameIndex = -1;
        frames_selection_reset_for(document);
        return;
      }

      document.reference.frameIndex = glm::clamp(document.reference.frameIndex, 0, (int)item->children.size() - 1);
      frames_selection_set_reference_for(document);
    };

    auto frames_delete_action = [&]()
    {
      auto selectedFrames = frame_references_for_current_get();
      if (selectedFrames.empty()) return;
      edit_command_push(EDIT_DELETE_FRAMES, Document::FRAMES,
                        [=](Manager&, Document& document) mutable
                        { frames_delete_for(document, selectedFrames); });
    };

    auto frames_bake = [&]()
    {
      auto selectedFrames = frame_references_for_current_get();
      auto bakeInterval = settings.bakeInterval;
      auto isRoundScale = settings.bakeIsRoundScale;
      auto isRoundRotation = settings.bakeIsRoundRotation;
      std::erase_if(selectedFrames,
                    [](const Reference& frameReference) { return frameReference.itemType == TRIGGER; });
      if (selectedFrames.empty()) return;

      edit_command_push(EDIT_BAKE_FRAMES, Document::FRAMES,
                        [=, this](Manager&, Document& document) mutable
                        {
                          std::map<Reference, std::set<int>> groupedFrames{};
                          for (auto frameReference : selectedFrames)
                          {
                            auto itemReference = item_reference_from_frame_get(frameReference);
                            groupedFrames[itemReference].insert(frameReference.frameIndex);
                          }

                          std::set<Reference> bakedSelection{};
                          for (auto& [itemReference, indices] : groupedFrames)
                          {
                            auto item = command_item_get(document, itemReference.animationIndex,
                                                         itemReference.itemType, itemReference.itemID);
                            if (!item) continue;

                            int insertedBefore = 0;
                            for (auto originalIndex : indices)
                            {
                              auto i = originalIndex + insertedBefore;
                              if (!vector::in_bounds(item->children, i)) continue;

                              auto originalDuration = item->children[i].duration;
                              frame_bake(*item, i, bakeInterval, isRoundScale, isRoundRotation);

                              auto bakedCount = originalDuration <= FRAME_DURATION_MIN
                                                    ? 1
                                                    : (int)std::ceil((float)originalDuration / bakeInterval);
                              for (int offset = 0; offset < bakedCount; ++offset)
                                bakedSelection.insert(
                                    {itemReference.animationIndex, itemReference.itemType, itemReference.itemID,
                                     i + offset});

                              insertedBefore += bakedCount - 1;
                            }
                          }

                          document.frames.references = std::move(bakedSelection);
                          if (!document.frames.references.empty())
                          {
                            document.reference = *document.frames.references.begin();
                            frames_selection_sync_for(document);
                            frameSelectionSnapshotReference = document.reference;
                            frameSelectionLocked.clear();
                            isFrameSelectionLocked = false;
                            frameFocusIndex = document.reference.frameIndex;
                            frameFocusRequested = true;
                          }
                          else
                            frames_selection_reset_for(document);
                        });
    };

    auto frame_split = [&]()
    {
      auto selectedFrames = frame_references_for_current_get();
      if (selectedFrames.size() != 1) return;
      auto targetReference = *selectedFrames.begin();
      auto playheadTime = (int)std::floor(playback.time);
      if (targetReference.itemType == TRIGGER) return;

      edit_command_push(EDIT_SPLIT_FRAME, Document::FRAMES,
                        [=](Manager&, Document& document) mutable
                        {
                          if (targetReference.itemType == TRIGGER) return;

                          auto item = command_item_get(document, targetReference.animationIndex,
                                                       targetReference.itemType, targetReference.itemID);
                          auto frame = command_frame_get(document, targetReference);

                          if (!item || !frame) return;

                          auto originalDuration = frame->duration;
                          if (originalDuration <= 1) return;

                          auto frameStartTime = frame_time_from_index_get(*item, targetReference.frameIndex);
                          int frameStart = (int)std::round(frameStartTime);
                          int firstDuration = playheadTime - frameStart + 1;

                          if (firstDuration <= 0 || firstDuration >= originalDuration) return;

                          int secondDuration = originalDuration - firstDuration;
                          auto splitFrame = *frame;
                          splitFrame.duration = secondDuration;

                          auto nextFrame = track_frame_get(*item, targetReference.frameIndex + 1);
                          if (frame->interpolation != Interpolation::NONE && nextFrame)
                          {
                            float interpolation = (float)firstDuration / (float)originalDuration;
                            switch (frame->interpolation)
                            {
                              case Interpolation::EASE_IN:
                                interpolation *= interpolation;
                                break;
                              case Interpolation::EASE_OUT:
                                interpolation = 1.0f - ((1.0f - interpolation) * (1.0f - interpolation));
                                break;
                              case Interpolation::EASE_IN_OUT:
                                interpolation = interpolation < 0.5f
                                                    ? (2.0f * interpolation * interpolation)
                                                    : (1.0f - std::pow(-2.0f * interpolation + 2.0f, 2.0f) * 0.5f);
                                break;
                              case Interpolation::LINEAR:
                              case Interpolation::NONE:
                              default:
                                break;
                            }

                            splitFrame.rotation = glm::mix(frame->rotation, nextFrame->rotation, interpolation);
                            splitFrame.position = glm::mix(frame->position, nextFrame->position, interpolation);
                            splitFrame.scale = glm::mix(frame->scale, nextFrame->scale, interpolation);
                            splitFrame.colorOffset = glm::mix(frame->colorOffset, nextFrame->colorOffset, interpolation);
                            splitFrame.tint = glm::mix(frame->tint, nextFrame->tint, interpolation);
                          }

                          frame->duration = firstDuration;
                          item->children.insert(item->children.begin() + targetReference.frameIndex + 1, splitFrame);
                          document.reference = targetReference;
                          frames_selection_set_reference_for(document);
                        });
    };

    auto reference_clear = [&]()
    {
      group_selection_reset();
      reference_clear_for(document);
    };

    auto reference_set_timeline_item = [&](int type, int id)
    {
      group_selection_reset();
      reference_set_timeline_item_for(document, type, id);
    };

    auto timeline_item_rows_get = [&]()
    {
      std::vector<TimelineItemRow> rows{};
      if (!animation) return rows;

      auto track_row_push = [&](const Element& item, int type, int index, int depth = 0)
      {
        rows.push_back({.type = type,
                        .id = track_id_get(item, type),
                        .index = index,
                        .groupId = item.groupId,
                        .depth = depth});
      };
      auto group_row_push = [&](const Element& group, int type, int index)
      {
        rows.push_back({.type = type, .id = group.id, .index = index, .isGroup = true});
      };
      auto group_ids_get = [](const Element& container)
      {
        std::set<int> result{};
        for (const auto& item : container.children)
          if (item.type == ElementType::GROUP) result.insert(item.id);
        return result;
      };

      rows.push_back({.type = ROOT});

      if (auto layerAnimations = element_child_first_get(*animation, ElementType::LAYER_ANIMATIONS))
        {
          auto groupIds = group_ids_get(*layerAnimations);
          auto layer_track_push = [&](int groupId, int depth)
          {
            for (int j = (int)layerAnimations->children.size() - 1; j >= 0; --j)
            {
              auto& item = layerAnimations->children[j];
              if (item.type != ElementType::LAYER_ANIMATION || item.groupId != groupId) continue;
              if (settings.timelineIsShowUnused || !item.children.empty()) track_row_push(item, LAYER, j, depth);
            }
          };

          for (int i = (int)layerAnimations->children.size() - 1; i >= 0; --i)
          {
            auto& item = layerAnimations->children[i];
            if (item.type == ElementType::GROUP)
            {
              group_row_push(item, LAYER, i);
              if (item.isExpanded) layer_track_push(item.id, 1);
            }
            else if (item.type == ElementType::LAYER_ANIMATION && !groupIds.contains(item.groupId) &&
                     (settings.timelineIsShowUnused || !item.children.empty()))
              track_row_push(item, LAYER, i);
          }
        }

      if (auto nullAnimations = element_child_first_get(*animation, ElementType::NULL_ANIMATIONS))
        {
          auto groupIds = group_ids_get(*nullAnimations);
          auto null_track_push = [&](int groupId, int depth)
          {
            for (int j = 0; j < (int)nullAnimations->children.size(); ++j)
            {
              auto& item = nullAnimations->children[j];
              if (item.type != ElementType::NULL_ANIMATION || item.groupId != groupId) continue;
              if (settings.timelineIsShowUnused || !item.children.empty()) track_row_push(item, NULL_, j, depth);
            }
          };

          for (int i = 0; i < (int)nullAnimations->children.size(); ++i)
          {
            auto& item = nullAnimations->children[i];
            if (item.type == ElementType::GROUP)
            {
              group_row_push(item, NULL_, i);
              if (item.isExpanded) null_track_push(item.id, 1);
            }
            else if (item.type == ElementType::NULL_ANIMATION && !groupIds.contains(item.groupId) &&
                     (settings.timelineIsShowUnused || !item.children.empty()))
              track_row_push(item, NULL_, i);
          }
        }

      rows.push_back({.type = TRIGGER});
      return rows;
    };

    auto timeline_item_references_get = [&]()
    {
      std::vector<Reference> itemReferences;
      for (const auto& row : timeline_item_rows_get())
      {
        if (row.isGroup) continue;
        itemReferences.push_back({reference.animationIndex, row.type, row.id});
      }
      return itemReferences;
    };

    auto group_reference_get = [&](const TimelineItemRow& row)
    { return TimelineGroupReference{manager.selected, reference.animationIndex, row.type, row.id}; };

    auto row_reference_get = [&](const TimelineItemRow& row)
    {
      return TimelineRowReference{manager.selected, reference.animationIndex, row.type, row.id, row.index, row.isGroup};
    };

    auto row_item_reference_get = [](const TimelineRowReference& row)
    { return Reference{row.animationIndex, row.type, row.id}; };

    auto timeline_row_references_get = [&]()
    {
      std::vector<TimelineRowReference> rowReferences;
      for (const auto& row : timeline_item_rows_get())
      {
        if (row.type == NONE) continue;
        rowReferences.push_back(row_reference_get(row));
      }
      return rowReferences;
    };

    auto is_group_selected = [&](const TimelineItemRow& row)
    {
      return groupReferences.contains(group_reference_get(row));
    };

    auto is_row_selected = [&](const TimelineItemRow& row)
    {
      if (row.isGroup) return is_group_selected(row);
      auto itemReference = item_reference_get(row.type, row.id);
      auto isReferenced = reference.itemType == row.type && reference.itemID == row.id;
      return document.items.references.contains(itemReference) ||
             (document.items.references.empty() && groupReferences.empty() && isReferenced);
    };

    auto row_selection_clear = [&]()
    {
      document.items.references.clear();
      groupReferences.clear();
    };

    auto row_selection_insert = [&](const TimelineRowReference& row)
    {
      if (row.isGroup)
        groupReferences.insert({row.documentIndex, row.animationIndex, row.type, row.id});
      else
        document.items.references.insert(row_item_reference_get(row));
    };

    auto row_selection_erase = [&](const TimelineRowReference& row)
    {
      if (row.isGroup)
        groupReferences.erase({row.documentIndex, row.animationIndex, row.type, row.id});
      else
        document.items.references.erase(row_item_reference_get(row));
    };

    auto is_row_reference_selected = [&](const TimelineRowReference& row)
    {
      if (row.isGroup) return groupReferences.contains({row.documentIndex, row.animationIndex, row.type, row.id});
      return document.items.references.contains(row_item_reference_get(row));
    };

    auto row_selection_count_get = [&]() { return document.items.references.size() + groupReferences.size(); };

    auto row_selection_set = [&](const TimelineItemRow& row)
    {
      auto rowReference = row_reference_get(row);
      auto isCtrlDown = ImGui::IsKeyDown(ImGuiMod_Ctrl);
      auto isShiftDown = ImGui::IsKeyDown(ImGuiMod_Shift);

      if (row.isGroup)
        reference = {reference.animationIndex};
      else
      {
        if (row.type == LAYER)
          if (auto layer = layer_get(row.id)) document.spritesheet.reference = layer->spritesheetId;
        reference = row_item_reference_get(rowReference);
      }
      frames_selection_reset_for(document);

      if (isShiftDown)
      {
        auto rowSelection = timeline_row_references_get();
        auto anchor = isRowSelectionAnchorSet ? rowSelectionAnchor : rowReference;
        auto first = std::find(rowSelection.begin(), rowSelection.end(), anchor);
        auto last = std::find(rowSelection.begin(), rowSelection.end(), rowReference);
        if (first == rowSelection.end() || last == rowSelection.end())
        {
          if (!isCtrlDown) row_selection_clear();
          row_selection_insert(rowReference);
          rowSelectionAnchor = rowReference;
        }
        else
        {
          auto firstIndex = (int)std::distance(rowSelection.begin(), first);
          auto lastIndex = (int)std::distance(rowSelection.begin(), last);
          if (firstIndex > lastIndex) std::swap(firstIndex, lastIndex);
          if (!isCtrlDown) row_selection_clear();
          for (int i = firstIndex; i <= lastIndex; ++i)
            row_selection_insert(rowSelection[i]);
          if (!isRowSelectionAnchorSet) rowSelectionAnchor = rowReference;
        }
      }
      else if (isCtrlDown)
      {
        if (is_row_reference_selected(rowReference) && row_selection_count_get() > 1)
          row_selection_erase(rowReference);
        else
          row_selection_insert(rowReference);
        rowSelectionAnchor = rowReference;
      }
      else
      {
        row_selection_clear();
        row_selection_insert(rowReference);
        rowSelectionAnchor = rowReference;
      }

      isRowSelectionAnchorSet = true;
    };

    auto reference_set_adjacent_item = [&](int direction)
    {
      auto itemReferences = timeline_item_references_get();
      if (itemReferences.empty()) return;

      auto it = std::find_if(
          itemReferences.begin(), itemReferences.end(), [&](const Reference& itemReference)
          { return itemReference.itemType == reference.itemType && itemReference.itemID == reference.itemID; });

      int index = direction > 0 ? 0 : (int)itemReferences.size() - 1;
      if (it != itemReferences.end()) index = (int)std::distance(itemReferences.begin(), it) + direction;
      index = std::clamp(index, 0, (int)itemReferences.size() - 1);

      auto& itemReference = itemReferences[index];
      reference_set_timeline_item(itemReference.itemType, itemReference.itemID);
    };

    auto selected_row_references_get = [&]()
    {
      std::vector<TimelineRowReference> result{};
      for (const auto& row : timeline_item_rows_get())
        if (is_row_selected(row)) result.push_back(row_reference_get(row));
      return result;
    };

    auto row_drag_references_get = [&](const TimelineItemRow& row)
    {
      auto clicked = row_reference_get(row);
      if (clicked.type != LAYER && clicked.type != NULL_) return std::vector<TimelineRowReference>{clicked};

      auto rows = selected_row_references_get();
      if (!is_row_reference_selected(clicked)) rows = {clicked};
      if (rows.empty()) rows = {clicked};

      std::erase_if(rows, [&](const TimelineRowReference& rowReference) { return rowReference.type != clicked.type; });
      if (rows.empty()) rows = {clicked};
      return rows;
    };

    auto row_label_get = [&](const TimelineRowReference& row) -> std::string
    {
      if (row.isGroup)
      {
        TimelineItemRow groupRow{.type = row.type, .id = row.id, .index = row.index, .isGroup = true};
        auto group = row_group_get(groupRow);
        if (!group || group->name.empty()) return localize.get(TEXT_NEW_GROUP);
        return group->name;
      }
      if (row.type == LAYER)
      {
        auto layer = layer_get(row.id);
        if (!layer) return localize.get(TYPE_STRINGS[row.type]);
        return std::vformat(localize.get(FORMAT_LAYER),
                            std::make_format_args(layer->id, layer->name, layer->spritesheetId));
      }
      if (row.type == NULL_)
      {
        auto null = null_get(row.id);
        if (!null) return localize.get(TYPE_STRINGS[row.type]);
        return std::vformat(localize.get(FORMAT_NULL), std::make_format_args(null->id, null->name));
      }
      return localize.get(TYPE_STRINGS[type_index(row.type)]);
    };

    auto row_drag_tooltip_draw = [&](const std::vector<TimelineRowReference>& rows)
    {
      for (auto row : rows)
      {
        auto label = row_label_get(row);
        ImGui::TextUnformatted(label.c_str());
      }
    };

    auto item_remove = [&]()
    {
      auto targetRows = selected_row_references_get();
      std::map<int, std::set<int>> targetIds{};
      std::map<int, std::set<int>> targetGroupIds{};
      for (auto row : targetRows)
      {
        if (row.type != LAYER && row.type != NULL_) continue;
        if (row.isGroup)
          targetGroupIds[row.type].insert(row.id);
        else
          targetIds[row.type].insert(row.id);
      }
      auto animationIndex = reference.animationIndex;
      if (!animation) return;
      if (targetIds.empty() && targetGroupIds.empty()) return;
      edit_command_push(EDIT_REMOVE_ITEMS, Document::ITEMS,
                        [=, this](Manager&, Document& document) mutable
                        {
                          auto animation = command_animation_get(document, animationIndex);
                          if (!animation) return;
                          for (auto targetType : {LAYER, NULL_})
                          {
                            auto containerType = container_type_get(targetType);
                            auto targetTrackType = track_type_get(targetType);
                            auto container = element_child_first_get(*animation, containerType);
                            if (!container) continue;
                            auto ids = targetIds.contains(targetType) ? targetIds.at(targetType) : std::set<int>{};
                            auto groupIds =
                                targetGroupIds.contains(targetType) ? targetGroupIds.at(targetType) : std::set<int>{};
                            for (int i = (int)container->children.size() - 1; i >= 0; --i)
                            {
                              auto& item = container->children[i];
                              if (item.type == ElementType::GROUP && groupIds.contains(item.id))
                              {
                                container->children.erase(container->children.begin() + i);
                                continue;
                              }
                              if (item.type == targetTrackType &&
                                  (ids.contains(track_id_get(item, targetType)) || groupIds.contains(item.groupId)))
                                container->children.erase(container->children.begin() + i);
                            }
                          }
                          reference_clear_for(document);
                          group_selection_reset();
                        });
    };

    auto item_references_groupable_get = [&]()
    {
      if (!groupReferences.empty()) return std::vector<Reference>{};
      auto selectedItems = item_references_for_current_get();
      std::erase_if(selectedItems, [](const Reference& itemReference)
                    { return itemReference.itemType != LAYER && itemReference.itemType != NULL_; });
      if (selectedItems.empty()) return std::vector<Reference>{};
      auto itemType = selectedItems.begin()->itemType;
      for (const auto& itemReference : selectedItems)
        if (itemReference.itemType != itemType) return std::vector<Reference>{};

      std::vector<Reference> result{};
      for (const auto& row : timeline_item_rows_get())
      {
        if (row.isGroup || row.type != itemType) continue;
        auto itemReference = item_reference_get(row.type, row.id);
        if (!selectedItems.contains(itemReference)) continue;
        if (row.groupId != -1) return std::vector<Reference>{};
        result.push_back(itemReference);
      }
      return result;
    };

    auto item_group = [&]()
    {
      auto targetReferences = item_references_groupable_get();
      if (targetReferences.empty()) return;
      auto targetType = targetReferences.front().itemType;
      auto animationIndex = reference.animationIndex;
      auto groupName = std::string(localize.get(TEXT_NEW_GROUP));

      edit_command_push(EDIT_GROUP_ITEMS, Document::ITEMS,
                        [=](Manager&, Document& document) mutable
                        {
                          auto animation = command_animation_get(document, animationIndex);
                          if (!animation) return;

                          auto containerType = container_type_get(targetType);
                          auto targetTrackType = track_type_get(targetType);
                          auto container = element_child_first_get(*animation, containerType);
                          if (!container) return;

                          std::set<int> targetIds{};
                          for (auto itemReference : targetReferences)
                            targetIds.insert(itemReference.itemID);

                          auto group = element_make(ElementType::GROUP);
                          group.id = element_child_next_id_get(*container, ElementType::GROUP);
                          group.name = groupName;
                          group.isExpanded = true;
                          int insertIndex = (int)container->children.size();

                          for (int i = 0; i < (int)container->children.size(); ++i)
                          {
                            auto& item = container->children[i];
                            if (item.type == targetTrackType && targetIds.contains(track_id_get(item, targetType)))
                            {
                              insertIndex = std::min(insertIndex, i);
                              item.groupId = group.id;
                            }
                          }

                          if (insertIndex == (int)container->children.size()) return;
                          insertIndex = std::clamp(insertIndex, 0, (int)container->children.size());
                          container->children.insert(container->children.begin() + insertIndex, group);

                          document.items.references.clear();
                          for (auto itemReference : targetReferences)
                            document.items.references.insert(itemReference);
                          document.reference = targetReferences.front();
                          frames_selection_reset_for(document);
                        });
    };

    auto rows_move_to_row = [&](std::vector<TimelineRowReference> draggedRows, TimelineItemRow targetRow,
                                bool isDropAfter)
    {
      if (draggedRows.empty()) return;
      auto targetType = draggedRows.front().type;
      if (targetType != LAYER && targetType != NULL_) return;
      for (const auto& draggedRow : draggedRows)
        if (draggedRow.type != targetType) return;

      if (targetRow.isGroup && targetRow.type != targetType) return;
      if (!targetRow.isGroup && targetRow.type != targetType && targetRow.type != NONE && targetRow.type != ROOT &&
          targetRow.type != TRIGGER)
        return;

      auto animationIndex = reference.animationIndex;
      edit_command_push(EDIT_MOVE_ITEMS, Document::ITEMS,
                        [=, this](Manager&, Document& document) mutable
                        {
                          auto animation = command_animation_get(document, animationIndex);
                          if (!animation) return;
                          auto container = element_child_first_get(*animation, container_type_get(targetType));
                          if (!container) return;
                          auto targetTrackType = track_type_get(targetType);

                          std::set<int> draggedIds{};
                          std::set<int> draggedGroupIds{};
                          for (const auto& draggedRow : draggedRows)
                          {
                            if (draggedRow.isGroup)
                              draggedGroupIds.insert(draggedRow.id);
                            else
                              draggedIds.insert(draggedRow.id);
                          }

                          if (targetRow.groupId != -1 && draggedGroupIds.contains(targetRow.groupId)) return;

                          std::vector<Element> movedItems{};
                          for (const auto& item : container->children)
                          {
                            auto isDraggedGroup = item.type == ElementType::GROUP && draggedGroupIds.contains(item.id);
                            auto isDraggedTrack = item.type == targetTrackType &&
                                                  (draggedIds.contains(track_id_get(item, targetType)) ||
                                                   draggedGroupIds.contains(item.groupId));
                            if (isDraggedGroup || isDraggedTrack) movedItems.push_back(item);
                          }
                          if (movedItems.empty()) return;

                          if (targetRow.isGroup && draggedGroupIds.contains(targetRow.id)) return;

                          auto row_index_get = [&](const TimelineItemRow& row)
                          {
                            for (int i = 0; i < (int)container->children.size(); ++i)
                            {
                              auto& item = container->children[i];
                              if (row.isGroup && item.type == ElementType::GROUP && item.id == row.id) return i;
                              if (!row.isGroup && row.type == targetType && item.type == targetTrackType &&
                                  track_id_get(item, targetType) == row.id)
                                return i;
                            }
                            return -1;
                          };

                          int targetIndex = (int)container->children.size();
                          if (targetRow.isGroup || targetRow.type == targetType)
                          {
                            auto rowIndex = row_index_get(targetRow);
                            if (rowIndex == -1) return;
                            targetIndex = rowIndex + (targetType == LAYER ? !isDropAfter : isDropAfter);
                          }

                          int removedBeforeTarget = 0;
                          for (int i = (int)container->children.size() - 1; i >= 0; --i)
                          {
                            auto& item = container->children[i];
                            auto isDraggedGroup = item.type == ElementType::GROUP && draggedGroupIds.contains(item.id);
                            auto isDraggedTrack = item.type == targetTrackType &&
                                                  (draggedIds.contains(track_id_get(item, targetType)) ||
                                                   draggedGroupIds.contains(item.groupId));
                            if (isDraggedGroup || isDraggedTrack)
                            {
                              if (i < targetIndex) ++removedBeforeTarget;
                              container->children.erase(container->children.begin() + i);
                            }
                          }

                          auto targetGroupId = -1;
                          if (targetRow.isGroup && isDropAfter)
                            targetGroupId = targetRow.id;
                          else if (targetRow.type == targetType)
                            targetGroupId = targetRow.groupId;

                          for (auto& item : movedItems)
                            if (item.type == targetTrackType && !draggedGroupIds.contains(item.groupId))
                              item.groupId = targetGroupId;

                          targetIndex -= removedBeforeTarget;
                          targetIndex = std::clamp(targetIndex, 0, (int)container->children.size());
                          container->children.insert(container->children.begin() + targetIndex, movedItems.begin(),
                                                     movedItems.end());

                          document.items.references.clear();
                          groupReferences.clear();
                          for (const auto& draggedRow : draggedRows)
                          {
                            if (draggedRow.isGroup)
                              groupReferences.insert({draggedRow.documentIndex, draggedRow.animationIndex,
                                                      draggedRow.type, draggedRow.id});
                            else
                              document.items.references.insert(row_item_reference_get(draggedRow));
                          }
                          if (!document.items.references.empty())
                            document.reference = *document.items.references.begin();
                          else
                            document.reference = {animationIndex};
                          frames_selection_reset_for(document);
                        });
    };

    auto fit_animation_length = [&]()
    {
      if (!animation) return;
      auto animationIndex = reference.animationIndex;
      edit_command_push(EDIT_FIT_ANIMATION_LENGTH, Document::ANIMATIONS,
                        [=](Manager&, Document& document)
                        {
                          auto animation = command_animation_get(document, animationIndex);
                          if (!animation) return;
                          animation->frameNum = animation_length_get(*animation);
                        });
    };

    auto context_menu = [&]()
    {
      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, style.WindowPadding);
      ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, style.ItemSpacing);

      auto copy = [&]()
      {
        if (!animation) return;
        auto selectedFrames = frame_references_for_current_get();
        if (selectedFrames.empty()) return;

        std::string clipboardString{};
        for (auto frameReference : selectedFrames)
        {
          auto item = item_get(frameReference.itemType, frameReference.itemID);
          auto frame = item ? track_frame_get(*item, frameReference.frameIndex) : nullptr;
          if (!frame) continue;
          auto parentType = item_type_to_track_type_get(item_type_get(frameReference.itemType));
          clipboardString += element_to_string(*frame, parentType);
        }
        if (!clipboardString.empty()) clipboard.set(clipboardString);
      };

      auto cut = [&]()
      {
        copy();
        auto selectedFrames = frame_references_for_current_get();
        edit_command_push(EDIT_CUT_FRAMES, Document::FRAMES,
                          [=](Manager&, Document& document) mutable
                          { frames_delete_for(document, selectedFrames); });
      };

      auto paste = [&]()
      {
        if (clipboard.is_empty()) return;
        auto targetReference = reference;
        auto selectedFrames = frame_references_for_current_get();
        auto clipboardString = clipboard.get();
        auto targetHoveredTime = hoveredTime;
        auto message = std::string(localize.get(EDIT_PASTE_FRAMES));
        command_push([=](Manager&, Document& document) mutable
                     {
                       document.snapshot(message);
                       auto animation = command_animation_get(document, targetReference.animationIndex);
                       if (!animation) return;
                       if (auto item = command_item_get(document, targetReference.animationIndex,
                                                        targetReference.itemType, targetReference.itemID))
                       {
                         std::set<int> indices{};
                         std::string errorString{};
                         int insertIndex = (int)item->children.size();
                         std::set<int> selectedIndices{};
                         for (auto frameReference : selectedFrames)
                           if (is_same_item(frameReference, targetReference))
                             selectedIndices.insert(frameReference.frameIndex);

                         if (!selectedIndices.empty())
                           insertIndex = std::min((int)item->children.size(), *selectedIndices.rbegin() + 1);
                         else if (targetReference.frameIndex >= 0 &&
                                  targetReference.frameIndex < (int)item->children.size())
                           insertIndex = targetReference.frameIndex + 1;

                         auto start = targetReference.itemType == TRIGGER ? targetHoveredTime : insertIndex;
                         if (frames_deserialize(*item, clipboardString, start, indices, &errorString))
                         {
                           if (targetReference.itemType == LAYER && targetReference.itemID != -1)
                           {
                             auto layer = command_layer_get(document, targetReference.itemID);
                             auto spritesheet =
                                 layer ? command_spritesheet_get(document, layer->spritesheetId) : nullptr;

                             for (auto i : indices)
                             {
                               auto frame = track_frame_get(*item, i);
                               if (!frame || frame->regionId == -1) continue;
                               auto region = spritesheet ? element_child_id_get(*spritesheet, ElementType::REGION,
                                                                                 frame->regionId)
                                                         : nullptr;
                               if (!region) frame->regionId = -1;
                             }
                           }

                           document.reference = targetReference;
                           document.frames.selection.clear();
                           document.frames.references.clear();
                           for (auto i : indices)
                           {
                             document.frames.selection.insert(i);
                             document.frames.references.insert(
                                 {targetReference.animationIndex, targetReference.itemType, targetReference.itemID, i});
                           }
                           document.reference.frameIndex = *indices.begin();
                           document.anm2_change(Document::FRAMES);
                         }
                         else
                         {
                           toasts.push(std::format("{} {}", localize.get(TOAST_DESERIALIZE_FRAMES_FAILED),
                                                   errorString));
                           logger.error(std::format("{} {}", localize.get(TOAST_DESERIALIZE_FRAMES_FAILED,
                                                                          anm2ed::ENGLISH),
                                                    errorString));
                         }
                       }
                       else
                       {
                         toasts.push(localize.get(TOAST_DESERIALIZE_FRAMES_NO_SELECTION));
                         logger.warning(localize.get(TOAST_DESERIALIZE_FRAMES_NO_SELECTION, anm2ed::ENGLISH));
                       }
                     });
      };

      if (shortcut(manager.chords[SHORTCUT_CUT], shortcut::FOCUSED)) cut();
      if (shortcut(manager.chords[SHORTCUT_COPY], shortcut::FOCUSED)) copy();
      if (shortcut(manager.chords[SHORTCUT_PASTE], shortcut::FOCUSED)) paste();
      if (shortcut(manager.chords[SHORTCUT_SPLIT], shortcut::FOCUSED)) frame_split();
      if (shortcut(manager.chords[SHORTCUT_BAKE], shortcut::FOCUSED)) frames_bake();
      if (shortcut(manager.chords[SHORTCUT_FIT], shortcut::FOCUSED)) fit_animation_length();

      auto make_region = [&]()
      {
        auto targetReference = reference;
        auto frame = frame_get();
        if (!frame || targetReference.itemType != LAYER || targetReference.itemID == -1) return;
        if (frame->regionId != -1) return;
        auto layer = layer_get(targetReference.itemID);
        if (!layer) return;

        auto spritesheetID = layer->spritesheetId;
        if (!spritesheet_get(spritesheetID)) return;

        auto settingsPtr = &settings;
        command_push([=](Manager& manager, Document& document)
                     {
                       auto frame = command_frame_get(document, targetReference);
                       if (!frame || frame->regionId != -1) return;
                       auto layer = command_layer_get(document, targetReference.itemID);
                       if (!layer) return;

                       auto spritesheetID = layer->spritesheetId;
                       if (!command_spritesheet_get(document, spritesheetID)) return;

                       auto region = element_make(ElementType::REGION);
                       region.crop = frame->crop;
                       region.size = frame->size;
                       region.pivot = frame->pivot;
                       region.origin = Origin::CUSTOM;

                       document.spritesheet.reference = spritesheetID;
                       settingsPtr->windowIsRegions = true;
                       manager.makeRegionSpritesheetId = spritesheetID;
                       manager.makeRegion = region;
                       manager.isMakeRegionRequested = true;
                     });
      };

      auto item = item_get(reference.itemType, reference.itemID);
      auto frame = frame_get();
      auto selectedFrames = frame_references_for_current_get();
      auto selectedBakeFrames = selectedFrames;
      std::erase_if(selectedBakeFrames,
                    [](const Reference& frameReference) { return frameReference.itemType == TRIGGER; });
      bool isMakeRegion = frame && reference.itemType == LAYER && reference.itemID != -1 && frame->regionId == -1 &&
                          layer_get(reference.itemID) && spritesheet_get(layer_get(reference.itemID)->spritesheetId);
      Actions actions{};
      actions_undo_redo_add(actions, manager, document);
      actions.separator();
      actions.add({.label = playback.isPlaying ? LABEL_PAUSE : LABEL_PLAY,
                   .shortcut = SHORTCUT_PLAY_PAUSE,
                   .isEnabled = []() { return true; },
                   .run = [&]() { playback.toggle(); }});
      actions.add({.label = LABEL_INSERT,
                   .shortcut = SHORTCUT_INSERT_FRAME,
                   .isEnabled = [&]() { return item; },
                   .run = [&]() { frame_insert(); }});
      actions.add({.label = LABEL_DELETE,
                   .shortcut = SHORTCUT_REMOVE,
                   .isEnabled = [=]() { return !selectedFrames.empty(); },
                   .run = [&]() { frames_delete_action(); }});
      actions.add({.label = LABEL_BAKE,
                   .shortcut = SHORTCUT_BAKE,
                   .isEnabled = [=]() { return !selectedBakeFrames.empty(); },
                   .run = [&]() { frames_bake(); }});
      actions.add({.label = LABEL_FIT_ANIMATION_LENGTH,
                   .shortcut = SHORTCUT_FIT,
                   .isEnabled = [&]() { return animation && animation->frameNum != animation_length_get(*animation); },
                   .run = [&]() { fit_animation_length(); }});
      actions.add({.label = LABEL_SPLIT,
                   .shortcut = SHORTCUT_SPLIT,
                   .isEnabled = [=]() { return selectedBakeFrames.size() == 1; },
                   .run = [&]() { frame_split(); }});
      actions.add({.label = LABEL_MAKE_REGION,
                   .shortcut = -1,
                   .isEnabled = [&]() { return isMakeRegion; },
                   .run = [&]() { make_region(); }});
      actions.separator();
      actions.add(ACTION_CUT, [=]() { return !selectedFrames.empty(); }, cut);
      actions.add(ACTION_COPY, [=]() { return !selectedFrames.empty(); }, copy);
      actions.add(ACTION_PASTE, [&]() { return !clipboard.is_empty(); }, paste);
      actions_context_window_draw("##Context Menu", actions, settings);

      ImGui::PopStyleVar(2);
    };

    auto item_base_properties_open = [&](int type, int id)
    {
      switch (type)
      {
        case LAYER:
          manager.layer_properties_open(id);
          break;
        case NULL_:
          manager.null_properties_open(id);
        default:
          break;
      };
    };

    auto group_properties_close = [&]()
    {
      groupName.clear();
      groupAnimationIndex = -1;
      groupType = NONE;
      groupId = -1;
      groupPropertiesPopup.close();
    };

    auto group_properties_open = [&](const TimelineItemRow& row, const Element& group)
    {
      groupName = group.name.empty() ? std::string(localize.get(TEXT_NEW_GROUP)) : group.name;
      groupAnimationIndex = reference.animationIndex;
      groupType = row.type;
      groupId = row.id;
      groupPropertiesPopup.open();
    };

    auto group_properties_update = [&]()
    {
      groupPropertiesPopup.trigger();

      if (ImGui::BeginPopupModal(groupPropertiesPopup.label(), &groupPropertiesPopup.isOpen, ImGuiWindowFlags_NoResize))
      {
        auto childSize = child_size_get(1);
        if (ImGui::BeginChild("##Group Properties Child", childSize, ImGuiChildFlags_Borders))
        {
          if (groupPropertiesPopup.isJustOpened) ImGui::SetKeyboardFocusHere();
          input_text_string(localize.get(BASIC_NAME), &groupName);
          ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_ITEM_NAME));
        }
        ImGui::EndChild();

        auto widgetSize = widget_size_with_row_get(2);

        shortcut(manager.chords[SHORTCUT_CONFIRM]);
        if (ImGui::Button(localize.get(BASIC_CONFIRM), widgetSize))
        {
          auto targetName = groupName;
          auto targetAnimationIndex = groupAnimationIndex;
          auto targetType = groupType;
          auto targetId = groupId;
          edit_command_push(EDIT_RENAME_GROUP, Document::ITEMS,
                            [=](Manager&, Document& document)
                            {
                              auto animation = command_animation_get(document, targetAnimationIndex);
                              auto container =
                                  animation ? element_child_first_get(*animation, container_type_get(targetType))
                                            : nullptr;
                              auto group = container ? element_child_id_get(*container, ElementType::GROUP, targetId)
                                                     : nullptr;
                              if (!group) return;
                              group->name = targetName;
                            });
          group_properties_close();
        }

        ImGui::SameLine();

        shortcut(manager.chords[SHORTCUT_CANCEL]);
        if (ImGui::Button(localize.get(BASIC_CANCEL), widgetSize)) group_properties_close();

        ImGui::EndPopup();
      }

      groupPropertiesPopup.end();
    };

    auto item_context_menu = [&]()
    {
      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, style.WindowPadding);
      ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, style.ItemSpacing);

      auto& type = reference.itemType;
      auto& id = reference.itemID;
      auto item = selected_item_get();
      auto selectedItems = item_references_for_current_get();
      auto selectedRows = selected_row_references_get();
      auto selectedGroupableItems = item_references_groupable_get();
      TimelineItemRow selectedGroupRow{};
      Element* selectedGroup{};
      if (selectedRows.size() == 1 && selectedRows.front().isGroup)
      {
        auto row = selectedRows.front();
        selectedGroupRow = {.type = row.type, .id = row.id, .index = row.index, .isGroup = true};
        selectedGroup = row_group_get(selectedGroupRow);
      }
      auto isRemoveAvailable = std::ranges::any_of(selectedRows, [](const TimelineRowReference& row)
      { return row.type == LAYER || row.type == NULL_; });

      if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenOverlappedByWindow) &&
          ImGui::IsMouseClicked(ImGuiMouseButton_Right))
        ImGui::OpenPopup("##Items Context Menu");

      Actions actions{};
      actions_undo_redo_add(actions, manager, document);
      actions.separator();
      actions.add(ACTION_PROPERTIES, [&]() { return selectedGroup || (item && (type == LAYER || type == NULL_)); },
                  [&]()
                  {
                    if (selectedGroup)
                      group_properties_open(selectedGroupRow, *selectedGroup);
                    else
                      item_base_properties_open(type, id);
                  });
      actions.add(ACTION_ADD, [&]() { return animation; }, [&]() { itemProperties.open(); });
      actions.add(ACTION_REMOVE, [=]() { return isRemoveAvailable; }, [&]() { item_remove(); });
      actions.add(ACTION_GROUP, [=]() { return !selectedGroupableItems.empty(); }, [&]() { item_group(); });
      actions_popup_draw("##Items Context Menu", actions, settings);

      ImGui::PopStyleVar(2);
    };

    auto item_child = [&](const TimelineItemRow& row, int index)
    {
      ImGui::PushID(index);

      auto type = row.type;
      auto id = row.id;
      if (row.isGroup)
      {
        auto group = row_group_get(row);
        if (!group)
        {
          ImGui::PopID();
          return;
        }

        auto label = group->name.empty() ? std::string(localize.get(TEXT_NEW_GROUP)) : group->name;
        auto itemSize = ImVec2(ImGui::GetContentRegionAvail().x, rowFrameChildHeight);
        auto isGroupVisible = group->isVisible;
        auto colorVec = item_color_vec(type);
        if (is_group_selected(row))
        {
          if (isLightTheme)
            colorVec = ITEM_COLOR_LIGHT_SELECTED[type_index(type)];
          else
            colorVec = item_color_active_vec(type);
        }
        auto color = to_imvec4(isGroupVisible ? colorVec : colorVec * COLOR_HIDDEN_MULTIPLIER);
        ImGui::PushStyleColor(ImGuiCol_ChildBg, color);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, style.ItemSpacing);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, style.WindowPadding);

        if (ImGui::BeginChild("##Group Child", itemSize, ImGuiChildFlags_Borders, ImGuiWindowFlags_NoScrollWithMouse))
        {
          auto cursorPos = ImGui::GetCursorPos();
          auto groupChildMin = ImGui::GetWindowPos();
          auto groupChildMax = ImVec2(groupChildMin.x + ImGui::GetWindowSize().x,
                                      groupChildMin.y + ImGui::GetWindowSize().y);

          auto toggle_group = [&]()
          {
            auto targetRow = row;
            auto targetAnimationIndex = reference.animationIndex;
            edit_command_push(EDIT_TOGGLE_GROUP_EXPANDED, Document::ITEMS,
                              [=](Manager&, Document& document) mutable
                              {
                                auto animation = command_animation_get(document, targetAnimationIndex);
                                auto container =
                                    animation ? element_child_first_get(*animation, container_type_get(targetRow.type))
                                              : nullptr;
                                auto group = container ? element_child_id_get(*container, ElementType::GROUP,
                                                                              targetRow.id)
                                                       : nullptr;
                                if (!group) return;
                                group->isExpanded = !group->isExpanded;
                              });
          };

          ImGui::SetCursorPos(to_imvec2(to_vec2(cursorPos) - to_vec2(style.ItemSpacing)));
          ImGui::SetNextItemAllowOverlap();
          ImGui::InvisibleButton("##Group Button", itemSize);
          auto groupButtonMin = ImGui::GetItemRectMin();
          auto groupButtonMax = ImGui::GetItemRectMax();
          auto mousePos = ImGui::GetIO().MousePos;
          auto is_mouse_in_rect = [](ImVec2 mouse, ImVec2 min, ImVec2 max)
          { return mouse.x >= min.x && mouse.x < max.x && mouse.y >= min.y && mouse.y < max.y; };
          auto isGroupHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) &&
                                is_mouse_in_rect(mousePos, groupButtonMin, groupButtonMax);
          bool isGroupTooltipDelayed{};
          if (isGroupHovered)
          {
            auto& imguiStyle = ImGui::GetStyle();
            auto previousTooltipDelay = imguiStyle.HoverDelayNormal;
            imguiStyle.HoverDelayNormal = FRAME_TOOLTIP_HOVER_DELAY;
            isGroupTooltipDelayed =
                ImGui::IsItemHovered(ImGuiHoveredFlags_Stationary | ImGuiHoveredFlags_DelayNormal |
                                     ImGuiHoveredFlags_AllowWhenDisabled | ImGuiHoveredFlags_NoSharedDelay);
            imguiStyle.HoverDelayNormal = previousTooltipDelay;
          }

          if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
          {
            rowDragReferences = row_drag_references_get(row);
            ImGui::SetDragDropPayload("Timeline Row Drag Drop", rowDragReferences.data(),
                                      (int)rowDragReferences.size() * (int)sizeof(TimelineRowReference));
            row_drag_tooltip_draw(rowDragReferences);
            ImGui::EndDragDropSource();
          }

          if (ImGui::BeginDragDropTarget())
          {
            if (auto payload = ImGui::AcceptDragDropPayload(
                    "Timeline Row Drag Drop",
                    ImGuiDragDropFlags_AcceptBeforeDelivery | ImGuiDragDropFlags_AcceptNoDrawDefaultRect))
            {
              auto isDropAfter = is_drop_after(groupButtonMin, groupButtonMax);
              auto payloadRows = (TimelineRowReference*)payload->Data;
              auto payloadCount = payload->DataSize / sizeof(TimelineRowReference);
              std::vector<TimelineRowReference> draggedRows(payloadRows, payloadRows + payloadCount);
              auto isDropIntoGroup = isDropAfter && (row.type == LAYER || row.type == NULL_);
              for (auto draggedRow : draggedRows)
                if (draggedRow.isGroup || draggedRow.type != row.type) isDropIntoGroup = false;

              if (isDropIntoGroup)
                drop_box_draw(ImGui::GetWindowDrawList(), groupChildMin, groupChildMax);
              else
                drop_line_draw(ImGui::GetWindowDrawList(), groupChildMin, groupChildMax, isDropAfter);

              if (payload->IsDelivery()) rows_move_to_row(draggedRows, row, isDropAfter);
            }
            ImGui::EndDragDropTarget();
          }

          ImGui::SetCursorPos(cursorPos);
          auto folderIcon = group->isExpanded ? icon::FOLDER_OPEN : icon::FOLDER;
          ImGui::Image(resources.icons[folderIcon].id, icon_size_get());
          auto iconMin = ImGui::GetItemRectMin();
          auto iconMax = ImGui::GetItemRectMax();
          overlay_icon(resources.icons[folderIcon].id, itemIconTint);
          ImGui::SameLine();

          ImGui::PushStyleColor(ImGuiCol_Text, itemTextColor);
          ImGui::TextUnformatted(label.c_str());
          ImGui::PopStyleColor();

          ImGui::PushStyleColor(ImGuiCol_Button, ImVec4());
          ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4());
          ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4());
          ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2());
          ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);

          ImGui::SetCursorPos(
              ImVec2(itemSize.x - ImGui::GetTextLineHeightWithSpacing() - ImGui::GetStyle().ItemSpacing.x,
                     (itemSize.y - ImGui::GetTextLineHeightWithSpacing()) / 2));
          int visibleIcon = isGroupVisible ? icon::VISIBLE : icon::INVISIBLE;
          if (ImGui::ImageButton("##Group Visible Toggle", resources.icons[visibleIcon].id, icon_size_get()))
          {
            auto targetAnimationIndex = reference.animationIndex;
            auto targetType = type;
            auto targetGroupId = group->id;
            auto targetVisible = !isGroupVisible;
            edit_command_push(EDIT_TOGGLE_ITEM_VISIBILITY, Document::FRAMES,
                              [=](Manager&, Document& document)
                              {
                                auto animation = command_animation_get(document, targetAnimationIndex);
                                auto container =
                                    animation ? element_child_first_get(*animation, container_type_get(targetType))
                                              : nullptr;
                                if (!container) return;
                                auto group = element_child_id_get(*container, ElementType::GROUP, targetGroupId);
                                if (!group) return;
                                group->isVisible = targetVisible;
                              });
          }
          auto visibleButtonMin = ImGui::GetItemRectMin();
          auto visibleButtonMax = ImGui::GetItemRectMax();
          overlay_icon(resources.icons[visibleIcon].id, itemIconTint);
          ImGui::SetItemTooltip("%s", isGroupVisible ? localize.get(TOOLTIP_ITEM_VISIBILITY_SHOWN)
                                                     : localize.get(TOOLTIP_ITEM_VISIBILITY_HIDDEN));
          ImGui::PopStyleVar(2);
          ImGui::PopStyleColor(3);

          auto isIconHovered = isGroupHovered && is_mouse_in_rect(mousePos, iconMin, iconMax);
          auto isVisibleButtonHovered = isGroupHovered && is_mouse_in_rect(mousePos, visibleButtonMin, visibleButtonMax);
          if (isGroupTooltipDelayed && !isVisibleButtonHovered)
          {
            ImGui::BeginTooltip();
            ImGui::PushFont(resources.fonts[font::BOLD].get(), font::SIZE);
            ImGui::TextUnformatted(label.c_str());
            ImGui::PopFont();
            ImGui::TextUnformatted(std::vformat(localize.get(FORMAT_ID), std::make_format_args(group->id)).c_str());
            auto groupItemsCount = group_items_count_get(type, group->id);
            ImGui::TextUnformatted(
                std::vformat(localize.get(FORMAT_ITEMS_COUNT), std::make_format_args(groupItemsCount)).c_str());
            ImGui::EndTooltip();
          }
          if (isIconHovered && ImGui::IsMouseReleased(ImGuiMouseButton_Left) &&
              !ImGui::IsMouseDragPastThreshold(ImGuiMouseButton_Left))
            toggle_group();
          else if (isGroupHovered && !isIconHovered && !isVisibleButtonHovered &&
                   ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            group_properties_open(row, *group);
          else if (isGroupHovered && !isIconHovered && !isVisibleButtonHovered &&
                   ImGui::IsMouseReleased(ImGuiMouseButton_Left) && !ImGui::IsMouseDragPastThreshold(ImGuiMouseButton_Left))
            row_selection_set(row);
        }
        ImGui::EndChild();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor();
        ImGui::PopID();
        return;
      }

      auto item = item_get(type, id);
      if (type != NONE && !item)
      {
        ImGui::PopID();
        return;
      }
      auto isItemVisible = item ? item->isVisible : false;
      auto isVisible = item ? item->isVisible && is_track_group_visible(type, row.groupId) : false;
      auto& isOnlyShowLayers = settings.timelineIsOnlyShowLayers;
      if (isOnlyShowLayers && type != LAYER) isVisible = false;
      auto isReferenced = reference.itemType == type && reference.itemID == id;
      auto isItemSelected = is_row_selected(row);

      auto label = [&]() -> std::string
      {
        if (type == LAYER)
        {
          auto layer = layer_get(id);
          if (!layer) return localize.get(TYPE_STRINGS[type]);
          return std::vformat(localize.get(FORMAT_LAYER), std::make_format_args(id, layer->name, layer->spritesheetId));
        }
        if (type == NULL_)
        {
          auto null = null_get(id);
          if (!null) return localize.get(TYPE_STRINGS[type]);
          return std::vformat(localize.get(FORMAT_NULL), std::make_format_args(id, null->name));
        }
        return localize.get(TYPE_STRINGS[type]);
      }();
      auto icon = TYPE_ICONS[type];
      auto iconTintCurrent = isLightTheme && type == NONE ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f) : itemIconTint;
      auto baseColorVec = item_color_vec(type);
      auto activeColorVec = item_color_active_vec(type);
      auto colorVec = baseColorVec;
      if (isItemSelected && type != NONE)
      {
        if (isLightTheme)
          colorVec = ITEM_COLOR_LIGHT_SELECTED[type_index(type)];
        else
          colorVec = activeColorVec;
      }
      auto color = to_imvec4(colorVec);
      color = !isVisible ? to_imvec4(colorVec * COLOR_HIDDEN_MULTIPLIER) : color;
      ImGui::PushStyleColor(ImGuiCol_ChildBg, color);

      ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, style.ItemSpacing);
      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, style.WindowPadding);

      auto itemSize = ImVec2(ImGui::GetContentRegionAvail().x, rowFrameChildHeight);

      if (ImGui::BeginChild(label.c_str(), itemSize, ImGuiChildFlags_Borders, ImGuiWindowFlags_NoScrollWithMouse))
      {
        auto cursorPos = ImGui::GetCursorPos();
        auto itemChildMin = ImGui::GetWindowPos();
        auto itemChildMax = ImVec2(itemChildMin.x + ImGui::GetWindowSize().x, itemChildMin.y + ImGui::GetWindowSize().y);

        if (type != NONE)
        {
          ImGui::SetCursorPos(to_imvec2(to_vec2(cursorPos) - to_vec2(style.ItemSpacing)));
          ImGui::SetNextItemAllowOverlap();
          ImGui::SetNextItemStorageID(id);
          ImGui::InvisibleButton("##Item Button", itemSize);
          auto itemButtonMin = ImGui::GetItemRectMin();
          auto itemButtonMax = ImGui::GetItemRectMax();
          auto isItemButtonHovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);

          if (type == LAYER || type == NULL_)
          {
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
            {
              rowDragReferences = row_drag_references_get(row);
              ImGui::SetDragDropPayload("Timeline Row Drag Drop", rowDragReferences.data(),
                                        (int)rowDragReferences.size() * (int)sizeof(TimelineRowReference));
              row_drag_tooltip_draw(rowDragReferences);
              ImGui::EndDragDropSource();
            }

            if (ImGui::BeginDragDropTarget())
            {
              if (auto payload = ImGui::AcceptDragDropPayload(
                      "Timeline Row Drag Drop",
                      ImGuiDragDropFlags_AcceptBeforeDelivery | ImGuiDragDropFlags_AcceptNoDrawDefaultRect))
              {
                auto isDropAfter = is_drop_after(itemButtonMin, itemButtonMax);
                drop_line_draw(ImGui::GetWindowDrawList(), itemChildMin, itemChildMax, isDropAfter);

                auto payloadRows = (TimelineRowReference*)payload->Data;
                auto payloadCount = payload->DataSize / sizeof(TimelineRowReference);
                std::vector<TimelineRowReference> draggedRows(payloadRows, payloadRows + payloadCount);
                if (payload->IsDelivery()) rows_move_to_row(draggedRows, row, isDropAfter);
              }
              ImGui::EndDragDropTarget();
            }
          }

          if (isItemButtonHovered)
          {
            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) item_base_properties_open(type, id);
            else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) &&
                     !ImGui::IsMouseDragPastThreshold(ImGuiMouseButton_Left))
              row_selection_set(row);

            auto& imguiStyle = ImGui::GetStyle();
            auto previousTooltipFlags = imguiStyle.HoverFlagsForTooltipMouse;
            auto previousTooltipDelay = imguiStyle.HoverDelayNormal;
            imguiStyle.HoverFlagsForTooltipMouse = ImGuiHoveredFlags_Stationary | ImGuiHoveredFlags_DelayNormal |
                                                   ImGuiHoveredFlags_AllowWhenDisabled |
                                                   ImGuiHoveredFlags_NoSharedDelay;
            imguiStyle.HoverDelayNormal = FRAME_TOOLTIP_HOVER_DELAY;
            bool showItemTooltip = ImGui::BeginItemTooltip();
            imguiStyle.HoverFlagsForTooltipMouse = previousTooltipFlags;
            imguiStyle.HoverDelayNormal = previousTooltipDelay;

            if (showItemTooltip)
            {
              auto yesNoLabel = [&](bool value) { return value ? localize.get(BASIC_YES) : localize.get(BASIC_NO); };
              auto visibleLabel = yesNoLabel(isVisible);
              auto framesCount = item_frames_count(item);

              switch (type)
              {
                case ROOT:
                {
                  ImGui::PushFont(resources.fonts[font::BOLD].get(), font::SIZE);
                  ImGui::TextUnformatted(localize.get(BASIC_ROOT));
                  ImGui::PopFont();

                  ImGui::TextUnformatted(
                      std::vformat(localize.get(FORMAT_VISIBLE), std::make_format_args(visibleLabel)).c_str());
                  auto transformLabel = yesNoLabel(settings.previewIsRootTransform);
                  ImGui::TextUnformatted(
                      std::vformat(localize.get(FORMAT_TRANSFORM), std::make_format_args(transformLabel)).c_str());
                  ImGui::TextUnformatted(
                      std::vformat(localize.get(FORMAT_FRAMES_COUNT), std::make_format_args(framesCount)).c_str());
                  break;
                }
                case LAYER:
                {
                  auto layer = layer_get(id);
                  if (!layer) break;
                  ImGui::PushFont(resources.fonts[font::BOLD].get(), font::SIZE);
                  ImGui::TextUnformatted(layer->name.c_str());
                  ImGui::PopFont();

                  ImGui::TextUnformatted(std::vformat(localize.get(FORMAT_ID), std::make_format_args(id)).c_str());
                  ImGui::TextUnformatted(
                      std::vformat(localize.get(FORMAT_SPRITESHEET_ID), std::make_format_args(layer->spritesheetId))
                          .c_str());
                  ImGui::TextUnformatted(
                      std::vformat(localize.get(FORMAT_VISIBLE), std::make_format_args(visibleLabel)).c_str());
                  ImGui::TextUnformatted(
                      std::vformat(localize.get(FORMAT_FRAMES_COUNT), std::make_format_args(framesCount)).c_str());
                  break;
                }
                case NULL_:
                {
                  auto nullInfo = null_get(id);
                  if (!nullInfo) break;
                  ImGui::PushFont(resources.fonts[font::BOLD].get(), font::SIZE);
                  ImGui::TextUnformatted(nullInfo->name.c_str());
                  ImGui::PopFont();

                  auto rectLabel = yesNoLabel(nullInfo->isShowRect);
                  ImGui::TextUnformatted(std::vformat(localize.get(FORMAT_ID), std::make_format_args(id)).c_str());
                  ImGui::TextUnformatted(
                      std::vformat(localize.get(FORMAT_RECT), std::make_format_args(rectLabel)).c_str());
                  ImGui::TextUnformatted(
                      std::vformat(localize.get(FORMAT_VISIBLE), std::make_format_args(visibleLabel)).c_str());
                  ImGui::TextUnformatted(
                      std::vformat(localize.get(FORMAT_FRAMES_COUNT), std::make_format_args(framesCount)).c_str());
                  break;
                }
                case TRIGGER:
                {
                  ImGui::PushFont(resources.fonts[font::BOLD].get(), font::SIZE);
                  ImGui::TextUnformatted(localize.get(BASIC_TRIGGERS));
                  ImGui::PopFont();

                  ImGui::TextUnformatted(
                      std::vformat(localize.get(FORMAT_VISIBLE), std::make_format_args(visibleLabel)).c_str());
                  ImGui::TextUnformatted(
                      std::vformat(localize.get(FORMAT_TRIGGERS_COUNT), std::make_format_args(framesCount)).c_str());
                  break;
                }
                default:
                  break;
              }

              ImGui::EndTooltip();
            }
          }

          auto contentCursorPos = cursorPos;
          contentCursorPos.x += (float)row.depth * ImGui::GetTextLineHeightWithSpacing();
          ImGui::SetCursorPos(contentCursorPos);

          ImGui::Image(resources.icons[icon].id, icon_size_get());
          overlay_icon(resources.icons[icon].id, iconTintCurrent);
          ImGui::SameLine();
          if (isReferenced) ImGui::PushFont(resources.fonts[font::ITALICS].get(), font::SIZE);
          ImGui::PushStyleColor(ImGuiCol_Text, itemTextColor);
          ImGui::TextUnformatted(label.c_str());
          ImGui::PopStyleColor();
          if (isReferenced) ImGui::PopFont();

          ImGui::PushStyleColor(ImGuiCol_Button, ImVec4());
          ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4());
          ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4());
          ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2());
          ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);

          ImGui::SetCursorPos(
              ImVec2(itemSize.x - ImGui::GetTextLineHeightWithSpacing() - ImGui::GetStyle().ItemSpacing.x,
                     (itemSize.y - ImGui::GetTextLineHeightWithSpacing()) / 2));
          int visibleIcon = isItemVisible ? icon::VISIBLE : icon::INVISIBLE;
          if (ImGui::ImageButton("##Visible Toggle", resources.icons[visibleIcon].id, icon_size_get()))
          {
            auto animationIndex = reference.animationIndex;
            auto targetType = type;
            auto targetID = id;
            edit_command_push(EDIT_TOGGLE_ITEM_VISIBILITY, Document::FRAMES,
                              [=](Manager&, Document& document)
                              {
                                auto item = command_item_get(document, animationIndex, targetType, targetID);
                                if (!item) return;
                                item->isVisible = !item->isVisible;
                              });
          }
          overlay_icon(resources.icons[visibleIcon].id, iconTintCurrent);
          ImGui::SetItemTooltip("%s", isItemVisible ? localize.get(TOOLTIP_ITEM_VISIBILITY_SHOWN)
                                                    : localize.get(TOOLTIP_ITEM_VISIBILITY_HIDDEN));

          if (type == NULL_)
          {
            if (auto null = null_get(id))
            {
              auto& isShowRect = null->isShowRect;
              auto rectIcon = isShowRect ? icon::SHOW_RECT : icon::HIDE_RECT;
              ImGui::SetCursorPos(
                  ImVec2(itemSize.x - (ImGui::GetTextLineHeightWithSpacing() * 2) - ImGui::GetStyle().ItemSpacing.x,
                         (itemSize.y - ImGui::GetTextLineHeightWithSpacing()) / 2));
              if (ImGui::ImageButton("##Rect Toggle", resources.icons[rectIcon].id, icon_size_get()))
              {
                auto nullID = id;
                edit_command_push(EDIT_TOGGLE_NULL_RECT, Document::FRAMES,
                                  [=](Manager&, Document& document)
                                  {
                                    auto nulls = document.anm2.element_get(ElementType::NULLS);
                                    auto null = nulls ? element_child_id_get(*nulls, ElementType::NULL_ELEMENT, nullID)
                                                      : nullptr;
                                    if (!null) return;
                                    null->isShowRect = !null->isShowRect;
                                  });
              }
              overlay_icon(resources.icons[rectIcon].id, iconTintCurrent);
              ImGui::SetItemTooltip("%s", isShowRect ? localize.get(TOOLTIP_NULL_RECT_SHOWN)
                                                     : localize.get(TOOLTIP_NULL_RECT_HIDDEN));
            }
          }

          ImGui::PopStyleVar(2);
          ImGui::PopStyleColor(3);
        }
        else
        {
          auto cursorPos = ImGui::GetCursorPos();
          ImGui::PushStyleColor(ImGuiCol_Button, ImVec4());
          ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4());
          ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4());
          ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2());
          ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
          ImGui::SetCursorPos(
              ImVec2(itemSize.x - ImGui::GetTextLineHeightWithSpacing() - ImGui::GetStyle().ItemSpacing.x,
                     (itemSize.y - ImGui::GetTextLineHeightWithSpacing()) / 2));

          auto& isShowUnused = settings.timelineIsShowUnused;
          auto unusedIcon = isShowUnused ? icon::SHOW_UNUSED : icon::HIDE_UNUSED;
          if (ImGui::ImageButton("##Unused Toggle", resources.icons[unusedIcon].id, icon_size_get()))
            isShowUnused = !isShowUnused;
          overlay_icon(resources.icons[unusedIcon].id, iconTintCurrent);
          ImGui::SetItemTooltip("%s", isShowUnused ? localize.get(TOOLTIP_UNUSED_ITEMS_SHOWN)
                                                   : localize.get(TOOLTIP_UNUSED_ITEMS_HIDDEN));

          auto& showLayersOnly = settings.timelineIsOnlyShowLayers;
          auto layersIcon = showLayersOnly ? icon::SHOW_LAYERS : icon::HIDE_LAYERS;
          ImGui::SetCursorPos(
              ImVec2(itemSize.x - (ImGui::GetTextLineHeightWithSpacing() * 2) - ImGui::GetStyle().ItemSpacing.x,
                     (itemSize.y - ImGui::GetTextLineHeightWithSpacing()) / 2));
          if (ImGui::ImageButton("##Layers Toggle", resources.icons[layersIcon].id, icon_size_get()))
            showLayersOnly = !showLayersOnly;
          overlay_icon(resources.icons[layersIcon].id, iconTintCurrent);
          ImGui::SetItemTooltip("%s", showLayersOnly ? localize.get(TOOLTIP_ONLY_LAYERS_VISIBLE)
                                                     : localize.get(TOOLTIP_ALL_ITEMS_VISIBLE));
          ImGui::PopStyleVar(2);
          ImGui::PopStyleColor(3);

          ImGui::SetCursorPos(cursorPos);

          ImGui::BeginDisabled();
          ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
          ImGui::Text("(?)");
          ImGui::PopStyleColor();
          auto tooltipShortcuts = std::vformat(
              localize.get(TOOLTIP_TIMELINE_SHORTCUTS),
              std::make_format_args(settings.shortcutMovePlayheadBack, settings.shortcutMovePlayheadForward,
                                    settings.shortcutShortenFrame, settings.shortcutExtendFrame,
                                    settings.shortcutPreviousFrame, settings.shortcutNextFrame,
                                    settings.shortcutPreviousItem, settings.shortcutNextItem));
          ImGui::SetItemTooltip("%s", tooltipShortcuts.c_str());
          ImGui::EndDisabled();
        }
      }
      ImGui::EndChild();
      ImGui::PopStyleColor();
      ImGui::PopStyleVar(2);

      ImGui::PopID();
    };

    auto items_child = [&]()
    {
      auto itemsChildSize = ImVec2(ITEM_CHILD_WIDTH, ImGui::GetContentRegionAvail().y);

      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2());
      bool isItemsChildOpen = ImGui::BeginChild("##Items Child", itemsChildSize, ImGuiChildFlags_Borders);
      ImGui::PopStyleVar();
      if (isItemsChildOpen)
      {
        auto itemsListChildSize = ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y -
                                                                               ImGui::GetTextLineHeightWithSpacing() -
                                                                               ImGui::GetStyle().ItemSpacing.y * 2);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2());
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2());
        if (ImGui::BeginChild("##Items List Child", itemsListChildSize, ImGuiChildFlags_Borders,
                              ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar))
        {
          if (animation && ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_A, ImGuiInputFlags_RouteFocused))
          {
            auto rowReferences = timeline_row_references_get();
            row_selection_clear();
            for (auto rowReference : rowReferences)
              row_selection_insert(rowReference);
            if (!rowReferences.empty())
            {
              rowSelectionAnchor = rowReferences.front();
              isRowSelectionAnchorSet = true;
            }
            frames_selection_reset_for(document);
          }

          if (animation && shortcut(manager.chords[SHORTCUT_GROUP], shortcut::FOCUSED) &&
              !item_references_groupable_get().empty())
            item_group();

          ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2());
          ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 0.0f);
          if (ImGui::BeginTable("##Item Table", 1, ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY))
          {
            ImGui::GetCurrentWindow()->Flags |= ImGuiWindowFlags_NoScrollWithMouse;
            ImGui::SetScrollY(scroll.y);

            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableSetupColumn("##Items");

            auto item_child_row = [&](const TimelineItemRow& row, int index)
            {
              ImGui::TableNextRow();
              ImGui::TableSetColumnIndex(0);
              item_child(row, index);
            };

            int index{};
            item_child_row({.type = NONE}, index++);
            if (animation)
              for (const auto& row : timeline_item_rows_get())
                item_child_row(row, index++);

            if (isHorizontalScroll && ImGui::GetCurrentWindow()->ScrollbarY)
            {
              ImGui::TableNextRow();
              ImGui::TableSetColumnIndex(0);
              ImGui::Dummy(ImVec2(0, style.ScrollbarSize));
            }

            ImGui::EndTable();
          }
          ImGui::PopStyleVar(2);

          item_context_menu();
        }
        ImGui::PopStyleVar(2);
        ImGui::EndChild();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, style.WindowPadding);

        ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() + style.WindowPadding.x, ImGui::GetCursorPosY()));
        auto widgetSize = widget_size_with_row_get(2, ImGui::GetContentRegionAvail().x - style.WindowPadding.x);

        ImGui::BeginDisabled(!animation);
        {
          shortcut(manager.chords[SHORTCUT_ADD]);
          if (ImGui::Button(localize.get(BASIC_ADD), widgetSize)) itemProperties.open();
          set_item_tooltip_shortcut(localize.get(TOOLTIP_ADD_ITEM), settings.shortcutAdd);
          ImGui::SameLine();

          auto selectedRows = selected_row_references_get();
          auto isRemoveAvailable = std::ranges::any_of(selectedRows, [](const TimelineRowReference& row)
          { return row.type == LAYER || row.type == NULL_; });
          ImGui::BeginDisabled(!isRemoveAvailable);
          shortcut(manager.chords[SHORTCUT_REMOVE]);
          if (ImGui::Button(localize.get(BASIC_REMOVE), widgetSize)) item_remove();
          set_item_tooltip_shortcut(localize.get(TOOLTIP_REMOVE_ITEMS), settings.shortcutRemove);
          ImGui::EndDisabled();
        }
        ImGui::EndDisabled();

        ImGui::PopStyleVar();
      }
      ImGui::EndChild();
    };

    int frameMoveDropType = NONE;
    int frameMoveDropItemID = -1;
    int frameMoveDropIndex = -1;
    bool isFrameMoveDropTarget = false;

    auto frame_move_drag_clear = [&]()
    {
      frameMoveDrag = {};
      frameSelectionLocked.clear();
    };

    auto frames_move_to = [&](int targetType, int targetID, int insertIndex)
    {
      if (!frameMoveDrag.isActive || !animation || frameMoveDrag.animationIndex != reference.animationIndex) return;
      if (targetType == TRIGGER) return;

      auto drag = frameMoveDrag;
      std::erase_if(drag.references, [](const Reference& frameReference)
                    { return frameReference.itemType == TRIGGER || frameReference.frameIndex < 0; });
      if (drag.references.empty() && drag.frameIndex >= 0)
        drag.references.push_back({drag.animationIndex, drag.type, drag.itemID, drag.frameIndex});
      std::erase_if(drag.references, [](const Reference& frameReference)
                    { return frameReference.itemType == TRIGGER || frameReference.frameIndex < 0; });
      if (drag.references.empty()) return;

      edit_command_push(EDIT_MOVE_FRAMES, Document::FRAMES,
                        [=, this](Manager&, Document& document) mutable
                        {
                          auto targetItem = command_item_get(document, drag.animationIndex, targetType, targetID);
                          if (!targetItem) return;

                          std::map<Reference, std::set<int>> groupedFrames{};
                          for (auto frameReference : drag.references)
                          {
                            auto itemReference = item_reference_from_frame_get(frameReference);
                            groupedFrames[itemReference].insert(frameReference.frameIndex);
                          }

                          int removedBeforeTarget = 0;
                          std::vector<Element> movedFrames;
                          for (auto& [itemReference, indices] : groupedFrames)
                          {
                            auto sourceItem = command_item_get(document, itemReference.animationIndex,
                                                               itemReference.itemType, itemReference.itemID);
                            if (!sourceItem) continue;

                            for (auto i : indices)
                            {
                              if (i < 0 || i >= (int)sourceItem->children.size()) continue;
                              movedFrames.push_back(std::move(sourceItem->children[i]));
                              if (itemReference.itemType == targetType && itemReference.itemID == targetID &&
                                  i < insertIndex)
                                ++removedBeforeTarget;
                            }

                            for (auto it = indices.rbegin(); it != indices.rend(); ++it)
                            {
                              auto i = *it;
                              if (i >= 0 && i < (int)sourceItem->children.size())
                                sourceItem->children.erase(sourceItem->children.begin() + i);
                            }
                          }

                          if (movedFrames.empty()) return;

                          int desired = std::clamp(insertIndex, 0, (int)targetItem->children.size());
                          desired -= removedBeforeTarget;
                          desired = std::clamp(desired, 0, (int)targetItem->children.size());

                          auto insertPosResult = desired;
                          auto insertedCount = (int)movedFrames.size();
                          targetItem->children.insert(targetItem->children.begin() + insertPosResult,
                                                      std::make_move_iterator(movedFrames.begin()),
                                                      std::make_move_iterator(movedFrames.end()));

                          if (insertedCount <= 0) return;

                          document.frames.selection.clear();
                          document.frames.references.clear();
                          for (int offset = 0; offset < insertedCount; ++offset)
                          {
                            document.frames.selection.insert(insertPosResult + offset);
                            document.frames.references.insert(
                                {drag.animationIndex, targetType, targetID, insertPosResult + offset});
                          }

                          document.reference = {drag.animationIndex, targetType, targetID, insertPosResult};
                          document.frameTime = frame_time_from_index_get(*targetItem, document.reference.frameIndex);
                          frameSelectionSnapshot.assign(document.frames.selection.begin(),
                                                        document.frames.selection.end());
                          frameSelectionSnapshotReference = document.reference;
                          frameSelectionLocked.clear();
                          isFrameSelectionLocked = false;
                          frameFocusIndex = document.reference.frameIndex;
                          frameFocusRequested = true;
                          if (targetType == LAYER)
                            if (auto layer = command_layer_get(document, targetID))
                              document.spritesheet.reference = layer->spritesheetId;
                        });
    };

    float playheadLineCenterX{};
    float playheadLineTopY{};
    bool isPlayheadLineSet{};
    ImVec2 frameBoxClipMin{};
    ImVec2 frameBoxClipMax{};
    bool isFrameBoxClipSet{};
    auto frame_box_content_point_get = [&]()
    {
      auto mousePos = ImGui::GetIO().MousePos;
      return ImVec2(mousePos.x + scroll.x - frameBoxClipMin.x, mousePos.y + scroll.y - frameBoxClipMin.y);
    };
    auto frame_box_screen_point_get = [&](ImVec2 point)
    { return ImVec2(point.x - scroll.x + frameBoxClipMin.x, point.y - scroll.y + frameBoxClipMin.y); };
    auto is_frame_box_overlapping = [](ImVec2 leftMin, ImVec2 leftMax, ImVec2 rightMin, ImVec2 rightMax)
    {
      return leftMin.x <= rightMax.x && leftMax.x >= rightMin.x && leftMin.y <= rightMax.y &&
             leftMax.y >= rightMin.y;
    };
    auto frame_overlay_draw = [&](ImDrawList* drawList, ImVec2 clipMin, ImVec2 clipMax)
    {
      if (isFrameBoxSelecting && isFrameBoxClipSet)
      {
        auto boxContentMin =
            ImVec2(std::min(frameBoxStart.x, frameBoxEnd.x), std::min(frameBoxStart.y, frameBoxEnd.y));
        auto boxContentMax =
            ImVec2(std::max(frameBoxStart.x, frameBoxEnd.x), std::max(frameBoxStart.y, frameBoxEnd.y));
        auto boxMin = frame_box_screen_point_get(boxContentMin);
        auto boxMax = frame_box_screen_point_get(boxContentMax);
        auto boxClipMin = clipMin;
        if (isPlayheadLineSet) boxClipMin.y = std::max(boxClipMin.y, playheadLineTopY);
        drawList->PushClipRect(boxClipMin, clipMax, true);
        drawList->AddRectFilled(boxMin, boxMax, ImGui::GetColorU32(ImGuiCol_DragDropTargetBg));
        drawList->AddRect(boxMin, boxMax, ImGui::GetColorU32(ImGuiCol_DragDropTarget));
        drawList->PopClipRect();
      }

      if (isPlayheadLineSet)
      {
        auto lineTopY = std::max(clipMin.y, playheadLineTopY);
        if (clipMax.y > lineTopY)
        {
          auto linePos = ImVec2(playheadLineCenterX - (PLAYHEAD_LINE_THICKNESS * 0.5f), lineTopY);
          auto lineSize = ImVec2(PLAYHEAD_LINE_THICKNESS * 0.5f, clipMax.y - lineTopY);
          drawList->PushClipRect(clipMin, clipMax, true);
          drawList->AddRectFilled(linePos, ImVec2(linePos.x + lineSize.x, linePos.y + lineSize.y),
                                  ImGui::GetColorU32(playheadLineColor));
          drawList->PopClipRect();
        }
      }
    };

    auto frame_child = [&](const TimelineItemRow& row, int& index, float width)
    {
      auto type = row.type;
      auto id = row.id;
      auto childSize = ImVec2(width, rowFrameChildHeight);
      if (row.isGroup)
      {
        auto group = row_group_get(row);
        if (!group) return;

        ImGui::PushID(index);
        ImGui::BeginChild("##Frames Group Child", childSize, ImGuiChildFlags_None,
                          ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar);
        ImGui::EndChild();
        index++;
        ImGui::PopID();
        return;
      }

      auto item = item_get(type, id);
      if (type != NONE && !item) return;

      auto isVisible = item ? item->isVisible && is_track_group_visible(type, row.groupId) : false;
      auto& isOnlyShowLayers = settings.timelineIsOnlyShowLayers;
      if (isOnlyShowLayers && type != LAYER) isVisible = false;

      auto colorVec = type_color_base_vec(type);
      auto colorActiveVec = type_color_active_vec(type);
      auto colorHoveredVec = type_color_hovered_vec(type);
      auto color = to_imvec4(colorVec);
      auto colorActive = to_imvec4(colorActiveVec);
      auto colorHovered = to_imvec4(colorHoveredVec);
      auto colorHidden = to_imvec4(colorVec * COLOR_HIDDEN_MULTIPLIER);
      auto colorActiveHidden = to_imvec4(colorActiveVec * COLOR_HIDDEN_MULTIPLIER);
      auto colorHoveredHidden = to_imvec4(colorHoveredVec * COLOR_HIDDEN_MULTIPLIER);

      ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, style.ItemSpacing);
      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, style.WindowPadding);

      ImGui::PopStyleVar(2);

      ImGui::PushID(index);

      ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2());
      bool isDefaultChild = type == NONE;
      if (isLightTheme && isDefaultChild) ImGui::PushStyleColor(ImGuiCol_ChildBg, TIMELINE_CHILD_BG_COLOR_LIGHT);

      bool isFramesChildVisible = ImGui::BeginChild("##Frames Child", childSize, ImGuiChildFlags_Borders);
      auto drawList = ImGui::GetWindowDrawList();
      auto clipMax = drawList->GetClipRectMax();
      auto length = animation ? animation->frameNum : 0;
      auto frameSize = ImVec2(ImGui::GetTextLineHeight(), ImGui::GetContentRegionAvail().y);
      auto framesSize = ImVec2(frameSize.x * length, frameSize.y);
      auto cursorPos = ImGui::GetCursorPos();
      auto cursorScreenPos = ImGui::GetCursorScreenPos();
      auto border = glm::max(0.5f, ImGui::GetStyle().FrameBorderSize * 0.5f);
      auto borderLineLength = frameSize.y / 5;
      auto frameMin = std::max(0, (int)std::floor(scroll.x / frameSize.x) - 1);
      auto frameMax = std::min(FRAME_NUM_MAX, (int)std::ceil((scroll.x + clipMax.x) / frameSize.x) + 1);

      if (isFrameBoxSelecting && isFrameBoxClipSet && type != NONE && animation && item)
      {
        auto boxMin = ImVec2(std::min(frameBoxStart.x, frameBoxEnd.x), std::min(frameBoxStart.y, frameBoxEnd.y));
        auto boxMax = ImVec2(std::max(frameBoxStart.x, frameBoxEnd.x), std::max(frameBoxStart.y, frameBoxEnd.y));
        auto rowMinY = cursorScreenPos.y + scroll.y - frameBoxClipMin.y;
        auto rowMaxY = rowMinY + childSize.y;
        float selectionFrameTime{};
        for (auto [i, frame] : std::views::enumerate(item->children))
        {
          auto frameReference = Reference{reference.animationIndex, type, id, (int)i};
          auto frameStart = type == TRIGGER ? frame.atFrame : selectionFrameTime;
          auto frameEnd = type == TRIGGER ? frameStart + 1.0f : frameStart + frame.duration;
          auto frameContentMin = ImVec2(frameStart * frameSize.x, rowMinY);
          auto frameContentMax = ImVec2(frameEnd * frameSize.x, rowMaxY);
          if (is_frame_box_overlapping(frameContentMin, frameContentMax, boxMin, boxMax))
            frameBoxSelection.insert(frameReference);
          if (type != TRIGGER) selectionFrameTime += frame.duration;
        }
      }

      if (isFramesChildVisible)
      {
        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
            ImGui::Shortcut(ImGuiKey_Escape, ImGuiInputFlags_RouteFocused))
        {
          if (!frames.references.empty() || !frames.selection.empty())
          {
            reference.frameIndex = -1;
            frames_selection_reset_for(document);
          }
          else if (reference.itemType != NONE || reference.itemID != -1)
            reference_clear();
        }

        if (type == NONE)
        {
          if (length > 0)
          {
            if (isLightTheme)
            {
              auto totalMax = ImVec2(cursorScreenPos.x + framesSize.x, cursorScreenPos.y + framesSize.y);
              drawList->AddRectFilled(cursorScreenPos, totalMax, ImGui::GetColorU32(timelineBackgroundColor));
              float animationWidth = std::min(framesSize.x, frameSize.x * (float)length);
              drawList->AddRectFilled(cursorScreenPos,
                                      ImVec2(cursorScreenPos.x + animationWidth, cursorScreenPos.y + framesSize.y),
                                      ImGui::GetColorU32(timelinePlayheadRectColor));
            }
            else
            {
              drawList->AddRectFilled(cursorScreenPos,
                                      ImVec2(cursorScreenPos.x + framesSize.x, cursorScreenPos.y + framesSize.y),
                                      ImGui::GetColorU32(ImGui::GetStyleColorVec4(ImGuiCol_Header)));
            }
          }

          for (int i = frameMin; i < frameMax; i++)
          {
            auto frameScreenPos = ImVec2(cursorScreenPos.x + frameSize.x * (float)i, cursorScreenPos.y);

            drawList->AddRect(frameScreenPos, ImVec2(frameScreenPos.x + border, frameScreenPos.y + borderLineLength),
                              ImGui::GetColorU32(timelineTickColor), 0, 0, 0.5f);

            drawList->AddRect(ImVec2(frameScreenPos.x, frameScreenPos.y + frameSize.y - borderLineLength),
                              ImVec2(frameScreenPos.x + border, frameScreenPos.y + frameSize.y),
                              ImGui::GetColorU32(timelineTickColor), 0, 0, 0.5);

            if (i % FRAME_MULTIPLE == 0)
            {
              auto string = std::to_string(i);
              auto textSize = ImGui::CalcTextSize(string.c_str());
              auto textPos = ImVec2(frameScreenPos.x + (frameSize.x - textSize.x) / 2,
                                    frameScreenPos.y + (frameSize.y - textSize.y) / 2);

              drawList->AddRectFilled(frameScreenPos,
                                      ImVec2(frameScreenPos.x + frameSize.x, frameScreenPos.y + frameSize.y),
                                      ImGui::GetColorU32(frameMultipleOverlayColor));

              drawList->AddText(textPos, ImGui::GetColorU32(textMultipleColor), string.c_str());
            }
          }

          if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) &&
              ImGui::IsMouseClicked(ImGuiMouseButton_Left))
          {
            playback_stop();
            isDragging = true;
          }

          auto childPos = ImGui::GetWindowPos();
          auto mousePos = ImGui::GetIO().MousePos;
          auto localMousePos = ImVec2(mousePos.x - childPos.x, mousePos.y - childPos.y);
          hoveredTime = floorf(localMousePos.x / frameSize.x);

          if (isDragging)
          {
            playback.time = hoveredTime;
            playback.clamp(settings.playbackIsClamp ? length : FRAME_NUM_MAX);
            document.frameTime = playback.time;
          }

          if (!playback.isPlaying) playback.clamp(settings.playbackIsClamp ? length : FRAME_NUM_MAX);

          if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) isDragging = false;

          if (length > 0)
          {
            ImGui::SetCursorPos(ImVec2(cursorPos.x + frameSize.x * floorf(playback.time), cursorPos.y));
            ImGui::Image(resources.icons[icon::PLAYHEAD].id, frameSize);
            auto playheadMin = ImGui::GetItemRectMin();
            auto playheadMax = ImGui::GetItemRectMax();
            playheadLineCenterX = (playheadMin.x + playheadMax.x) * 0.5f;
            playheadLineTopY = playheadMax.y;
            isPlayheadLineSet = true;
            overlay_icon(resources.icons[icon::PLAYHEAD].id, playheadIconTint, true);
          }
        }
        else if (animation)
        {
          float frameTime{};

          for (int i = frameMin; i < frameMax; i++)
          {
            auto frameScreenPos = ImVec2(cursorScreenPos.x + frameSize.x * (float)i, cursorScreenPos.y);
            auto frameRectMax = ImVec2(frameScreenPos.x + frameSize.x, frameScreenPos.y + frameSize.y);

            drawList->AddRect(frameScreenPos, frameRectMax, ImGui::GetColorU32(frameBorderColor));

            if (i % FRAME_MULTIPLE == 0)
              drawList->AddRectFilled(frameScreenPos, frameRectMax, ImGui::GetColorU32(frameMultipleOverlayColor));
          }

          if (!frameMoveDrag.isActive && !ImGui::IsKeyDown(ImGuiMod_Shift) &&
              ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) &&
              ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemHovered())
          {
            isFrameBoxPending = true;
            isFrameBoxAdditive = ImGui::IsKeyDown(ImGuiMod_Ctrl);
            frameBoxStart = frame_box_content_point_get();
            frameBoxEnd = frameBoxStart;
            frameBoxSelection.clear();
          }

          bool isFrameMovePreview = false;
          ImVec2 frameMovePreviewMin{};
          ImVec2 frameMovePreviewMax{};
          bool isFrameMoveHoveredFrame = false;
          ImVec2 frameMoveHoveredFrameMin{};
          ImVec2 frameMoveHoveredFrameMax{};

          if (frameMoveDrag.isActive && type != TRIGGER)
          {
            auto mousePos = ImGui::GetIO().MousePos;
            auto rowMin = cursorScreenPos;
            auto rowMax = ImVec2(cursorScreenPos.x + width, cursorScreenPos.y + frameSize.y);
            if (mousePos.x >= rowMin.x && mousePos.x < rowMax.x && mousePos.y >= rowMin.y && mousePos.y < rowMax.y)
            {
              auto mouseX = mousePos.x - cursorScreenPos.x;
              auto targetTime = glm::max(0.0f, mouseX / frameSize.x);
              int dropIndex = (int)item->children.size();
              float dropFrameTime{};
              float frameTime{};

              for (auto [i, frame] : std::views::enumerate(item->children))
              {
                auto frameStart = frameTime;
                auto frameEnd = frameStart + frame.duration;
                if (!isFrameMoveHoveredFrame && targetTime >= frameStart && targetTime < frameEnd)
                {
                  isFrameMoveHoveredFrame = true;
                  frameMoveHoveredFrameMin = ImVec2(cursorScreenPos.x + frameStart * frameSize.x, cursorScreenPos.y);
                  frameMoveHoveredFrameMax =
                      ImVec2(cursorScreenPos.x + frameEnd * frameSize.x, cursorScreenPos.y + frameSize.y);
                }

                auto midpoint = frameStart + ((float)frame.duration * 0.5f);
                if (targetTime < midpoint)
                {
                  dropIndex = (int)i;
                  dropFrameTime = frameStart;
                  break;
                }

                frameTime = frameEnd;
                dropFrameTime = frameTime;
              }

              frameMoveDropType = type;
              frameMoveDropItemID = id;
              frameMoveDropIndex = dropIndex;
              isFrameMoveDropTarget = true;

              auto dropX = cursorScreenPos.x + dropFrameTime * frameSize.x;
              auto previewWidth = glm::max(frameSize.x, (float)frameMoveDrag.duration * frameSize.x);
              frameMovePreviewMin = ImVec2(dropX, cursorScreenPos.y);
              frameMovePreviewMax = ImVec2(dropX + previewWidth, cursorScreenPos.y + frameSize.y);
              isFrameMovePreview = true;
            }
          }

          for (auto [i, frame] : std::views::enumerate(item->children))
          {
            ImGui::PushID((int)i);

            auto frameReference = Reference{reference.animationIndex, type, id, (int)i};
            auto isFrameVisible = isVisible && frame.isVisible;
            auto isReferenced = reference == frameReference;
            auto isSelected = frames.references.contains(frameReference) ||
                              (frames.references.empty() && frames.selection.contains((int)i) &&
                               reference.itemType == type && reference.itemID == id);

            if (type == TRIGGER) frameTime = frame.atFrame;

            auto buttonSize = type == TRIGGER ? frameSize : to_imvec2(vec2(frameSize.x * frame.duration, frameSize.y));
            auto frameStart = type == TRIGGER ? frame.atFrame : frameTime;
            auto frameEnd = type == TRIGGER ? frameStart + 1.0f : frameStart + frame.duration;
            if (frameEnd <= (float)frameMin || frameStart >= (float)frameMax)
            {
              if (type != TRIGGER) frameTime += frame.duration;
              ImGui::PopID();
              continue;
            }
            auto buttonPos = ImVec2(cursorPos.x + (frameTime * frameSize.x), cursorPos.y);

            if (frameFocusRequested && frameFocusIndex == (int)i && reference == frameReference)
              frameFocusRequested = false;

            ImGui::SetCursorPos(buttonPos);

            auto buttonScreenPos = ImGui::GetCursorScreenPos();
            auto fillColor = isSelected ? (isFrameVisible ? colorActive : colorActiveHidden)
                                        : (isFrameVisible ? color : colorHidden);
            drawList->AddRectFilled(buttonScreenPos,
                                    ImVec2(buttonScreenPos.x + buttonSize.x, buttonScreenPos.y + buttonSize.y),
                                    ImGui::GetColorU32(fillColor), FRAME_ROUNDING);

            ImGui::PushStyleColor(ImGuiCol_Header, isFrameVisible ? colorActive : colorActiveHidden);
            ImGui::PushStyleColor(ImGuiCol_HeaderActive, isFrameVisible ? colorActive : colorActiveHidden);
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, isFrameVisible ? colorHovered : colorHoveredHidden);
            ImGui::PushStyleColor(ImGuiCol_NavCursor, isFrameVisible ? colorHovered : colorHoveredHidden);

            ImGui::SetNextItemAllowOverlap();
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, FRAME_ROUNDING);
            ImGui::SetNextItemSelectionUserData((int)i);
            bool isDifferentItem = reference.itemType != type || reference.itemID != id;
            if (ImGui::Selectable("##Frame Button", isSelected, ImGuiSelectableFlags_None, buttonSize))
            {
              if (type == LAYER)
                if (auto layer = layer_get(id)) document.spritesheet.reference = layer->spritesheetId;

              if (type != TRIGGER)
              {
                if (ImGui::IsKeyDown(ImGuiMod_Alt))
                {
                  auto targetReference = frameReference;
                  edit_command_push(EDIT_FRAME_INTERPOLATION, Document::FRAMES,
                                    [=](Manager&, Document& document)
                                    {
                                      auto frame = command_frame_get(document, targetReference);
                                      if (!frame) return;
                                      frame->interpolation = frame->interpolation == Interpolation::NONE
                                                                 ? Interpolation::LINEAR
                                                                 : Interpolation::NONE;
                                    });
                }

                document.frameTime = frameTime;
              }

              auto isCtrlDown = ImGui::IsKeyDown(ImGuiMod_Ctrl);
              auto isShiftDown = ImGui::IsKeyDown(ImGuiMod_Shift);
              if (isShiftDown)
              {
                auto isHadAnchor = isFrameSelectionAnchorSet;
                auto anchorReference = isHadAnchor ? frameSelectionAnchor : frameReference;
                auto isRangeSelected =
                    frame_selection_range_set_for(document, anchorReference, frameReference, isCtrlDown);
                if (!isRangeSelected) frame_selection_set_for(document, frameReference);
                if (!isHadAnchor || !isRangeSelected) frameSelectionAnchor = frameReference;
                isFrameSelectionAnchorSet = true;
              }
              else if (isCtrlDown)
              {
                frame_selection_toggle_for(document, frameReference);
                frameSelectionAnchor = frameReference;
                isFrameSelectionAnchorSet = true;
              }
              else
              {
                frame_selection_set_for(document, frameReference);
                frameSelectionAnchor = frameReference;
                isFrameSelectionAnchorSet = true;
              }
              reference = frameReference;
              isReferenced = true;
              region.reference = -1;
              region.selection.clear();
              if (isDifferentItem) frames_selection_sync_for(document);
            }
            ImGui::PopStyleVar();

            ImGui::PopStyleColor(4);

            if (!isDraggedFrameActive && ImGui::IsItemHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Left))
            {
              if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
              {
                if (type == TRIGGER || ImGui::IsKeyDown(ImGuiMod_Ctrl))
                {
                  isDraggedFrameActive = true;
                  draggedFrameReference = frameReference;
                  draggedFrameType = type;
                  draggedFrameIndex = (int)i;
                  draggedFrameStart = hoveredTime;
                  if (type != TRIGGER) draggedFrameStartDuration = frame.duration;
                  draggedFrameStartMouseX = ImGui::GetIO().MousePos.x;
                  draggedFrameWidth = frameSize.x;
                }
              }
            }

            if (type != TRIGGER)
            {
              if (!isDraggedFrameActive && !frameMoveDrag.isActive && ImGui::IsItemActive() &&
                  ImGui::IsMouseDragging(ImGuiMouseButton_Left))
              {
                auto selectedReferences = frame_references_for_current_get();
                if (!selectedReferences.contains(frameReference)) selectedReferences = {frameReference};
                std::erase_if(selectedReferences, [](const Reference& selectedReference)
                              { return selectedReference.itemType == TRIGGER; });
                if (selectedReferences.empty()) selectedReferences = {frameReference};
                int dragDuration = 0;
                for (auto selectedReference : selectedReferences)
                {
                  auto selectedItem = item_get(selectedReference.itemType, selectedReference.itemID);
                  auto selectedFrame =
                      selectedItem ? track_frame_get(*selectedItem, selectedReference.frameIndex) : nullptr;
                  if (selectedFrame) dragDuration += selectedFrame->duration;
                }
                dragDuration = glm::max(1, dragDuration);

                frameMoveDrag = {
                    .type = type,
                    .itemID = id,
                    .animationIndex = reference.animationIndex,
                    .frameIndex = (int)i,
                    .duration = dragDuration,
                    .indices = {},
                    .references = {selectedReferences.begin(), selectedReferences.end()},
                    .isActive = true,
                };
              }
            }

            auto rectMin = ImGui::GetItemRectMin();
            auto rectMax = ImGui::GetItemRectMax();
            auto borderColor = isReferenced ? frameBorderColorReferenced : frameBorderColor;
            auto borderThickness = isReferenced ? FRAME_BORDER_THICKNESS_REFERENCED : FRAME_BORDER_THICKNESS;
            drawList->AddRect(rectMin, rectMax, ImGui::GetColorU32(borderColor), FRAME_ROUNDING, 0, borderThickness);

            auto icon = icon::UNINTERPOLATED;
            if (type == TRIGGER)
              icon = icon::TRIGGER;
            else
            {
              switch (frame.interpolation)
              {
                case Interpolation::NONE:
                  icon = icon::UNINTERPOLATED;
                  break;
                case Interpolation::LINEAR:
                  icon = icon::INTERPOLATED;
                  break;
                case Interpolation::EASE_IN:
                  icon = icon::EASE_IN;
                  break;
                case Interpolation::EASE_OUT:
                  icon = icon::EASE_OUT;
                  break;
                case Interpolation::EASE_IN_OUT:
                  icon = icon::EASE_IN_OUT;
                  break;
                default:
                  icon = icon::UNINTERPOLATED;
                  break;
              }
            }
            auto iconPos = ImVec2(cursorPos.x + (frameTime * frameSize.x),
                                  cursorPos.y + (frameSize.y / 2) - (icon_size_get().y / 2));
            ImGui::SetCursorPos(iconPos);
            ImGui::Image(resources.icons[icon].id, icon_size_get());
            overlay_icon(resources.icons[icon].id, iconTintDefault);

            if (type != TRIGGER) frameTime += frame.duration;

            ImGui::PopID();
          }

          if (isFrameMovePreview)
          {
            drawList->AddRectFilled(frameMovePreviewMin, frameMovePreviewMax,
                                    ImGui::GetColorU32(ImGuiCol_DragDropTargetBg), FRAME_ROUNDING);
            drawList->AddRect(frameMovePreviewMin, frameMovePreviewMax, ImGui::GetColorU32(ImGuiCol_DragDropTarget),
                              FRAME_ROUNDING, 0, ImGui::GetStyle().DragDropTargetBorderSize);
          }

          if (isFrameMoveHoveredFrame)
            drawList->AddRect(frameMoveHoveredFrameMin, frameMoveHoveredFrameMax,
                              ImGui::GetColorU32(ImGuiCol_DragDropTarget), FRAME_ROUNDING, 0,
                              ImGui::GetStyle().DragDropTargetBorderSize * 1.5f);

          if (!isFrameBoxSelecting && !frameMoveDrag.isActive && !isDraggedFrameActive && ImGui::IsWindowHovered() &&
              (ImGui::IsMouseReleased(ImGuiMouseButton_Left) || ImGui::IsMouseReleased(ImGuiMouseButton_Right)) &&
              !ImGui::IsAnyItemHovered())
            row_selection_set(row);

          if (isFrameSelectionLocked)
          {
            frames.selection.clear();
            frames.references.clear();
            for (int idx : frameSelectionLocked)
            {
              frames.selection.insert(idx);
              frames.references.insert({reference.animationIndex, type, id, idx});
            }
            isFrameSelectionLocked = false;
            frameSelectionLocked.clear();
          }
          if (reference.itemType == type && reference.itemID == id)
          {
            frameSelectionSnapshot.assign(frames.selection.begin(), frames.selection.end());
            frameSelectionSnapshotReference = reference;
          }
        }

      }

      if (isDraggedFrameActive)
      {
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        auto durationDelta = draggedFrameWidth > 0.0f
                                 ? static_cast<int>((ImGui::GetIO().MousePos.x - draggedFrameStartMouseX) /
                                                    draggedFrameWidth)
                                 : hoveredTime - draggedFrameStart;
        auto isDraggedFrameChanged =
            draggedFrameType == TRIGGER ? hoveredTime != draggedFrameStart : durationDelta != 0;

        if (!isDraggedFrameSnapshot && isDraggedFrameChanged)
        {
          isDraggedFrameSnapshot = true;
          snapshot_command_push(draggedFrameType == TRIGGER ? EDIT_TRIGGER_AT_FRAME : EDIT_FRAME_DURATION);
        }

        if (isDraggedFrameSnapshot)
        {
          auto targetReference = draggedFrameReference;
          auto targetType = draggedFrameType;
          auto targetIndex = draggedFrameIndex;
          auto targetStartDuration = draggedFrameStartDuration;
          auto targetHoveredTime = hoveredTime;
          auto targetDurationDelta = durationDelta;
          auto isPlaybackClamp = settings.playbackIsClamp;
          auto animationLength = animation ? animation->frameNum : FRAME_NUM_MAX;
          command_push([=](Manager&, Document& document)
                       {
                         auto item = command_item_get(document, targetReference.animationIndex,
                                                      targetReference.itemType, targetReference.itemID);
                         auto frame = command_frame_get(document, targetReference);
                         if (!item || !frame) return;

                         if (targetType == TRIGGER)
                         {
                           frame->atFrame =
                               glm::clamp(targetHoveredTime, 0,
                                          isPlaybackClamp ? animationLength - 1 : FRAME_NUM_MAX - 1);

                           for (auto [i, trigger] : std::views::enumerate(item->children))
                           {
                             if ((int)i == targetIndex) continue;
                             if (trigger.atFrame == frame->atFrame) frame->atFrame--;
                           }
                         }
                         else
                         {
                           frame->duration = glm::clamp(targetStartDuration + targetDurationDelta, FRAME_DURATION_MIN,
                                                        FRAME_DURATION_MAX);
                         }
                       });
        }

        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        {
          auto targetReference = draggedFrameReference;
          auto targetType = draggedFrameType;
          command_push([=](Manager&, Document& document)
                       {
                         auto item = command_item_get(document, targetReference.animationIndex,
                                                      targetReference.itemType, targetReference.itemID);
                         if (targetType == TRIGGER && item) frames_sort_by_at_frame(*item);
                         document.anm2_change(Document::FRAMES);
                       });
          isDraggedFrameActive = false;
          draggedFrameReference = {};
          draggedFrameType = NONE;
          draggedFrameIndex = -1;
          draggedFrameStart = -1;
          draggedFrameStartDuration = -1;
          draggedFrameStartMouseX = 0.0f;
          draggedFrameWidth = 0.0f;
          isDraggedFrameSnapshot = false;
          frameSelectionLocked.clear();
        }
      }

      context_menu();

      ImGui::EndChild();
      if (isLightTheme && isDefaultChild) ImGui::PopStyleColor();
      ImGui::PopStyleVar();

      index++;
      ImGui::PopID();
    };

    auto frames_child = [&]()
    {
      auto cursorPos = ImGui::GetCursorPos();
      ImGui::SetCursorPos(ImVec2(cursorPos.x + ITEM_CHILD_WIDTH, cursorPos.y));

      auto framesChildSize = ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y);

      if (ImGui::BeginChild("##Frames Child", framesChildSize, ImGuiChildFlags_Borders))
      {
        auto viewListChildSize =
            ImVec2(ImGui::GetContentRegionAvail().x,
                   ImGui::GetContentRegionAvail().y - ImGui::GetTextLineHeightWithSpacing() - style.ItemSpacing.y * 2);

        auto animationsLength = [&]()
        {
          int length{};
          if (auto animations = anm2.element_get(ElementType::ANIMATIONS))
            for (auto& item : animations->children)
              if (item.type == ElementType::ANIMATION) length = std::max(length, animation_length_get(item));
          return length;
        }();
        auto childWidth = animationsLength * ImGui::GetTextLineHeight();
        if (animation && animation->frameNum > animationsLength)
          childWidth = animation->frameNum * ImGui::GetTextLineHeight();
        childWidth = std::max(childWidth, ImGui::GetContentRegionAvail().x);

        childWidth *= WIDTH_MULTIPLIER;

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2());
        if (ImGui::BeginChild("##Frames List Child", viewListChildSize, true, ImGuiWindowFlags_HorizontalScrollbar))
        {
          playheadLineCenterX = 0.0f;
          playheadLineTopY = 0.0f;
          isPlayheadLineSet = false;
          isFrameBoxClipSet = false;
          if (animation && ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_A, ImGuiInputFlags_RouteFocused))
          {
            group_selection_reset();
            document.frames.references = all_frame_references_for_items_get();
            if (!document.frames.references.empty())
            {
              reference = *document.frames.references.begin();
              frames_selection_sync_for(document);
            }
          }

          ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2());
          if (ImGui::BeginTable("##Frames List Table", 1,
                                ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY))
          {
            ImGuiWindow* window = ImGui::GetCurrentWindow();
            window->Flags |= ImGuiWindowFlags_NoScrollWithMouse;

            scroll.x = ImGui::GetScrollX();
            scroll.y = ImGui::GetScrollY();

            isHorizontalScroll = window->ScrollbarX;

            if (isWindowHovered)
            {
              auto& io = ImGui::GetIO();
              auto lineHeight = ImGui::GetTextLineHeightWithSpacing() * 2;

              scroll.x -= io.MouseWheelH * lineHeight;
              scroll.y -= io.MouseWheel * lineHeight;
            }

            ImGui::SetScrollX(scroll.x);
            ImGui::SetScrollY(scroll.y);

            auto frameBoxDrawList = ImGui::GetWindowDrawList();
            frameBoxClipMin = frameBoxDrawList->GetClipRectMin();
            frameBoxClipMax = frameBoxDrawList->GetClipRectMax();
            isFrameBoxClipSet = true;
            if (isFrameBoxPending || isFrameBoxSelecting)
            {
              auto& io = ImGui::GetIO();
              auto threshold = ImGui::GetTextLineHeightWithSpacing() * 0.125f;
              frameBoxEnd = frame_box_content_point_get();
              auto distance = ImVec2(frameBoxEnd.x - frameBoxStart.x, frameBoxEnd.y - frameBoxStart.y);
              if (isFrameBoxPending && distance.x * distance.x + distance.y * distance.y >= threshold * threshold)
              {
                isFrameBoxPending = false;
                isFrameBoxSelecting = true;
              }
              auto edgeSize = ImGui::GetTextLineHeightWithSpacing();
              auto scrollStep = edgeSize * 0.5f;
              if (isFrameBoxSelecting)
              {
                if (io.MousePos.x < frameBoxClipMin.x + edgeSize)
                  scroll.x -= scrollStep;
                else if (io.MousePos.x > frameBoxClipMax.x - edgeSize)
                  scroll.x += scrollStep;
                if (io.MousePos.y < frameBoxClipMin.y + edgeSize)
                  scroll.y -= scrollStep;
                else if (io.MousePos.y > frameBoxClipMax.y - edgeSize)
                  scroll.y += scrollStep;
                ImGui::SetScrollX(scroll.x);
                ImGui::SetScrollY(scroll.y);
                frameBoxEnd = frame_box_content_point_get();
                frameBoxSelection.clear();
              }
            }

            int index{};

            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableSetupColumn("##Frames");

            auto frames_child_row = [&](const TimelineItemRow& row)
            {
              ImGui::TableNextRow();
              ImGui::TableSetColumnIndex(0);
              frame_child(row, index, childWidth);
            };

            frames_child_row({.type = NONE});

            if (animation)
            {
              ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);

              for (const auto& row : timeline_item_rows_get())
                frames_child_row(row);

              ImGui::PopStyleVar();
            }
            ImGui::EndTable();
          }

          if (isFrameBoxClipSet)
          {
            auto overlayCursor = ImGui::GetCursorScreenPos();
            auto overlayPos = ImGui::GetWindowPos();
            auto overlaySize = ImGui::GetWindowSize();
            ImGui::SetCursorScreenPos(overlayPos);
            if (ImGui::BeginChild("##Frames Overlay Child", overlaySize, ImGuiChildFlags_None,
                                  ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoBackground |
                                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                                      ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoNavInputs))
              frame_overlay_draw(ImGui::GetWindowDrawList(), frameBoxClipMin, frameBoxClipMax);
            ImGui::EndChild();
            ImGui::SetCursorScreenPos(overlayCursor);
          }

          if (isFrameBoxPending || isFrameBoxSelecting)
          {
            if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
            {
              if (isFrameBoxSelecting)
              {
                group_selection_reset();
                if (isFrameBoxAdditive)
                  document.frames.references.insert(frameBoxSelection.begin(), frameBoxSelection.end());
                else
                  document.frames.references = frameBoxSelection;
                document.items.references.clear();
                for (auto frameReference : document.frames.references)
                  document.items.references.insert(item_reference_from_frame_get(frameReference));
                if (!document.frames.references.empty()) reference = *document.frames.references.begin();
                frames_selection_sync_for(document);
              }
              isFrameBoxPending = false;
              isFrameBoxSelecting = false;
              frameBoxSelection.clear();
            }
          }

          ImGui::PopStyleVar();
        }
        ImGui::EndChild();
        ImGui::PopStyleVar();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, style.WindowPadding);

        ImGui::SetCursorPos(
            ImVec2(ImGui::GetStyle().WindowPadding.x, ImGui::GetCursorPos().y + ImGui::GetStyle().ItemSpacing.y));

        auto widgetSize = widget_size_with_row_get(10);

        ImGui::BeginDisabled(!animation);
        {
          auto label = playback.isPlaying ? localize.get(LABEL_PAUSE) : localize.get(LABEL_PLAY);
          auto tooltip =
              playback.isPlaying ? localize.get(TOOLTIP_PAUSE_ANIMATION) : localize.get(TOOLTIP_PLAY_ANIMATION);

          shortcut(manager.chords[SHORTCUT_PLAY_PAUSE]);
          if (ImGui::Button(label, widgetSize)) playback.toggle();
          set_item_tooltip_shortcut(tooltip, settings.shortcutPlayPause);

          ImGui::SameLine();

          auto item = selected_item_get();
          auto selectedFrames = frame_references_for_current_get();
          auto selectedBakeFrames = selectedFrames;
          std::erase_if(selectedBakeFrames, [](const Reference& frameReference)
                        { return frameReference.itemType == TRIGGER; });

          ImGui::BeginDisabled(!item);
          {
            shortcut(manager.chords[SHORTCUT_INSERT_FRAME]);
            if (ImGui::Button(localize.get(LABEL_INSERT), widgetSize)) frame_insert();
            set_item_tooltip_shortcut(localize.get(TOOLTIP_INSERT_FRAME), settings.shortcutInsertFrame);

            ImGui::SameLine();

            ImGui::BeginDisabled(selectedFrames.empty());
            {
              shortcut(manager.chords[SHORTCUT_REMOVE]);
              if (ImGui::Button(localize.get(LABEL_DELETE), widgetSize)) frames_delete_action();
              set_item_tooltip_shortcut(localize.get(TOOLTIP_DELETE_FRAMES), settings.shortcutRemove);

              ImGui::SameLine();

              ImGui::BeginDisabled(selectedBakeFrames.empty());
              if (ImGui::Button(localize.get(LABEL_BAKE), widgetSize)) bakePopup.open();
              set_item_tooltip_shortcut(localize.get(TOOLTIP_BAKE_FRAMES), settings.shortcutBake);
              ImGui::EndDisabled();
            }
            ImGui::EndDisabled();
          }
          ImGui::EndDisabled();

          ImGui::SameLine();

          ImGui::BeginDisabled(!animation || animation->frameNum == animation_length_get(*animation));
          shortcut(manager.chords[SHORTCUT_FIT]);
          if (ImGui::Button(localize.get(LABEL_FIT_ANIMATION_LENGTH), widgetSize)) fit_animation_length();
          set_item_tooltip_shortcut(localize.get(TOOLTIP_FIT_ANIMATION_LENGTH), settings.shortcutFit);
          ImGui::EndDisabled();

          ImGui::SameLine();

          auto frameNum = animation ? animation->frameNum : dummy_value<int>();
          ImGui::SetNextItemWidth(widgetSize.x);
          if (input_int_range(localize.get(LABEL_ANIMATION_LENGTH), frameNum, FRAME_NUM_MIN, FRAME_NUM_MAX, STEP,
                              STEP_FAST, !animation ? ImGuiInputTextFlags_DisplayEmptyRefVal : 0) &&
              animation)
          {
            auto animationIndex = reference.animationIndex;
            edit_command_push(EDIT_ANIMATION_LENGTH, Document::ANIMATIONS,
                              [=](Manager&, Document& document)
                              {
                                auto animation = command_animation_get(document, animationIndex);
                                if (!animation) return;
                                animation->frameNum = frameNum;
                              });
          }
          ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_ANIMATION_LENGTH));

          ImGui::SameLine();

          auto isLoop = animation ? animation->isLoop : dummy_value<bool>();
          ImGui::SetNextItemWidth(widgetSize.x);
          if (ImGui::Checkbox(localize.get(LABEL_LOOP), &isLoop) && animation)
          {
            auto animationIndex = reference.animationIndex;
            edit_command_push(EDIT_LOOP, Document::ANIMATIONS,
                              [=](Manager&, Document& document)
                              {
                                auto animation = command_animation_get(document, animationIndex);
                                if (!animation) return;
                                animation->isLoop = isLoop;
                              });
          }
          ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_LOOP_ANIMATION));
        }
        ImGui::EndDisabled();

        ImGui::SameLine();

        auto info = info_get();
        auto fps = info ? info->fps : 30;
        ImGui::SetNextItemWidth(widgetSize.x);
        if (input_int_range(localize.get(LABEL_FPS), fps, FPS_MIN, FPS_MAX))
        {
          edit_command_push(EDIT_FPS, Document::INFO,
                            [=](Manager&, Document& document)
                            {
                              auto info = command_info_get(document);
                              if (!info)
                              {
                                document.anm2.root.children.push_back(element_make(ElementType::INFO));
                                info = &document.anm2.root.children.back();
                              }
                              info->fps = fps;
                            });
        }
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_FPS));

        ImGui::SameLine();

        info = info_get();
        auto createdBy = info ? info->createdBy : std::string{};
        ImGui::SetNextItemWidth(widgetSize.x);
        if (input_text_string(localize.get(LABEL_AUTHOR), &createdBy))
        {
          edit_command_push(EDIT_AUTHOR, Document::INFO,
                            [=](Manager&, Document& document)
                            {
                              auto info = command_info_get(document);
                              if (!info)
                              {
                                document.anm2.root.children.push_back(element_make(ElementType::INFO));
                                info = &document.anm2.root.children.back();
                              }
                              info->createdBy = createdBy;
                            });
        }
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_AUTHOR));

        ImGui::SameLine();

        ImGui::SetNextItemWidth(widgetSize.x);
        ImGui::Checkbox(localize.get(LABEL_SOUND), &settings.timelineIsSound);
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_SOUND));

        ImGui::PopStyleVar();
      }
      ImGui::EndChild();

      ImGui::SetCursorPos(cursorPos);
    };

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2());
    if (ImGui::Begin(localize.get(LABEL_TIMELINE_WINDOW), &settings.windowIsTimeline))
    {
      isWindowHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows |
                                               ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
      frames_child();
      if (frameMoveDrag.isActive && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
      {
        if (isFrameMoveDropTarget) frames_move_to(frameMoveDropType, frameMoveDropItemID, frameMoveDropIndex);
        frame_move_drag_clear();
      }
      items_child();
    }
    ImGui::PopStyleVar();
    ImGui::End();

    if (itemProperties.update(manager, settings, document, reference)) group_selection_reset();
    group_properties_update();

    bakePopup.trigger();

    if (ImGui::BeginPopupModal(bakePopup.label(), &bakePopup.isOpen, ImGuiWindowFlags_NoResize))
    {
      auto& interval = settings.bakeInterval;
      auto& isRoundRotation = settings.bakeIsRoundRotation;
      auto& isRoundScale = settings.bakeIsRoundScale;

      auto frame = frame_get();

      input_int_range(localize.get(LABEL_INTERVAL), interval, FRAME_DURATION_MIN,
                      frame ? frame->duration : FRAME_DURATION_MIN);
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_INTERVAL));

      ImGui::Checkbox(localize.get(LABEL_ROUND_ROTATION), &isRoundRotation);
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_ROUND_ROTATION));

      ImGui::Checkbox(localize.get(LABEL_ROUND_SCALE), &isRoundScale);
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_ROUND_SCALE));

      auto widgetSize = widget_size_with_row_get(2);

      shortcut(manager.chords[SHORTCUT_CONFIRM]);
      if (ImGui::Button(localize.get(LABEL_BAKE), widgetSize))
      {
        frames_bake();
        bakePopup.close();
      }
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_BAKE_FRAMES_OPTIONS));

      ImGui::SameLine();

      shortcut(manager.chords[SHORTCUT_CANCEL]);
      if (ImGui::Button(localize.get(BASIC_CANCEL), widgetSize)) bakePopup.close();
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_CANCEL_BAKE_FRAMES));

      ImGui::EndPopup();
    }

    if (animation)
    {
      if (shortcut(manager.chords[SHORTCUT_PLAY_PAUSE], shortcut::GLOBAL)) playback.toggle();

      if (shortcut(manager.chords[SHORTCUT_MOVE_PLAYHEAD_BACK], shortcut::GLOBAL))
      {
        playback_stop();
        playback.decrement(settings.playbackIsClamp ? animation->frameNum : FRAME_NUM_MAX);
        document.frameTime = playback.time;
      }

      if (shortcut(manager.chords[SHORTCUT_MOVE_PLAYHEAD_FORWARD], shortcut::GLOBAL))
      {
        playback_stop();
        playback.increment(settings.playbackIsClamp ? animation->frameNum : FRAME_NUM_MAX);
        document.frameTime = playback.time;
      }

      static bool isShortenChordHeld = false;
      auto isShortenFrame = shortcut(manager.chords[SHORTCUT_SHORTEN_FRAME], shortcut::GLOBAL);

      if (isShortenFrame && !isShortenChordHeld) snapshot_command_push(EDIT_SHORTEN_FRAME);
      if (isShortenFrame)
      {

        auto selectedFrames = frame_references_for_current_get();
        std::erase_if(selectedFrames,
                      [](const Reference& frameReference) { return frameReference.itemType == TRIGGER; });
        if (!selectedFrames.empty())
        {
          command_push([=](Manager&, Document& document)
                       {
                         for (auto frameReference : selectedFrames)
                         {
                           auto frame = command_frame_get(document, frameReference);
                           if (!frame) continue;
                           frame->duration = std::max(FRAME_DURATION_MIN, frame->duration - 1);
                         }
                         document.anm2_change(Document::FRAMES);
                       });
        }
      }
      isShortenChordHeld = isShortenFrame;

      static bool isExtendChordHeld = false;
      auto isExtendFrame = shortcut(manager.chords[SHORTCUT_EXTEND_FRAME], shortcut::GLOBAL);
      if (isExtendFrame && !isExtendChordHeld) snapshot_command_push(EDIT_EXTEND_FRAME);
      if (isExtendFrame)
      {

        auto selectedFrames = frame_references_for_current_get();
        std::erase_if(selectedFrames,
                      [](const Reference& frameReference) { return frameReference.itemType == TRIGGER; });
        if (!selectedFrames.empty())
        {
          command_push([=](Manager&, Document& document)
                       {
                         for (auto frameReference : selectedFrames)
                         {
                           auto frame = command_frame_get(document, frameReference);
                           if (!frame) continue;
                           frame->duration = std::min(FRAME_DURATION_MAX, frame->duration + 1);
                         }
                         document.anm2_change(Document::FRAMES);
                       });
        }
      }
      isExtendChordHeld = isExtendFrame;

      auto isPreviousFrame = shortcut(manager.chords[SHORTCUT_PREVIOUS_FRAME], shortcut::GLOBAL);
      auto isNextFrame = shortcut(manager.chords[SHORTCUT_NEXT_FRAME], shortcut::GLOBAL);
      auto isPreviousItem = shortcut(manager.chords[SHORTCUT_PREVIOUS_ITEM], shortcut::GLOBAL);
      auto isNextItem = shortcut(manager.chords[SHORTCUT_NEXT_ITEM], shortcut::GLOBAL);

      if (isPreviousFrame)
        if (auto item = selected_item_get(); item && !item->children.empty())
          reference.frameIndex = glm::clamp(--reference.frameIndex, 0, (int)item->children.size() - 1);

      if (isNextFrame)
        if (auto item = selected_item_get(); item && !item->children.empty())
          reference.frameIndex = glm::clamp(++reference.frameIndex, 0, (int)item->children.size() - 1);

      if (isPreviousFrame || isNextFrame)
      {
        if (auto item = selected_item_get(); item && !item->children.empty())
        {
          frames_selection_set_reference();
          document.frameTime = frame_time_from_index_get(*item, reference.frameIndex);
        }
      }

      if (isPreviousItem) reference_set_adjacent_item(-1);
      if (isNextItem) reference_set_adjacent_item(1);
    }

    if (isTextPushed) ImGui::PopStyleColor();
  }
}
