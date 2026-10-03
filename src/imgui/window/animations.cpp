#include "window.hpp"

#include <algorithm>
#include <format>
#include <optional>

#include "toast.hpp"
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

  enum class AnimationDropZone
  {
    BEFORE,
    INSIDE,
    AFTER
  };

  enum class AnimationDragDropType
  {
    ANIMATION,
    GROUP
  };

  struct AnimationDragDropItem
  {
    AnimationDragDropType type{AnimationDragDropType::ANIMATION};
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

  int animation_group_child_insert_index_get(const Element& animations, int groupId, AnimationDropZone dropZone)
  {
    auto group = std::ranges::find_if(animations.children, [&](const Element& item)
                                      { return item.type == ElementType::GROUP && item.id == groupId; });
    if (group == animations.children.end()) return (int)animations.children.size();
    auto groupChildIndex = (int)(group - animations.children.begin());
    if (dropZone != AnimationDropZone::INSIDE) return groupChildIndex + (dropZone == AnimationDropZone::AFTER);

    auto insertIndex = groupChildIndex + 1;
    for (int i = 0; i < (int)animations.children.size(); ++i)
      if (animations.children[i].type == ElementType::ANIMATION && animations.children[i].groupId == groupId)
        insertIndex = i + 1;
    return insertIndex;
  }

  bool is_animation_grouped(const std::set<int>& groupIds, const Element& animation)
  {
    return animation.groupId != -1 && groupIds.contains(animation.groupId);
  }

  std::set<int> animation_selected_indices_get(Document& document, const Window& window)
  {
    std::set<int> result(document.animation.selection.begin(), document.animation.selection.end());
    if (auto animations = document.anm2.element_get(ElementType::ANIMATIONS))
      for (auto groupId : window.selection)
        result.merge(edit::animation_group_indices_get(*animations, groupId));
    return result;
  }

  std::vector<int> animation_groupable_indices_get(Document& document)
  {
    auto animations = document.anm2.element_get(ElementType::ANIMATIONS);
    auto& selection = document.animation.selection;
    if (!animations || selection.empty()) return {};

    auto groupIds = edit::animation_group_ids_get(*animations);
    std::vector<int> result{};
    int index{};
    for (const auto& animation : animations->children)
      if (animation.type == ElementType::ANIMATION)
      {
        if (selection.contains(index) && is_animation_grouped(groupIds, animation)) return {};
        if (selection.contains(index)) result.push_back(index);
        ++index;
      }
    return result.size() == selection.size() ? result : std::vector<int>{};
  }

  std::set<int> animation_merge_indices_get(Document& document, const std::set<int>& keys, int reference)
  {
    std::set<int> result{};
    auto animations = document.anm2.element_get(ElementType::ANIMATIONS);
    for (auto key : keys)
      if (!is_window_group_key(key))
        result.insert(key);
      else if (animations)
        result.merge(edit::animation_group_indices_get(*animations, window_group_id_from_key_get(key)));
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

  void animations_erase(Window& window, Document& document, const std::set<int>& indices, const std::set<int>& groupIds,
                        StringType label)
  {
    document.edit_apply(label, window.changeType,
                        [&](Anm2& anm2)
                        {
                          animation_overlay_reset(document, indices);
                          if (indices.contains(document.reference.animationIndex))
                            document.reference.animationIndex = -1;
                          document.animation.selection.clear();
                          window.selection.clear();
                          return edit::animations_remove(anm2, indices, groupIds);
                        });
  }

  std::set<int> animation_indices_get(Document& document, const edit::Uids& uids)
  {
    std::set<int> indices{};
    for (auto reference : document.references_get(uids))
      indices.insert(reference.animationIndex);
    return indices;
  }

  std::string animation_clipboard_text_get(Document& document, const Window& window)
  {
    std::string clipboardText{};
    auto animations = document.anm2.element_get(ElementType::ANIMATIONS);
    if (!animations) return clipboardText;
    int animationIndex{};
    for (const auto& item : animations->children)
    {
      if (item.type == ElementType::GROUP && window.selection.contains(item.id))
        clipboardText += element_to_string(item);
      if (item.type != ElementType::ANIMATION) continue;
      if (document.animation.selection.contains(animationIndex) || window.selection.contains(item.groupId))
        clipboardText += element_to_string(item);
      ++animationIndex;
    }
    return clipboardText;
  }

  void animations_insert(Window& window, Document& document, const std::string& text, int start, int targetGroupId,
                         StringType edit)
  {
    std::set<int> groupIds{};
    std::string errorString{};
    auto uids = document.edit_apply(edit, window.changeType,
                                    [&](Anm2& anm2)
                                    {
                                      auto uids = edit::animations_paste(anm2, text, start, targetGroupId, groupIds,
                                                                         &errorString);
                                      if (uids.empty()) return uids;
                                      auto indices = animation_indices_get(document, uids);
                                      document.animation.selection = groupIds.empty() ? indices : std::set<int>{};
                                      window.selection = groupIds;
                                      document.reference = {};
                                      window.newElementId = -1;
                                      if (indices.empty()) return uids;
                                      window.scrollQueued = *indices.rbegin();
                                      if (!groupIds.empty()) return uids;
                                      document.reference = {*indices.rbegin()};
                                      if (indices.size() == 1) window.newElementId = *indices.rbegin();
                                      return uids;
                                    });
    if (!errorString.empty()) toast_log(Level::ERROR, TOAST_DESERIALIZE_ANIMATIONS_FAILED, errorString);
  }

  int animations_merge(Document& document, const AnimationMergeOptions& options,
                       const std::set<int>* quickSelection = nullptr,
                       const std::set<int>* quickGroupSelection = nullptr)
  {
    auto target = options.reference;
    auto sources =
        options.selection.empty() ? *quickSelection : animation_merge_indices_get(document, options.selection, target);
    auto type = options.selection.empty() ? merge::APPEND : options.type;
    auto isDeleteAfter = options.selection.empty() || options.isDeleteAnimationsAfter;
    auto ungroupIds = options.selection.empty() && quickGroupSelection ? *quickGroupSelection : std::set<int>{};

    if (options.selection.empty())
    {
      auto animations = document.anm2.element_get(ElementType::ANIMATIONS);
      auto count = animations ? animations_count_get(*animations) : 0;
      if (sources.empty()) return -1;
      if (sources.size() == 1 && ungroupIds.empty() && *sources.begin() != count - 1)
        sources.insert(*sources.begin() + 1);
      if (sources.size() == 1 && ungroupIds.empty()) return -1;
      target = *sources.begin();
    }
    else if (sources.empty())
      return -1;

    int merged{-1};
    document.edit_apply(EDIT_MERGE_ANIMATIONS, Document::ANIMATIONS,
                        [&](Anm2& anm2)
                        {
                          animation_overlay_reset(document, sources);
                          auto uids = edit::animations_merge(anm2, target, sources, type, isDeleteAfter, ungroupIds);
                          auto indices = animation_indices_get(document, uids);
                          if (indices.empty()) return uids;
                          merged = *indices.begin();
                          document.animation.selection = {merged};
                          document.reference = {merged};
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

  void animation_items_move(Window& window, Document& document, Anm2& anm2,
                            const std::vector<AnimationDragDropItem>& items, int targetChildIndex, int targetGroupId)
  {
    std::set<int> indices{};
    std::set<int> groupIds{};
    for (auto item : items)
      (item.type == AnimationDragDropType::GROUP ? groupIds : indices).insert(item.id);
    auto uids = edit::animations_move(anm2, indices, groupIds, targetChildIndex, targetGroupId);
    if (uids.empty() && groupIds.empty()) return;
    document.animation.selection = animation_indices_get(document, uids);
    window.selection = groupIds;
  }

  void animation_move_command_push(Window& window, Manager& manager, std::vector<AnimationDragDropItem> items,
                                   int targetGroupId, std::function<int(const Element&)> target_child_index_get)
  {
    if (items.empty()) return;
    manager.command_push(
        {manager.selected, [&window, items, targetGroupId, target_child_index_get](Manager&, Document& document)
         {
           auto animations = window_container_get(window, document);
           if (!animations) return;
           document.edit_apply(EDIT_MOVE_ANIMATIONS, window.changeType,
                               [&](Anm2& anm2)
                               {
                                 animation_items_move(window, document, anm2, items,
                                                      target_child_index_get(*animations), targetGroupId);
                               });
         }});
  }

  void animation_drag_drop_source_update(Document& document, const Window& window, AnimationDragDropItem fallback)
  {
    if (!ImGui::BeginDragDropSource(DRAG_DROP_SOURCE_FLAGS)) return;

    static std::vector<AnimationDragDropItem> items{};
    items.clear();
    auto animations = document.anm2.element_get(ElementType::ANIMATIONS);
    auto isFallbackSelected = fallback.type == AnimationDragDropType::GROUP
                                  ? window.selection.contains(fallback.id)
                                  : document.animation.selection.contains(fallback.id);
    if (animations && isFallbackSelected)
    {
      auto groupIds = edit::animation_group_ids_get(*animations);
      for (auto groupId : window.selection)
        if (groupIds.contains(groupId)) items.push_back({AnimationDragDropType::GROUP, groupId});
      for (auto animationIndex : document.animation.selection)
        if (animations_child_index_get(*animations, animationIndex) != -1)
          items.push_back({AnimationDragDropType::ANIMATION, animationIndex});
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

  bool animation_group_drag_drop_update(Window& window, Manager& manager, int groupId, ImVec2 itemMin, ImVec2 itemMax)
  {
    if (!ImGui::BeginDragDropTarget()) return false;
    auto items = animation_drag_drop_target_items_get();
    auto isDelivered = false;
    if (!items.empty())
    {
      auto dropZone = animation_drop_zone_get(itemMin, itemMax);
      auto isGroupDragged = std::ranges::any_of(items, [](const AnimationDragDropItem& item)
                                                { return item.type == AnimationDragDropType::GROUP; });
      auto isDropIntoGroup = dropZone == AnimationDropZone::INSIDE && !isGroupDragged;
      if (isDropIntoGroup)
        drop_box_draw(ImGui::GetWindowDrawList(), itemMin, itemMax);
      else
        drop_line_draw(ImGui::GetWindowDrawList(), itemMin, itemMax, dropZone != AnimationDropZone::BEFORE);

      isDelivered = ImGui::GetDragDropPayload()->IsDelivery();
      if (isDelivered)
      {
        auto targetDropZone =
            dropZone == AnimationDropZone::INSIDE && !isDropIntoGroup ? AnimationDropZone::AFTER : dropZone;
        animation_move_command_push(
            window, manager, items, isDropIntoGroup ? groupId : -1, [groupId, targetDropZone](const Element& animations)
            { return animation_group_child_insert_index_get(animations, groupId, targetDropZone); });
      }
    }
    ImGui::EndDragDropTarget();
    return isDelivered;
  }

  bool animation_row_drag_drop_update(Window& window, Manager& manager, Document& document, const Element& animation,
                                      int index)
  {
    animation_drag_drop_source_update(document, window, {AnimationDragDropType::ANIMATION, index});
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
      if (isDelivered)
        animation_move_command_push(window, manager, items, animation.groupId,
                                    [targetIndex = index + (isDropAfter ? 1 : 0)](const Element& animations)
                                    { return animations_child_insert_index_get(animations, targetIndex); });
    }
    ImGui::EndDragDropTarget();
    return isDelivered;
  }

  void animation_group_set_push(Manager& manager, int groupId, StringType edit, std::optional<std::string> name,
                                std::optional<bool> isExpanded)
  {
    manager.command_push({manager.selected, [groupId, edit, name, isExpanded](Manager&, Document& document)
                          {
                            auto animations = document.anm2.element_get(ElementType::ANIMATIONS);
                            auto group = animations ? child_id_get(*animations, ElementType::GROUP, groupId) : nullptr;
                            if (!group || (name.value_or(group->name) == group->name &&
                                           isExpanded.value_or(group->isExpanded) == group->isExpanded))
                              return;
                            document.edit_apply(edit, Document::ANIMATIONS,
                                                [&](Anm2&)
                                                {
                                                  group->name = name.value_or(group->name);
                                                  group->isExpanded = isExpanded.value_or(group->isExpanded);
                                                });
                          }});
  }

  void animation_group_draw(Window& window, Manager& manager, Resources& resources, Document& document, Element& group,
                            int arrowSelectionId, bool& isRowsDone)
  {
    auto& groupSelection = window.selection;
    auto container = window_container_get(window, document);
    auto renameKey = window_group_key_get(group.id);
    auto isRenaming = window.renameId == renameKey;
    if (window.renameQueued == renameKey)
    {
      window.renameState = RENAME_FORCE_EDIT;
      window.renameQueued = -1;
    }

    ImGui::PushID("Animation Group");
    ImGui::PushID(group.id);
    ImGui::SetNextItemOpen(group.isExpanded, ImGuiCond_Always);
    auto tree =
        tree_node_input_text(group.name.empty() ? std::string(localize.get(TEXT_NEW_GROUP)) : group.name,
                             std::format("###Document #{} Animation Group #{}", manager.selected, group.id),
                             isRenaming ? window.renameText : group.name, groupSelection.contains(group.id),
                             ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth, window.renameState);
    auto itemMin = ImGui::GetItemRectMin();
    auto itemMax = ImGui::GetItemRectMax();
    if (tree.isRenameStarted)
    {
      window.renameId = renameKey;
      window.renameText = group.name;
    }
    else if (tree.isRenameFinished)
    {
      if (isRenaming) animation_group_set_push(manager, group.id, EDIT_RENAME_GROUP, window.renameText, std::nullopt);
      window.renameId = -1;
      window.renameText.clear();
    }
    if (tree.isOpen != group.isExpanded && !is_drag_drop_active() && !ImGui::IsMouseDragging(ImGuiMouseButton_Left))
      animation_group_set_push(manager, group.id, EDIT_TOGGLE_GROUP_EXPANDED, std::nullopt, tree.isOpen);

    if (tree.isClicked)
    {
      auto isCtrl = ImGui::GetIO().KeyCtrl;
      auto& selection = document.animation.selection;
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
        document.animation.reference = -1;
        document.reference = {};
        document.frames.clear();
      }
    }

    animation_drag_drop_source_update(document, window, {AnimationDragDropType::GROUP, group.id});
    isRowsDone = animation_group_drag_drop_update(window, manager, group.id, itemMin, itemMax);

    if (tree.isOpen)
    {
      int animationIndex{};
      for (auto& animation : container->children)
        if (animation.type == ElementType::ANIMATION && !isRowsDone)
        {
          if (animation.groupId == group.id)
            isRowsDone = window_row_draw(window, manager, resources, document, animation, animationIndex,
                                         animationIndex, arrowSelectionId);
          ++animationIndex;
        }
      ImGui::TreePop();
    }
    ImGui::PopID();
    ImGui::PopID();
  }

  Window animations_window_register()
  {
    Window window{};
    window.title = LABEL_ANIMATIONS_WINDOW;
    window.isOpen = &Settings::windowIsAnimations;
    window.changeType = Document::ANIMATIONS;
    window.containerType = ElementType::ANIMATIONS;
    window.elementType = ElementType::ANIMATION;
    window.childLabel = "##Animations Child";
    window.renameEdit = SNAPSHOT_RENAME_ANIMATION;
    window.flags = WINDOW_ADD | WINDOW_DUPLICATE | WINDOW_MERGE | WINDOW_GROUP | WINDOW_REMOVE | WINDOW_DEFAULT |
                   WINDOW_CUT | WINDOW_COPY | WINDOW_PASTE | WINDOW_RENAME;
    window.footer = {{WINDOW_ADD, WINDOW_DUPLICATE, WINDOW_MERGE, WINDOW_REMOVE, WINDOW_DEFAULT}};
    window.tooltips = {{WINDOW_ADD, TOOLTIP_ADD_ANIMATION},
                       {WINDOW_DUPLICATE, TOOLTIP_DUPLICATE_ANIMATION},
                       {WINDOW_MERGE, TOOLTIP_OPEN_MERGE_POPUP},
                       {WINDOW_REMOVE, TOOLTIP_REMOVE_ANIMATION},
                       {WINDOW_DEFAULT, TOOLTIP_SET_DEFAULT_ANIMATION}};
    window.enabled = {
        {WINDOW_MERGE,
         [](Window& window, Document& document) { return !animation_selected_indices_get(document, window).empty(); }},
        {WINDOW_GROUP, [](Window&, Document& document) { return !animation_groupable_indices_get(document).empty(); }}};
    window.popup = PopupHelper(LABEL_ANIMATIONS_MERGE_POPUP);
    window.storage_get = [](Document& document) -> Storage& { return document.animation; };
    window.element_get = [](Anm2& anm2, int index) { return anm2.element_get(ElementType::ANIMATION, index); };
    window.element_key_get = [](const Element&, int index) { return index; };
    window.row_font_get = [](Document& document, const Element& animation, int index)
    {
      auto animations = document.anm2.element_get(ElementType::ANIMATIONS);
      auto isDefault = animations && animations->defaultAnimation == animation.name;
      auto isReferenced = document.reference.animationIndex == index;
      return isDefault && isReferenced ? font::BOLD_ITALICS
             : isDefault               ? font::BOLD
             : isReferenced            ? font::ITALICS
                                       : font::REGULAR;
    };
    window.row_select = [](Window& window, Document& document, int index)
    {
      auto& io = ImGui::GetIO();
      if (!io.KeyCtrl && !io.KeyShift) window.selection.clear();
      document.reference = {index};
      document.frames.clear();
    };
    window.rename_finish = [](Document& document, Element& animation, int, int count)
    {
      auto animations = document.anm2.element_get(ElementType::ANIMATIONS);
      if (animations && count == 1) animations->defaultAnimation = animation.name;
    };
    window.tooltip_draw = [](Document& document, Resources& resources, const Element& animation)
    {
      auto animations = document.anm2.element_get(ElementType::ANIMATIONS);
      window_tooltip_name_draw(resources, animation.name);
      if (animations && animations->defaultAnimation == animation.name)
      {
        ImGui::PushFont(resources.fonts[font::ITALICS].get(), font::SIZE);
        ImGui::TextUnformatted(localize.get(BASIC_DEFAULT));
        ImGui::PopFont();
      }
      ImGui::TextUnformatted(
          std::vformat(localize.get(FORMAT_LENGTH), std::make_format_args(animation.frameNum)).c_str());
      auto loopLabel = localize.get(animation.isLoop ? BASIC_YES : BASIC_NO);
      ImGui::TextUnformatted(std::vformat(localize.get(FORMAT_LOOP), std::make_format_args(loopLabel)).c_str());
    };
    window.row_drag_drop_update = animation_row_drag_drop_update;
    window.rows_update = [](Window& window, Manager& manager, Settings& settings, Resources& resources,
                            Clipboard& clipboard, Document& document)
    {
      auto container = window_container_get(window, document);
      if (!container) return;
      auto groupIds = edit::animation_group_ids_get(*container);
      std::erase_if(window.selection, [&](int groupId) { return !groupIds.contains(groupId); });

      std::vector<int> visibleIds{};
      window.order.clear();
      int animationIndex{};
      for (auto& item : container->children)
      {
        if (item.type == ElementType::GROUP && item.isExpanded)
          visibleIds.insert_range(visibleIds.end(), edit::animation_group_indices_get(*container, item.id));
        if (item.type != ElementType::ANIMATION) continue;
        if (!is_animation_grouped(groupIds, item)) visibleIds.push_back(animationIndex);
        window.order.push_back(animationIndex++);
      }

      auto arrowSelectionId = window_selection_start(window, document, visibleIds);
      if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_A, ImGuiInputFlags_RouteFocused))
      {
        document.animation.selection = std::set<int>(window.order.begin(), window.order.end());
        window.selection = groupIds;
      }

      auto isRowsDone = false;
      animationIndex = 0;
      for (auto& item : container->children)
      {
        if (isRowsDone) break;
        if (item.type == ElementType::GROUP)
          animation_group_draw(window, manager, resources, document, item, arrowSelectionId, isRowsDone);
        if (item.type != ElementType::ANIMATION) continue;
        if (!is_animation_grouped(groupIds, item))
          isRowsDone = window_row_draw(window, manager, resources, document, item, animationIndex, animationIndex,
                                       arrowSelectionId);
        ++animationIndex;
      }
      window_selection_finish(window, manager, settings, clipboard, document, arrowSelectionId);
    };
    window.add = [](Window& window, Manager&, Settings&, Document& document, Clipboard&)
    {
      auto& selection = document.animation.selection;
      auto animations = window_container_get(window, document);
      if (!animations) return;

      auto groupIds = edit::animation_group_ids_get(*animations);
      auto targetGroupId =
          window.selection.size() == 1 && groupIds.contains(*window.selection.begin()) ? *window.selection.begin() : -1;
      auto count = animations_count_get(*animations);
      auto groupIndices =
          targetGroupId != -1 ? edit::animation_group_indices_get(*animations, targetGroupId) : std::set<int>{};
      auto index = !selection.empty()      ? std::min(*selection.rbegin() + 1, count)
                   : !groupIndices.empty() ? std::min(*groupIndices.rbegin() + 1, count)
                                           : count;
      auto selected =
          selection.empty() ? nullptr : document.anm2.element_get(ElementType::ANIMATION, *selection.rbegin());
      auto groupId = selection.empty()                                  ? targetGroupId
                     : selected && groupIds.contains(selected->groupId) ? selected->groupId
                                                                        : -1;

      document.edit_apply(EDIT_ADD_ANIMATION, window.changeType,
                          [&](Anm2& anm2)
                          {
                            auto uids = edit::animation_add(anm2, index, groupId, document.reference.animationIndex,
                                                            localize.get(TEXT_NEW_ANIMATION));
                            selection = {index};
                            window.selection.clear();
                            document.reference = {index};
                            window.newElementId = index;
                            window.scrollQueued = index;
                            return uids;
                          });
    };
    window.remove = [](Window& window, Manager&, Settings&, Document& document, Clipboard&)
    {
      auto selection = std::set<int>(document.animation.selection.begin(), document.animation.selection.end());
      auto groupSelection = window.selection;
      animations_erase(window, document, selection, groupSelection,
                       groupSelection.empty() ? EDIT_REMOVE_ANIMATIONS : EDIT_REMOVE_GROUP);
    };
    window.cut = [](Window& window, Manager&, Settings&, Document& document, Clipboard& clipboard)
    {
      if (auto text = animation_clipboard_text_get(document, window); !text.empty()) clipboard.set(text);
      auto selectedIndices = animation_selected_indices_get(document, window);
      auto groupSelection = window.selection;
      animations_erase(window, document, selectedIndices, groupSelection, EDIT_CUT_ANIMATIONS);
    };
    window.copy = [](Window& window, Manager&, Settings&, Document& document, Clipboard& clipboard)
    {
      if (auto text = animation_clipboard_text_get(document, window); !text.empty()) clipboard.set(text);
    };
    window.duplicate = [](Window& window, Manager&, Settings&, Document& document, Clipboard&)
    {
      auto animations = window_container_get(window, document);
      auto text = animation_clipboard_text_get(document, window);
      if (!animations || text.empty()) return;
      auto selectedIndices = animation_selected_indices_get(document, window);
      auto count = animations_count_get(*animations);
      animations_insert(window, document, text,
                        selectedIndices.empty() ? count : std::min(*selectedIndices.rbegin() + 1, count), -1,
                        EDIT_DUPLICATE_ANIMATIONS);
    };
    window.paste = [](Window& window, Manager&, Settings&, Document& document, Clipboard& clipboard)
    {
      auto animations = window_container_get(window, document);
      if (clipboard.is_empty() || !animations) return;
      auto& selection = document.animation.selection;
      auto count = animations_count_get(*animations);
      auto groupIds = edit::animation_group_ids_get(*animations);
      auto targetGroupId =
          selection.empty() && window.selection.size() == 1 && groupIds.contains(*window.selection.begin())
              ? *window.selection.begin()
              : -1;
      auto groupIndices =
          targetGroupId != -1 ? edit::animation_group_indices_get(*animations, targetGroupId) : std::set<int>{};
      auto start = !selection.empty()      ? *selection.rbegin() + 1
                   : !groupIndices.empty() ? std::min(*groupIndices.rbegin() + 1, count)
                                           : count;
      animations_insert(window, document, clipboard.get(), start, targetGroupId, EDIT_PASTE_ANIMATIONS);
    };
    window.merge = [](Window& window, Manager&, Settings& settings, Document& document, Clipboard&)
    {
      auto& mergeSelection = document.merge.selection;
      auto quickSelection = animation_selected_indices_get(document, window);
      auto quickGroupSelection = window.selection;
      auto merged = animations_merge(document,
                                     {.selection = mergeSelection,
                                      .reference = document.merge.reference,
                                      .type = (merge::Type)settings.mergeType,
                                      .isDeleteAnimationsAfter = settings.mergeIsDeleteAnimationsAfter},
                                     &quickSelection, &quickGroupSelection);
      if (merged != -1)
      {
        window.scrollQueued = merged;
        window.selection.clear();
      }
      mergeSelection.clear();
    };
    window.merge_open = [](Window& window, Manager&, Settings&, Document& document, Clipboard&)
    {
      if (document.animation.selection.empty()) return;
      window.popup.open();
      document.merge.selection.clear();
      document.merge.reference = *document.animation.selection.begin();
    };
    window.group = [](Window& window, Manager&, Settings&, Document& document, Clipboard&)
    {
      auto targetIndices = animation_groupable_indices_get(document);
      auto animations = window_container_get(window, document);
      if (targetIndices.empty() || !animations) return;

      std::set<int> targetSet(targetIndices.begin(), targetIndices.end());
      auto groupId = element_child_next_id_get(*animations, ElementType::GROUP);
      document.edit_apply(EDIT_GROUP_ITEMS, window.changeType,
                          [&](Anm2& anm2)
                          {
                            auto uids = edit::animations_group(anm2, targetSet, localize.get(TEXT_NEW_GROUP));
                            document.animation.selection = targetSet;
                            window.selection = {groupId};
                            document.reference = {*targetSet.begin()};
                            window.scrollQueued = *targetSet.begin();
                            return uids;
                          });
    };
    window.default_set = [](Window& window, Manager&, Settings&, Document& document, Clipboard&)
    {
      auto animations = window_container_get(window, document);
      auto animation = document.anm2.element_get(ElementType::ANIMATION, *document.animation.selection.begin());
      if (!animations || !animation) return;
      document.edit_apply(EDIT_DEFAULT_ANIMATION, window.changeType,
                          [&](Anm2&) { animations->defaultAnimation = animation->name; });
    };
    window.body_update =
        [](Window& window, Manager& manager, Settings& settings, Resources&, Clipboard&, Document& document)
    {
      auto& mergeSelection = document.merge.selection;
      auto mergeReference = document.merge.reference;

      window.popup.trigger();
      if (!ImGui::BeginPopupModal(window.popup.label(), &window.popup.isOpen, ImGuiWindowFlags_NoResize)) return;

      struct AnimationMergeCandidate
      {
        int key{};
        std::string label{};
      };
      std::vector<AnimationMergeCandidate> candidates{};
      auto animations = window_container_get(window, document);
      auto groupIds = animations ? edit::animation_group_ids_get(*animations) : std::set<int>{};
      static const Element EMPTY{};
      auto candidate_animation_push = [&](const Element& animation, int animationIndex, bool isGrouped)
      {
        if (animationIndex != mergeReference)
          candidates.push_back({animationIndex, isGrouped ? std::format("  {}", animation.name) : animation.name});
      };

      int animationIndex{};
      for (const auto& item : (animations ? *animations : EMPTY).children)
      {
        if (item.type == ElementType::GROUP)
        {
          auto groupIndices = edit::animation_group_indices_get(*animations, item.id);
          groupIndices.erase(mergeReference);
          if (groupIndices.empty()) continue;
          candidates.push_back({window_group_key_get(item.id),
                                item.name.empty() ? std::string(localize.get(TEXT_NEW_GROUP)) : item.name});
          int groupAnimationIndex{};
          for (const auto& animation : animations->children)
            if (animation.type == ElementType::ANIMATION)
            {
              if (animation.groupId == item.id) candidate_animation_push(animation, groupAnimationIndex, true);
              ++groupAnimationIndex;
            }
        }
        if (item.type != ElementType::ANIMATION) continue;
        if (!is_animation_grouped(groupIds, item)) candidate_animation_push(item, animationIndex, false);
        ++animationIndex;
      }

      auto footerSize = footer_size_get();
      auto optionsSize = child_size_get(MERGE_OPTIONS_ROWS);
      auto deleteAfterSize = child_size_get();
      auto animationsSize =
          ImVec2(0, ImGui::GetContentRegionAvail().y - (optionsSize.y + deleteAfterSize.y + footerSize.y +
                                                        ImGui::GetStyle().ItemSpacing.y * MERGE_POPUP_CHILD_COUNT));

      if (ImGui::BeginChild(localize.get(LABEL_ANIMATIONS_CHILD), animationsSize, ImGuiChildFlags_Borders))
      {
        std::vector<int> candidateKeys{};
        for (const auto& candidate : candidates)
          candidateKeys.push_back(candidate.key);
        mergeSelection.set_index_map(&candidateKeys);
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

      auto result = window_popup_buttons_draw(
          manager, localize.get(LABEL_MERGE),
          !animation_merge_indices_get(document, mergeSelection, mergeReference).empty(), LABEL_CLOSE);
      if (result == PopupButton::CONFIRM)
        manager.command_push(
            {manager.selected,
             [options = AnimationMergeOptions{.selection = mergeSelection,
                                              .reference = mergeReference,
                                              .type = (merge::Type)settings.mergeType,
                                              .isDeleteAnimationsAfter = settings.mergeIsDeleteAnimationsAfter}](
                 Manager&, Document& document) { animations_merge(document, options); }});
      if (result != PopupButton::NONE)
      {
        mergeSelection.clear();
        window.popup.close();
      }
      ImGui::EndPopup();
    };
    window.post_update = [](Window& window, Manager& manager, Settings&, Resources&, Clipboard&, Document& document)
    {
      auto isNext = shortcut(manager.chords[SHORTCUT_NEXT_ANIMATION], shortcut::GLOBAL);
      auto isPrevious = shortcut(manager.chords[SHORTCUT_PREVIOUS_ANIMATION], shortcut::GLOBAL);
      auto animations = window_container_get(window, document);
      auto count = animations ? animations_count_get(*animations) : 0;
      if ((!isPrevious && !isNext) || count <= 0) return;

      auto& reference = document.reference;
      reference.animationIndex = std::clamp(reference.animationIndex + (isNext ? 1 : -1), 0, count - 1);
      document.animation.selection = {reference.animationIndex};
      window.scrollQueued = reference.animationIndex;
    };
    return window;
  }
}
