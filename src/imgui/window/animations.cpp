#include "panel.hpp"

#include <algorithm>
#include <format>
#include <optional>

#include "util/imgui/draw.hpp"
#include "util/imgui/layout.hpp"
#include "util/imgui/shortcut.hpp"
#include "util/imgui/tree.hpp"

using namespace anm2ed::resource;
using namespace anm2ed::types;

namespace anm2ed::imgui
{
  constexpr const char* ANIMATION_DRAG_DROP = "Animation Drag Drop";
  constexpr float DROP_ZONE_FRACTION = 1.0f / 3.0f;
  constexpr int MERGE_OPTIONS_ROWS = 2;
  constexpr float MERGE_OPTIONS_HALF = 0.5f;
  constexpr int MERGE_POPUP_CHILD_COUNT = 3;
  constexpr int GROUP_KEY_OFFSET = 2;

  // Animation groups share the key space with animation indices as negative keys.
  constexpr int group_key_get(int groupId) { return -groupId - GROUP_KEY_OFFSET; }
  constexpr int group_id_from_key_get(int key) { return -key - GROUP_KEY_OFFSET; }
  constexpr bool is_group_key(int key) { return key <= -GROUP_KEY_OFFSET; }

  enum class AnimationDropZone
  {
    BEFORE,
    INSIDE,
    AFTER
  };

  struct AnimationDragDropItem
  {
    bool isGroup{};
    int id{-1};

    bool operator==(const AnimationDragDropItem&) const = default;
  };

  struct AnimationMergeOptions
  {
    std::set<int> selection{};
    int reference{-1};
    merge::Type type{};
    bool isDeleteAnimationsAfter{};
  };

  // Calls back for each top-level entry: (animation index, animation) for loose animations, (index, group) for groups.
  template <class Callback> void animation_entries_each(const model::Model& model, Callback&& callback)
  {
    int index{};
    for (const auto& entry : model.animations.entries)
      if (auto animation = std::get_if<std::shared_ptr<const model::Animation>>(&entry))
        callback(index++, animation->get(), (const model::AnimationGroup*)nullptr);
      else
      {
        auto& group = std::get<model::AnimationGroup>(entry);
        callback(index, (const model::Animation*)nullptr, &group);
        index += (int)group.animations.size();
      }
  }

  // The flat index of a group's first animation.
  int animation_group_first_index_get(const model::Model& model, int groupId)
  {
    int first{-1};
    animation_entries_each(model,
                           [&](int index, auto*, const model::AnimationGroup* group)
                           {
                             if (group && group->id == groupId) first = index;
                           });
    return first;
  }

  std::set<int> animation_groups_selected_get(const Document& document)
  {
    std::set<int> groupIds{};
    auto uids = document.selection.uids.find(SelectionKind::ANIMATION_GROUPS);
    if (uids == document.selection.uids.end()) return groupIds;
    for (const auto& entry : document.model.animations.entries)
      if (auto group = std::get_if<model::AnimationGroup>(&entry); group && uids->second.contains(group->uid))
        groupIds.insert(group->id);
    return groupIds;
  }

  void animations_selection_set(Document& document, const std::set<int>& indices, const std::set<int>& groupIds)
  {
    document.animations_selected_set(indices);
    auto& groupUids = document.selection.uids[SelectionKind::ANIMATION_GROUPS];
    groupUids.clear();
    for (const auto& entry : document.model.animations.entries)
      if (auto group = std::get_if<model::AnimationGroup>(&entry); group && groupIds.contains(group->id))
        groupUids.insert(group->uid);
  }

  // Selected animations plus every animation of a selected group.
  std::set<int> animation_selected_indices_get(const Document& document)
  {
    auto result = document.animations_selected_get();
    for (auto groupId : animation_groups_selected_get(document))
      result.merge(edit::animation_group_indices_get(document.model, groupId));
    return result;
  }

  std::vector<int> animation_groupable_indices_get(const Document& document)
  {
    std::vector<int> result{};
    for (auto index : document.animations_selected_get())
    {
      if (!document.model.animation_get(index) || document.model.animation_group_id_get(index) != -1) return {};
      result.push_back(index);
    }
    return result;
  }

  std::set<int> animation_merge_indices_get(const Document& document, const std::set<int>& keys, int reference)
  {
    std::set<int> result{};
    for (auto key : keys)
      if (!is_group_key(key))
        result.insert(key);
      else
        result.merge(edit::animation_group_indices_get(document.model, group_id_from_key_get(key)));
    result.erase(reference);
    return result;
  }

  void animation_overlay_reset(Document& document, const std::set<int>& indices)
  {
    auto isOverlayDocument = document.overlayDocumentId == 0 || document.overlayDocumentId == document.tabId;
    if (!isOverlayDocument || !indices.contains(document.overlayIndex)) return;
    document.overlayIndex = -1;
    document.overlayDocumentId = 0;
  }

  void animations_erase(Document& document, const std::set<int>& indices, const std::set<int>& groupIds,
                        StringType label)
  {
    document.edit_apply(label,
                        [&](model::Model& model)
                        {
                          animation_overlay_reset(document, indices);
                          animations_selection_set(document, {}, {});
                          return edit::animations_remove(model, indices, groupIds);
                        });
  }

  std::set<int> animation_indices_get(Document& document, const edit::Uids& uids)
  {
    std::set<int> indices{};
    for (auto reference : document.references_get(uids))
      indices.insert(reference.animationIndex);
    return indices;
  }

  std::string animation_clipboard_text_get(const Document& document)
  {
    std::string clipboardText{};
    auto& model = document.model;
    auto groupIds = animation_groups_selected_get(document);
    auto selection = document.animations_selected_get();
    for (const auto& entry : model.animations.entries)
      if (auto group = std::get_if<model::AnimationGroup>(&entry); group && groupIds.contains(group->id))
        clipboardText += model::animation_group_to_string(*group);
    for (int i = 0; i < model.animations_count_get(); ++i)
    {
      auto groupId = model.animation_group_id_get(i);
      if (selection.contains(i) || groupIds.contains(groupId))
        clipboardText += model::animation_to_string(*model.animation_get(i), groupId);
    }
    return clipboardText;
  }

  void animations_insert(PanelState& state, Document& document, const std::string& text, int start, int targetGroupId,
                         StringType edit)
  {
    std::set<int> groupIds{};
    std::string errorString{};
    document.edit_apply(edit,
                        [&](model::Model& model)
                        {
                          auto uids = edit::animations_paste(model, text, start, targetGroupId, groupIds, &errorString);
                          if (uids.empty()) return uids;
                          auto indices = animation_indices_get(document, uids);
                          animations_selection_set(document, groupIds.empty() ? indices : std::set<int>{}, groupIds);
                          document.reference_set({});
                          if (indices.empty()) return uids;
                          state.scrollId = *indices.rbegin();
                          if (!groupIds.empty()) return uids;
                          document.reference_set({*indices.rbegin()});
                          if (indices.size() == 1) state.newId = *indices.rbegin();
                          return uids;
                        });
    if (!errorString.empty()) toast_log(Level::ERROR, TOAST_DESERIALIZE_ANIMATIONS_FAILED, errorString);
  }

  // Merges into options.reference; without a merge-popup selection, the selected animations merge into the first.
  int animations_merge(Document& document, const AnimationMergeOptions& options)
  {
    auto isQuick = options.selection.empty();
    auto target = options.reference;
    auto sources = isQuick ? animation_selected_indices_get(document)
                           : animation_merge_indices_get(document, options.selection, target);
    auto type = isQuick ? merge::APPEND : options.type;
    auto isDeleteAfter = isQuick || options.isDeleteAnimationsAfter;
    auto ungroupIds = isQuick ? animation_groups_selected_get(document) : std::set<int>{};

    if (isQuick)
    {
      auto count = document.model.animations_count_get();
      if (sources.empty()) return -1;
      if (sources.size() == 1 && ungroupIds.empty() && *sources.begin() != count - 1)
        sources.insert(*sources.begin() + 1);
      if (sources.size() == 1 && ungroupIds.empty()) return -1;
      target = *sources.begin();
    }
    else if (sources.empty())
      return -1;

    int merged{-1};
    document.edit_apply(EDIT_MERGE_ANIMATIONS,
                        [&](model::Model& model)
                        {
                          animation_overlay_reset(document, sources);
                          auto uids = edit::animations_merge(model, target, sources, type, isDeleteAfter, ungroupIds);
                          auto indices = animation_indices_get(document, uids);
                          if (indices.empty()) return uids;
                          merged = *indices.begin();
                          animations_selection_set(document, {merged}, {});
                          document.reference_set({merged});
                          return uids;
                        });
    return merged;
  }

  AnimationDropZone animation_drop_zone_get(ImVec2 min, ImVec2 max)
  {
    auto zoneHeight = (max.y - min.y) * DROP_ZONE_FRACTION;
    auto mouseY = ImGui::GetIO().MousePos.y;
    if (mouseY < min.y + zoneHeight) return AnimationDropZone::BEFORE;
    if (mouseY >= max.y - zoneHeight) return AnimationDropZone::AFTER;
    return AnimationDropZone::INSIDE;
  }

  void animation_move_command_push(const Panel& panel, std::vector<AnimationDragDropItem> items,
                                   edit::AnimationTarget target)
  {
    if (items.empty()) return;
    command_push(panel,
                 [items, target](Document& document)
                 {
                   std::set<int> indices{};
                   std::set<int> groupIds{};
                   for (auto item : items)
                     (item.isGroup ? groupIds : indices).insert(item.id);
                   document.edit_apply(EDIT_MOVE_ANIMATIONS,
                                       [&](model::Model& model)
                                       {
                                         auto uids = edit::animations_move(model, indices, groupIds, target);
                                         if (uids.empty() && groupIds.empty()) return;
                                         animations_selection_set(document, animation_indices_get(document, uids),
                                                                  groupIds);
                                       });
                 });
  }

  void animation_drag_drop_source_update(const Document& document, AnimationDragDropItem fallback)
  {
    if (!ImGui::BeginDragDropSource(DRAG_DROP_SOURCE_FLAGS)) return;

    static std::vector<AnimationDragDropItem> items{};
    items.clear();
    auto groupIds = animation_groups_selected_get(document);
    auto selection = document.animations_selected_get();
    if (fallback.isGroup ? groupIds.contains(fallback.id) : selection.contains(fallback.id))
    {
      for (auto groupId : groupIds)
        items.push_back({true, groupId});
      for (auto index : selection)
        if (document.model.animation_get(index)) items.push_back({false, index});
    }
    if (items.empty() && fallback.id >= 0) items.push_back(fallback);
    ImGui::SetDragDropPayload(ANIMATION_DRAG_DROP, items.data(), items.size() * sizeof(AnimationDragDropItem));
    ImGui::EndDragDropSource();
  }

  std::vector<AnimationDragDropItem> animation_drag_drop_target_items_get()
  {
    auto payload = ImGui::AcceptDragDropPayload(ANIMATION_DRAG_DROP, ImGuiDragDropFlags_AcceptBeforeDelivery |
                                                                         ImGuiDragDropFlags_AcceptNoDrawDefaultRect);
    if (!payload || payload->DataSize % (int)sizeof(AnimationDragDropItem) != 0) return {};
    auto items = static_cast<const AnimationDragDropItem*>(payload->Data);
    return {items, items + payload->DataSize / sizeof(AnimationDragDropItem)};
  }

  bool animation_group_drag_drop_update(const Panel& panel, int groupId, ImVec2 itemMin, ImVec2 itemMax)
  {
    if (!ImGui::BeginDragDropTarget()) return false;
    auto items = animation_drag_drop_target_items_get();
    auto isDelivered = false;
    if (!items.empty())
    {
      auto dropZone = animation_drop_zone_get(itemMin, itemMax);
      auto isGroupDragged = std::ranges::any_of(items, &AnimationDragDropItem::isGroup);
      auto isDropIntoGroup = dropZone == AnimationDropZone::INSIDE && !isGroupDragged;
      if (isDropIntoGroup)
        drop_box_draw(ImGui::GetWindowDrawList(), itemMin, itemMax);
      else
        drop_line_draw(ImGui::GetWindowDrawList(), itemMin, itemMax, dropZone != AnimationDropZone::BEFORE);

      isDelivered = ImGui::GetDragDropPayload()->IsDelivery();
      if (isDelivered)
        animation_move_command_push(
            panel, items,
            {.groupId = groupId, .isAfter = dropZone != AnimationDropZone::BEFORE, .isInto = isDropIntoGroup});
    }
    ImGui::EndDragDropTarget();
    return isDelivered;
  }

  bool animation_row_drag_drop_update(const Panel& panel, int index)
  {
    animation_drag_drop_source_update(panel.document, {false, index});
    if (!ImGui::BeginDragDropTarget()) return false;

    auto items = animation_drag_drop_target_items_get();
    auto isDelivered = false;
    if (!items.empty())
    {
      auto itemMin = ImGui::GetItemRectMin();
      auto itemMax = ImGui::GetItemRectMax();
      auto isDropAfter = is_drop_after(itemMin, itemMax);
      drop_line_draw(ImGui::GetWindowDrawList(), itemMin, itemMax, isDropAfter);
      isDelivered = ImGui::GetDragDropPayload()->IsDelivery();
      if (isDelivered) animation_move_command_push(panel, items, {.animationIndex = index, .isAfter = isDropAfter});
    }
    ImGui::EndDragDropTarget();
    return isDelivered;
  }

  void animation_group_set_push(const Panel& panel, int groupId, StringType edit, std::optional<std::string> name,
                                std::optional<bool> isExpanded)
  {
    command_push(panel,
                 [groupId, edit, name, isExpanded](Document& document)
                 {
                   auto first = animation_group_first_index_get(document.model, groupId);
                   auto group = document.model.animation_group_get(first);
                   if (!group || (name.value_or(group->name) == group->name &&
                                  isExpanded.value_or(group->isExpanded) == group->isExpanded))
                     return;
                   document.edit_apply(edit,
                                       [&](model::Model& model)
                                       {
                                         auto group = model.animation_group_edit(first);
                                         group->name = name.value_or(group->name);
                                         group->isExpanded = isExpanded.value_or(group->isExpanded);
                                       });
                 });
  }

  // A group's tree node (renamable, a drop target) and, when expanded, its animations' rows.
  bool animation_group_draw(Panel& panel, std::set<int>& selection, std::set<int>& groupSelection, int& focus,
                            const ListRows& rows, const model::AnimationGroup& group, int firstIndex, int arrowKey)
  {
    auto& state = panel.state;
    auto key = group_key_get(group.id);
    if (state.newId == key)
    {
      state.renameState = RENAME_FORCE_EDIT;
      state.newId = -1;
    }
    auto isRenaming = state.renameId == key;

    ImGui::PushID("Animation Group");
    ImGui::PushID(group.id);
    ImGui::SetNextItemOpen(group.isExpanded, ImGuiCond_Always);
    auto groupName = group.name;
    auto tree =
        tree_node_input_text(group.name.empty() ? std::string(localize.get(TEXT_NEW_GROUP)) : group.name,
                             std::format("###Document #{} Animation Group #{}", panel.manager.selected, group.id),
                             isRenaming ? state.renameText : groupName, groupSelection.contains(group.id),
                             ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth, state.renameState);
    auto itemMin = ImGui::GetItemRectMin();
    auto itemMax = ImGui::GetItemRectMax();
    if (tree.isRenameStarted)
    {
      state.renameId = key;
      state.renameText = group.name;
    }
    else if (tree.isRenameFinished)
    {
      if (isRenaming) animation_group_set_push(panel, group.id, EDIT_RENAME_GROUP, state.renameText, std::nullopt);
      state.renameId = -1;
      state.renameText.clear();
    }
    if (tree.isOpen != group.isExpanded && !is_drag_drop_active() && !ImGui::IsMouseDragging(ImGuiMouseButton_Left))
      animation_group_set_push(panel, group.id, EDIT_TOGGLE_GROUP_EXPANDED, std::nullopt, tree.isOpen);

    if (tree.isClicked)
    {
      auto isCtrl = ImGui::GetIO().KeyCtrl;
      auto isDeselected = isCtrl && groupSelection.contains(group.id);
      if (!isCtrl)
      {
        selection.clear();
        groupSelection.clear();
      }
      if (isDeselected)
        groupSelection.erase(group.id);
      else
        groupSelection.insert(group.id);
      if (isDeselected ? selection.empty() && groupSelection.empty() : !isCtrl || selection.empty())
      {
        panel.document.reference_set({});
        panel.document.frame_references_clear();
      }
    }

    animation_drag_drop_source_update(panel.document, {true, group.id});
    auto isRowsDone = animation_group_drag_drop_update(panel, group.id, itemMin, itemMax);

    if (tree.isOpen)
    {
      for (int i = 0; i < (int)group.animations.size() && !isRowsDone; ++i)
        isRowsDone = row_draw(panel, selection, focus, rows, firstIndex + i, firstIndex + i, arrowKey);
      ImGui::TreePop();
    }
    ImGui::PopID();
    ImGui::PopID();
    return isRowsDone;
  }

  void animations_merge_popup_update(Panel& panel, AnimationsPanel& animations)
  {
    auto& [manager, settings, resources, dialog, clipboard, document, state] = panel;
    auto& popup = animations.mergePopup;
    auto& mergeSelection = animations.mergeSelection;
    auto mergeReference = animations.mergeReference;

    popup.trigger();
    if (!ImGui::BeginPopupModal(popup.label(), &popup.isOpen, ImGuiWindowFlags_NoResize)) return popup.end();

    struct AnimationMergeCandidate
    {
      int key{};
      std::string label{};
    };
    std::vector<AnimationMergeCandidate> candidates{};
    auto candidate_animation_push = [&](const model::Animation& animation, int animationIndex, bool isGrouped)
    {
      if (animationIndex != mergeReference)
        candidates.push_back({animationIndex, isGrouped ? std::format("  {}", animation.name) : animation.name});
    };
    animation_entries_each(document.model,
                           [&](int index, const model::Animation* animation, const model::AnimationGroup* group)
                           {
                             if (animation) return candidate_animation_push(*animation, index, false);
                             auto isReferenceOnly = group->animations.size() == 1 && index == mergeReference;
                             if (group->animations.empty() || isReferenceOnly) return;
                             candidates.push_back(
                                 {group_key_get(group->id),
                                  group->name.empty() ? std::string(localize.get(TEXT_NEW_GROUP)) : group->name});
                             for (int i = 0; i < (int)group->animations.size(); ++i)
                               candidate_animation_push(*group->animations[i], index + i, true);
                           });

    auto footerSize = footer_size_get();
    auto optionsSize = child_size_get(MERGE_OPTIONS_ROWS);
    auto deleteAfterSize = child_size_get();
    auto animationsSize =
        ImVec2(0, ImGui::GetContentRegionAvail().y - (optionsSize.y + deleteAfterSize.y + footerSize.y +
                                                      ImGui::GetStyle().ItemSpacing.y * MERGE_POPUP_CHILD_COUNT));

    if (ImGui::BeginChild(localize.get(LABEL_ANIMATIONS_CHILD), animationsSize, ImGuiChildFlags_Borders))
    {
      animations.mergeKeys.clear();
      for (const auto& candidate : candidates)
        animations.mergeKeys.push_back(candidate.key);
      mergeSelection.set_index_map(&animations.mergeKeys);
      mergeSelection.start(candidates.size());
      for (int i = 0; i < (int)candidates.size(); ++i)
      {
        ImGui::PushID(candidates[i].key);
        ImGui::SetNextItemSelectionUserData(i);
        ImGui::Selectable(candidates[i].label.c_str(), mergeSelection.contains(candidates[i].key));
        ImGui::PopID();
      }
      mergeSelection.finish();
      mergeSelection.set_index_map(nullptr);
    }
    ImGui::EndChild();

    if (ImGui::BeginChild("##Merge Options", optionsSize, ImGuiChildFlags_Borders))
    {
      auto size = ImVec2(optionsSize.x * MERGE_OPTIONS_HALF, optionsSize.y - ImGui::GetStyle().WindowPadding.y * 2);
      if (ImGui::BeginChild("##Merge Options 1", size))
      {
        ImGui::RadioButton(localize.get(LABEL_APPEND_FRAMES), &settings.mergeType, merge::APPEND);
        ImGui::RadioButton(localize.get(LABEL_PREPEND_FRAMES), &settings.mergeType, merge::PREPEND);
      }
      ImGui::EndChild();
      ImGui::SameLine();
      if (ImGui::BeginChild("##Merge Options 2", size))
      {
        ImGui::RadioButton(localize.get(LABEL_REPLACE_FRAMES), &settings.mergeType, merge::REPLACE);
        ImGui::RadioButton(localize.get(LABEL_IGNORE_FRAMES), &settings.mergeType, merge::IGNORE);
      }
      ImGui::EndChild();
    }
    ImGui::EndChild();

    if (ImGui::BeginChild("##Merge Delete After", deleteAfterSize, ImGuiChildFlags_Borders))
      ImGui::Checkbox(localize.get(LABEL_DELETE_ANIMATIONS_AFTER), &settings.mergeIsDeleteAnimationsAfter);
    ImGui::EndChild();

    auto result =
        popup_buttons_draw(manager, localize.get(LABEL_MERGE),
                           !animation_merge_indices_get(document, mergeSelection, mergeReference).empty(), LABEL_CLOSE);
    if (result == PopupButton::CONFIRM)
      command_push(panel,
                   [options = AnimationMergeOptions{.selection = mergeSelection,
                                                    .reference = mergeReference,
                                                    .type = (merge::Type)settings.mergeType,
                                                    .isDeleteAnimationsAfter = settings.mergeIsDeleteAnimationsAfter}](
                       Document& document) { animations_merge(document, options); });
    if (result != PopupButton::NONE)
    {
      mergeSelection.clear();
      popup.close();
    }
    ImGui::EndPopup();
    popup.end();
  }

  void animations_update(Panel& panel, AnimationsPanel& animations)
  {
    auto& [manager, settings, resources, dialog, clipboard, document, state] = panel;
    auto& model = document.model;

    if (ImGui::Begin(localize.get(LABEL_ANIMATIONS_WINDOW), &settings.windowIsAnimations))
    {
      auto selection = document.animations_selected_get();
      auto groupSelection = animation_groups_selected_get(document);
      auto groupIds = edit::animation_group_ids_get(model);
      auto count = model.animations_count_get();
      auto isItemSelected = [&]() { return !selection.empty() || !groupSelection.empty(); };
      // A single selected group is where adds and pastes go.
      auto targetGroupId = [&]()
      {
        return groupSelection.size() == 1 && groupIds.contains(*groupSelection.begin()) ? *groupSelection.begin() : -1;
      };
      auto group_end_get = [&](int groupId)
      {
        auto indices = groupId != -1 ? edit::animation_group_indices_get(model, groupId) : std::set<int>{};
        return indices.empty() ? count : std::min(*indices.rbegin() + 1, count);
      };

      Actions actions{};
      actions.add(
          ACTION_RENAME, [&]() { return selection.size() == 1 || (selection.empty() && groupSelection.size() == 1); },
          [&]() { state.newId = selection.size() == 1 ? *selection.begin() : group_key_get(*groupSelection.begin()); });
      actions.add(
          ACTION_ADD, {},
          [&]()
          {
            auto index = !selection.empty() ? std::min(*selection.rbegin() + 1, count) : group_end_get(targetGroupId());
            auto groupId = selection.empty() ? targetGroupId() : model.animation_group_id_get(*selection.rbegin());
            command_push(panel,
                         [&state, index, groupId](Document& document)
                         {
                           document.edit_apply(EDIT_ADD_ANIMATION,
                                               [&](model::Model& model)
                                               {
                                                 auto uids = edit::animation_add(
                                                     model, index, groupId, document.reference_get().animationIndex,
                                                     localize.get(TEXT_NEW_ANIMATION));
                                                 animations_selection_set(document, {index}, {});
                                                 document.reference_set({index});
                                                 state.newId = index;
                                                 return uids;
                                               });
                         });
          },
          TOOLTIP_ADD_ANIMATION);
      actions.add(
          ACTION_DUPLICATE, isItemSelected,
          [&]()
          {
            auto indices = animation_selected_indices_get(document);
            auto start = indices.empty() ? count : std::min(*indices.rbegin() + 1, count);
            command_push(panel,
                         [&state, start](Document& document)
                         {
                           auto text = animation_clipboard_text_get(document);
                           if (!text.empty())
                             animations_insert(state, document, text, start, -1, EDIT_DUPLICATE_ANIMATIONS);
                         });
          },
          TOOLTIP_DUPLICATE_ANIMATION);
      actions.add(
          ACTION_MERGE, [&]() { return !animation_selected_indices_get(document).empty(); },
          [&]()
          {
            command_push(panel,
                         [&state](Document& document)
                         {
                           auto merged = animations_merge(document, {});
                           if (merged != -1) state.scrollId = merged;
                         });
          },
          TOOLTIP_OPEN_MERGE_POPUP);
      actions.add(
          ACTION_GROUP, [&]() { return !animation_groupable_indices_get(document).empty(); },
          [&]()
          {
            auto targets = animation_groupable_indices_get(document);
            command_push(panel,
                         [&state, targets = std::set<int>(targets.begin(), targets.end())](Document& document)
                         {
                           auto groupIds = edit::animation_group_ids_get(document.model);
                           auto groupId = groupIds.empty() ? 0 : *groupIds.rbegin() + 1;
                           document.edit_apply(EDIT_GROUP_ITEMS,
                                               [&](model::Model& model)
                                               {
                                                 auto uids = edit::animations_group(model, targets,
                                                                                    localize.get(TEXT_NEW_GROUP));
                                                 animations_selection_set(document, targets, {groupId});
                                                 document.reference_set({*targets.begin()});
                                                 state.scrollId = *targets.begin();
                                                 return uids;
                                               });
                         });
          });
      actions.add(
          ACTION_REMOVE, isItemSelected,
          [&]()
          {
            command_push(panel,
                         [selection, groupSelection](Document& document)
                         {
                           animations_erase(document, selection, groupSelection,
                                            groupSelection.empty() ? EDIT_REMOVE_ANIMATIONS : EDIT_REMOVE_GROUP);
                         });
          },
          TOOLTIP_REMOVE_ANIMATION);
      actions.add(
          ACTION_DEFAULT, [&]() { return selection.size() == 1; },
          [&]()
          {
            command_push(panel,
                         [index = *selection.begin()](Document& document)
                         {
                           auto animation = document.model.animation_get(index);
                           if (!animation) return;
                           document.edit_apply(EDIT_DEFAULT_ANIMATION, [name = animation->name](model::Model& model)
                                               { model.animations.defaultAnimation = name; });
                         });
          },
          TOOLTIP_SET_DEFAULT_ANIMATION);
      actions.separator();
      actions.add(ACTION_CUT, isItemSelected,
                  [&]()
                  {
                    clipboard.set(animation_clipboard_text_get(document));
                    command_push(
                        panel, [indices = animation_selected_indices_get(document), groupSelection](Document& document)
                        { animations_erase(document, indices, groupSelection, EDIT_CUT_ANIMATIONS); });
                  });
      actions.add(ACTION_COPY, isItemSelected, [&]() { clipboard.set(animation_clipboard_text_get(document)); });
      actions.add(
          ACTION_PASTE, [&]() { return !clipboard.is_empty(); },
          [&]()
          {
            auto groupId = selection.empty() ? targetGroupId() : -1;
            auto start = !selection.empty() ? *selection.rbegin() + 1 : group_end_get(groupId);
            command_push(panel, [&state, text = clipboard.get(), start, groupId](Document& document)
                         { animations_insert(state, document, text, start, groupId, EDIT_PASTE_ANIMATIONS); });
          });

      // The footer's merge opens the merge popup (merging into the first selected animation).
      auto footerActions = actions;
      if (groupSelection.empty())
        std::ranges::find(footerActions.items, ACTION_MERGE, &Action::type)->run = [&]()
        {
          if (selection.empty()) return;
          animations.mergeSelection.clear();
          animations.mergeReference = *selection.begin();
          animations.mergePopup.open();
        };

      ListRows rows{.label_get = [&](int index) { return model.animation_get(index)->name; },
                    .name_get = [&](int index) { return model.animation_get(index)->name; },
                    .rename =
                        [&](int index, const std::string& name)
                    {
                      command_push(panel,
                                   [index, name](Document& document)
                                   {
                                     document.edit_apply(SNAPSHOT_RENAME_ANIMATION,
                                                         [&](model::Model& model)
                                                         {
                                                           auto animation = model.animation_edit(index);
                                                           if (!animation) return;
                                                           if (model.animations_count_get() == 1)
                                                             model.animations.defaultAnimation = name;
                                                           animation->name = name;
                                                         });
                                   });
                    },
                    .font_get =
                        [&](int index)
                    {
                      auto animation = model.animation_get(index);
                      auto isDefault = animation && model.animations.defaultAnimation == animation->name;
                      auto isFocused = document.reference_get().animationIndex == index;
                      return isDefault && isFocused ? font::BOLD_ITALICS
                             : isDefault            ? font::BOLD
                             : isFocused            ? font::ITALICS
                                                    : font::REGULAR;
                    },
                    .tooltip_draw =
                        [&](int index)
                    {
                      auto& animation = *model.animation_get(index);
                      tooltip_name_draw(resources, animation.name);
                      if (model.animations.defaultAnimation == animation.name)
                      {
                        ImGui::PushFont(resources.fonts[font::ITALICS].get(), font::SIZE);
                        ImGui::TextUnformatted(localize.get(BASIC_DEFAULT));
                        ImGui::PopFont();
                      }
                      ImGui::TextUnformatted(
                          std::vformat(localize.get(FORMAT_LENGTH), std::make_format_args(animation.frameNum)).c_str());
                      auto loopLabel = localize.get(animation.isLoop ? BASIC_YES : BASIC_NO);
                      ImGui::TextUnformatted(
                          std::vformat(localize.get(FORMAT_LOOP), std::make_format_args(loopLabel)).c_str());
                    },
                    .select =
                        [&](int index)
                    {
                      auto& io = ImGui::GetIO();
                      if (!io.KeyCtrl && !io.KeyShift) groupSelection.clear();
                      document.reference_set({index});
                      document.frame_references_clear();
                    },
                    .drag_drop_update = [&](int index, int) { return animation_row_drag_drop_update(panel, index); }};

      list_panel_draw(
          panel, actions, {{ACTION_ADD, ACTION_DUPLICATE, ACTION_MERGE, ACTION_REMOVE, ACTION_DEFAULT}},
          [&]()
          {
            std::vector<int> visibleKeys{};
            state.keys.clear();
            for (int i = 0; i < count; ++i)
              state.keys.push_back(i);
            animation_entries_each(model,
                                   [&](int index, const model::Animation* animation, const model::AnimationGroup* group)
                                   {
                                     if (animation) visibleKeys.push_back(index);
                                     if (group && group->isExpanded)
                                       for (int i = 0; i < (int)group->animations.size(); ++i)
                                         visibleKeys.push_back(index + i);
                                   });

            auto focus = document.reference_get().animationIndex;
            auto arrowKey = list_select_begin(panel, selection, focus, rows, visibleKeys);
            auto isRowsDone = false;
            animation_entries_each(model,
                                   [&](int index, const model::Animation* animation, const model::AnimationGroup* group)
                                   {
                                     if (isRowsDone) return;
                                     if (group)
                                       isRowsDone = animation_group_draw(panel, selection, groupSelection, focus, rows,
                                                                         *group, index, arrowKey);
                                     else if (animation)
                                       isRowsDone = row_draw(panel, selection, focus, rows, index, index, arrowKey);
                                   });
            list_select_end(panel, selection, focus, rows, arrowKey);
            animations_selection_set(document, selection, groupSelection);
          },
          &footerActions);
    }
    ImGui::End();

    animations_merge_popup_update(panel, animations);

    auto isNext = shortcut(manager.chords[SHORTCUT_NEXT_ANIMATION], shortcut::GLOBAL);
    auto isPrevious = shortcut(manager.chords[SHORTCUT_PREVIOUS_ANIMATION], shortcut::GLOBAL);
    auto count = model.animations_count_get();
    if ((!isPrevious && !isNext) || count <= 0) return;
    auto reference = document.reference_get();
    reference.animationIndex = std::clamp(reference.animationIndex + (isNext ? 1 : -1), 0, count - 1);
    document.reference_set(reference);
    animations_selection_set(document, {reference.animationIndex}, animation_groups_selected_get(document));
    state.scrollId = reference.animationIndex;
  }
}
