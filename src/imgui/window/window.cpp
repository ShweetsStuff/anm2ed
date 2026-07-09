#include "window.hpp"

#include <algorithm>
#include <filesystem>
#include <format>
#include <set>
#include <utility>
#include <vector>

#include "actions.hpp"
#include "log.hpp"
#include "math.hpp"
#include "path.hpp"
#include "strings.hpp"
#include "toast.hpp"
#include "util/imgui/draw.hpp"
#include "util/imgui/input.hpp"
#include "util/imgui/layout.hpp"
#include "util/imgui/shortcut.hpp"
#include "util/imgui/tooltip.hpp"
#include "util/imgui/tree.hpp"
#include "vector.hpp"
#include "working_directory.hpp"

using namespace anm2ed::resource;
using namespace anm2ed::types;
using namespace anm2ed::util;
using namespace glm;

namespace anm2ed::imgui
{
  static constexpr auto PADDING_MAX = 100;
  static constexpr auto MERGE_APPEND_RIGHT = 0;
  static constexpr auto MERGE_APPEND_BOTTOM = 1;

  struct SpritesheetMergeOptions
  {
    bool isAppendRight{};
    bool isMakeRegions{};
    bool isMakePrimaryRegion{};
    origin::Type regionOrigin{};
  };

  struct AnimationMergeOptions
  {
    std::set<int> selection{};
    int reference{-1};
    merge::Type type{};
    bool isDeleteAnimationsAfter{};
  };

  int window_element_count_get(const Window& window, const Element* container)
  {
    int count{};
    if (!container) return count;
    for (const auto& element : container->children)
      if (element.type == window.elementType) ++count;
    return count;
  }

  Element* window_container_get(const Window& window, Anm2& anm2) { return anm2.element_get(window.containerType); }

  Element* window_element_get(const Window& window, Anm2& anm2, int key)
  {
    if (window.element_get) return window.element_get(anm2, key);
    auto container = window_container_get(window, anm2);
    return container ? element_child_id_get(*container, window.elementType, key) : nullptr;
  }

  int region_insert_index_get(const Element& spritesheet, const Storage& storage)
  {
    int index = (int)spritesheet.children.size();
    for (int i = 0; i < (int)spritesheet.children.size(); i++)
    {
      const auto& child = spritesheet.children[i];
      if (child.type != ElementType::REGION) continue;
      if (storage.selection.contains(child.id) || (storage.selection.empty() && storage.reference == child.id))
        index = i + 1;
    }
    return index;
  }

  int window_track_id_get(const Element& item, ElementType trackType)
  {
    return trackType == ElementType::LAYER_ANIMATION ? item.layerId : item.nullId;
  }

  void window_track_shell_push(Element& destination, std::set<int>& copiedTrackIds, const Element& source,
                               ElementType trackType, int groupId)
  {
    if (source.type != trackType) return;
    auto trackId = window_track_id_get(source, trackType);
    if (copiedTrackIds.contains(trackId)) return;
    copiedTrackIds.insert(trackId);

    auto item = element_make(trackType);
    item.layerId = source.layerId;
    item.nullId = source.nullId;
    item.groupId = source.groupId != -1 ? source.groupId : groupId;
    item.isVisible = source.isVisible;
    destination.children.push_back(item);
  }

  void window_group_root_push(Element& group)
  {
    auto root = element_make(ElementType::ROOT_ANIMATION);
    root.children.push_back(element_make(ElementType::FRAME));
    group.children.push_back(root);
  }

  Element window_track_container_shell_copy(const Element* source, ElementType containerType, ElementType trackType)
  {
    auto destination = element_make(containerType);
    if (!source) return destination;

    auto nextGroupId = element_child_next_id_get(*source, ElementType::GROUP);
    std::set<int> copiedTrackIds{};
    for (const auto& sourceItem : source->children)
    {
      if (sourceItem.type != ElementType::GROUP)
      {
        window_track_shell_push(destination, copiedTrackIds, sourceItem, trackType, -1);
        continue;
      }

      auto group = element_make(ElementType::GROUP);
      group.id = sourceItem.id >= 0 ? sourceItem.id : nextGroupId++;
      group.name = sourceItem.name;
      group.isExpanded = sourceItem.isExpanded;
      group.isVisible = sourceItem.isVisible;
      window_group_root_push(group);
      destination.children.push_back(group);

      for (const auto& child : sourceItem.children)
        window_track_shell_push(destination, copiedTrackIds, child, trackType, group.id);
    }
    return destination;
  }

  void window_edit(Window& window, Document& document, const std::string& message, auto behavior)
  {
    document.anm2_snapshot(message);
    behavior();
    document.anm2_change(window.changeType);
  }

  void window_edit(Document& document, Document::ChangeType changeType, const std::string& message, auto behavior)
  {
    document.anm2_snapshot(message);
    behavior();
    document.anm2_change(changeType);
  }

  void window_rename_finish(Window& window, Manager& manager, int key, int count, const std::string& name)
  {
    auto changeType = window.changeType;
    auto containerType = window.containerType;
    auto elementType = window.elementType;
    auto elementGet = window.element_get;
    auto renameEdit = window.renameEdit;
    auto renameFinish = window.rename_finish;
    manager.command_push({manager.selected,
                          [changeType, containerType, elementType, elementGet, renameEdit, renameFinish, key, count,
                           name](Manager&, Document& document) mutable
                          {
                            auto element = elementGet ? elementGet(document.anm2, key) : nullptr;
                            if (!element)
                              if (auto container = document.anm2.element_get(containerType))
                                element = element_child_id_get(*container, elementType, key);
                            if (!element || element->name == name) return;

                            document.anm2_snapshot(localize.get(renameEdit));
                            element->name = name;
                            if (renameFinish) renameFinish(document, *element, key, count);
                            document.anm2_change(changeType);
                          }});
  }

  void window_command_push(Window& window, Manager& manager, Settings& settings, Clipboard& clipboard,
                           const Window::Command& command)
  {
    if (!command) return;
    auto queuedCommand = command;
    manager.command_push({manager.selected,
                          [&window, &settings, &clipboard, queuedCommand](Manager& manager, Document& document) mutable
                          { queuedCommand(window, manager, settings, document, clipboard); }});
  }

  void window_copy(Window& window, Manager& manager, Settings& settings, Document& document, Clipboard& clipboard)
  {
    if (window.copy)
    {
      window_command_push(window, manager, settings, clipboard, window.copy);
      return;
    }

    auto& selection = window.storage_get(document).selection;
    if (selection.empty()) return;

    std::string clipboardText{};
    for (auto key : selection)
      if (auto element = window_element_get(window, document.anm2, key)) clipboardText += element_to_string(*element);
    clipboard.set(clipboardText);
  }

  void window_paste(Window& window, Manager& manager, Settings& settings, Document&, Clipboard& clipboard)
  {
    if (window.paste)
    {
      window_command_push(window, manager, settings, clipboard, window.paste);
      return;
    }

    if (clipboard.is_empty()) return;

    auto paste = [](Window& window, Manager&, Settings&, Document& document, Clipboard& clipboard)
    {
      auto storage = &window.storage_get(document);
      auto container = window_container_get(window, document.anm2);
      auto maxIdBefore = container ? element_child_max_id_get(*container, window.elementType) : -1;
      auto pasted = document.anm2;
      std::string errorString{};

      if (pasted.deserialize(window.elementType, clipboard.get(), true, &errorString, document.directory_get()))
      {
        document.anm2_snapshot(localize.get(window.pasteEdit));
        document.anm2 = std::move(pasted);
        container = window_container_get(window, document.anm2);
        auto maxIdAfter = container ? element_child_max_id_get(*container, window.elementType) : -1;
        if (maxIdAfter > maxIdBefore)
        {
          window.newElementId = maxIdAfter;
          storage->selection = {maxIdAfter};
          storage->reference = maxIdAfter;
        }
        document.anm2_change(window.changeType);
        return;
      }

      if (window.deserializeFailedToast)
      {
        toasts.push(std::vformat(localize.get(window.deserializeFailedToast), std::make_format_args(errorString)));
        logger.error(std::vformat(localize.get(window.deserializeFailedToast, anm2ed::ENGLISH),
                                  std::make_format_args(errorString)));
      }
    };

    window_command_push(window, manager, settings, clipboard, paste);
  }

  void window_remove_unused(Window& window, Manager& manager, Settings& settings, Document&, Clipboard& clipboard)
  {
    auto removeUnused = [](Window& window, Manager&, Settings&, Document& document, Clipboard&)
    {
      auto unused = document.anm2.element_unused(window.elementType);
      if (unused.empty()) return;

      auto behavior = [&]()
      {
        auto container = window_container_get(window, document.anm2);
        if (!container) return;
        for (auto id : unused)
          element_child_id_erase(*container, window.elementType, id);
      };

      window_edit(window, document, localize.get(window.removeUnusedEdit), behavior);
    };

    window_command_push(window, manager, settings, clipboard, removeUnused);
  }

  void window_add(Window& window, Manager& manager, Settings& settings, Document&, Clipboard& clipboard)
  {
    if (window.add)
    {
      window_command_push(window, manager, settings, clipboard, window.add);
      return;
    }

    if (window.properties_open) window.properties_open(manager, -1);
  }

  void window_command_run(Window& window, Manager& manager, Settings& settings, Document&, Clipboard& clipboard,
                          const Window::Command& command)
  {
    window_command_push(window, manager, settings, clipboard, command);
  }

  void window_properties(Window& window, Manager& manager, int id)
  {
    if (window.properties_open) window.properties_open(manager, id);
  }

  void window_scroll_to_item(float rowHeight, bool isTarget)
  {
    if (!isTarget) return;
    auto windowHeight = ImGui::GetWindowHeight();
    auto targetTop = ImGui::GetCursorPosY();
    auto targetBottom = targetTop + rowHeight;
    auto visibleTop = ImGui::GetScrollY();
    auto visibleBottom = visibleTop + windowHeight;
    if (targetTop < visibleTop)
      ImGui::SetScrollY(targetTop);
    else if (targetBottom > visibleBottom)
      ImGui::SetScrollY(targetBottom - windowHeight);
  }

  int window_arrow_selection_get(const std::vector<int>& ids, int reference, const std::set<int>& selection)
  {
    if (ids.empty()) return -1;
    auto current = reference;
    if (current == -1 && !selection.empty()) current = *selection.begin();
    auto it = std::find(ids.begin(), ids.end(), current);
    auto index = it == ids.end() ? 0 : (int)std::distance(ids.begin(), it);
    auto delta = ImGui::IsKeyPressed(ImGuiKey_UpArrow, true) ? -1 : 1;
    index = std::clamp(index + delta, 0, (int)ids.size() - 1);
    return ids[index];
  }

  std::filesystem::path window_asset_path_get(Document& document, const std::filesystem::path& path)
  {
    auto loadPath = path::backslash_handle(path);
    return path::backslash_replace(path::make_relative(loadPath, document.directory_get()));
  }

  void window_directory_open(Dialog& dialog, Document& document, const std::filesystem::path& path)
  {
    if (path.empty()) return;
    std::error_code ec{};
    auto absolutePath = std::filesystem::weakly_canonical(document.directory_get() / path, ec);
    if (ec) absolutePath = document.directory_get() / path;
    auto target = std::filesystem::is_directory(absolutePath)                 ? absolutePath
                  : std::filesystem::is_directory(absolutePath.parent_path()) ? absolutePath.parent_path()
                                                                              : document.directory_get();
    dialog.file_explorer_open(target);
  }

  int window_animation_count_get(Anm2& anm2)
  {
    int count{};
    auto animations = anm2.element_get(ElementType::ANIMATIONS);
    if (!animations) return count;
    for (auto& animation : animations->children)
      if (animation.type == ElementType::ANIMATION) ++count;
    return count;
  }

  int window_animation_child_index_get(const Element& animations, int animationIndex)
  {
    int current{};
    for (int i = 0; i < (int)animations.children.size(); ++i)
    {
      if (animations.children[i].type != ElementType::ANIMATION) continue;
      if (current == animationIndex) return i;
      ++current;
    }
    return -1;
  }

  int window_animation_child_insert_index_get(const Element& animations, int animationIndex)
  {
    if (animationIndex <= 0)
    {
      for (int i = 0; i < (int)animations.children.size(); ++i)
        if (animations.children[i].type == ElementType::ANIMATION) return i;
      return (int)animations.children.size();
    }
    int current{};
    for (int i = 0; i < (int)animations.children.size(); ++i)
    {
      if (animations.children[i].type != ElementType::ANIMATION) continue;
      if (current == animationIndex) return i;
      ++current;
    }
    return (int)animations.children.size();
  }

  int window_animation_index_from_child_index_get(const Element& animations, int childIndex)
  {
    int current{};
    for (int i = 0; i < (int)animations.children.size(); ++i)
    {
      if (animations.children[i].type != ElementType::ANIMATION) continue;
      if (i == childIndex) return current;
      ++current;
    }
    return -1;
  }

  std::set<int> window_animation_group_ids_get(const Element& animations)
  {
    std::set<int> result{};
    for (const auto& item : animations.children)
      if (item.type == ElementType::GROUP) result.insert(item.id);
    return result;
  }

  bool is_window_animation_grouped(const std::set<int>& groupIds, const Element& animation)
  {
    return animation.groupId != -1 && groupIds.contains(animation.groupId);
  }

  std::vector<int> window_animation_groupable_indices_get(Document& document)
  {
    std::vector<int> result{};
    auto animations = document.anm2.element_get(ElementType::ANIMATIONS);
    if (!animations || document.animation.selection.empty()) return result;

    auto groupIds = window_animation_group_ids_get(*animations);
    int index{};
    for (const auto& animation : animations->children)
    {
      if (animation.type != ElementType::ANIMATION) continue;
      if (document.animation.selection.contains(index))
      {
        if (is_window_animation_grouped(groupIds, animation)) return {};
        result.push_back(index);
      }
      ++index;
    }
    if (result.size() != document.animation.selection.size()) return {};
    return result;
  }

  int window_animation_group_key_get(int groupId) { return -groupId - 2; }

  bool is_window_animation_group_key(int key) { return key <= -2; }

  int window_animation_group_id_from_key_get(int key) { return -key - 2; }

  bool is_window_animation_group_selected(const Window& window)
  {
    return window.elementType == ElementType::ANIMATION && !window.selection.empty();
  }

  std::set<int> window_animation_group_indices_get(const Element& animations, int groupId)
  {
    std::set<int> result{};
    int animationIndex{};
    for (const auto& animation : animations.children)
    {
      if (animation.type != ElementType::ANIMATION) continue;
      if (animation.groupId == groupId) result.insert(animationIndex);
      ++animationIndex;
    }
    return result;
  }

  std::set<int> window_animation_selected_indices_get(Document& document, const Window& window)
  {
    std::set<int> result(document.animation.selection.begin(), document.animation.selection.end());
    auto animations = document.anm2.element_get(ElementType::ANIMATIONS);
    if (!animations || window.elementType != ElementType::ANIMATION) return result;

    for (auto groupId : window.selection)
    {
      auto indices = window_animation_group_indices_get(*animations, groupId);
      result.insert(indices.begin(), indices.end());
    }
    return result;
  }

  std::set<int> window_animation_merge_indices_get(Document& document, const std::set<int>& keys, int reference)
  {
    std::set<int> result{};
    auto animations = document.anm2.element_get(ElementType::ANIMATIONS);

    for (auto key : keys)
    {
      if (is_window_animation_group_key(key))
      {
        if (!animations) continue;
        auto indices = window_animation_group_indices_get(*animations, window_animation_group_id_from_key_get(key));
        result.insert(indices.begin(), indices.end());
      }
      else
        result.insert(key);
    }

    result.erase(reference);
    return result;
  }

  void window_animation_groups_remove(Document& document, const std::set<int>& groupIds)
  {
    auto animations = document.anm2.element_get(ElementType::ANIMATIONS);
    if (!animations || groupIds.empty()) return;

    for (auto& item : animations->children)
      if (item.type == ElementType::ANIMATION && groupIds.contains(item.groupId)) item.groupId = -1;
    std::erase_if(animations->children, [&](const Element& item)
    { return item.type == ElementType::GROUP && groupIds.contains(item.id); });
  }

  std::string window_animation_clipboard_text_get(Document& document, const Window& window)
  {
    std::string clipboardText{};
    auto animations = document.anm2.element_get(ElementType::ANIMATIONS);
    if (!animations) return clipboardText;

    int animationIndex{};
    for (const auto& item : animations->children)
    {
      if (item.type == ElementType::GROUP)
      {
        if (window.selection.contains(item.id)) clipboardText += element_to_string(item);
      }
      else if (item.type == ElementType::ANIMATION)
      {
        if (document.animation.selection.contains(animationIndex) || window.selection.contains(item.groupId))
          clipboardText += element_to_string(item);
        ++animationIndex;
      }
    }

    return clipboardText;
  }

  bool is_window_item_selected(Window& window, Document& document)
  {
    return !window.storage_get(document).selection.empty() || is_window_animation_group_selected(window);
  }

  bool is_window_item_renameable(Window& window, Document& document)
  {
    auto& selection = window.storage_get(document).selection;
    return selection.size() == 1 ||
           (window.elementType == ElementType::ANIMATION && selection.empty() && window.selection.size() == 1);
  }

  bool is_window_merge_available(Window& window, Document& document)
  {
    auto& selection = window.storage_get(document).selection;
    if (window.elementType != ElementType::ANIMATION) return !selection.empty();
    return !window_animation_selected_indices_get(document, window).empty();
  }

  int window_animations_merge(Document& document, const AnimationMergeOptions& options,
                              const std::set<int>* quickSelection = nullptr,
                              const std::set<int>* quickGroupSelection = nullptr)
  {
    auto& anm2 = document.anm2;
    auto& selection = document.animation.selection;
    auto& reference = document.reference;
    auto& overlayIndex = document.overlayIndex;
    int merged{-1};

    document.anm2_snapshot(localize.get(EDIT_MERGE_ANIMATIONS));
    if (options.selection.empty())
    {
      auto selected = quickSelection ? *quickSelection : std::set<int>(selection.begin(), selection.end());
      auto isQuickGroupMerge = quickGroupSelection && !quickGroupSelection->empty();
      if (selected.contains(overlayIndex)) overlayIndex = -1;

      if (selected.size() > 1)
        merged = anm2.animations_merge(*selected.begin(), selected);
      else if (selected.size() == 1 && isQuickGroupMerge)
        merged = *selected.begin();
      else if (selected.size() == 1 && *selected.begin() != window_animation_count_get(anm2) - 1)
      {
        auto start = *selected.begin();
        auto next = *selected.begin() + 1;
        std::set<int> animationSet{};
        animationSet.insert(start);
        animationSet.insert(next);
        merged = anm2.animations_merge(start, animationSet);
      }
      else
        return -1;

      if (isQuickGroupMerge) window_animation_groups_remove(document, *quickGroupSelection);
    }
    else
    {
      auto mergeSelection = window_animation_merge_indices_get(document, options.selection, options.reference);
      if (mergeSelection.empty()) return -1;
      if (mergeSelection.contains(overlayIndex)) overlayIndex = -1;
      merged = anm2.animations_merge(options.reference, mergeSelection, options.type, options.isDeleteAnimationsAfter);
    }

    if (merged == -1) return -1;
    selection = {merged};
    reference = {merged};
    document.anm2_change(Document::ANIMATIONS);
    return merged;
  }

  void window_spritesheets_merge(Document& document, const std::set<int>& ids,
                                 const SpritesheetMergeOptions& options)
  {
    if (ids.size() <= 1) return;

    auto behavior = [&]()
    {
      auto baseID = *ids.begin();
      if (document.spritesheets_merge(ids, options.isAppendRight, options.isMakeRegions, options.isMakePrimaryRegion,
                                      options.regionOrigin))
      {
        document.spritesheet.selection = {baseID};
        document.spritesheet.reference = baseID;
        document.region.reference = -1;
        document.region.selection.clear();
        toasts.push(localize.get(TOAST_MERGE_SPRITESHEETS));
        logger.info(localize.get(TOAST_MERGE_SPRITESHEETS, anm2ed::ENGLISH));
      }
      else
      {
        toasts.push(localize.get(TOAST_MERGE_SPRITESHEETS_FAILED));
        logger.error(localize.get(TOAST_MERGE_SPRITESHEETS_FAILED, anm2ed::ENGLISH));
      }
    };

    document.anm2_textures_snapshot(localize.get(EDIT_MERGE_SPRITESHEETS));
    behavior();
    document.change(Document::ALL);
  }

  void window_spritesheet_pack(Document& document, int id, int padding)
  {
    if (id == -1) return;
    auto spritesheet = document.anm2.element_get(ElementType::SPRITESHEET, id);
    if (!spritesheet) return;
    bool isRegionsEmpty = true;
    for (auto& child : spritesheet->children)
      if (child.type == ElementType::REGION) isRegionsEmpty = false;
    if (isRegionsEmpty) return;

    auto behavior = [&]()
    {
      if (document.spritesheet_pack(id, std::max(0, padding)))
      {
        toasts.push(localize.get(TOAST_PACK_SPRITESHEET));
        logger.info(localize.get(TOAST_PACK_SPRITESHEET, anm2ed::ENGLISH));
      }
      else
      {
        toasts.push(localize.get(TOAST_PACK_SPRITESHEET_FAILED));
        logger.error(localize.get(TOAST_PACK_SPRITESHEET_FAILED, anm2ed::ENGLISH));
      }
    };

    document.anm2_textures_snapshot(localize.get(EDIT_PACK_SPRITESHEET));
    behavior();
    document.change(Document::SPRITESHEETS);
  }

  void window_spritesheets_save(Document& document, const std::set<int>& ids)
  {
    if (ids.empty()) return;

    for (auto& id : ids)
    {
      auto spritesheet = document.anm2.element_get(ElementType::SPRITESHEET, id);
      auto texture = document.texture_get(id);
      if (!spritesheet || !texture) continue;
      auto pathString = path::to_utf8(spritesheet->path);
      WorkingDirectory workingDirectory(document.directory_get());
      path::ensure_directory(spritesheet->path.parent_path());
      if (texture->write_png(spritesheet->path))
      {
        document.spritesheet_hash_set_saved(id);
        toasts.push(std::vformat(localize.get(TOAST_SAVE_SPRITESHEET), std::make_format_args(id, pathString)));
        logger.info(std::vformat(localize.get(TOAST_SAVE_SPRITESHEET, anm2ed::ENGLISH),
                                 std::make_format_args(id, pathString)));
      }
      else
      {
        toasts.push(std::vformat(localize.get(TOAST_SAVE_SPRITESHEET_FAILED), std::make_format_args(id, pathString)));
        logger.error(std::vformat(localize.get(TOAST_SAVE_SPRITESHEET_FAILED, anm2ed::ENGLISH),
                                  std::make_format_args(id, pathString)));
      }
    }
  }

  void window_overlays_save(Document& document, const std::set<int>& ids)
  {
    if (ids.empty()) return;

    for (auto& id : ids)
    {
      auto overlay = document.overlay_get(id);
      auto texture = document.overlay_texture_get(id);
      if (!overlay || !texture) continue;
      auto pathString = path::to_utf8(overlay->path);
      WorkingDirectory workingDirectory(document.directory_get());
      path::ensure_directory(overlay->path.parent_path());
      if (texture->write_png(overlay->path))
      {
        document.overlay_hash_set_saved(id);
        toasts.push(std::vformat(localize.get(TOAST_SAVE_OVERLAY), std::make_format_args(id, pathString)));
        logger.info(
            std::vformat(localize.get(TOAST_SAVE_OVERLAY, anm2ed::ENGLISH), std::make_format_args(id, pathString)));
      }
      else
      {
        toasts.push(std::vformat(localize.get(TOAST_SAVE_OVERLAY_FAILED), std::make_format_args(id, pathString)));
        logger.error(std::vformat(localize.get(TOAST_SAVE_OVERLAY_FAILED, anm2ed::ENGLISH),
                                  std::make_format_args(id, pathString)));
      }
    }
  }

  int window_overlay_next_id_get(const Document& document)
  {
    int nextId{};
    if (auto spritesheets = document.anm2.element_get(ElementType::SPRITESHEETS))
      for (auto& spritesheet : spritesheets->children)
      {
        if (spritesheet.type != ElementType::SPRITESHEET) continue;
        for (auto& overlay : spritesheet.children)
          if (overlay.type == ElementType::OVERLAY) nextId = std::max(nextId, overlay.id + 1);
      }
    return nextId;
  }

  void window_overlays_context_actions_add(Window& window, Manager& manager, Settings& settings, Document& document,
                                           Clipboard& clipboard, Actions& actions)
  {
    auto& selection = document.overlay.selection;
    actions_undo_redo_add(actions, manager, document);
    actions.separator();
    actions.add(ACTION_OPEN_DIRECTORY, [&]() { return selection.size() == 1 && (bool)window.open; },
                [&]() { window_command_run(window, manager, settings, document, clipboard, window.open); });
    actions.add(ACTION_SET_FILE_PATH, [&]() { return selection.size() == 1 && (bool)window.path_set; },
                [&]() { window_command_run(window, manager, settings, document, clipboard, window.path_set); });
    actions.add(ACTION_ADD, [&]() { return (bool)window.add; },
                [&]() { window_command_run(window, manager, settings, document, clipboard, window.add); });
    actions.add(ACTION_REMOVE, [&]() { return !selection.empty() && (bool)window.remove; },
                [&]() { window_command_run(window, manager, settings, document, clipboard, window.remove); },
                TOOLTIP_REMOVE_OVERLAYS);
    actions.add(ACTION_RELOAD, [&]() { return !selection.empty() && (bool)window.reload; },
                [&]() { window_command_run(window, manager, settings, document, clipboard, window.reload); },
                TOOLTIP_RELOAD_OVERLAYS);
    actions.add(ACTION_REPLACE, [&]() { return selection.size() == 1 && (bool)window.replace; },
                [&]() { window_command_run(window, manager, settings, document, clipboard, window.replace); },
                TOOLTIP_REPLACE_OVERLAY);
    actions.add(ACTION_SAVE, [&]() { return !selection.empty(); },
                [&]()
                {
                  if (settings.fileIsWarnOverwrite)
                  {
                    window.selection2 = selection;
                    window.popup.open();
                  }
                  else if (window.save)
                    window_command_run(window, manager, settings, document, clipboard, window.save);
                },
                TOOLTIP_SAVE_OVERLAYS);
    actions.separator();
    actions.add(ACTION_COPY, [&]() { return !selection.empty() && (bool)window.copy; },
                [&]() { window_command_run(window, manager, settings, document, clipboard, window.copy); });
    actions.add(ACTION_PASTE, [&]() { return !clipboard.is_empty() && (bool)window.paste; },
                [&]() { window_command_run(window, manager, settings, document, clipboard, window.paste); });
  }

  int window_footer_button_count_get(const Window& window)
  {
    int count{};
    if (window_flag_has(window.flags, WINDOW_ADD)) ++count;
    if (window_flag_has(window.flags, WINDOW_DUPLICATE)) ++count;
    if (window_flag_has(window.flags, WINDOW_MERGE)) ++count;
    if (window_flag_has(window.flags, WINDOW_REMOVE)) ++count;
    if (window_flag_has(window.flags, WINDOW_REMOVE_UNUSED)) ++count;
    if (window_flag_has(window.flags, WINDOW_DEFAULT)) ++count;
    return count;
  }

  Actions window_footer_actions_get(Window& window, Manager& manager, Settings& settings, Document& document,
                                    Clipboard& clipboard)
  {
    Actions actions{};
    auto& selection = window.storage_get(document).selection;

    if (window_flag_has(window.flags, WINDOW_ADD))
      actions.add(ACTION_ADD, []() { return true; },
                  [&]() { window_add(window, manager, settings, document, clipboard); }, window.addTooltip);

    if (window_flag_has(window.flags, WINDOW_DUPLICATE))
      actions.add(ACTION_DUPLICATE, [&]() { return is_window_item_selected(window, document); },
                  [&]() { window_command_run(window, manager, settings, document, clipboard, window.duplicate); },
                  window.duplicateTooltip);

    if (window_flag_has(window.flags, WINDOW_MERGE))
      actions.add(ACTION_MERGE,
                  [&]()
                  {
                    if (window.elementType == ElementType::ANIMATION)
                    {
                      if (is_window_animation_group_selected(window)) return is_window_merge_available(window, document);
                      return selection.size() == 1;
                    }
                    return selection.size() == 1;
                  },
                  [&]()
                  {
                    auto command = window.merge_open && !is_window_animation_group_selected(window) ? window.merge_open
                                                                                                    : window.merge;
                    window_command_run(window, manager, settings, document, clipboard, command);
                  },
                  window.mergeTooltip);

    if (window_flag_has(window.flags, WINDOW_REMOVE))
      actions.add(ACTION_REMOVE, [&]() { return !selection.empty() || is_window_animation_group_selected(window); },
                  [&]() { window_command_run(window, manager, settings, document, clipboard, window.remove); },
                  window.removeTooltip);

    if (window_flag_has(window.flags, WINDOW_REMOVE_UNUSED))
      actions.add(ACTION_REMOVE_UNUSED, []() { return true; },
                  [&]() { window_remove_unused(window, manager, settings, document, clipboard); },
                  window.removeUnusedTooltip);

    if (window_flag_has(window.flags, WINDOW_DEFAULT))
      actions.add(ACTION_DEFAULT, [&]() { return selection.size() == 1; },
                  [&]() { window_command_run(window, manager, settings, document, clipboard, window.default_set); },
                  window.defaultTooltip);

    return actions;
  }

  Actions window_context_actions_get(Window& window, Manager& manager, Settings& settings, Document& document,
                                     Clipboard& clipboard, bool isUndoRedoIncluded = true)
  {
    Actions actions{};
    auto& selection = window.storage_get(document).selection;

    if (isUndoRedoIncluded)
    {
      actions_undo_redo_add(actions, manager, document);
      actions.separator();
    }

    if (window_flag_has(window.flags, WINDOW_RENAME))
      actions.add(ACTION_RENAME, [&]() { return is_window_item_renameable(window, document); },
                  [&]()
                  {
                    if (selection.size() == 1)
                      window.renameQueued = *selection.begin();
                    else if (window.elementType == ElementType::ANIMATION && window.selection.size() == 1)
                      window.renameQueued = window_animation_group_key_get(*window.selection.begin());
                  });
    if (window_flag_has(window.flags, WINDOW_PROPERTIES))
      actions.add(ACTION_PROPERTIES, [&]() { return selection.size() == 1; },
                  [&]() { window_properties(window, manager, *selection.begin()); });
    if (window_flag_has(window.flags, WINDOW_ADD))
      actions.add(ACTION_ADD, []() { return true; },
                  [&]() { window_add(window, manager, settings, document, clipboard); });
    if (window_flag_has(window.flags, WINDOW_DUPLICATE))
      actions.add(ACTION_DUPLICATE, [&]() { return is_window_item_selected(window, document); },
                  [&]() { window_command_run(window, manager, settings, document, clipboard, window.duplicate); });
    if (window_flag_has(window.flags, WINDOW_MERGE))
      actions.add(ACTION_MERGE, [&]() { return is_window_merge_available(window, document); },
                  [&]() { window_command_run(window, manager, settings, document, clipboard, window.merge); });
    if (window_flag_has(window.flags, WINDOW_GROUP))
      actions.add(ACTION_GROUP,
                  [&]()
                  {
                    if (window.elementType == ElementType::ANIMATION)
                      return !window_animation_groupable_indices_get(document).empty();
                    return !selection.empty();
                  },
                  [&]() { window_command_run(window, manager, settings, document, clipboard, window.group); });
    if (window_flag_has(window.flags, WINDOW_REMOVE))
      actions.add(ACTION_REMOVE, [&]() { return !selection.empty() || is_window_animation_group_selected(window); },
                  [&]() { window_command_run(window, manager, settings, document, clipboard, window.remove); });
    if (window_flag_has(window.flags, WINDOW_REMOVE_UNUSED))
      actions.add(ACTION_REMOVE_UNUSED, []() { return true; },
                  [&]() { window_remove_unused(window, manager, settings, document, clipboard); });
    if (window_flag_has(window.flags, WINDOW_DEFAULT))
      actions.add(ACTION_DEFAULT, [&]() { return selection.size() == 1; },
                  [&]() { window_command_run(window, manager, settings, document, clipboard, window.default_set); });

    actions.separator();

    if (window_flag_has(window.flags, WINDOW_CUT))
      actions.add(ACTION_CUT, [&]() { return is_window_item_selected(window, document); },
                  [&]() { window_command_run(window, manager, settings, document, clipboard, window.cut); });
    if (window_flag_has(window.flags, WINDOW_COPY))
      actions.add(ACTION_COPY, [&]() { return is_window_item_selected(window, document); },
                  [&]() { window_copy(window, manager, settings, document, clipboard); });
    if (window_flag_has(window.flags, WINDOW_PASTE))
      actions.add(ACTION_PASTE, [&]() { return !clipboard.is_empty(); },
                  [&]() { window_paste(window, manager, settings, document, clipboard); });

    return actions;
  }

  void window_footer_draw(Window& window, Manager& manager, Settings& settings, Document& document,
                          Clipboard& clipboard)
  {
    auto actions = window_footer_actions_get(window, manager, settings, document, clipboard);
    auto buttonCount = (int)actions.items.size();
    if (buttonCount == 0) return;

    auto widgetSize = widget_size_with_row_get(buttonCount);
    bool isSameLine{};
    for (auto& action : actions.items)
      action_button_draw(action, manager, settings, widgetSize, isSameLine);
  }

  void window_context_menu_draw(Window& window, Manager& manager, Settings& settings, Document& document,
                                Clipboard& clipboard)
  {
    auto actions = window_context_actions_get(window, manager, settings, document, clipboard);
    actions_context_window_draw("##Context Menu", actions, settings);
  }

  std::string window_rename_format_get(Window& window, Manager& manager, int id)
  {
    return std::format("###Document #{} Window #{} Element #{}", manager.selected, (int)window.elementType, id);
  }

  void window_rows_draw(Window& window, Manager& manager, Resources& resources, Document& document)
  {
    auto container = window_container_get(window, document.anm2);
    auto& storage = window.storage_get(document);
    auto& selection = storage.selection;
    std::vector<int> ids{};
    if (container)
      for (auto& element : container->children)
        if (element.type == window.elementType)
        {
          auto key = window.element_key_get ? window.element_key_get(element, (int)ids.size()) : element.id;
          ids.push_back(key);
        }
    auto count = (int)ids.size();
    int index{};
    int scrollTargetId = -1;
    int arrowSelectionId = -1;

    selection.start(count);
    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
        (ImGui::IsKeyPressed(ImGuiKey_UpArrow, true) || ImGui::IsKeyPressed(ImGuiKey_DownArrow, true)))
    {
      auto nextId = window_arrow_selection_get(ids, storage.reference, selection);
      if (nextId != -1)
      {
        arrowSelectionId = nextId;
        scrollTargetId = nextId;
      }
    }

    if (container)
    {
      for (auto& element : container->children)
      {
        if (element.type != window.elementType) continue;
        auto key = window.element_key_get ? window.element_key_get(element, index) : element.id;
        auto isSelected = selection.contains(key) || arrowSelectionId == key;
        auto isReferenced = window_flag_has(window.flags, WINDOW_REFERENCE_ITALIC) &&
                            (storage.reference == key || arrowSelectionId == key);
        auto font = window.row_font_get ? window.row_font_get(document, element, key)
                    : isReferenced      ? resource::font::ITALICS
                                        : resource::font::REGULAR;
        auto isFontPushed = font != resource::font::REGULAR;

        ImGui::PushID(key);
        ImGui::SetNextItemSelectionUserData(key);
        auto label = window.row_label_get ? window.row_label_get(document, element) : element.name;
        if (isFontPushed) ImGui::PushFont(resources.fonts[font].get(), resource::font::SIZE);
        if (scrollTargetId == key) ImGui::SetKeyboardFocusHere();

        if (window_flag_has(window.flags, WINDOW_RENAME))
        {
          if (window.newElementId == key || window.renameQueued == key)
          {
            window.renameState = RENAME_FORCE_EDIT;
            window.renameQueued = -1;
          }

          auto isRenaming = window.renameId == key;
          auto& name = isRenaming ? window.renameText : element.name;
          bool isActivated{};
          if (selectable_input_text(label, window_rename_format_get(window, manager, key), name, isSelected,
                                    ImGuiSelectableFlags_None, window.renameState))
          {
            isActivated = true;
            if (window.renameState == RENAME_BEGIN)
            {
              window.renameId = key;
              window.renameText = element.name;
            }
            else if (window.renameState == RENAME_FINISHED)
            {
              if (isRenaming) window_rename_finish(window, manager, key, count, window.renameText);
              window.renameId = -1;
              window.renameText.clear();
            }
          }
          auto isClicked = ImGui::IsItemClicked(ImGuiMouseButton_Left) || ImGui::IsItemClicked(ImGuiMouseButton_Right);
          if (isActivated || isClicked)
          {
            storage.reference = key;
            if (window.row_select) window.row_select(window, document, key);
          }
        }
        else
        {
          auto isActivated = ImGui::Selectable(label.c_str(), isSelected);
          auto isClicked = ImGui::IsItemClicked(ImGuiMouseButton_Left) || ImGui::IsItemClicked(ImGuiMouseButton_Right);
          if (isActivated || isClicked)
          {
            storage.reference = key;
            if (window.row_select) window.row_select(window, document, key);
          }
        }

        if (isFontPushed) ImGui::PopFont();

        if (window.newElementId == key || window.scrollQueued == key || scrollTargetId == key)
        {
          ImGui::SetScrollHereY(0.5f);
          if (window.newElementId == key) window.newElementId = -1;
          if (window.scrollQueued == key) window.scrollQueued = -1;
        }

        if (window_flag_has(window.flags, WINDOW_PROPERTIES) && ImGui::IsItemHovered() &&
            ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
          window_properties(window, manager, key);

        if (window.row_drag_drop_update && window.row_drag_drop_update(window, manager, document, element, key))
        {
          ImGui::PopID();
          break;
        }

        if (window.tooltip_draw)
        {
          ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, window.tooltipItemSpacing);
          ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, window.tooltipWindowPadding);
          if (ImGui::BeginItemTooltip())
          {
            window.tooltip_draw(document, resources, element);
            ImGui::EndTooltip();
          }
          ImGui::PopStyleVar(2);
        }

        ImGui::PopID();
        ++index;
      }
    }

    selection.finish();
    if (arrowSelectionId != -1)
    {
      selection = {arrowSelectionId};
      storage.reference = arrowSelectionId;
      if (window.row_select) window.row_select(window, document, arrowSelectionId);
    }
    if (shortcut(manager.chords[SHORTCUT_CONFIRM], shortcut::FOCUSED) &&
        window_flag_has(window.flags, WINDOW_PROPERTIES) && selection.size() == 1)
      window_properties(window, manager, *selection.begin());
  }

  void window_update(Window& window, Manager& manager, Settings& settings, Resources& resources, Dialog& dialog,
                     Clipboard& clipboard)
  {
    auto document = manager.get();
    if (!document || !window.isOpen || !window.storage_get) return;
    window.dialog = &dialog;

    if (ImGui::Begin(localize.get(window.title), &(settings.*window.isOpen)))
    {
      if (window.begin_update) window.begin_update(window, manager, settings, resources, clipboard, *document);

      auto isAvailable = !window.is_available || window.is_available(*document);
      if (!isAvailable)
      {
        if (window.unavailableText != STRING_UNDEFINED) ImGui::TextUnformatted(localize.get(window.unavailableText));
      }
      else
      {
        auto footerRows = window.footerRows != -1                      ? window.footerRows
                          : window_footer_button_count_get(window) > 0 ? 1
                                                                       : 0;
        auto childSize = size_without_footer_get(footerRows);
        auto style = ImGui::GetStyle();
        window.tooltipWindowPadding = style.WindowPadding;
        window.tooltipItemSpacing = style.ItemSpacing;

        if (window.isChildPaddingZero) ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2());
        auto isChildOpen = ImGui::BeginChild(window.childLabel, childSize, true);
        if (isChildOpen)
        {
          if (window.rows_update)
            window.rows_update(window, manager, settings, resources, clipboard, *document, childSize);
          else
            window_rows_draw(window, manager, resources, *document);

          auto actions = window_context_actions_get(window, manager, settings, *document, clipboard, false);
          actions_shortcuts_update(actions, manager);

          if (!window.context_update) window_context_menu_draw(window, manager, settings, *document, clipboard);
        }
        ImGui::EndChild();
        if (window.isChildPaddingZero) ImGui::PopStyleVar();

        if (window.context_update) window.context_update(window, manager, settings, resources, clipboard, *document);

        if (window.footer_update)
          window.footer_update(window, manager, settings, resources, clipboard, *document);
        else
          window_footer_draw(window, manager, settings, *document, clipboard);
        if (window.body_update) window.body_update(window, manager, settings, resources, clipboard, *document);
      }
    }
    ImGui::End();

    if (window.popup_update) window.popup_update(window, manager, settings, resources, clipboard, *document);
    if (window.post_update) window.post_update(window, manager, settings, resources, clipboard, *document);
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
    window.addTooltip = TOOLTIP_ADD_ANIMATION;
    window.duplicateTooltip = TOOLTIP_DUPLICATE_ANIMATION;
    window.mergeTooltip = TOOLTIP_OPEN_MERGE_POPUP;
    window.removeTooltip = TOOLTIP_REMOVE_ANIMATION;
    window.defaultTooltip = TOOLTIP_SET_DEFAULT_ANIMATION;
    window.renameEdit = SNAPSHOT_RENAME_ANIMATION;
    window.pasteEdit = EDIT_PASTE_ANIMATIONS;
    window.deserializeFailedToast = TOAST_DESERIALIZE_ANIMATIONS_FAILED;
    window.flags = WINDOW_ADD | WINDOW_DUPLICATE | WINDOW_MERGE | WINDOW_GROUP | WINDOW_REMOVE | WINDOW_DEFAULT | WINDOW_CUT |
                   WINDOW_COPY | WINDOW_PASTE | WINDOW_RENAME;
    window.popup = PopupHelper(LABEL_ANIMATIONS_MERGE_POPUP);
    window.storage_get = [](Document& document) -> Storage& { return document.animation; };
    window.element_get = [](Anm2& anm2, int index) { return anm2.element_get(ElementType::ANIMATION, index); };
    window.element_key_get = [](const Element&, int index) { return index; };
    window.row_font_get = [](Document& document, const Element& animation, int index)
    {
      auto animations = document.anm2.element_get(ElementType::ANIMATIONS);
      auto isDefault = animations && animations->defaultAnimation == animation.name;
      auto isReferenced = document.reference.animationIndex == index;
      return isDefault && isReferenced ? resource::font::BOLD_ITALICS
             : isDefault               ? resource::font::BOLD
             : isReferenced            ? resource::font::ITALICS
                                       : resource::font::REGULAR;
    };
    window.row_select = [](Window&, Document& document, int index)
    {
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
      auto isDefault = animations && animations->defaultAnimation == animation.name;

      ImGui::PushFont(resources.fonts[resource::font::BOLD].get(), resource::font::SIZE);
      ImGui::TextUnformatted(animation.name.c_str());
      ImGui::PopFont();

      if (isDefault)
      {
        ImGui::PushFont(resources.fonts[resource::font::ITALICS].get(), resource::font::SIZE);
        ImGui::TextUnformatted(localize.get(BASIC_DEFAULT));
        ImGui::PopFont();
      }

      ImGui::TextUnformatted(
          std::vformat(localize.get(FORMAT_LENGTH), std::make_format_args(animation.frameNum)).c_str());
      auto loopLabel = animation.isLoop ? localize.get(BASIC_YES) : localize.get(BASIC_NO);
      ImGui::TextUnformatted(std::vformat(localize.get(FORMAT_LOOP), std::make_format_args(loopLabel)).c_str());
    };
    window.row_drag_drop_update = [](Window& window, Manager& manager, Document& document, const Element&, int index)
    {
      auto& anm2 = document.anm2;
      auto& selection = document.animation.selection;

      if (ImGui::BeginDragDropSource())
      {
        static std::vector<int> dragDropSelection{};
        dragDropSelection.assign(selection.begin(), selection.end());
        ImGui::SetDragDropPayload("Animation Drag Drop", dragDropSelection.data(),
                                  dragDropSelection.size() * sizeof(int));
        for (auto& dragIndex : dragDropSelection)
          if (auto dragAnimation = anm2.element_get(ElementType::ANIMATION, dragIndex))
            ImGui::Text("%s", dragAnimation->name.c_str());
        ImGui::EndDragDropSource();
      }

      if (ImGui::BeginDragDropTarget())
      {
        if (auto payload = ImGui::AcceptDragDropPayload(
                "Animation Drag Drop",
                ImGuiDragDropFlags_AcceptBeforeDelivery | ImGuiDragDropFlags_AcceptNoDrawDefaultRect))
        {
          auto itemMin = ImGui::GetItemRectMin();
          auto itemMax = ImGui::GetItemRectMax();
          auto isDropAfter = is_drop_after(itemMin, itemMax);
          drop_line_draw(ImGui::GetWindowDrawList(), itemMin, itemMax, isDropAfter);

          auto payloadIndices = (int*)(payload->Data);
          auto payloadCount = payload->DataSize / sizeof(int);
          std::vector<int> indices(payloadIndices, payloadIndices + payloadCount);
          std::sort(indices.begin(), indices.end());
          if (payload->IsDelivery())
          {
            auto targetIndex = index + (isDropAfter ? 1 : 0);
            auto targetGroupId = -1;
            if (auto targetAnimation = anm2.element_get(ElementType::ANIMATION, index))
              targetGroupId = targetAnimation->groupId;
            manager.command_push({manager.selected, [&window, indices, targetIndex, targetGroupId](Manager&, Document& document) mutable
                                  {
                                    auto move = [&]()
                                    {
                                      auto items = window_container_get(window, document.anm2);
                                      if (!items) return;

                                      auto groupIds = window_animation_group_ids_get(*items);
                                      auto groupId = groupIds.contains(targetGroupId) ? targetGroupId : -1;
                                      std::vector<int> childIndices{};
                                      childIndices.reserve(indices.size());
                                      for (auto animationIndex : indices)
                                      {
                                        auto childIndex = window_animation_child_index_get(*items, animationIndex);
                                        if (childIndex == -1) continue;
                                        if (auto animation = vector::find(items->children, childIndex))
                                          animation->groupId = groupId;
                                        childIndices.push_back(childIndex);
                                      }

                                      auto targetChildIndex = window_animation_child_insert_index_get(*items, targetIndex);
                                      auto movedChildIndices =
                                          anm2ed::util::vector::move_indices_to_position(items->children, childIndices,
                                                                                         targetChildIndex);
                                      document.animation.selection.clear();
                                      for (auto childIndex : movedChildIndices)
                                      {
                                        auto animationIndex = window_animation_index_from_child_index_get(*items, childIndex);
                                        if (animationIndex != -1) document.animation.selection.insert(animationIndex);
                                      }
                                    };
                                    window_edit(window, document, localize.get(EDIT_MOVE_ANIMATIONS), move);
                                  }});
            ImGui::EndDragDropTarget();
            return true;
          }
        }
        ImGui::EndDragDropTarget();
      }

      return false;
    };
    window.rows_update = [](Window& window, Manager& manager, Settings&, Resources& resources, Clipboard&,
                            Document& document, ImVec2)
    {
      auto container = window_container_get(window, document.anm2);
      auto& storage = window.storage_get(document);
      auto& selection = storage.selection;
      auto& groupSelection = window.selection;
      auto count = window_element_count_get(window, container);
      auto groupIds = container ? window_animation_group_ids_get(*container) : std::set<int>{};
      std::erase_if(groupSelection, [&](int groupId) { return !groupIds.contains(groupId); });
      std::vector<int> visibleIds{};
      int visibleIndex{};

      auto visible_group_children_push = [&](int groupId)
      {
        int animationIndex{};
        for (auto& child : container->children)
        {
          if (child.type != ElementType::ANIMATION) continue;
          if (child.groupId == groupId) visibleIds.push_back(animationIndex);
          ++animationIndex;
        }
      };

      if (container)
        for (auto& child : container->children)
        {
          if (child.type == ElementType::GROUP)
          {
            if (child.isExpanded) visible_group_children_push(child.id);
          }
          else if (child.type == ElementType::ANIMATION)
          {
            if (!is_window_animation_grouped(groupIds, child)) visibleIds.push_back(visibleIndex);
            ++visibleIndex;
          }
        }

      int scrollTargetId = -1;
      int arrowSelectionId = -1;

      selection.start(count);
      if (container && ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_A, ImGuiInputFlags_RouteFocused))
      {
        selection.clear();
        for (int i = 0; i < count; ++i)
          selection.insert(i);
        groupSelection = groupIds;
      }
      if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
          (ImGui::IsKeyPressed(ImGuiKey_UpArrow, true) || ImGui::IsKeyPressed(ImGuiKey_DownArrow, true)))
      {
        auto nextId = window_arrow_selection_get(visibleIds, storage.reference, selection);
        if (nextId != -1)
        {
          arrowSelectionId = nextId;
          scrollTargetId = nextId;
        }
      }

      auto animation_row_draw = [&](Element& element, int key)
      {
        auto isSelected = selection.contains(key) || arrowSelectionId == key;
        auto font = window.row_font_get ? window.row_font_get(document, element, key) : resource::font::REGULAR;
        auto isFontPushed = font != resource::font::REGULAR;

        ImGui::PushID(key);
        ImGui::SetNextItemSelectionUserData(key);
        auto label = window.row_label_get ? window.row_label_get(document, element) : element.name;
        if (isFontPushed) ImGui::PushFont(resources.fonts[font].get(), resource::font::SIZE);
        if (scrollTargetId == key) ImGui::SetKeyboardFocusHere();

        if (window_flag_has(window.flags, WINDOW_RENAME))
        {
          if (window.newElementId == key || window.renameQueued == key)
          {
            window.renameState = RENAME_FORCE_EDIT;
            window.renameQueued = -1;
          }

          auto isRenaming = window.renameId == key;
          auto& name = isRenaming ? window.renameText : element.name;
          bool isActivated{};
          if (selectable_input_text(label, window_rename_format_get(window, manager, key), name, isSelected,
                                    ImGuiSelectableFlags_None, window.renameState))
          {
            isActivated = true;
            if (window.renameState == RENAME_BEGIN)
            {
              window.renameId = key;
              window.renameText = element.name;
            }
            else if (window.renameState == RENAME_FINISHED)
            {
              if (isRenaming) window_rename_finish(window, manager, key, count, window.renameText);
              window.renameId = -1;
              window.renameText.clear();
            }
          }
          auto isClicked = ImGui::IsItemClicked(ImGuiMouseButton_Left) || ImGui::IsItemClicked(ImGuiMouseButton_Right);
          if (isActivated || isClicked)
          {
            auto& io = ImGui::GetIO();
            if (!io.KeyCtrl && !io.KeyShift) groupSelection.clear();
            storage.reference = key;
            if (window.row_select) window.row_select(window, document, key);
          }
        }
        else
        {
          auto isActivated = ImGui::Selectable(label.c_str(), isSelected);
          auto isClicked = ImGui::IsItemClicked(ImGuiMouseButton_Left) || ImGui::IsItemClicked(ImGuiMouseButton_Right);
          if (isActivated || isClicked)
          {
            auto& io = ImGui::GetIO();
            if (!io.KeyCtrl && !io.KeyShift) groupSelection.clear();
            storage.reference = key;
            if (window.row_select) window.row_select(window, document, key);
          }
        }

        if (isFontPushed) ImGui::PopFont();

        if (window.newElementId == key || window.scrollQueued == key || scrollTargetId == key)
        {
          ImGui::SetScrollHereY(0.5f);
          if (window.newElementId == key) window.newElementId = -1;
          if (window.scrollQueued == key) window.scrollQueued = -1;
        }

        bool isBreak{};
        if (window.row_drag_drop_update && window.row_drag_drop_update(window, manager, document, element, key))
          isBreak = true;

        if (window.tooltip_draw)
        {
          ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, window.tooltipItemSpacing);
          ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, window.tooltipWindowPadding);
          if (ImGui::BeginItemTooltip())
          {
            window.tooltip_draw(document, resources, element);
            ImGui::EndTooltip();
          }
          ImGui::PopStyleVar(2);
        }

        ImGui::PopID();
        return isBreak;
      };

      auto group_reference_clear = [&]()
      {
        storage.reference = -1;
        document.reference = {};
        document.frames.clear();
      };

      auto group_selection_apply = [&](int groupId)
      {
        auto& io = ImGui::GetIO();
        auto isSelected = groupSelection.contains(groupId);
        if (!io.KeyCtrl)
        {
          selection.clear();
          groupSelection.clear();
        }

        if (io.KeyCtrl && isSelected)
        {
          groupSelection.erase(groupId);
          if (selection.empty() && groupSelection.empty()) group_reference_clear();
          return;
        }

        groupSelection.insert(groupId);
        if (!io.KeyCtrl || selection.empty()) group_reference_clear();
      };

      bool isRowsDone{};
      if (container)
      {
        int animationIndex{};
        for (auto& item : container->children)
        {
          if (item.type == ElementType::GROUP)
          {
            ImGui::PushID("Animation Group");
            ImGui::PushID(item.id);
            auto label = item.name.empty() ? std::string(localize.get(TEXT_NEW_GROUP)) : item.name;
            auto isGroupSelected = groupSelection.contains(item.id);
            auto renameKey = window_animation_group_key_get(item.id);
            auto isRenaming = window.renameId == renameKey;
            auto& name = isRenaming ? window.renameText : item.name;
            if (window.renameQueued == renameKey)
            {
              window.renameState = RENAME_FORCE_EDIT;
              window.renameQueued = -1;
            }
            ImGui::SetNextItemOpen(item.isExpanded, ImGuiCond_Always);
            auto treeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
            auto tree =
                tree_node_input_text(label, std::format("###Document #{} Animation Group #{}", manager.selected,
                                                        item.id),
                                     name, isGroupSelected, treeFlags, window.renameState);
            auto isGroupOpen = tree.isOpen;
            auto isGroupClicked = tree.isClicked;
            if (tree.isRenameStarted)
            {
              window.renameId = renameKey;
              window.renameText = item.name;
            }
            else if (tree.isRenameFinished)
            {
              if (isRenaming && item.name != window.renameText)
              {
                auto targetGroupId = item.id;
                auto targetName = window.renameText;
                manager.command_push({manager.selected, [targetGroupId, targetName](Manager&, Document& document)
                                      {
                                        auto animations = document.anm2.element_get(ElementType::ANIMATIONS);
                                        auto group = animations ? element_child_id_get(*animations, ElementType::GROUP,
                                                                                       targetGroupId)
                                                                : nullptr;
                                        if (!group || group->name == targetName) return;
                                        document.anm2_snapshot(localize.get(EDIT_RENAME_GROUP));
                                        group->name = targetName;
                                        document.anm2_change(Document::ANIMATIONS);
                                      }});
              }
              window.renameId = -1;
              window.renameText.clear();
            }
            if (isGroupOpen != item.isExpanded)
            {
              auto targetGroupId = item.id;
              manager.command_push({manager.selected, [targetGroupId, isGroupOpen](Manager&, Document& document)
                                    {
                                      auto animations = document.anm2.element_get(ElementType::ANIMATIONS);
                                      auto group = animations ? element_child_id_get(*animations, ElementType::GROUP,
                                                                                     targetGroupId)
                                                              : nullptr;
                                      if (!group || group->isExpanded == isGroupOpen) return;
                                      document.anm2_snapshot(localize.get(EDIT_TOGGLE_GROUP_EXPANDED));
                                      group->isExpanded = isGroupOpen;
                                      document.anm2_change(Document::ANIMATIONS);
                                    }});
            }
            if (isGroupClicked) group_selection_apply(item.id);

            if (isGroupOpen)
            {
              int groupAnimationIndex{};
              for (auto& animation : container->children)
              {
                if (animation.type != ElementType::ANIMATION) continue;
                if (animation.groupId == item.id)
                  if (animation_row_draw(animation, groupAnimationIndex))
                  {
                    isRowsDone = true;
                    break;
                  }
                ++groupAnimationIndex;
              }
              ImGui::TreePop();
            }
            ImGui::PopID();
            ImGui::PopID();
            if (isRowsDone) break;
          }
          else if (item.type == ElementType::ANIMATION)
          {
            if (!is_window_animation_grouped(groupIds, item))
              if (animation_row_draw(item, animationIndex))
              {
                isRowsDone = true;
                break;
              }
            ++animationIndex;
          }
        }
      }

      selection.finish();
      if (arrowSelectionId != -1)
      {
        groupSelection.clear();
        selection = {arrowSelectionId};
        storage.reference = arrowSelectionId;
        if (window.row_select) window.row_select(window, document, arrowSelectionId);
      }
    };
    window.add = [](Window& window, Manager&, Settings&, Document& document, Clipboard&)
    {
      auto& anm2 = document.anm2;
      auto& reference = document.reference;
      auto& selection = document.animation.selection;

      auto animations = [&]() { return anm2.element_get(ElementType::ANIMATIONS); };
      auto animation_count = [&]() { return window_element_count_get(window, animations()); };

      auto behavior = [&]()
      {
        auto items = animations();
        if (!items) return;
        auto groupIds = window_animation_group_ids_get(*items);
        auto targetGroupId =
            window.selection.size() == 1 && groupIds.contains(*window.selection.begin()) ? *window.selection.begin()
                                                                                         : -1;

        auto animation = element_make(ElementType::ANIMATION);
        animation.name = localize.get(TEXT_NEW_ANIMATION);
        animation.frameNum = 1;
        animation.isLoop = true;

        auto rootAnimation = element_make(ElementType::ROOT_ANIMATION);
        rootAnimation.children.push_back(element_make(ElementType::FRAME));
        animation.children.push_back(rootAnimation);

        const Element* referenceLayerAnimations{};
        const Element* referenceNullAnimations{};
        if (auto referenceAnimation = anm2.element_get(ElementType::ANIMATION, reference.animationIndex))
        {
          referenceLayerAnimations = element_child_first_get(*referenceAnimation, ElementType::LAYER_ANIMATIONS);
          referenceNullAnimations = element_child_first_get(*referenceAnimation, ElementType::NULL_ANIMATIONS);
        }

        animation.children.push_back(window_track_container_shell_copy(referenceLayerAnimations,
                                                                       ElementType::LAYER_ANIMATIONS,
                                                                       ElementType::LAYER_ANIMATION));
        animation.children.push_back(window_track_container_shell_copy(referenceNullAnimations,
                                                                       ElementType::NULL_ANIMATIONS,
                                                                       ElementType::NULL_ANIMATION));
        animation.children.push_back(element_make(ElementType::TRIGGERS));

        auto count = animation_count();
        auto index = count;
        if (!selection.empty()) index = std::min(*selection.rbegin() + 1, count);
        else if (targetGroupId != -1)
        {
          auto groupIndices = window_animation_group_indices_get(*items, targetGroupId);
          if (!groupIndices.empty()) index = std::min(*groupIndices.rbegin() + 1, count);
        }
        if (!selection.empty())
          if (auto selectedAnimation = anm2.element_get(ElementType::ANIMATION, *selection.rbegin()))
          {
            if (groupIds.contains(selectedAnimation->groupId)) animation.groupId = selectedAnimation->groupId;
          }
        if (selection.empty() && targetGroupId != -1) animation.groupId = targetGroupId;
        if (count == 0) items->defaultAnimation = animation.name;

        auto childIndex = window_animation_child_insert_index_get(*items, index);
        items->children.insert(items->children.begin() + childIndex, animation);
        selection = {index};
        window.selection.clear();
        reference = {index};
        window.newElementId = index;
        window.scrollQueued = index;
      };

      window_edit(window, document, localize.get(EDIT_ADD_ANIMATION), behavior);
    };
    window.remove = [](Window& window, Manager&, Settings&, Document& document, Clipboard&)
    {
      auto& selection = document.animation.selection;
      auto& reference = document.reference;
      auto& overlayIndex = document.overlayIndex;
      auto groupSelection = window.selection;

      auto behavior = [&]()
      {
        auto items = document.anm2.element_get(ElementType::ANIMATIONS);
        if (!items || (selection.empty() && groupSelection.empty())) return;
        if (!groupSelection.empty())
        {
          for (auto& item : items->children)
            if (item.type == ElementType::ANIMATION && groupSelection.contains(item.groupId)) item.groupId = -1;
          std::erase_if(items->children, [&](const Element& item)
          { return item.type == ElementType::GROUP && groupSelection.contains(item.id); });
        }
        for (auto it = selection.rbegin(); it != selection.rend(); ++it)
        {
          auto i = *it;
          auto childIndex = window_animation_child_index_get(*items, i);
          if (childIndex == -1) continue;
          if (overlayIndex == i) overlayIndex = -1;
          if (reference.animationIndex == i) reference.animationIndex = -1;
          items->children.erase(items->children.begin() + childIndex);
        }
        selection.clear();
        window.selection.clear();
      };

      auto edit = groupSelection.empty() ? EDIT_REMOVE_ANIMATIONS : EDIT_REMOVE_GROUP;
      window_edit(window, document, localize.get(edit), behavior);
    };
    window.duplicate = [](Window& window, Manager&, Settings&, Document& document, Clipboard&)
    {
      auto& selection = document.animation.selection;

      auto behavior = [&]()
      {
        auto items = document.anm2.element_get(ElementType::ANIMATIONS);
        auto clipboardText = window_animation_clipboard_text_get(document, window);
        if (!items || clipboardText.empty()) return;

        auto selectedIndices = window_animation_selected_indices_get(document, window);
        auto count = window_element_count_get(window, items);
        auto start = selectedIndices.empty() ? count : std::min(*selectedIndices.rbegin() + 1, count);
        std::set<int> indices{};
        std::set<int> groupIds{};
        std::string errorString{};
        if (!document.anm2.animations_deserialize(clipboardText, start, indices, &errorString, &groupIds)) return;

        if (!groupIds.empty())
        {
          selection.clear();
          window.selection = groupIds;
          document.reference = {};
          window.newElementId = -1;
        }
        else
        {
          selection = indices;
          window.selection.clear();
        }
        if (!indices.empty() && groupIds.empty())
        {
          auto index = *indices.rbegin();
          document.reference = {index};
          window.newElementId = indices.size() == 1 && groupIds.empty() ? index : -1;
          window.scrollQueued = index;
        }
        else if (!indices.empty())
          window.scrollQueued = *indices.rbegin();
      };

      window_edit(window, document, localize.get(EDIT_DUPLICATE_ANIMATIONS), behavior);
    };
    window.merge = [](Window& window, Manager&, Settings& settings, Document& document, Clipboard&)
    {
      auto& mergeSelection = document.merge.selection;
      auto& mergeReference = document.merge.reference;
      auto quickSelection = mergeSelection.empty() ? window_animation_selected_indices_get(document, window)
                                                   : std::set<int>{};
      auto quickGroupSelection = mergeSelection.empty() ? window.selection : std::set<int>{};
      auto merged = window_animations_merge(document, {.selection = mergeSelection,
                                                       .reference = mergeReference,
                                                       .type = (merge::Type)settings.mergeType,
                                                       .isDeleteAnimationsAfter = settings.mergeIsDeleteAnimationsAfter},
                                            mergeSelection.empty() ? &quickSelection : nullptr,
                                            mergeSelection.empty() ? &quickGroupSelection : nullptr);
      if (merged != -1)
      {
        window.scrollQueued = merged;
        window.selection.clear();
      }
      mergeSelection.clear();
    };
    window.merge_open = [](Window& window, Manager&, Settings&, Document& document, Clipboard&)
    {
      auto& selection = document.animation.selection;
      if (selection.empty()) return;
      window.popup.open();
      document.merge.selection.clear();
      document.merge.reference = *selection.begin();
    };
    window.group = [](Window& window, Manager&, Settings&, Document& document, Clipboard&)
    {
      auto targetIndices = window_animation_groupable_indices_get(document);
      if (targetIndices.empty()) return;

      auto behavior = [&]()
      {
        auto items = document.anm2.element_get(ElementType::ANIMATIONS);
        if (!items) return;

        std::set<int> targetSet(targetIndices.begin(), targetIndices.end());
        auto group = element_make(ElementType::GROUP);
        group.id = element_child_next_id_get(*items, ElementType::GROUP);
        group.name = localize.get(TEXT_NEW_GROUP);
        group.isExpanded = true;
        int insertIndex = (int)items->children.size();
        int animationIndex{};

        for (int i = 0; i < (int)items->children.size(); ++i)
        {
          auto& item = items->children[i];
          if (item.type != ElementType::ANIMATION) continue;
          if (targetSet.contains(animationIndex))
          {
            insertIndex = std::min(insertIndex, i);
            item.groupId = group.id;
          }
          ++animationIndex;
        }

        if (insertIndex == (int)items->children.size()) return;
        items->children.insert(items->children.begin() + insertIndex, group);
        document.animation.selection = targetSet;
        window.selection = {group.id};
        document.reference = {*targetSet.begin()};
        window.scrollQueued = *targetSet.begin();
      };

      window_edit(window, document, localize.get(EDIT_GROUP_ITEMS), behavior);
    };
    window.default_set = [](Window& window, Manager&, Settings&, Document& document, Clipboard&)
    {
      auto& selection = document.animation.selection;

      auto behavior = [&]()
      {
        auto items = document.anm2.element_get(ElementType::ANIMATIONS);
        auto animation = document.anm2.element_get(ElementType::ANIMATION, *selection.begin());
        if (!items || !animation) return;
        items->defaultAnimation = animation->name;
      };

      window_edit(window, document, localize.get(EDIT_DEFAULT_ANIMATION), behavior);
    };
    window.copy = [](Window& window, Manager&, Settings&, Document& document, Clipboard& clipboard)
    {
      auto clipboardText = window_animation_clipboard_text_get(document, window);
      if (!clipboardText.empty()) clipboard.set(clipboardText);
    };
    window.cut = [](Window& window, Manager& manager, Settings& settings, Document& document, Clipboard& clipboard)
    {
      if (window.copy) window.copy(window, manager, settings, document, clipboard);

      auto& selection = document.animation.selection;
      auto groupSelection = window.selection;
      auto selectedIndices = window_animation_selected_indices_get(document, window);
      auto behavior = [&]()
      {
        auto items = document.anm2.element_get(ElementType::ANIMATIONS);
        if (!items) return;
        for (auto it = selectedIndices.rbegin(); it != selectedIndices.rend(); ++it)
        {
          auto i = *it;
          auto childIndex = window_animation_child_index_get(*items, i);
          if (childIndex == -1) continue;
          if (document.overlayIndex == i) document.overlayIndex = -1;
          if (document.reference.animationIndex == i) document.reference.animationIndex = -1;
          items->children.erase(items->children.begin() + childIndex);
        }
        std::erase_if(items->children, [&](const Element& item)
        { return item.type == ElementType::GROUP && groupSelection.contains(item.id); });
        selection.clear();
        window.selection.clear();
      };

      window_edit(window, document, localize.get(EDIT_CUT_ANIMATIONS), behavior);
    };
    window.paste = [](Window& window, Manager&, Settings&, Document& document, Clipboard& clipboard)
    {
      if (clipboard.is_empty()) return;

      auto& anm2 = document.anm2;
      auto& selection = document.animation.selection;
      auto& reference = document.reference;
      auto animation_count = [&]()
      { return window_element_count_get(window, anm2.element_get(ElementType::ANIMATIONS)); };

      auto behavior = [&]()
      {
        auto clipboardText = clipboard.get();
        auto start = selection.empty() ? animation_count() : *selection.rbegin() + 1;
        auto items = anm2.element_get(ElementType::ANIMATIONS);
        auto groupIdsBefore = items ? window_animation_group_ids_get(*items) : std::set<int>{};
        auto targetGroupId =
            selection.empty() && window.selection.size() == 1 && groupIdsBefore.contains(*window.selection.begin())
                ? *window.selection.begin()
                : -1;
        if (selection.empty() && targetGroupId != -1)
        {
          auto groupIndices = window_animation_group_indices_get(*items, targetGroupId);
          if (!groupIndices.empty()) start = std::min(*groupIndices.rbegin() + 1, animation_count());
        }
        std::set<int> indices{};
        std::set<int> groupIds{};
        std::string errorString{};
        if (anm2.animations_deserialize(clipboardText, start, indices, &errorString, &groupIds))
        {
          if (targetGroupId != -1 && groupIds.empty())
            for (auto index : indices)
              if (auto animation = anm2.element_get(ElementType::ANIMATION, index)) animation->groupId = targetGroupId;

          if (!groupIds.empty())
          {
            selection.clear();
            window.selection = groupIds;
            reference = {};
            window.newElementId = -1;
          }
          else if (!indices.empty())
          {
            auto index = *indices.rbegin();
            selection = {index};
            window.selection.clear();
            reference = {index};
            window.newElementId = indices.size() == 1 ? index : -1;
            window.scrollQueued = index;
          }
          if (!indices.empty()) window.scrollQueued = *indices.rbegin();
        }
        else
        {
          toasts.push(
              std::vformat(localize.get(TOAST_DESERIALIZE_ANIMATIONS_FAILED), std::make_format_args(errorString)));
          logger.error(std::vformat(localize.get(TOAST_DESERIALIZE_ANIMATIONS_FAILED, anm2ed::ENGLISH),
                                    std::make_format_args(errorString)));
        }
      };

      window_edit(window, document, localize.get(EDIT_PASTE_ANIMATIONS), behavior);
    };
    window.begin_update = [](Window&, Manager&, Settings&, Resources&, Clipboard&, Document& document)
    {
      if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && ImGui::IsKeyPressed(ImGuiKey_Escape))
        document.reference = {};
    };
    window.body_update =
        [](Window& window, Manager& manager, Settings& settings, Resources&, Clipboard&, Document& document)
    {
      auto animations = [&]() { return document.anm2.element_get(ElementType::ANIMATIONS); };
      auto& mergeSelection = document.merge.selection;
      auto& mergeReference = document.merge.reference;

      window.popup.trigger();

      if (ImGui::BeginPopupModal(window.popup.label(), &window.popup.isOpen, ImGuiWindowFlags_NoResize))
      {
        auto close = [&]()
        {
          mergeSelection.clear();
          window.popup.close();
        };

        auto& type = settings.mergeType;
        auto& isDeleteAnimationsAfter = settings.mergeIsDeleteAnimationsAfter;
        struct AnimationMergeCandidate
        {
          int key{};
          std::string label{};
        };
        std::vector<AnimationMergeCandidate> candidates{};
        auto items = animations();
        auto groupIds = items ? window_animation_group_ids_get(*items) : std::set<int>{};

        if (items)
        {
          auto candidate_animation_push = [&](const Element& animation, int animationIndex, bool isGrouped)
          {
            if (animationIndex == mergeReference) return;
            auto label = isGrouped ? std::format("  {}", animation.name) : animation.name;
            candidates.push_back({.key = animationIndex, .label = label});
          };

          int animationIndex{};
          for (const auto& item : items->children)
          {
            if (item.type == ElementType::GROUP)
            {
              auto groupIndices = window_animation_group_indices_get(*items, item.id);
              groupIndices.erase(mergeReference);
              if (!groupIndices.empty())
              {
                auto label = item.name.empty() ? std::string(localize.get(TEXT_NEW_GROUP)) : item.name;
                candidates.push_back({.key = window_animation_group_key_get(item.id), .label = label});
                int groupAnimationIndex{};
                for (const auto& animation : items->children)
                {
                  if (animation.type != ElementType::ANIMATION) continue;
                  if (animation.groupId == item.id) candidate_animation_push(animation, groupAnimationIndex, true);
                  ++groupAnimationIndex;
                }
              }
            }
            else if (item.type == ElementType::ANIMATION)
            {
              if (!is_window_animation_grouped(groupIds, item)) candidate_animation_push(item, animationIndex, false);
              ++animationIndex;
            }
          }
        }

        auto footerSize = footer_size_get();
        auto optionsSize = child_size_get(2);
        auto deleteAfterSize = child_size_get();
        auto animationsSize =
            ImVec2(0, ImGui::GetContentRegionAvail().y -
                          (optionsSize.y + deleteAfterSize.y + footerSize.y + ImGui::GetStyle().ItemSpacing.y * 3));

        if (ImGui::BeginChild(localize.get(LABEL_ANIMATIONS_CHILD), animationsSize, ImGuiChildFlags_Borders))
        {
          std::vector<int> candidateKeys{};
          candidateKeys.reserve(candidates.size());
          for (const auto& candidate : candidates)
            candidateKeys.push_back(candidate.key);

          mergeSelection.set_index_map(&candidateKeys);
          mergeSelection.start(candidates.size());

          for (int i = 0; i < (int)candidates.size(); ++i)
          {
            const auto& candidate = candidates[i];
            ImGui::PushID(candidate.key);
            ImGui::SetNextItemSelectionUserData(i);
            ImGui::Selectable(candidate.label.c_str(), mergeSelection.contains(candidate.key));
            ImGui::PopID();
          }

          mergeSelection.finish();
          mergeSelection.set_index_map(nullptr);
        }
        ImGui::EndChild();

        if (ImGui::BeginChild("##Merge Options", optionsSize, ImGuiChildFlags_Borders))
        {
          auto size = ImVec2(optionsSize.x * 0.5f, optionsSize.y - ImGui::GetStyle().WindowPadding.y * 2);

          if (ImGui::BeginChild("##Merge Options 1", size))
          {
            ImGui::RadioButton(localize.get(LABEL_APPEND_FRAMES), &type, merge::APPEND);
            ImGui::RadioButton(localize.get(LABEL_PREPEND_FRAMES), &type, merge::PREPEND);
          }
          ImGui::EndChild();

          ImGui::SameLine();

          if (ImGui::BeginChild("##Merge Options 2", size))
          {
            ImGui::RadioButton(localize.get(LABEL_REPLACE_FRAMES), &type, merge::REPLACE);
            ImGui::RadioButton(localize.get(LABEL_IGNORE_FRAMES), &type, merge::IGNORE);
          }
          ImGui::EndChild();
        }
        ImGui::EndChild();

        if (ImGui::BeginChild("##Merge Delete After", deleteAfterSize, ImGuiChildFlags_Borders))
          ImGui::Checkbox(localize.get(LABEL_DELETE_ANIMATIONS_AFTER), &isDeleteAnimationsAfter);
        ImGui::EndChild();

        auto widgetSize = widget_size_with_row_get(2);

        ImGui::BeginDisabled(window_animation_merge_indices_get(document, mergeSelection, mergeReference).empty());
        if (ImGui::Button(localize.get(LABEL_MERGE), widgetSize))
        {
          auto queuedSelection = mergeSelection;
          auto queuedReference = mergeReference;
          AnimationMergeOptions options{.selection = queuedSelection,
                                        .reference = queuedReference,
                                        .type = (merge::Type)type,
                                        .isDeleteAnimationsAfter = isDeleteAnimationsAfter};
          manager.command_push({manager.selected, [options](Manager&, Document& document) mutable
                                { window_animations_merge(document, options); }});
          close();
        }
        ImGui::EndDisabled();

        ImGui::SameLine();
        if (ImGui::Button(localize.get(LABEL_CLOSE), widgetSize)) close();

        ImGui::EndPopup();
      }
    };
    window.post_update = [](Window& window, Manager& manager, Settings&, Resources&, Clipboard&, Document& document)
    {
      auto isNextAnimation = shortcut(manager.chords[SHORTCUT_NEXT_ANIMATION], shortcut::GLOBAL);
      auto isPreviousAnimation = shortcut(manager.chords[SHORTCUT_PREVIOUS_ANIMATION], shortcut::GLOBAL);
      auto count = window_element_count_get(window, window_container_get(window, document.anm2));

      if ((isPreviousAnimation || isNextAnimation) && count > 0)
      {
        auto& reference = document.reference;
        if (isPreviousAnimation) reference.animationIndex = std::clamp(reference.animationIndex - 1, 0, count - 1);
        if (isNextAnimation) reference.animationIndex = std::clamp(reference.animationIndex + 1, 0, count - 1);

        window.storage_get(document).selection = {reference.animationIndex};
        window.scrollQueued = reference.animationIndex;
      }
    };
    return window;
  }

  Window regions_window_register()
  {
    Window window{};
    window.title = LABEL_REGIONS_WINDOW;
    window.isOpen = &Settings::windowIsRegions;
    window.changeType = Document::SPRITESHEETS;
    window.elementType = ElementType::REGION;
    window.childLabel = "##Regions Child";
    window.footerRows = 1;
    window.flags = 0;
    window.isChildPaddingZero = true;
    window.unavailableText = TEXT_SELECT_SPRITESHEET;
    window.editElement = element_make(ElementType::REGION);
    window.popup = PopupHelper(LABEL_REGION_PROPERTIES, POPUP_SMALL_NO_HEIGHT);
    window.storage_get = [](Document& document) -> Storage& { return document.region; };
    window.is_available = [](Document& document)
    { return document.anm2.element_get(ElementType::SPRITESHEET, document.spritesheet.reference); };
    window.add = [](Window& window, Manager&, Settings&, Document& document, Clipboard&)
    {
      document.region.reference = -1;
      window.editElement = element_make(ElementType::REGION);
      window.popup.open();
    };
    window.properties = [](Window& window, Manager&, Settings&, Document& document, Clipboard&)
    {
      auto& reference = document.region.reference;
      auto& selection = document.region.selection;
      auto spritesheet = document.anm2.element_get(ElementType::SPRITESHEET, document.spritesheet.reference);
      if (!spritesheet || selection.size() != 1) return;
      auto id = *selection.begin();
      if (!element_child_id_get(*spritesheet, ElementType::REGION, id)) return;
      reference = id;
      window.popup.open();
    };
    window.remove = [](Window& window, Manager&, Settings&, Document& document, Clipboard&)
    {
      auto& anm2 = document.anm2;
      auto spritesheetReference = document.spritesheet.reference;
      auto spritesheet = anm2.element_get(ElementType::SPRITESHEET, spritesheetReference);
      if (!spritesheet) return;

      auto unused = anm2.element_unused(ElementType::REGION, spritesheetReference);
      if (unused.empty()) return;

      auto behavior = [&]()
      {
        auto target = anm2.element_get(ElementType::SPRITESHEET, spritesheetReference);
        if (!target) return;
        for (auto& id : unused)
        {
          if (auto animations = anm2.element_get(ElementType::ANIMATIONS))
            for (auto& animation : animations->children)
            {
              if (animation.type != ElementType::ANIMATION) continue;
              auto layerAnimations = element_child_first_get(animation, ElementType::LAYER_ANIMATIONS);
              if (!layerAnimations) continue;
              auto region_clear = [&](auto&& self, Element& layerAnimation) -> void
              {
                if (layerAnimation.type == ElementType::GROUP)
                {
                  for (auto& child : layerAnimation.children)
                    self(self, child);
                  return;
                }
                if (layerAnimation.type != ElementType::LAYER_ANIMATION) return;
                for (auto& frame : layerAnimation.children)
                  if (frame.type == ElementType::FRAME && frame.regionId == id) frame.regionId = -1;
              };
              for (auto& layerAnimation : layerAnimations->children)
                region_clear(region_clear, layerAnimation);
            }

          element_child_id_erase(*target, ElementType::REGION, id);
        }
      };

      window_edit(window, document, localize.get(EDIT_REMOVE_UNUSED_REGIONS), behavior);
    };
    window.trim = [](Window&, Manager&, Settings&, Document& document, Clipboard&)
    {
      auto& selection = document.region.selection;
      auto& reference = document.region.reference;
      auto& frame = document.frames;
      auto spritesheetReference = document.spritesheet.reference;
      auto spritesheet = document.anm2.element_get(ElementType::SPRITESHEET, spritesheetReference);
      if (!spritesheet || selection.empty()) return;

      auto behavior = [&]()
      {
        if (document.regions_trim(spritesheetReference, selection))
        {
          if (reference != -1 && !selection.contains(reference)) reference = *selection.begin();
          document.reference = {document.reference.animationIndex};
          frame.reference = -1;
          frame.selection.clear();
        }
      };

      document.anm2_snapshot(localize.get(EDIT_TRIM_REGIONS));
      behavior();
      document.change(Document::SPRITESHEETS);
    };
    window.copy = [](Window&, Manager&, Settings&, Document& document, Clipboard& clipboard)
    {
      auto spritesheet = document.anm2.element_get(ElementType::SPRITESHEET, document.spritesheet.reference);
      auto& selection = document.region.selection;
      if (!spritesheet || selection.empty()) return;

      std::string clipboardText{};
      for (auto& region : spritesheet->children)
        if (region.type == ElementType::REGION && selection.contains(region.id))
          clipboardText += element_to_string(region);
      clipboard.set(clipboardText);
    };
    window.paste = [](Window& window, Manager&, Settings&, Document& document, Clipboard& clipboard)
    {
      auto spritesheetReference = document.spritesheet.reference;
      auto spritesheet = document.anm2.element_get(ElementType::SPRITESHEET, spritesheetReference);
      if (!spritesheet || clipboard.is_empty()) return;

      auto& anm2 = document.anm2;
      auto& selection = document.region.selection;
      auto& reference = document.region.reference;
      auto maxRegionIdBefore = element_child_max_id_get(*spritesheet, ElementType::REGION);
      auto insertIndex = region_insert_index_get(*spritesheet, document.region);
      auto pasted = anm2;
      std::string errorString{};
      if (pasted.regions_deserialize(spritesheetReference, clipboard.get(), true, &errorString))
      {
        document.anm2_snapshot(localize.get(EDIT_PASTE_REGIONS));
        anm2 = std::move(pasted);
        if (auto pastedSpritesheet = anm2.element_get(ElementType::SPRITESHEET, spritesheetReference))
        {
          auto maxRegionIdAfter = element_child_max_id_get(*pastedSpritesheet, ElementType::REGION);
          if (maxRegionIdAfter > maxRegionIdBefore)
          {
            std::vector<int> pastedIndices{};
            for (int i = 0; i < (int)pastedSpritesheet->children.size(); i++)
              if (pastedSpritesheet->children[i].type == ElementType::REGION &&
                  pastedSpritesheet->children[i].id > maxRegionIdBefore)
                pastedIndices.push_back(i);
            vector::move_indices_to_position(pastedSpritesheet->children, pastedIndices, insertIndex);
            window.newElementId = maxRegionIdAfter;
            selection = {maxRegionIdAfter};
            reference = maxRegionIdAfter;
          }
        }
        document.anm2_change(Document::SPRITESHEETS);
      }
      else
      {
        toasts.push(std::vformat(localize.get(TOAST_DESERIALIZE_REGIONS_FAILED), std::make_format_args(errorString)));
        logger.error(std::vformat(localize.get(TOAST_DESERIALIZE_REGIONS_FAILED, anm2ed::ENGLISH),
                                  std::make_format_args(errorString)));
      }
    };
    window.begin_update = [](Window& window, Manager& manager, Settings&, Resources&, Clipboard&, Document& document)
    {
      if (!manager.isMakeRegionRequested) return;
      document.spritesheet.reference = manager.makeRegionSpritesheetId;
      document.region.reference = -1;
      window.editElement = manager.makeRegion;
      window.isPreserveEditElementOnOpen = true;
      window.popup.open();
      manager.isMakeRegionRequested = false;
    };
    window.rows_update = [](Window& window, Manager& manager, Settings& settings, Resources& resources,
                            Clipboard& clipboard, Document& document, ImVec2)
    {
      auto& anm2 = document.anm2;
      auto& selection = document.region.selection;
      auto& frame = document.frames;
      auto& spritesheetReference = document.spritesheet.reference;
      auto& reference = document.region.reference;
      auto style = ImGui::GetStyle();
      auto spritesheet = anm2.element_get(ElementType::SPRITESHEET, spritesheetReference);
      if (!spritesheet)
      {
        ImGui::TextUnformatted(localize.get(TEXT_SELECT_SPRITESHEET));
        return;
      }

      auto regionChildSize = ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetTextLineHeightWithSpacing() * 2);

      ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2());

      window.order.clear();
      window.order.reserve(spritesheet->children.size());
      for (auto& region : spritesheet->children)
        if (region.type == ElementType::REGION) window.order.push_back(region.id);

      selection.set_index_map(&window.order);
      selection.start(window.order.size());
      if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_A, ImGuiInputFlags_RouteFocused))
      {
        selection.clear();
        for (auto& id : window.order)
          selection.insert(id);
      }
      if (ImGui::Shortcut(ImGuiKey_Escape, ImGuiInputFlags_RouteFocused))
      {
        selection.clear();
        reference = -1;
        document.reference = {document.reference.animationIndex};
        frame.reference = -1;
        frame.selection.clear();
      }

      int scrollTargetId = -1;
      int arrowSelectionId = -1;
      if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
          (ImGui::IsKeyPressed(ImGuiKey_UpArrow, true) || ImGui::IsKeyPressed(ImGuiKey_DownArrow, true)))
      {
        auto nextId = window_arrow_selection_get(window.order, reference, selection);
        if (nextId != -1)
        {
          arrowSelectionId = nextId;
          scrollTargetId = nextId;
        }
      }

      auto textureInfo = document.texture_get(spritesheetReference);
      bool isValid = textureInfo && textureInfo->is_valid();
      auto& texture = isValid ? *textureInfo : resources.icons[icon::NONE];
      auto tintColor = !isValid ? ImVec4(1.0f, 0.25f, 0.25f, 1.0f) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
      bool isRegionDragDropSourceSubmitted{};
      bool isRegionMoved{};

      for (int i = 0; i < (int)window.order.size(); i++)
      {
        int id = window.order[i];
        auto region = element_child_id_get(*spritesheet, ElementType::REGION, id);
        if (!region) continue;
        auto isNewRegion = window.newElementId == id;
        auto nameCStr = region->name.c_str();
        auto isSelected = selection.contains(id) || arrowSelectionId == id;
        auto isReferenced = id == reference || arrowSelectionId == id;

        ImGui::PushID(id);

        window_scroll_to_item(regionChildSize.y, scrollTargetId == id);
        if (scrollTargetId == id) ImGui::SetNextWindowFocus();

        if (ImGui::BeginChild("##Region Child", regionChildSize, ImGuiChildFlags_Borders))
        {
          auto cursorPos = ImGui::GetCursorPos();
          auto regionChildMin = ImGui::GetWindowPos();
          auto regionChildMax = ImVec2(regionChildMin.x + ImGui::GetWindowSize().x,
                                       regionChildMin.y + ImGui::GetWindowSize().y);

          ImGui::SetNextItemSelectionUserData(i);
          ImGui::SetNextItemStorageID(id);
          if (scrollTargetId == id) ImGui::SetKeyboardFocusHere();
          auto isActivated = ImGui::Selectable("##Region Selectable", isSelected, 0, regionChildSize);
          auto isClicked = ImGui::IsItemClicked(ImGuiMouseButton_Left) || ImGui::IsItemClicked(ImGuiMouseButton_Right);
          if (isActivated || isClicked)
          {
            document.editTarget = Document::EditTarget::REGION;
            reference = id;
            document.reference = {document.reference.animationIndex};
            frame.reference = -1;
            frame.selection.clear();
          }
          auto regionRowMin = ImGui::GetItemRectMin();
          auto regionRowMax = ImGui::GetItemRectMax();
          if (scrollTargetId == id) ImGui::SetItemDefaultFocus();
          if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) window.popup.open();

          auto viewport = ImGui::GetMainViewport();
          auto maxPreviewSize = to_vec2(viewport->Size) * 0.5f;
          vec2 regionSize = glm::max(region->size, vec2(1.0f));
          vec2 previewSize = regionSize;
          if (previewSize.x > maxPreviewSize.x || previewSize.y > maxPreviewSize.y)
          {
            auto scale = glm::min(maxPreviewSize.x / previewSize.x, maxPreviewSize.y / previewSize.y);
            previewSize *= scale;
          }
          vec2 uvMin{};
          vec2 uvMax{1.0f, 1.0f};
          if (isValid)
          {
            uvMin = region->crop / vec2(texture.size);
            uvMax = (region->crop + region->size) / vec2(texture.size);
          }

          auto textWidth = ImGui::CalcTextSize(nameCStr).x;
          auto tooltipPadding = window.tooltipWindowPadding.x * 4.0f;
          auto minWidth = previewSize.x + window.tooltipItemSpacing.x + textWidth + tooltipPadding;

          ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, window.tooltipItemSpacing);
          ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, window.tooltipWindowPadding);
          if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip) && !ImGui::IsMouseDragging(ImGuiMouseButton_Left))
          {
            ImGui::SetNextWindowSize(ImVec2(minWidth, 0), ImGuiCond_Appearing);
            if (ImGui::BeginItemTooltip())
            {
              ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2());
              auto childFlags = ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY;
              auto noScrollFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

              if (ImGui::BeginChild("##Region Tooltip Image Child", to_imvec2(previewSize), childFlags, noScrollFlags))
                ImGui::ImageWithBg(texture.id, to_imvec2(previewSize), to_imvec2(uvMin), to_imvec2(uvMax), ImVec4(),
                                   tintColor);
              ImGui::EndChild();
              ImGui::PopStyleVar();

              ImGui::SameLine();

              auto infoChildFlags = ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY;
              if (ImGui::BeginChild("##Region Info Tooltip Child", ImVec2(), infoChildFlags, noScrollFlags))
              {
                ImGui::PushFont(resources.fonts[font::BOLD].get(), font::SIZE);
                ImGui::TextUnformatted(nameCStr);
                ImGui::PopFont();

                ImGui::TextUnformatted(
                    std::vformat(localize.get(FORMAT_CROP), std::make_format_args(region->crop.x, region->crop.y))
                        .c_str());
                ImGui::TextUnformatted(
                    std::vformat(localize.get(FORMAT_SIZE), std::make_format_args(region->size.x, region->size.y))
                        .c_str());
                if (region->origin == Origin::CUSTOM)
                  ImGui::TextUnformatted(
                      std::vformat(localize.get(FORMAT_PIVOT), std::make_format_args(region->pivot.x, region->pivot.y))
                          .c_str());
                else
                {
                  StringType originString = LABEL_REGION_ORIGIN_CENTER;
                  if (region->origin == Origin::TOP_LEFT) originString = LABEL_REGION_ORIGIN_TOP_LEFT;
                  auto originLabel = localize.get(originString);
                  ImGui::TextUnformatted(
                      std::vformat(localize.get(FORMAT_ORIGIN), std::make_format_args(originLabel)).c_str());
                }
              }
              ImGui::EndChild();
              ImGui::EndTooltip();
            }
          }
          ImGui::PopStyleVar(2);

          if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceNoPreviewTooltip))
          {
            isRegionDragDropSourceSubmitted = true;
            if (selection.contains(id))
              window.dragSelection.assign(selection.begin(), selection.end());
            else
              window.dragSelection = {id};

            ImGui::SetDragDropPayload("Region Drag Drop", window.dragSelection.data(),
                                      window.dragSelection.size() * sizeof(int));
            ImGui::EndDragDropSource();
          }

          bool isDropLineActive{};
          bool isDropLineAfter{};
          if (ImGui::BeginDragDropTarget())
          {
            if (auto payload = ImGui::AcceptDragDropPayload(
                    "Region Drag Drop",
                    ImGuiDragDropFlags_AcceptBeforeDelivery | ImGuiDragDropFlags_AcceptNoDrawDefaultRect))
            {
              isDropLineActive = true;
              isDropLineAfter = is_drop_after(regionRowMin, regionRowMax);

              auto payloadIds = (int*)(payload->Data);
              int payloadCount = (int)(payload->DataSize / sizeof(int));
              std::vector<int> indices{};
              indices.reserve(payloadCount);
              for (int payloadIndex = 0; payloadIndex < payloadCount; payloadIndex++)
              {
                int payloadId = payloadIds[payloadIndex];
                int index = vector::find_index(window.order, payloadId);
                if (index != -1) indices.push_back(index);
              }
              if (!indices.empty() && payload->IsDelivery())
              {
                std::sort(indices.begin(), indices.end());
                auto movedIds = window.dragSelection;
                auto targetIndex = i + (isDropLineAfter ? 1 : 0);
                auto targetSpritesheetReference = spritesheetReference;
                manager.command_push(
                    {manager.selected, [&window, indices, movedIds, targetIndex,
                                        targetSpritesheetReference](Manager&, Document& document) mutable
                     {
                       auto spritesheet =
                           document.anm2.element_get(ElementType::SPRITESHEET, targetSpritesheetReference);
                       if (!spritesheet) return;

                       auto move = [&]()
                       {
                         vector::move_indices_to_position(spritesheet->children, indices, targetIndex);
                         document.region.selection.clear();
                         for (auto id : movedIds)
                           document.region.selection.insert(id);
                       };
                       window_edit(window, document, localize.get(EDIT_MOVE_REGIONS), move);
                     }});
                isRegionMoved = true;
              }
            }
            ImGui::EndDragDropTarget();
          }

          auto imageSize = to_imvec2(vec2(regionChildSize.y));
          auto aspectRatio = region->size.y != 0.0f ? (float)region->size.x / region->size.y : 1.0f;

          if (imageSize.x / imageSize.y > aspectRatio)
            imageSize.x = imageSize.y * aspectRatio;
          else
            imageSize.y = imageSize.x / aspectRatio;

          ImGui::SetCursorPos(cursorPos);
          ImGui::ImageWithBg(texture.id, imageSize, to_imvec2(uvMin), to_imvec2(uvMax), ImVec4(), tintColor);

          ImGui::SetCursorPos(ImVec2(regionChildSize.y + style.ItemSpacing.x,
                                     regionChildSize.y - regionChildSize.y / 2 - ImGui::GetTextLineHeight() / 2));

          if (isReferenced) ImGui::PushFont(resources.fonts[font::ITALICS].get(), font::SIZE);
          ImGui::TextUnformatted(nameCStr);
          if (isReferenced) ImGui::PopFont();

          if (isDropLineActive)
            drop_line_draw(ImGui::GetWindowDrawList(), regionChildMin, regionChildMax, isDropLineAfter);
        }

        ImGui::EndChild();

        if (isNewRegion)
        {
          ImGui::SetScrollHereY(0.5f);
          window.newElementId = -1;
        }

        ImGui::PopID();
        if (isRegionMoved) break;
      }

      auto regionDragPayload = isRegionMoved ? nullptr : ImGui::GetDragDropPayload();
      bool isRegionDragActive = regionDragPayload && regionDragPayload->IsDataType("Region Drag Drop");
      if (isRegionDragActive && !isRegionDragDropSourceSubmitted && !window.dragSelection.empty() &&
          ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceExtern | ImGuiDragDropFlags_SourceNoPreviewTooltip))
      {
        ImGui::SetDragDropPayload("Region Drag Drop", window.dragSelection.data(),
                                  window.dragSelection.size() * sizeof(int));
        ImGui::EndDragDropSource();
      }

      if (isRegionDragActive && !window.dragSelection.empty())
      {
        auto mousePos = ImGui::GetIO().MousePos;
        auto tooltipOffset = ImVec2(16.0f, 16.0f);
        ImGui::SetNextWindowPos(ImVec2(mousePos.x + tooltipOffset.x, mousePos.y + tooltipOffset.y));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, window.tooltipWindowPadding);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, window.tooltipItemSpacing);
        if (ImGui::BeginTooltip())
        {
          for (auto regionId : window.dragSelection)
          {
            auto drag = element_child_id_get(*spritesheet, ElementType::REGION, regionId);
            if (drag) ImGui::TextUnformatted(drag->name.c_str());
          }
          ImGui::EndTooltip();
        }
        ImGui::PopStyleVar(2);
      }

      ImGui::PopStyleVar();
      selection.finish();
      selection.set_index_map(nullptr);
      if (arrowSelectionId != -1)
      {
        document.editTarget = Document::EditTarget::REGION;
        selection = {arrowSelectionId};
        reference = arrowSelectionId;
        document.reference = {document.reference.animationIndex};
        frame.reference = -1;
        frame.selection.clear();
      }

      if (shortcut(manager.chords[SHORTCUT_ADD], shortcut::FOCUSED) && window.add)
        window_command_run(window, manager, settings, document, clipboard, window.add);
      if (shortcut(manager.chords[SHORTCUT_REMOVE], shortcut::FOCUSED) && window.remove)
        window_command_run(window, manager, settings, document, clipboard, window.remove);
      if (shortcut(manager.chords[SHORTCUT_COPY], shortcut::FOCUSED) && window.copy)
        window_command_run(window, manager, settings, document, clipboard, window.copy);
      if (shortcut(manager.chords[SHORTCUT_PASTE], shortcut::FOCUSED) && window.paste)
        window_command_run(window, manager, settings, document, clipboard, window.paste);
      if (shortcut(manager.chords[SHORTCUT_CONFIRM], shortcut::FOCUSED) && selection.size() == 1 && window.properties)
        window_command_run(window, manager, settings, document, clipboard, window.properties);
    };
    window.context_update =
        [](Window& window, Manager& manager, Settings& settings, Resources&, Clipboard& clipboard, Document& document)
    {
      auto& selection = document.region.selection;
      auto style = ImGui::GetStyle();

      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, style.WindowPadding);
      ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, style.ItemSpacing);

      if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenOverlappedByWindow) &&
          ImGui::IsMouseClicked(ImGuiMouseButton_Right))
        ImGui::OpenPopup("##Region Context Menu");

      Actions actions{};
      actions_undo_redo_add(actions, manager, document);
      actions.separator();
      actions.add(ACTION_PROPERTIES, [&]() { return selection.size() == 1 && (bool)window.properties; },
                  [&]() { window_command_run(window, manager, settings, document, clipboard, window.properties); });
      actions.add(ACTION_ADD, [&]() { return (bool)window.add; },
                  [&]() { window_command_run(window, manager, settings, document, clipboard, window.add); });
      actions.add(ACTION_REMOVE_UNUSED, [&]() { return (bool)window.remove; },
                  [&]() { window_command_run(window, manager, settings, document, clipboard, window.remove); });
      actions.add(ACTION_TRIM, [&]() { return !selection.empty() && (bool)window.trim; },
                  [&]() { window_command_run(window, manager, settings, document, clipboard, window.trim); },
                  TOOLTIP_TRIM_REGIONS);
      actions.separator();
      actions.add(ACTION_COPY, [&]() { return !selection.empty() && (bool)window.copy; },
                  [&]() { window_command_run(window, manager, settings, document, clipboard, window.copy); });
      actions.add(ACTION_PASTE, [&]() { return !clipboard.is_empty() && (bool)window.paste; },
                  [&]() { window_command_run(window, manager, settings, document, clipboard, window.paste); });
      actions_popup_draw("##Region Context Menu", actions, settings);
      ImGui::PopStyleVar(2);
    };
    window.footer_update =
        [](Window& window, Manager& manager, Settings& settings, Resources&, Clipboard& clipboard, Document& document)
    {
      auto rowOneWidgetSize = widget_size_with_row_get(2);

      shortcut(manager.chords[SHORTCUT_ADD]);
      if (ImGui::Button(localize.get(BASIC_ADD), rowOneWidgetSize) && window.add)
        window_command_run(window, manager, settings, document, clipboard, window.add);
      set_item_tooltip_shortcut(localize.get(TOOLTIP_ADD_REGION), settings.shortcutAdd);

      ImGui::SameLine();

      shortcut(manager.chords[SHORTCUT_REMOVE]);
      if (ImGui::Button(localize.get(BASIC_REMOVE_UNUSED), rowOneWidgetSize) && window.remove)
        window_command_run(window, manager, settings, document, clipboard, window.remove);
      set_item_tooltip_shortcut(localize.get(TOOLTIP_REMOVE_UNUSED_REGIONS), settings.shortcutAdd);
    };
    window.popup_update = [](Window& window, Manager& manager, Settings&, Resources&, Clipboard&, Document& document)
    {
      auto& anm2 = document.anm2;
      auto& spritesheetReference = document.spritesheet.reference;
      auto& reference = document.region.reference;
      auto spritesheet_get = [&]() { return anm2.element_get(ElementType::SPRITESHEET, spritesheetReference); };
      auto region_get = [&](int id)
      {
        auto spritesheet = spritesheet_get();
        return spritesheet ? element_child_id_get(*spritesheet, ElementType::REGION, id) : nullptr;
      };

      window.popup.trigger();

      if (ImGui::BeginPopupModal(window.popup.label(), &window.popup.isOpen, ImGuiWindowFlags_NoResize))
      {
        auto spritesheet = spritesheet_get();
        auto targetRegion = reference == -1 ? nullptr : region_get(reference);
        if (!spritesheet || (reference != -1 && !targetRegion))
        {
          window.popup.close();
          ImGui::EndPopup();
          return;
        }

        auto childSize = child_size_get(5);

        if (window.popup.isJustOpened)
        {
          if (!window.isPreserveEditElementOnOpen)
            window.editElement = targetRegion ? *targetRegion : element_make(ElementType::REGION);
          window.isPreserveEditElementOnOpen = false;
        }
        auto& region = window.editElement;

        if (ImGui::BeginChild("##Child", childSize, ImGuiChildFlags_Borders))
        {
          const char* originOptions[] = {localize.get(LABEL_REGION_ORIGIN_TOP_LEFT),
                                         localize.get(LABEL_REGION_ORIGIN_CENTER),
                                         localize.get(LABEL_REGION_ORIGIN_CUSTOM)};

          if (window.popup.isJustOpened) ImGui::SetKeyboardFocusHere();
          input_text_string(localize.get(BASIC_NAME), &region.name);
          ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_ITEM_NAME));
          ImGui::DragFloat2(localize.get(BASIC_CROP), value_ptr(region.crop), DRAG_SPEED, 0.0f, 0.0f,
                            math::vec2_format_get(region.crop));
          ImGui::DragFloat2(localize.get(BASIC_SIZE), value_ptr(region.size), DRAG_SPEED, 0.0f, 0.0f,
                            math::vec2_format_get(region.size));
          ImGui::BeginDisabled(region.origin != Origin::CUSTOM);
          ImGui::DragFloat2(localize.get(BASIC_PIVOT), value_ptr(region.pivot), DRAG_SPEED, 0.0f, 0.0f,
                            math::vec2_format_get(region.pivot));
          ImGui::EndDisabled();

          int originIndex = region.origin == Origin::TOP_LEFT ? 0 : region.origin == Origin::CENTER ? 1 : 2;
          if (ImGui::Combo(localize.get(LABEL_REGION_PROPERTIES_ORIGIN), &originIndex, originOptions,
                           IM_ARRAYSIZE(originOptions)))
          {
            region.origin = originIndex == 0 ? Origin::TOP_LEFT : originIndex == 1 ? Origin::CENTER : Origin::CUSTOM;
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_REGION_PROPERTIES_ORIGIN));
          }

          if (region.origin == Origin::TOP_LEFT)
            region.pivot = {};
          else if (region.origin == Origin::CENTER)
            region.pivot = region.size * 0.5f;
        }
        ImGui::EndChild();

        auto widgetSize = widget_size_with_row_get(2);

        shortcut(manager.chords[SHORTCUT_CONFIRM]);
        if (ImGui::Button(reference == -1 ? localize.get(BASIC_ADD) : localize.get(BASIC_CONFIRM), widgetSize))
        {
          auto editedRegion = region;
          auto editedReference = reference;
          auto editedSpritesheetReference = spritesheetReference;
          manager.command_push({manager.selected, [&window, editedRegion, editedReference,
                                                   editedSpritesheetReference](Manager&, Document& document) mutable
                                {
                                  auto spritesheet =
                                      document.anm2.element_get(ElementType::SPRITESHEET, editedSpritesheetReference);
                                  if (!spritesheet) return;

                                  auto& selection = document.region.selection;
                                  auto& reference = document.region.reference;
                                  auto& frame = document.frames;

                                  if (editedReference == -1)
                                  {
                                    auto add = [&]()
                                    {
                                      auto added = editedRegion;
                                      auto id = element_child_next_id_get(*spritesheet, ElementType::REGION);
                                      added.type = ElementType::REGION;
                                      added.tag = "Region";
                                      added.id = id;
                                      auto insertIndex = region_insert_index_get(*spritesheet, document.region);
                                      spritesheet->children.insert(spritesheet->children.begin() + insertIndex, added);
                                      selection = {id};
                                      reference = id;
                                      window.newElementId = id;
                                    };

                                    window_edit(window, document, localize.get(EDIT_ADD_REGION), add);
                                  }
                                  else
                                  {
                                    auto set = [&]()
                                    {
                                      auto target =
                                          element_child_id_get(*spritesheet, ElementType::REGION, editedReference);
                                      if (!target) return;
                                      auto changed = editedRegion;
                                      changed.type = ElementType::REGION;
                                      changed.tag = "Region";
                                      changed.id = editedReference;
                                      *target = changed;
                                      selection = {editedReference};
                                    };

                                    window_edit(window, document, localize.get(EDIT_SET_REGION_PROPERTIES), set);
                                  }

                                  frame.reference = -1;
                                  frame.selection.clear();
                                }});

          window.popup.close();
        }

        ImGui::SameLine();

        shortcut(manager.chords[SHORTCUT_CANCEL]);
        if (ImGui::Button(localize.get(BASIC_CANCEL), widgetSize)) window.popup.close();

        ImGui::EndPopup();
      }

      window.popup.end();
    };
    return window;
  }

  Window overlays_window_register()
  {
    Window window{};
    window.title = LABEL_OVERLAYS_WINDOW;
    window.isOpen = &Settings::windowIsOverlays;
    window.changeType = Document::SPRITESHEETS;
    window.elementType = ElementType::OVERLAY;
    window.childLabel = "##Overlays Child";
    window.footerRows = 2;
    window.flags = 0;
    window.isChildPaddingZero = true;
    window.unavailableText = TEXT_SELECT_SPRITESHEET;
    window.popup = PopupHelper(LABEL_TASKBAR_OVERWRITE_FILE, POPUP_SMALL_NO_HEIGHT);
    window.storage_get = [](Document& document) -> Storage& { return document.overlay; };
    window.is_available = [](Document& document)
    { return document.anm2.element_get(ElementType::SPRITESHEET, document.spritesheet.reference); };
    window.add = [](Window& window, Manager&, Settings&, Document&, Clipboard&)
    {
      if (window.dialog) window.dialog->file_open(Dialog::OVERLAY_OPEN, true);
    };
    window.replace = [](Window& window, Manager&, Settings&, Document&, Clipboard&)
    {
      if (window.dialog) window.dialog->file_open(Dialog::OVERLAY_REPLACE);
    };
    window.path_set = [](Window& window, Manager&, Settings&, Document&, Clipboard&)
    {
      if (window.dialog) window.dialog->file_save(Dialog::OVERLAY_PATH_SET);
    };
    window.open = [](Window& window, Manager&, Settings&, Document& document, Clipboard&)
    {
      auto& selection = document.overlay.selection;
      if (selection.size() != 1 || !window.dialog) return;
      if (auto overlay = document.overlay_get(*selection.begin())) window_directory_open(*window.dialog, document, overlay->path);
    };
    window.remove = [](Window& window, Manager&, Settings&, Document& document, Clipboard&)
    {
      auto selected = document.overlay.selection;
      if (selected.empty()) return;
      auto spritesheet = document.anm2.element_get(ElementType::SPRITESHEET, document.spritesheet.reference);
      if (!spritesheet) return;

      auto behavior = [&]()
      {
        for (auto id : selected)
        {
          auto overlay = element_child_id_get(*spritesheet, ElementType::OVERLAY, id);
          if (!overlay) continue;
          auto pathString = path::to_utf8(overlay->path);
          toasts.push(std::vformat(localize.get(TOAST_REMOVE_OVERLAY), std::make_format_args(id, pathString)));
          logger.info(std::vformat(localize.get(TOAST_REMOVE_OVERLAY, anm2ed::ENGLISH),
                                   std::make_format_args(id, pathString)));
          element_child_id_erase(*spritesheet, ElementType::OVERLAY, id);
        }
        document.overlay.reference = -1;
        document.overlay.selection.clear();
      };

      window_edit(window, document, localize.get(EDIT_REMOVE_OVERLAYS), behavior);
    };
    window.reload = [](Window&, Manager&, Settings&, Document& document, Clipboard&)
    {
      auto selected = document.overlay.selection;
      if (selected.empty()) return;
      document.textures_snapshot(localize.get(EDIT_RELOAD_OVERLAYS));
      for (auto id : selected)
        document.overlay_texture_reload(id);
      document.change(Document::TEXTURES);

      for (auto id : selected)
      {
        auto overlay = document.overlay_get(id);
        if (!overlay) continue;
        document.overlay_hash_set_saved(id);
        auto pathString = path::to_utf8(overlay->path);
        toasts.push(std::vformat(localize.get(TOAST_RELOAD_OVERLAY), std::make_format_args(id, pathString)));
        logger.info(std::vformat(localize.get(TOAST_RELOAD_OVERLAY, anm2ed::ENGLISH),
                                 std::make_format_args(id, pathString)));
      }
    };
    window.save = [](Window&, Manager&, Settings&, Document& document, Clipboard&)
    {
      window_overlays_save(document, document.overlay.selection);
    };
    window.copy = [](Window&, Manager&, Settings&, Document& document, Clipboard& clipboard)
    {
      auto& selection = document.overlay.selection;
      if (selection.empty()) return;

      std::string clipboardText{};
      for (auto id : selection)
        if (auto overlay = document.overlay_get(id)) clipboardText += element_to_string(*overlay);
      clipboard.set(clipboardText);
    };
    window.paste = [](Window& window, Manager&, Settings&, Document& document, Clipboard& clipboard)
    {
      if (clipboard.is_empty()) return;

      auto spritesheet = document.anm2.element_get(ElementType::SPRITESHEET, document.spritesheet.reference);
      if (!spritesheet) return;

      tinyxml2::XMLDocument xmlDocument{};
      std::string errorString{};
      if (xmlDocument.Parse(clipboard.get().c_str()) != tinyxml2::XML_SUCCESS)
      {
        errorString = xmlDocument.ErrorStr();
        toasts.push(std::vformat(localize.get(TOAST_DESERIALIZE_OVERLAYS_FAILED), std::make_format_args(errorString)));
        logger.error(std::vformat(localize.get(TOAST_DESERIALIZE_OVERLAYS_FAILED, anm2ed::ENGLISH),
                                  std::make_format_args(errorString)));
        return;
      }

      std::vector<Element> pasted{};
      for (auto xmlElement = xmlDocument.FirstChildElement("Overlay"); xmlElement;
           xmlElement = xmlElement->NextSiblingElement("Overlay"))
      {
        auto overlay = element_read(xmlElement);
        if (overlay.type == ElementType::OVERLAY) pasted.push_back(std::move(overlay));
      }
      if (pasted.empty())
      {
        errorString = "No valid Overlay(s).";
        toasts.push(std::vformat(localize.get(TOAST_DESERIALIZE_OVERLAYS_FAILED), std::make_format_args(errorString)));
        logger.error(std::vformat(localize.get(TOAST_DESERIALIZE_OVERLAYS_FAILED, anm2ed::ENGLISH),
                                  std::make_format_args(errorString)));
        return;
      }

      auto behavior = [&]()
      {
        std::set<int> added{};
        for (auto overlay : pasted)
        {
          overlay.id = window_overlay_next_id_get(document);
          overlay.path = path::backslash_handle(overlay.path);
          added.insert(overlay.id);
          spritesheet->children.push_back(std::move(overlay));
          document.overlay_texture_reload(*added.rbegin());
        }

        document.overlay.selection = added;
        document.overlay.reference = added.empty() ? -1 : *added.rbegin();
        document.editTarget = Document::EditTarget::OVERLAY;
        window.newElementId = document.overlay.reference;
      };

      window_edit(window, document, localize.get(EDIT_PASTE_OVERLAYS), behavior);
    };
    window.rows_update =
        [](Window& window, Manager& manager, Settings& settings, Resources& resources, Clipboard& clipboard,
           Document& document, ImVec2)
    {
      auto spritesheet = document.anm2.element_get(ElementType::SPRITESHEET, document.spritesheet.reference);
      if (!spritesheet) return;

      auto& selection = document.overlay.selection;
      auto& reference = document.overlay.reference;
      auto style = ImGui::GetStyle();
      auto overlayChildSize = ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetTextLineHeightWithSpacing() * 4);
      bool isContextMenuQueued{};
      int contextMenuTargetId{-1};
      StringType visibilityTooltip{STRING_UNDEFINED};

      std::vector<int> ids{};
      for (auto& overlay : spritesheet->children)
        if (overlay.type == ElementType::OVERLAY) ids.push_back(overlay.id);

      if (reference != -1 && std::ranges::find(ids, reference) == ids.end())
      {
        reference = -1;
        selection.clear();
      }

      selection.start((int)ids.size());
      if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_A, ImGuiInputFlags_RouteFocused))
      {
        selection.clear();
        for (auto id : ids)
          selection.insert(id);
      }
      if (ImGui::Shortcut(ImGuiKey_Escape, ImGuiInputFlags_RouteFocused))
      {
        selection.clear();
        reference = -1;
      }

      int scrollTargetId = -1;
      int arrowSelectionId = -1;
      if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
          (ImGui::IsKeyPressed(ImGuiKey_UpArrow, true) || ImGui::IsKeyPressed(ImGuiKey_DownArrow, true)))
      {
        auto nextId = window_arrow_selection_get(ids, reference, selection);
        if (nextId != -1)
        {
          arrowSelectionId = nextId;
          scrollTargetId = nextId;
        }
      }

      ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2());
      for (auto& overlay : spritesheet->children)
      {
        if (overlay.type != ElementType::OVERLAY) continue;
        auto id = overlay.id;
        auto isNewOverlay = window.newElementId == id;
        ImGui::PushID(id);

        window_scroll_to_item(overlayChildSize.y, scrollTargetId == id);
        if (scrollTargetId == id) ImGui::SetNextWindowFocus();

        if (ImGui::BeginChild("##Overlay Child", overlayChildSize, ImGuiChildFlags_Borders))
        {
          auto isSelected = selection.contains(id) || arrowSelectionId == id;
          auto isReferenced = id == reference || arrowSelectionId == id;
          auto cursorPos = ImGui::GetCursorPos();
          auto textureInfo = document.overlay_texture_get(id);
          bool isValid = textureInfo && textureInfo->is_valid();
          auto& texture = isValid ? *textureInfo : resources.icons[icon::NONE];
          auto tintColor = !isValid       ? ImVec4(1.0f, 0.25f, 0.25f, 1.0f)
                           : overlay.isVisible ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f)
                                               : ImVec4(1.0f, 1.0f, 1.0f, 0.35f);
          auto pathString = path::to_utf8(overlay.path);
          auto pathCStr = pathString.c_str();
          auto visibleButtonSize = icon_size_get();

          ImGui::SetNextItemSelectionUserData(id);
          ImGui::SetNextItemStorageID(id);
          if (scrollTargetId == id) ImGui::SetKeyboardFocusHere();
          auto isActivated =
              ImGui::Selectable("##Overlay Selectable", isSelected, ImGuiSelectableFlags_AllowOverlap,
                                overlayChildSize);
          auto isRowRightClicked = ImGui::IsItemClicked(ImGuiMouseButton_Right);
          auto isClicked = ImGui::IsItemClicked(ImGuiMouseButton_Left) || isRowRightClicked;
          if (isActivated || isClicked)
          {
            document.editTarget = Document::EditTarget::OVERLAY;
            reference = id;
            document.region.reference = -1;
            document.region.selection.clear();
          }
          if (isRowRightClicked)
          {
            isContextMenuQueued = true;
            contextMenuTargetId = id;
          }
          if (scrollTargetId == id) ImGui::SetItemDefaultFocus();
          if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && window.dialog)
            window_directory_open(*window.dialog, document, overlay.path);

          auto imageSize = to_imvec2(vec2(overlayChildSize.y));
          auto aspectRatio = texture.size.y != 0 ? (float)texture.size.x / texture.size.y : 1.0f;
          if (imageSize.x / imageSize.y > aspectRatio)
            imageSize.x = imageSize.y * aspectRatio;
          else
            imageSize.y = imageSize.x / aspectRatio;

          ImGui::SetCursorPos(cursorPos);
          ImGui::ImageWithBg(texture.id, imageSize, ImVec2(), ImVec2(1, 1), ImVec4(), tintColor);

          ImGui::SetCursorPos(
              ImVec2(overlayChildSize.y + style.ItemSpacing.x,
                     overlayChildSize.y - overlayChildSize.y / 2 - ImGui::GetTextLineHeight() / 2));

          if (isReferenced) ImGui::PushFont(resources.fonts[font::ITALICS].get(), font::SIZE);
          auto overlayLabel = std::vformat(localize.get(FORMAT_OVERLAY), std::make_format_args(id, pathCStr));
          if (document.overlay_is_dirty(id))
            overlayLabel =
                std::vformat(localize.get(FORMAT_SPRITESHEET_NOT_SAVED), std::make_format_args(overlayLabel));
          ImGui::TextUnformatted(overlayLabel.c_str());
          if (isReferenced) ImGui::PopFont();

          auto visibleIcon = overlay.isVisible ? icon::VISIBLE : icon::INVISIBLE;
          auto visibleButtonPos =
              ImVec2(overlayChildSize.x - visibleButtonSize.x - style.ItemSpacing.x,
                     overlayChildSize.y * 0.5f - visibleButtonSize.y * 0.5f);
          ImGui::SetCursorPos(visibleButtonPos);
          ImGui::PushStyleColor(ImGuiCol_Button, ImVec4());
          ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4());
          ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4());
          ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2());
          ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
          if (ImGui::ImageButton("##Overlay Visible Toggle", resources.icons[visibleIcon].id, visibleButtonSize))
          {
            auto queuedId = id;
            manager.command_push({manager.selected,
                                  [queuedId](Manager&, Document& document)
                                  {
                                    auto behavior = [&]()
                                    {
                                      auto overlay = document.overlay_get(queuedId);
                                      if (!overlay) return;
                                      overlay->isVisible = !overlay->isVisible;
                                    };
                                    window_edit(document, Document::SPRITESHEETS,
                                                localize.get(EDIT_TOGGLE_ITEM_VISIBILITY), behavior);
                                  }});
          }
          auto isVisibleButtonHovered = ImGui::IsItemHovered();
          auto isVisibleButtonRightClicked = ImGui::IsItemClicked(ImGuiMouseButton_Right);
          if (isVisibleButtonRightClicked)
          {
            document.editTarget = Document::EditTarget::OVERLAY;
            reference = id;
            document.region.reference = -1;
            document.region.selection.clear();
            isContextMenuQueued = true;
            contextMenuTargetId = id;
          }
          ImGui::PopStyleVar(2);
          ImGui::PopStyleColor(3);
          if (isVisibleButtonHovered)
            visibilityTooltip =
                overlay.isVisible ? TOOLTIP_ITEM_VISIBILITY_SHOWN : TOOLTIP_ITEM_VISIBILITY_HIDDEN;
        }

        ImGui::EndChild();

        if (isNewOverlay)
        {
          ImGui::SetScrollHereY(0.5f);
          window.newElementId = -1;
        }

        ImGui::PopID();
      }
      ImGui::PopStyleVar();

      if (visibilityTooltip != STRING_UNDEFINED)
      {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, window.tooltipWindowPadding);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, window.tooltipItemSpacing);
        ImGui::SetTooltip("%s", localize.get(visibilityTooltip));
        ImGui::PopStyleVar(2);
      }

      selection.finish();
      if (contextMenuTargetId != -1)
      {
        document.editTarget = Document::EditTarget::OVERLAY;
        if (!selection.contains(contextMenuTargetId)) selection = {contextMenuTargetId};
        reference = contextMenuTargetId;
        document.region.reference = -1;
        document.region.selection.clear();
      }
      if (arrowSelectionId != -1)
      {
        document.editTarget = Document::EditTarget::OVERLAY;
        selection = {arrowSelectionId};
        reference = arrowSelectionId;
        document.region.reference = -1;
        document.region.selection.clear();
      }
      if (shortcut(manager.chords[SHORTCUT_CONFIRM], shortcut::FOCUSED) && reference != -1 && window.dialog)
        if (auto overlay = document.overlay_get(reference)) window_directory_open(*window.dialog, document, overlay->path);
      if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup) &&
          ImGui::IsMouseReleased(ImGuiMouseButton_Right) && !ImGui::IsAnyItemHovered())
        isContextMenuQueued = true;
      if (isContextMenuQueued) ImGui::OpenPopup("##Overlay Context Menu");

      Actions actions{};
      window_overlays_context_actions_add(window, manager, settings, document, clipboard, actions);
      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, window.tooltipWindowPadding);
      ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, window.tooltipItemSpacing);
      actions_popup_draw("##Overlay Context Menu", actions, settings);
      ImGui::PopStyleVar(2);
    };
    window.context_update =
        [](Window&, Manager&, Settings&, Resources&, Clipboard&, Document&) {};
    window.footer_update =
        [](Window& window, Manager& manager, Settings& settings, Resources&, Clipboard& clipboard, Document& document)
    {
      auto& selection = document.overlay.selection;

      auto rowOneWidgetSize = widget_size_with_row_get(3);
      Actions rowOneActions{};
      rowOneActions.add(ACTION_ADD, [&]() { return (bool)window.add; },
                        [&]() { window_command_run(window, manager, settings, document, clipboard, window.add); },
                        TOOLTIP_ADD_OVERLAY);
      rowOneActions.add(ACTION_RELOAD, [&]() { return !selection.empty() && (bool)window.reload; },
                        [&]() { window_command_run(window, manager, settings, document, clipboard, window.reload); },
                        TOOLTIP_RELOAD_OVERLAYS);
      rowOneActions.add(ACTION_REPLACE, [&]() { return selection.size() == 1 && (bool)window.replace; },
                        [&]() { window_command_run(window, manager, settings, document, clipboard, window.replace); },
                        TOOLTIP_REPLACE_OVERLAY);

      bool isSameLine{};
      for (auto& action : rowOneActions.items)
        action_button_draw(action, manager, settings, rowOneWidgetSize, isSameLine);

      if (window.dialog && window.dialog->is_selected(Dialog::OVERLAY_OPEN))
      {
        auto paths = window.dialog->paths;
        manager.command_push({manager.selected, [&window, paths](Manager&, Document& document)
                              {
                                document.overlays_add(document.spritesheet.reference, paths);
                                document.region.reference = -1;
                                document.region.selection.clear();
                                window.newElementId = document.overlay.reference;
                              }});
        window.dialog->reset();
      }

      if (window.dialog && window.dialog->is_selected(Dialog::OVERLAY_REPLACE))
      {
        if (selection.size() == 1 && !window.dialog->path.empty())
        {
          auto id = *selection.begin();
          auto dialogPath = window.dialog->path;
          manager.command_push({manager.selected, [id, dialogPath](Manager&, Document& document)
                                {
                                  auto behavior = [&]()
                                  {
                                    auto overlay = document.overlay_get(id);
                                    if (!overlay) return;
                                    overlay->path = window_asset_path_get(document, dialogPath);
                                    document.overlay_texture_reload(id);
                                  };

                                  window_edit(document, Document::SPRITESHEETS, localize.get(EDIT_REPLACE_OVERLAY),
                                              behavior);
                                  if (auto overlay = document.overlay_get(id))
                                  {
                                    document.overlay_hash_set_saved(id);
                                    auto pathString = path::to_utf8(overlay->path);
                                    toasts.push(std::vformat(localize.get(TOAST_REPLACE_OVERLAY),
                                                             std::make_format_args(id, pathString)));
                                    logger.info(std::vformat(localize.get(TOAST_REPLACE_OVERLAY, anm2ed::ENGLISH),
                                                             std::make_format_args(id, pathString)));
                                  }
                                }});
        }
        window.dialog->reset();
      }

      if (window.dialog && window.dialog->is_selected(Dialog::OVERLAY_PATH_SET))
      {
        if (selection.size() == 1 && !window.dialog->path.empty())
        {
          auto id = *selection.begin();
          auto dialogPath = window.dialog->path;
          manager.command_push({manager.selected, [id, dialogPath](Manager&, Document& document)
                                {
                                  auto behavior = [&]()
                                  {
                                    auto overlay = document.overlay_get(id);
                                    auto texture = document.overlay_texture_get(id);
                                    if (!overlay || !texture) return;
                                    auto newPath = window_asset_path_get(document, dialogPath);
                                    auto pathString = path::to_utf8(newPath);
                                    WorkingDirectory workingDirectory(document.directory_get());
                                    path::ensure_directory(newPath.parent_path());
                                    if (!texture->write_png(newPath))
                                    {
                                      toasts.push(std::vformat(localize.get(TOAST_SAVE_OVERLAY_FAILED),
                                                               std::make_format_args(id, pathString)));
                                      logger.error(
                                          std::vformat(localize.get(TOAST_SAVE_OVERLAY_FAILED, anm2ed::ENGLISH),
                                                       std::make_format_args(id, pathString)));
                                      return;
                                    }
                                    overlay->path = newPath;
                                    document.overlayTexturePaths[id] = overlay->path;
                                    document.overlay_hash_set_saved(id);
                                    toasts.push(std::vformat(localize.get(TOAST_SAVE_OVERLAY),
                                                             std::make_format_args(id, pathString)));
                                    logger.info(std::vformat(localize.get(TOAST_SAVE_OVERLAY, anm2ed::ENGLISH),
                                                             std::make_format_args(id, pathString)));
                                  };

                                  window_edit(document, Document::SPRITESHEETS, localize.get(EDIT_SET_OVERLAY_FILE_PATH),
                                              behavior);
                                }});
        }
        window.dialog->reset();
      }

      auto rowTwoWidgetSize = widget_size_with_row_get(2);
      Actions rowTwoActions{};
      rowTwoActions.add(ACTION_REMOVE, [&]() { return !selection.empty() && (bool)window.remove; },
                        [&]() { window_command_run(window, manager, settings, document, clipboard, window.remove); },
                        TOOLTIP_REMOVE_OVERLAYS);
      rowTwoActions.add(ACTION_SAVE, [&]() { return !selection.empty(); },
                        [&]()
                        {
                          if (settings.fileIsWarnOverwrite)
                          {
                            window.selection2 = selection;
                            window.popup.open();
                          }
                          else if (window.save)
                            window_command_run(window, manager, settings, document, clipboard, window.save);
                        },
                        TOOLTIP_SAVE_OVERLAYS);

      isSameLine = false;
      for (auto& action : rowTwoActions.items)
        action_button_draw(action, manager, settings, rowTwoWidgetSize, isSameLine);

      if (shortcut(manager.chords[SHORTCUT_ADD], shortcut::FOCUSED) && window.add)
        window_command_run(window, manager, settings, document, clipboard, window.add);
      if (shortcut(manager.chords[SHORTCUT_REMOVE], shortcut::FOCUSED) && window.remove)
        window_command_run(window, manager, settings, document, clipboard, window.remove);
      if (shortcut(manager.chords[SHORTCUT_COPY], shortcut::FOCUSED) && window.copy)
        window_command_run(window, manager, settings, document, clipboard, window.copy);
      if (shortcut(manager.chords[SHORTCUT_PASTE], shortcut::FOCUSED) && window.paste)
        window_command_run(window, manager, settings, document, clipboard, window.paste);
    };
    window.popup_update =
        [](Window& window, Manager& manager, Settings&, Resources&, Clipboard&, Document&)
    {
      window.popup.trigger();
      if (ImGui::BeginPopupModal(window.popup.label(), &window.popup.isOpen, ImGuiWindowFlags_NoResize))
      {
        ImGui::TextUnformatted(localize.get(LABEL_OVERWRITE_CONFIRMATION));

        auto widgetSize = widget_size_with_row_get(2);

        if (ImGui::Button(localize.get(BASIC_YES), widgetSize))
        {
          auto queuedSelection = window.selection2;
          manager.command_push({manager.selected, [queuedSelection](Manager&, Document& document) mutable
                                { window_overlays_save(document, queuedSelection); }});
          window.selection2.clear();
          window.popup.close();
        }

        ImGui::SameLine();

        if (ImGui::Button(localize.get(BASIC_NO), widgetSize))
        {
          window.selection2.clear();
          window.popup.close();
        }

        ImGui::EndPopup();
      }
      window.popup.end();
    };
    return window;
  }

  Window spritesheets_window_register()
  {
    Window window{};
    window.title = LABEL_SPRITESHEETS_WINDOW;
    window.isOpen = &Settings::windowIsSpritesheets;
    window.changeType = Document::SPRITESHEETS;
    window.containerType = ElementType::SPRITESHEETS;
    window.elementType = ElementType::SPRITESHEET;
    window.childLabel = "##Spritesheets Child";
    window.footerRows = 2;
    window.flags = 0;
    window.isChildPaddingZero = true;
    window.popup = PopupHelper(LABEL_SPRITESHEETS_MERGE_POPUP, POPUP_SMALL_NO_HEIGHT);
    window.popup2 = PopupHelper(LABEL_SPRITESHEETS_PACK_POPUP, POPUP_SMALL_NO_HEIGHT);
    window.popup3 = PopupHelper(LABEL_TASKBAR_OVERWRITE_FILE, POPUP_SMALL_NO_HEIGHT);
    window.storage_get = [](Document& document) -> Storage& { return document.spritesheet; };
    window.add = [](Window& window, Manager&, Settings&, Document&, Clipboard&)
    {
      if (window.dialog) window.dialog->file_open(Dialog::SPRITESHEET_OPEN, true);
    };
    window.replace = [](Window& window, Manager&, Settings&, Document&, Clipboard&)
    {
      if (window.dialog) window.dialog->file_open(Dialog::SPRITESHEET_REPLACE);
    };
    window.path_set = [](Window& window, Manager&, Settings&, Document&, Clipboard&)
    {
      if (window.dialog) window.dialog->file_save(Dialog::SPRITESHEET_PATH_SET);
    };
    window.open = [](Window& window, Manager&, Settings&, Document& document, Clipboard&)
    {
      auto& selection = document.spritesheet.selection;
      if (selection.size() != 1 || !window.dialog) return;
      if (auto spritesheet = document.anm2.element_get(ElementType::SPRITESHEET, *selection.begin()))
        window_directory_open(*window.dialog, document, spritesheet->path);
    };
    window.remove = [](Window& window, Manager&, Settings&, Document& document, Clipboard&)
    {
      auto& anm2 = document.anm2;
      auto unused = anm2.element_unused(ElementType::SPRITESHEET);
      if (unused.empty()) return;

      auto behavior = [&]()
      {
        auto items = anm2.element_get(ElementType::SPRITESHEETS);
        if (!items) return;
        for (auto& id : unused)
        {
          auto spritesheet = anm2.element_get(ElementType::SPRITESHEET, id);
          if (!spritesheet) continue;
          auto pathString = path::to_utf8(spritesheet->path);
          toasts.push(std::vformat(localize.get(TOAST_REMOVE_SPRITESHEET), std::make_format_args(id, pathString)));
          logger.info(std::vformat(localize.get(TOAST_REMOVE_SPRITESHEET, anm2ed::ENGLISH),
                                   std::make_format_args(id, pathString)));
          element_child_id_erase(*items, ElementType::SPRITESHEET, id);
        }
      };

      window_edit(window, document, localize.get(EDIT_REMOVE_UNUSED_SPRITESHEETS), behavior);
    };
    window.reload = [](Window& window, Manager&, Settings&, Document& document, Clipboard&)
    {
      auto selected = document.spritesheet.selection;
      window_edit(window, document, localize.get(EDIT_RELOAD_SPRITESHEETS),
                  [&]()
                  {
                    for (auto& id : selected)
                      document.texture_reload(id);
                  });

      for (auto& id : selected)
      {
        auto spritesheet = document.anm2.element_get(ElementType::SPRITESHEET, id);
        if (!spritesheet) continue;
        document.spritesheet_hash_set_saved(id);
        auto pathString = path::to_utf8(spritesheet->path);
        toasts.push(std::vformat(localize.get(TOAST_RELOAD_SPRITESHEET), std::make_format_args(id, pathString)));
        logger.info(std::vformat(localize.get(TOAST_RELOAD_SPRITESHEET, anm2ed::ENGLISH),
                                 std::make_format_args(id, pathString)));
      }
    };
    window.save = [](Window&, Manager&, Settings&, Document& document, Clipboard&)
    {
      window_spritesheets_save(document, document.spritesheet.selection);
    };
    window.merge_open = [](Window& window, Manager&, Settings&, Document& document, Clipboard&)
    {
      auto& selection = document.spritesheet.selection;
      if (selection.size() <= 1) return;
      window.selection = selection;
      window.popup.open();
    };
    window.merge = [](Window& window, Manager&, Settings& settings, Document& document, Clipboard&)
    {
      auto ids = window.selection.empty() ? document.spritesheet.selection : window.selection;
      window_spritesheets_merge(document, ids,
                                {.isAppendRight = settings.mergeSpritesheetsOrigin == MERGE_APPEND_RIGHT,
                                 .isMakeRegions = settings.mergeSpritesheetsIsMakeRegions,
                                 .isMakePrimaryRegion = settings.mergeSpritesheetsIsMakePrimaryRegion,
                                 .regionOrigin = (origin::Type)settings.mergeSpritesheetsRegionOrigin});
      window.selection.clear();
    };
    window.pack = [](Window& window, Manager&, Settings& settings, Document& document, Clipboard&)
    {
      auto id = window.editId != -1                          ? window.editId
                : document.spritesheet.selection.size() == 1 ? *document.spritesheet.selection.begin()
                                                             : -1;
      window_spritesheet_pack(document, id, settings.packPadding);
      window.editId = -1;
    };
    window.copy = [](Window&, Manager&, Settings&, Document& document, Clipboard& clipboard)
    {
      auto& selection = document.spritesheet.selection;
      if (selection.empty()) return;

      std::string clipboardText{};
      for (auto& id : selection)
        if (auto spritesheet = document.anm2.element_get(ElementType::SPRITESHEET, id))
          clipboardText += element_to_string(*spritesheet);
      clipboard.set(clipboardText);
    };
    window.paste = [](Window& window, Manager&, Settings&, Document& document, Clipboard& clipboard)
    {
      if (clipboard.is_empty()) return;

      auto& anm2 = document.anm2;
      auto& selection = document.spritesheet.selection;
      auto& reference = document.spritesheet.reference;
      auto& region = document.region;
      auto items = anm2.element_get(ElementType::SPRITESHEETS);
      auto maxSpritesheetIdBefore = items ? element_child_max_id_get(*items, ElementType::SPRITESHEET) : -1;
      auto pasted = anm2;
      std::string errorString{};
      if (pasted.deserialize(ElementType::SPRITESHEET, clipboard.get(), true, &errorString, document.directory_get()))
      {
        document.anm2_snapshot(localize.get(EDIT_PASTE_SPRITESHEETS));
        anm2 = std::move(pasted);
        if (auto pastedItems = anm2.element_get(ElementType::SPRITESHEETS))
        {
          auto maxSpritesheetIdAfter = element_child_max_id_get(*pastedItems, ElementType::SPRITESHEET);
          if (maxSpritesheetIdAfter > maxSpritesheetIdBefore)
          {
            window.newElementId = maxSpritesheetIdAfter;
            selection = {maxSpritesheetIdAfter};
            reference = maxSpritesheetIdAfter;
            region.reference = -1;
            region.selection.clear();
          }
        }
        document.anm2_change(Document::SPRITESHEETS);
      }
      else
      {
        toasts.push(
            std::vformat(localize.get(TOAST_DESERIALIZE_SPRITESHEETS_FAILED), std::make_format_args(errorString)));
        logger.error(std::vformat(localize.get(TOAST_DESERIALIZE_SPRITESHEETS_FAILED, anm2ed::ENGLISH),
                                  std::make_format_args(errorString)));
      }
    };
    window.rows_update =
        [](Window& window, Manager& manager, Settings&, Resources& resources, Clipboard&, Document& document, ImVec2)
    {
      auto& selection = document.spritesheet.selection;
      auto& reference = document.spritesheet.reference;
      auto& region = document.region;
      auto style = ImGui::GetStyle();
      auto spritesheetChildSize = ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetTextLineHeightWithSpacing() * 4);

      ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2());

      auto items = document.anm2.element_get(ElementType::SPRITESHEETS);
      int spritesheetCount{};
      std::vector<int> ids{};
      if (items)
        for (auto& spritesheet : items->children)
          if (spritesheet.type == ElementType::SPRITESHEET)
          {
            ++spritesheetCount;
            ids.push_back(spritesheet.id);
          }

      selection.start(spritesheetCount);
      if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_A, ImGuiInputFlags_RouteFocused))
      {
        selection.clear();
        for (auto& id : ids)
          selection.insert(id);
      }
      if (ImGui::Shortcut(ImGuiKey_Escape, ImGuiInputFlags_RouteFocused))
      {
        selection.clear();
        reference = -1;
        region.reference = -1;
        region.selection.clear();
      }

      int scrollTargetId = -1;
      int arrowSelectionId = -1;
      if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
          (ImGui::IsKeyPressed(ImGuiKey_UpArrow, true) || ImGui::IsKeyPressed(ImGuiKey_DownArrow, true)))
      {
        auto nextId = window_arrow_selection_get(ids, reference, selection);
        if (nextId != -1)
        {
          arrowSelectionId = nextId;
          scrollTargetId = nextId;
        }
      }

      if (items)
      {
        for (auto& spritesheet : items->children)
        {
          if (spritesheet.type != ElementType::SPRITESHEET) continue;
          auto id = spritesheet.id;
          auto isNewSpritesheet = window.newElementId == id;
          ImGui::PushID(id);

          window_scroll_to_item(spritesheetChildSize.y, scrollTargetId == id);
          if (scrollTargetId == id) ImGui::SetNextWindowFocus();

          if (ImGui::BeginChild("##Spritesheet Child", spritesheetChildSize, ImGuiChildFlags_Borders))
          {
            auto isSelected = selection.contains(id) || arrowSelectionId == id;
            auto isReferenced = id == reference || arrowSelectionId == id;
            auto cursorPos = ImGui::GetCursorPos();
            auto textureInfo = document.texture_get(id);
            bool isValid = textureInfo && textureInfo->is_valid();
            auto& texture = isValid ? *textureInfo : resources.icons[icon::NONE];
            auto tintColor = !isValid ? ImVec4(1.0f, 0.25f, 0.25f, 1.0f) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
            auto pathString = path::to_utf8(spritesheet.path);
            auto pathCStr = pathString.c_str();

            ImGui::SetNextItemSelectionUserData(id);
            ImGui::SetNextItemStorageID(id);
            if (scrollTargetId == id) ImGui::SetKeyboardFocusHere();
            auto isActivated = ImGui::Selectable("##Spritesheet Selectable", isSelected, 0, spritesheetChildSize);
            auto isClicked = ImGui::IsItemClicked(ImGuiMouseButton_Left) || ImGui::IsItemClicked(ImGuiMouseButton_Right);
            if (isActivated || isClicked)
            {
              document.editTarget = Document::EditTarget::SPRITESHEET;
              reference = id;
              region.reference = -1;
              region.selection.clear();
            }
            if (scrollTargetId == id) ImGui::SetItemDefaultFocus();
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && window.dialog)
              window_directory_open(*window.dialog, document, spritesheet.path);

            auto viewport = ImGui::GetMainViewport();
            auto maxPreviewSize = to_vec2(viewport->Size) * 0.5f;
            vec2 textureSize = vec2(glm::max(texture.size.x, 1), glm::max(texture.size.y, 1));
            if (textureSize.x > maxPreviewSize.x || textureSize.y > maxPreviewSize.y)
            {
              auto scale = glm::min(maxPreviewSize.x / textureSize.x, maxPreviewSize.y / textureSize.y);
              textureSize *= scale;
            }

            auto textWidth = ImGui::CalcTextSize(pathCStr).x;
            auto tooltipPadding = window.tooltipWindowPadding.x * 4.0f;
            auto minWidth = textureSize.x + window.tooltipItemSpacing.x + textWidth + tooltipPadding;

            ImGui::SetNextWindowSize(ImVec2(minWidth, 0), ImGuiCond_Appearing);

            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, window.tooltipItemSpacing);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, window.tooltipWindowPadding);
            if (ImGui::BeginItemTooltip())
            {
              ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2());
              auto childFlags = ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY;
              auto noScrollFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
              if (ImGui::BeginChild("##Spritesheet Tooltip Image Child", to_imvec2(textureSize), childFlags,
                                    noScrollFlags))
                ImGui::ImageWithBg(texture.id, to_imvec2(textureSize), ImVec2(), ImVec2(1, 1), ImVec4(), tintColor);
              ImGui::EndChild();
              ImGui::PopStyleVar();

              ImGui::SameLine();

              auto infoChildFlags = ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY;
              if (ImGui::BeginChild("##Spritesheet Info Tooltip Child", ImVec2(), infoChildFlags, noScrollFlags))
              {
                ImGui::PushFont(resources.fonts[font::BOLD].get(), font::SIZE);
                ImGui::TextUnformatted(pathCStr);
                ImGui::PopFont();

                ImGui::TextUnformatted(std::vformat(localize.get(FORMAT_ID), std::make_format_args(id)).c_str());

                if (!isValid)
                  ImGui::TextUnformatted(localize.get(TOOLTIP_SPRITESHEET_INVALID));
                else
                  ImGui::TextUnformatted(std::vformat(localize.get(FORMAT_TEXTURE_SIZE),
                                                      std::make_format_args(texture.size.x, texture.size.y))
                                             .c_str());

                ImGui::TextUnformatted(localize.get(TEXT_OPEN_DIRECTORY));
              }
              ImGui::EndChild();

              ImGui::EndTooltip();
            }
            ImGui::PopStyleVar(2);

            auto imageSize = to_imvec2(vec2(spritesheetChildSize.y));
            auto aspectRatio = texture.size.y != 0 ? (float)texture.size.x / texture.size.y : 1.0f;

            if (imageSize.x / imageSize.y > aspectRatio)
              imageSize.x = imageSize.y * aspectRatio;
            else
              imageSize.y = imageSize.x / aspectRatio;

            ImGui::SetCursorPos(cursorPos);
            ImGui::ImageWithBg(texture.id, imageSize, ImVec2(), ImVec2(1, 1), ImVec4(), tintColor);

            ImGui::SetCursorPos(
                ImVec2(spritesheetChildSize.y + style.ItemSpacing.x,
                       spritesheetChildSize.y - spritesheetChildSize.y / 2 - ImGui::GetTextLineHeight() / 2));

            if (isReferenced) ImGui::PushFont(resources.fonts[font::ITALICS].get(), font::SIZE);
            auto spritesheetLabel = std::vformat(localize.get(FORMAT_SPRITESHEET), std::make_format_args(id, pathCStr));
            if (document.spritesheet_is_dirty(id))
              spritesheetLabel =
                  std::vformat(localize.get(FORMAT_SPRITESHEET_NOT_SAVED), std::make_format_args(spritesheetLabel));
            ImGui::TextUnformatted(spritesheetLabel.c_str());
            if (isReferenced) ImGui::PopFont();
          }

          ImGui::EndChild();

          if (isNewSpritesheet)
          {
            ImGui::SetScrollHereY(0.5f);
            window.newElementId = -1;
          }

          ImGui::PopID();
        }
      }

      ImGui::PopStyleVar();
      selection.finish();
      if (arrowSelectionId != -1)
      {
        document.editTarget = Document::EditTarget::SPRITESHEET;
        selection = {arrowSelectionId};
        reference = arrowSelectionId;
        region.reference = -1;
        region.selection.clear();
      }
      if (shortcut(manager.chords[SHORTCUT_CONFIRM], shortcut::FOCUSED) && reference != -1 && window.dialog)
        if (auto spritesheet = document.anm2.element_get(ElementType::SPRITESHEET, reference))
          window_directory_open(*window.dialog, document, spritesheet->path);
    };
    window.context_update =
        [](Window& window, Manager& manager, Settings& settings, Resources&, Clipboard& clipboard, Document& document)
    {
      auto& selection = document.spritesheet.selection;
      auto style = ImGui::GetStyle();
      auto is_regions_empty = [&](int id)
      {
        auto spritesheet = document.anm2.element_get(ElementType::SPRITESHEET, id);
        if (!spritesheet) return true;
        for (auto& child : spritesheet->children)
          if (child.type == ElementType::REGION) return false;
        return true;
      };

      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, style.WindowPadding);
      ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, style.ItemSpacing);

      if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenOverlappedByWindow) &&
          ImGui::IsMouseClicked(ImGuiMouseButton_Right))
        ImGui::OpenPopup("##Spritesheet Context Menu");

      auto isPackable = selection.size() == 1 &&
                        document.anm2.element_get(ElementType::SPRITESHEET, *selection.begin()) &&
                        !is_regions_empty(*selection.begin());
      Actions actions{};
      actions_undo_redo_add(actions, manager, document);
      actions.separator();
      actions.add(ACTION_OPEN_DIRECTORY, [&]() { return selection.size() == 1 && (bool)window.open; },
                  [&]() { window_command_run(window, manager, settings, document, clipboard, window.open); });
      actions.add(ACTION_SET_FILE_PATH, [&]() { return selection.size() == 1 && (bool)window.path_set; },
                  [&]() { window_command_run(window, manager, settings, document, clipboard, window.path_set); });
      actions.add(ACTION_ADD, [&]() { return (bool)window.add; },
                  [&]() { window_command_run(window, manager, settings, document, clipboard, window.add); });
      actions.add(ACTION_REMOVE_UNUSED, [&]() { return (bool)window.remove; },
                  [&]() { window_command_run(window, manager, settings, document, clipboard, window.remove); });
      actions.add(ACTION_RELOAD, [&]() { return !selection.empty() && (bool)window.reload; },
                  [&]() { window_command_run(window, manager, settings, document, clipboard, window.reload); });
      actions.add(ACTION_REPLACE, [&]() { return selection.size() == 1 && (bool)window.replace; },
                  [&]() { window_command_run(window, manager, settings, document, clipboard, window.replace); });
      actions.add(ACTION_MERGE, [&]() { return selection.size() > 1 && (bool)window.merge_open; },
                  [&]() { window_command_run(window, manager, settings, document, clipboard, window.merge_open); });
      actions.add(ACTION_PACK, [&]() { return isPackable; },
                  [&]()
                  {
                    window.editId = *selection.begin();
                    window.popup2.open();
                  },
                  TOOLTIP_PACK_SPRITESHEET);
      actions.add(ACTION_SAVE, [&]() { return !selection.empty(); },
                  [&]()
                  {
                    if (settings.fileIsWarnOverwrite)
                    {
                      window.selection2 = selection;
                      window.popup3.open();
                    }
                    else if (window.save)
                      window_command_run(window, manager, settings, document, clipboard, window.save);
                  },
                  TOOLTIP_SAVE_SPRITESHEETS);
      actions.separator();
      actions.add(ACTION_COPY, [&]() { return !selection.empty() && (bool)window.copy; },
                  [&]() { window_command_run(window, manager, settings, document, clipboard, window.copy); });
      actions.add(ACTION_PASTE, [&]() { return !clipboard.is_empty() && (bool)window.paste; },
                  [&]() { window_command_run(window, manager, settings, document, clipboard, window.paste); });
      actions_popup_draw("##Spritesheet Context Menu", actions, settings);
      ImGui::PopStyleVar(2);
    };
    window.footer_update =
        [](Window& window, Manager& manager, Settings& settings, Resources&, Clipboard& clipboard, Document& document)
    {
      auto& selection = document.spritesheet.selection;

      auto rowOneWidgetSize = widget_size_with_row_get(3);
      Actions rowOneActions{};
      rowOneActions.add(ACTION_ADD, [&]() { return (bool)window.add; },
                        [&]() { window_command_run(window, manager, settings, document, clipboard, window.add); },
                        TOOLTIP_ADD_SPRITESHEET);
      rowOneActions.add(ACTION_RELOAD, [&]() { return !selection.empty() && (bool)window.reload; },
                        [&]() { window_command_run(window, manager, settings, document, clipboard, window.reload); },
                        TOOLTIP_RELOAD_SPRITESHEETS);
      rowOneActions.add(ACTION_REPLACE, [&]() { return selection.size() == 1 && (bool)window.replace; },
                        [&]() { window_command_run(window, manager, settings, document, clipboard, window.replace); },
                        TOOLTIP_REPLACE_SPRITESHEET);

      bool isSameLine{};
      for (auto& action : rowOneActions.items)
        action_button_draw(action, manager, settings, rowOneWidgetSize, isSameLine);

      if (window.dialog && window.dialog->is_selected(Dialog::SPRITESHEET_OPEN))
      {
        auto paths = window.dialog->paths;
        manager.command_push({manager.selected, [&window, paths](Manager&, Document& document)
                              {
                                document.spritesheets_add(paths);
                                document.region.reference = -1;
                                document.region.selection.clear();
                                window.newElementId = document.spritesheet.reference;
                              }});
        window.dialog->reset();
      }

      if (window.dialog && window.dialog->is_selected(Dialog::SPRITESHEET_REPLACE))
      {
        if (selection.size() == 1 && !window.dialog->path.empty())
        {
          auto id = *selection.begin();
          auto dialogPath = window.dialog->path;
          manager.command_push({manager.selected, [id, dialogPath](Manager&, Document& document)
                                {
                                  auto behavior = [&]()
                                  {
                                    auto spritesheet = document.anm2.element_get(ElementType::SPRITESHEET, id);
                                    if (!spritesheet) return;
                                    spritesheet->path = window_asset_path_get(document, dialogPath);
                                    document.texture_reload(id);
                                  };

                                  window_edit(document, Document::SPRITESHEETS, localize.get(EDIT_REPLACE_SPRITESHEET),
                                              behavior);
                                  if (auto spritesheet = document.anm2.element_get(ElementType::SPRITESHEET, id))
                                  {
                                    document.spritesheet_hash_set_saved(id);
                                    auto pathString = path::to_utf8(spritesheet->path);
                                    toasts.push(std::vformat(localize.get(TOAST_REPLACE_SPRITESHEET),
                                                             std::make_format_args(id, pathString)));
                                    logger.info(std::vformat(localize.get(TOAST_REPLACE_SPRITESHEET, anm2ed::ENGLISH),
                                                             std::make_format_args(id, pathString)));
                                  }
                                }});
        }
        window.dialog->reset();
      }

      if (window.dialog && window.dialog->is_selected(Dialog::SPRITESHEET_PATH_SET))
      {
        if (selection.size() == 1 && !window.dialog->path.empty())
        {
          auto id = *selection.begin();
          auto dialogPath = window.dialog->path;
          manager.command_push({manager.selected, [id, dialogPath](Manager&, Document& document)
                                {
                                  auto behavior = [&]()
                                  {
                                    auto spritesheet = document.anm2.element_get(ElementType::SPRITESHEET, id);
                                    auto texture = document.texture_get(id);
                                    if (!spritesheet || !texture) return;
                                    auto newPath = window_asset_path_get(document, dialogPath);
                                    auto pathString = path::to_utf8(newPath);
                                    WorkingDirectory workingDirectory(document.directory_get());
                                    path::ensure_directory(newPath.parent_path());
                                    if (!texture->write_png(newPath))
                                    {
                                      toasts.push(std::vformat(localize.get(TOAST_SAVE_SPRITESHEET_FAILED),
                                                               std::make_format_args(id, pathString)));
                                      logger.error(
                                          std::vformat(localize.get(TOAST_SAVE_SPRITESHEET_FAILED, anm2ed::ENGLISH),
                                                       std::make_format_args(id, pathString)));
                                      return;
                                    }
                                    spritesheet->path = newPath;
                                    document.texturePaths[id] = spritesheet->path;
                                    document.spritesheet_hash_set_saved(id);
                                    toasts.push(std::vformat(localize.get(TOAST_SAVE_SPRITESHEET),
                                                             std::make_format_args(id, pathString)));
                                    logger.info(std::vformat(localize.get(TOAST_SAVE_SPRITESHEET, anm2ed::ENGLISH),
                                                             std::make_format_args(id, pathString)));
                                  };

                                  window_edit(document, Document::SPRITESHEETS,
                                              localize.get(EDIT_SET_SPRITESHEET_FILE_PATH), behavior);
                                }});
        }
        window.dialog->reset();
      }

      auto rowTwoWidgetSize = widget_size_with_row_get(2);
      Actions rowTwoActions{};
      rowTwoActions.add(ACTION_REMOVE_UNUSED, [&]() { return (bool)window.remove; },
                        [&]() { window_command_run(window, manager, settings, document, clipboard, window.remove); },
                        TOOLTIP_REMOVE_UNUSED_SPRITESHEETS);
      rowTwoActions.add(ACTION_SAVE, [&]() { return !selection.empty(); },
                        [&]()
                        {
                          if (settings.fileIsWarnOverwrite)
                          {
                            window.selection2 = selection;
                            window.popup3.open();
                          }
                          else if (window.save)
                            window_command_run(window, manager, settings, document, clipboard, window.save);
                        },
                        TOOLTIP_SAVE_SPRITESHEETS);

      isSameLine = false;
      for (auto& action : rowTwoActions.items)
        action_button_draw(action, manager, settings, rowTwoWidgetSize, isSameLine);

      if (shortcut(manager.chords[SHORTCUT_ADD], shortcut::FOCUSED) && window.add)
        window_command_run(window, manager, settings, document, clipboard, window.add);
      if (shortcut(manager.chords[SHORTCUT_REMOVE], shortcut::FOCUSED) && window.remove)
        window_command_run(window, manager, settings, document, clipboard, window.remove);
      if (shortcut(manager.chords[SHORTCUT_COPY], shortcut::FOCUSED) && window.copy)
        window_command_run(window, manager, settings, document, clipboard, window.copy);
      if (shortcut(manager.chords[SHORTCUT_PASTE], shortcut::FOCUSED) && window.paste)
        window_command_run(window, manager, settings, document, clipboard, window.paste);
      if (shortcut(manager.chords[SHORTCUT_MERGE], shortcut::FOCUSED) && selection.size() > 1 && window.merge_open)
        window_command_run(window, manager, settings, document, clipboard, window.merge_open);
    };
    window.popup_update =
        [](Window& window, Manager& manager, Settings& settings, Resources&, Clipboard&, Document& document)
    {
      window.popup.trigger();
      if (ImGui::BeginPopupModal(window.popup.label(), &window.popup.isOpen, ImGuiWindowFlags_NoResize))
      {
        settings.mergeSpritesheetsRegionOrigin =
            glm::clamp(settings.mergeSpritesheetsRegionOrigin, (int)origin::TOP_LEFT, (int)origin::ORIGIN_CENTER);

        auto close = [&]()
        {
          window.selection.clear();
          window.popup.close();
        };

        auto optionsSize = child_size_get(6);
        if (ImGui::BeginChild("##Merge Spritesheets Options", optionsSize, ImGuiChildFlags_Borders))
        {
          ImGui::SeparatorText(localize.get(LABEL_REGION_PROPERTIES_ORIGIN));
          ImGui::RadioButton(localize.get(LABEL_MERGE_SPRITESHEETS_APPEND_BOTTOM), &settings.mergeSpritesheetsOrigin,
                             MERGE_APPEND_BOTTOM);
          ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_MERGE_SPRITESHEETS_BOTTOM_LEFT));
          ImGui::SameLine();
          ImGui::RadioButton(localize.get(LABEL_MERGE_SPRITESHEETS_APPEND_RIGHT), &settings.mergeSpritesheetsOrigin,
                             MERGE_APPEND_RIGHT);
          ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_MERGE_SPRITESHEETS_TOP_RIGHT));

          ImGui::SeparatorText(localize.get(LABEL_OPTIONS));
          ImGui::Checkbox(localize.get(LABEL_MERGE_MAKE_SPRITESHEET_REGIONS), &settings.mergeSpritesheetsIsMakeRegions);
          ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_MERGE_MAKE_SPRITESHEET_REGIONS));

          ImGui::BeginDisabled(!settings.mergeSpritesheetsIsMakeRegions);
          ImGui::Checkbox(localize.get(LABEL_MERGE_MAKE_PRIMARY_SPRITESHEET_REGION),
                          &settings.mergeSpritesheetsIsMakePrimaryRegion);
          ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_MERGE_MAKE_PRIMARY_SPRITESHEET_REGION));

          const char* regionOriginOptions[] = {localize.get(LABEL_REGION_ORIGIN_TOP_LEFT),
                                               localize.get(LABEL_REGION_ORIGIN_CENTER)};
          ImGui::Combo(localize.get(LABEL_REGION_PROPERTIES_ORIGIN), &settings.mergeSpritesheetsRegionOrigin,
                       regionOriginOptions, IM_ARRAYSIZE(regionOriginOptions));
          ImGui::EndDisabled();
        }
        ImGui::EndChild();

        auto widgetSize = widget_size_with_row_get(2);
        shortcut(manager.chords[SHORTCUT_CONFIRM]);
        ImGui::BeginDisabled(window.selection.size() <= 1);
        if (ImGui::Button(localize.get(BASIC_MERGE), widgetSize))
        {
          auto queuedSelection = window.selection;
          SpritesheetMergeOptions options{.isAppendRight = settings.mergeSpritesheetsOrigin == MERGE_APPEND_RIGHT,
                                          .isMakeRegions = settings.mergeSpritesheetsIsMakeRegions,
                                          .isMakePrimaryRegion = settings.mergeSpritesheetsIsMakePrimaryRegion,
                                          .regionOrigin = (origin::Type)settings.mergeSpritesheetsRegionOrigin};
          manager.command_push({manager.selected, [queuedSelection, options](Manager&, Document& document) mutable
                                {
                                  window_spritesheets_merge(document, queuedSelection, options);
                                }});
          close();
        }
        ImGui::EndDisabled();

        ImGui::SameLine();
        shortcut(manager.chords[SHORTCUT_CANCEL]);
        if (ImGui::Button(localize.get(BASIC_CANCEL), widgetSize)) close();

        ImGui::EndPopup();
      }
      window.popup.end();

      window.popup2.trigger();
      if (ImGui::BeginPopupModal(window.popup2.label(), &window.popup2.isOpen, ImGuiWindowFlags_NoResize))
      {
        settings.packPadding = std::max(0, settings.packPadding);

        auto close = [&]()
        {
          window.editId = -1;
          window.popup2.close();
        };

        auto optionsSize = child_size_get(1);
        if (ImGui::BeginChild("##Pack Spritesheet Options", optionsSize, ImGuiChildFlags_Borders))
          ImGui::DragInt(localize.get(LABEL_PACK_PADDING), &settings.packPadding, DRAG_SPEED, 0, PADDING_MAX);
        ImGui::EndChild();

        auto widgetSize = widget_size_with_row_get(2);
        shortcut(manager.chords[SHORTCUT_CONFIRM]);
        auto spritesheet =
            window.editId != -1 ? document.anm2.element_get(ElementType::SPRITESHEET, window.editId) : nullptr;
        bool isPackable{};
        if (spritesheet)
          for (auto& child : spritesheet->children)
            if (child.type == ElementType::REGION) isPackable = true;
        ImGui::BeginDisabled(!isPackable);
        if (ImGui::Button(localize.get(BASIC_PACK), widgetSize))
        {
          auto queuedEditId = window.editId;
          auto queuedPadding = settings.packPadding;
          manager.command_push({manager.selected, [queuedEditId, queuedPadding](Manager&, Document& document) mutable
                                { window_spritesheet_pack(document, queuedEditId, queuedPadding); }});
          close();
        }
        ImGui::EndDisabled();

        ImGui::SameLine();
        shortcut(manager.chords[SHORTCUT_CANCEL]);
        if (ImGui::Button(localize.get(BASIC_CANCEL), widgetSize)) close();

        ImGui::EndPopup();
      }
      window.popup2.end();

      window.popup3.trigger();
      if (ImGui::BeginPopupModal(window.popup3.label(), &window.popup3.isOpen, ImGuiWindowFlags_NoResize))
      {
        ImGui::TextUnformatted(localize.get(LABEL_OVERWRITE_CONFIRMATION));

        auto widgetSize = widget_size_with_row_get(2);

        if (ImGui::Button(localize.get(BASIC_YES), widgetSize))
        {
          auto queuedSelection = window.selection2;
          manager.command_push({manager.selected, [queuedSelection](Manager&, Document& document) mutable
                                { window_spritesheets_save(document, queuedSelection); }});
          window.selection2.clear();
          window.popup3.close();
        }

        ImGui::SameLine();

        if (ImGui::Button(localize.get(BASIC_NO), widgetSize))
        {
          window.selection2.clear();
          window.popup3.close();
        }

        ImGui::EndPopup();
      }
      window.popup3.end();
    };
    return window;
  }

  Window sounds_window_register()
  {
    Window window{};
    window.title = LABEL_SOUNDS_WINDOW;
    window.isOpen = &Settings::windowIsSounds;
    window.changeType = Document::SOUNDS;
    window.containerType = ElementType::SOUNDS;
    window.elementType = ElementType::SOUND_ELEMENT;
    window.childLabel = "##Sounds Child";
    window.footerRows = 1;
    window.flags = 0;
    window.isChildPaddingZero = true;
    window.storage_get = [](Document& document) -> Storage& { return document.sound; };
    window.add = [](Window& window, Manager&, Settings&, Document&, Clipboard&)
    {
      if (window.dialog) window.dialog->file_open(Dialog::SOUND_OPEN, true);
    };
    window.remove = [](Window& window, Manager&, Settings&, Document& document, Clipboard&)
    {
      auto unused = document.anm2.element_unused(ElementType::SOUND_ELEMENT);
      if (unused.empty()) return;

      auto behavior = [&]()
      {
        auto sounds = document.anm2.element_get(ElementType::SOUNDS);
        if (!sounds) return;
        for (auto& id : unused)
          element_child_id_erase(*sounds, ElementType::SOUND_ELEMENT, id);
      };

      window_edit(window, document, localize.get(EDIT_REMOVE_UNUSED_SOUNDS), behavior);
    };
    window.reload = [](Window& window, Manager&, Settings&, Document& document, Clipboard&)
    {
      auto& selection = document.sound.selection;
      auto behavior = [&]()
      {
        for (auto& id : selection)
        {
          auto sound = window_element_get(window, document.anm2, id);
          if (!sound) continue;
          document.sound_reload(id);
          auto pathString = path::to_utf8(sound->path);
          toasts.push(std::vformat(localize.get(TOAST_RELOAD_SOUND), std::make_format_args(id, pathString)));
          logger.info(
              std::vformat(localize.get(TOAST_RELOAD_SOUND, anm2ed::ENGLISH), std::make_format_args(id, pathString)));
        }
      };

      window_edit(window, document, localize.get(EDIT_RELOAD_SOUNDS), behavior);
    };
    window.replace = [](Window& window, Manager&, Settings&, Document&, Clipboard&)
    {
      if (window.dialog) window.dialog->file_open(Dialog::SOUND_REPLACE);
    };
    window.open = [](Window& window, Manager&, Settings&, Document& document, Clipboard&)
    {
      auto& selection = document.sound.selection;
      if (selection.size() != 1 || !window.dialog) return;
      if (auto sound = window_element_get(window, document.anm2, *selection.begin()))
        window_directory_open(*window.dialog, document, sound->path);
    };
    window.copy = [](Window&, Manager&, Settings&, Document& document, Clipboard& clipboard)
    {
      auto& selection = document.sound.selection;
      if (selection.empty()) return;

      std::string clipboardText{};
      auto sounds = document.anm2.element_get(ElementType::SOUNDS);
      if (!sounds) return;
      for (auto& id : selection)
        if (auto sound = element_child_id_get(*sounds, ElementType::SOUND_ELEMENT, id))
          clipboardText += element_to_string(*sound);
      clipboard.set(clipboardText);
    };
    window.paste = [](Window& window, Manager&, Settings&, Document& document, Clipboard& clipboard)
    {
      if (clipboard.is_empty()) return;

      auto& anm2 = document.anm2;
      auto& reference = document.sound.reference;
      auto& selection = document.sound.selection;
      auto sounds = anm2.element_get(ElementType::SOUNDS);
      auto maxSoundIdBefore = sounds ? element_child_max_id_get(*sounds, ElementType::SOUND_ELEMENT) : -1;
      auto pasted = anm2;
      std::string errorString{};
      if (pasted.deserialize(ElementType::SOUND_ELEMENT, clipboard.get(), true, &errorString, document.directory_get()))
      {
        document.anm2_snapshot(localize.get(TOAST_SOUNDS_PASTE));
        anm2 = std::move(pasted);
        if (auto pastedSounds = anm2.element_get(ElementType::SOUNDS))
        {
          auto maxSoundIdAfter = element_child_max_id_get(*pastedSounds, ElementType::SOUND_ELEMENT);
          if (maxSoundIdAfter > maxSoundIdBefore)
          {
            window.newElementId = maxSoundIdAfter;
            selection = {maxSoundIdAfter};
            reference = maxSoundIdAfter;
          }
        }
        document.anm2_change(Document::SOUNDS);
      }
      else
      {
        toasts.push(std::vformat(localize.get(TOAST_SOUNDS_DESERIALIZE_ERROR), std::make_format_args(errorString)));
        logger.error(std::vformat(localize.get(TOAST_SOUNDS_DESERIALIZE_ERROR, anm2ed::ENGLISH),
                                  std::make_format_args(errorString)));
      }
    };
    window.rows_update = [](Window& window, Manager& manager, Settings& settings, Resources& resources,
                            Clipboard& clipboard, Document& document, ImVec2)
    {
      auto& reference = document.sound.reference;
      auto& selection = document.sound.selection;
      auto style = ImGui::GetStyle();
      auto soundChildSize = ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetTextLineHeightWithSpacing() * 2);
      auto sounds = document.anm2.element_get(ElementType::SOUNDS);
      int soundCount{};
      std::vector<int> ids{};
      if (sounds)
        for (auto& sound : sounds->children)
          if (sound.type == ElementType::SOUND_ELEMENT)
          {
            ++soundCount;
            ids.push_back(sound.id);
          }

      auto play = [&](int id)
      {
        if (auto audio = document.sound_get(id)) audio->play();
      };

      ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2());
      selection.start(soundCount);
      if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_A, ImGuiInputFlags_RouteFocused))
      {
        selection.clear();
        for (auto& id : ids)
          selection.insert(id);
      }
      if (ImGui::Shortcut(ImGuiKey_Escape, ImGuiInputFlags_RouteFocused)) selection.clear();

      int scrollTargetId = -1;
      int arrowSelectionId = -1;
      if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
          (ImGui::IsKeyPressed(ImGuiKey_UpArrow, true) || ImGui::IsKeyPressed(ImGuiKey_DownArrow, true)))
      {
        auto nextId = window_arrow_selection_get(ids, reference, selection);
        if (nextId != -1)
        {
          arrowSelectionId = nextId;
          scrollTargetId = nextId;
        }
      }

      if (sounds)
      {
        for (auto& sound : sounds->children)
        {
          if (sound.type != ElementType::SOUND_ELEMENT) continue;
          auto id = sound.id;
          auto isNewSound = window.newElementId == id;
          ImGui::PushID(id);

          window_scroll_to_item(soundChildSize.y, scrollTargetId == id);
          if (scrollTargetId == id) ImGui::SetNextWindowFocus();

          if (ImGui::BeginChild("##Sound Child", soundChildSize, ImGuiChildFlags_Borders))
          {
            auto isSelected = selection.contains(id) || arrowSelectionId == id;
            auto cursorPos = ImGui::GetCursorPos();
            auto audio = document.sound_get(id);
            bool isValid = audio && audio->is_valid();
            auto& soundIcon = isValid ? resources.icons[icon::SOUND] : resources.icons[icon::NONE];
            auto tintColor = !isValid ? ImVec4(1.0f, 0.25f, 0.25f, 1.0f) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
            auto pathString = path::to_utf8(sound.path);

            ImGui::SetNextItemSelectionUserData(id);
            ImGui::SetNextItemStorageID(id);
            if (scrollTargetId == id) ImGui::SetKeyboardFocusHere();
            auto isActivated = ImGui::Selectable("##Sound Selectable", isSelected, 0, soundChildSize);
            auto isClicked = ImGui::IsItemClicked(ImGuiMouseButton_Left) || ImGui::IsItemClicked(ImGuiMouseButton_Right);
            if (isActivated || isClicked)
            {
              reference = id;
              if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) play(id);
            }
            if (scrollTargetId == id) ImGui::SetItemDefaultFocus();
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && window.dialog)
              window_directory_open(*window.dialog, document, sound.path);

            auto textWidth = ImGui::CalcTextSize(pathString.c_str()).x;
            auto tooltipPadding = window.tooltipWindowPadding.x * 4.0f;
            auto minWidth = textWidth + window.tooltipItemSpacing.x + tooltipPadding;

            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, window.tooltipItemSpacing);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, window.tooltipWindowPadding);
            ImGui::SetNextWindowSize(ImVec2(minWidth, 0), ImGuiCond_Appearing);
            if (ImGui::BeginItemTooltip())
            {
              ImGui::PushFont(resources.fonts[font::BOLD].get(), font::SIZE);
              ImGui::TextUnformatted(pathString.c_str());
              ImGui::PopFont();
              ImGui::Text("%s: %d", localize.get(BASIC_ID), id);
              if (!isValid)
              {
                ImGui::Spacing();
                ImGui::TextWrapped("%s", localize.get(TOOLTIP_SOUND_INVALID));
              }
              else
              {
                ImGui::Text("%s", localize.get(TEXT_OPEN_DIRECTORY));
              }
              ImGui::EndTooltip();
            }
            ImGui::PopStyleVar(2);

            ImGui::SetCursorPos(cursorPos);
            auto imageSize = to_imvec2(vec2(soundChildSize.y));
            ImGui::ImageWithBg(soundIcon.id, imageSize, ImVec2(), ImVec2(1, 1), ImVec4(), tintColor);

            ImGui::SetCursorPos(ImVec2(soundChildSize.y + style.ItemSpacing.x,
                                       soundChildSize.y - soundChildSize.y / 2 - ImGui::GetTextLineHeight() / 2));

            ImGui::TextUnformatted(
                std::vformat(localize.get(FORMAT_SOUND), std::make_format_args(id, pathString)).c_str());
          }

          ImGui::EndChild();

          if (isNewSound)
          {
            ImGui::SetScrollHereY(0.5f);
            window.newElementId = -1;
          }

          ImGui::PopID();
        }
      }

      ImGui::PopStyleVar();
      selection.finish();
      if (arrowSelectionId != -1)
      {
        selection = {arrowSelectionId};
        reference = arrowSelectionId;
      }
      if (shortcut(manager.chords[SHORTCUT_CONFIRM], shortcut::FOCUSED) && reference != -1 && window.dialog)
        if (auto sound = document.anm2.element_get(ElementType::SOUND_ELEMENT, reference))
          window_directory_open(*window.dialog, document, sound->path);

      if (shortcut(manager.chords[SHORTCUT_ADD], shortcut::FOCUSED) && window.add)
        window_command_run(window, manager, settings, document, clipboard, window.add);
      if (shortcut(manager.chords[SHORTCUT_REMOVE], shortcut::FOCUSED) && window.remove)
        window_command_run(window, manager, settings, document, clipboard, window.remove);
      if (shortcut(manager.chords[SHORTCUT_COPY], shortcut::FOCUSED) && window.copy)
        window_command_run(window, manager, settings, document, clipboard, window.copy);
      if (shortcut(manager.chords[SHORTCUT_PASTE], shortcut::FOCUSED) && window.paste)
        window_command_run(window, manager, settings, document, clipboard, window.paste);
    };
    window.context_update =
        [](Window& window, Manager& manager, Settings& settings, Resources&, Clipboard& clipboard, Document& document)
    {
      auto& selection = document.sound.selection;
      auto style = ImGui::GetStyle();
      auto play = [&](int id)
      {
        if (auto audio = document.sound_get(id)) audio->play();
      };

      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, style.WindowPadding);
      ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, style.ItemSpacing);

      if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenOverlappedByWindow) &&
          ImGui::IsMouseClicked(ImGuiMouseButton_Right))
        ImGui::OpenPopup("##Sound Context Menu");

      Actions actions{};
      actions_undo_redo_add(actions, manager, document);
      actions.separator();
      actions.add(ACTION_PLAY, [&]() { return selection.size() == 1; }, [&]() { play(*selection.begin()); },
                  STRING_UNDEFINED, -1);
      actions.add(ACTION_OPEN_DIRECTORY, [&]() { return selection.size() == 1 && (bool)window.open; },
                  [&]() { window_command_run(window, manager, settings, document, clipboard, window.open); });
      actions.add(ACTION_ADD, [&]() { return (bool)window.add; },
                  [&]() { window_command_run(window, manager, settings, document, clipboard, window.add); });
      actions.add(ACTION_REMOVE_UNUSED, [&]() { return (bool)window.remove; },
                  [&]() { window_command_run(window, manager, settings, document, clipboard, window.remove); });
      actions.add(ACTION_RELOAD, [&]() { return !selection.empty() && (bool)window.reload; },
                  [&]() { window_command_run(window, manager, settings, document, clipboard, window.reload); });
      actions.add(ACTION_REPLACE, [&]() { return selection.size() == 1 && (bool)window.replace; },
                  [&]() { window_command_run(window, manager, settings, document, clipboard, window.replace); });
      actions.separator();
      actions.add(ACTION_COPY, [&]() { return !selection.empty() && (bool)window.copy; },
                  [&]() { window_command_run(window, manager, settings, document, clipboard, window.copy); });
      actions.add(ACTION_PASTE, [&]() { return !clipboard.is_empty() && (bool)window.paste; },
                  [&]() { window_command_run(window, manager, settings, document, clipboard, window.paste); });
      actions_popup_draw("##Sound Context Menu", actions, settings);
      ImGui::PopStyleVar(2);
    };
    window.footer_update =
        [](Window& window, Manager& manager, Settings& settings, Resources&, Clipboard& clipboard, Document& document)
    {
      auto& selection = document.sound.selection;
      auto widgetSize = widget_size_with_row_get(4);

      shortcut(manager.chords[SHORTCUT_ADD]);
      if (ImGui::Button(localize.get(BASIC_ADD), widgetSize) && window.add)
        window_command_run(window, manager, settings, document, clipboard, window.add);
      set_item_tooltip_shortcut(localize.get(TOOLTIP_SOUND_ADD), settings.shortcutAdd);

      if (window.dialog && window.dialog->is_selected(Dialog::SOUND_OPEN))
      {
        auto paths = window.dialog->paths;
        manager.command_push({manager.selected, [&window, paths](Manager&, Document& document)
                              {
                                document.sounds_add(paths);
                                window.newElementId = document.sound.reference;
                              }});
        window.dialog->reset();
      }

      ImGui::SameLine();

      shortcut(manager.chords[SHORTCUT_REMOVE]);
      if (ImGui::Button(localize.get(BASIC_REMOVE_UNUSED), widgetSize) && window.remove)
        window_command_run(window, manager, settings, document, clipboard, window.remove);
      set_item_tooltip_shortcut(localize.get(TOOLTIP_REMOVE_UNUSED_SOUNDS), settings.shortcutRemove);

      ImGui::SameLine();

      ImGui::BeginDisabled(selection.empty());
      if (ImGui::Button(localize.get(BASIC_RELOAD), widgetSize) && window.reload)
        window_command_run(window, manager, settings, document, clipboard, window.reload);
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_RELOAD_SOUNDS));
      ImGui::EndDisabled();

      ImGui::SameLine();

      ImGui::BeginDisabled(selection.size() != 1);
      if (ImGui::Button(localize.get(BASIC_REPLACE), widgetSize) && window.replace)
        window_command_run(window, manager, settings, document, clipboard, window.replace);
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_REPLACE_SOUND));
      ImGui::EndDisabled();

      if (window.dialog && window.dialog->is_selected(Dialog::SOUND_REPLACE))
      {
        if (selection.size() == 1 && !window.dialog->path.empty())
        {
          auto id = *selection.begin();
          auto dialogPath = window.dialog->path;
          manager.command_push(
              {manager.selected, [id, dialogPath](Manager&, Document& document)
               {
                 auto behavior = [&]()
                 {
                   auto sound = document.anm2.element_get(ElementType::SOUND_ELEMENT, id);
                   if (!sound) return;
                   sound->path = window_asset_path_get(document, dialogPath);
                   document.sound_reload(id);
                   auto pathString = path::to_utf8(sound->path);
                   toasts.push(std::vformat(localize.get(TOAST_REPLACE_SOUND), std::make_format_args(id, pathString)));
                   logger.info(std::vformat(localize.get(TOAST_REPLACE_SOUND, anm2ed::ENGLISH),
                                            std::make_format_args(id, pathString)));
                 };

                 window_edit(document, Document::SOUNDS, localize.get(EDIT_REPLACE_SOUND), behavior);
               }});
        }
        window.dialog->reset();
      }
    };
    return window;
  }

  Window layers_window_register()
  {
    Window window{};
    window.title = LABEL_LAYERS_WINDOW;
    window.isOpen = &Settings::windowIsLayers;
    window.changeType = Document::LAYERS;
    window.containerType = ElementType::LAYERS;
    window.elementType = ElementType::LAYER_ELEMENT;
    window.childLabel = "##Layers Child";
    window.addTooltip = TOOLTIP_ADD_LAYER;
    window.removeUnusedTooltip = TOOLTIP_REMOVE_UNUSED_LAYERS;
    window.pasteEdit = EDIT_PASTE_LAYERS;
    window.removeUnusedEdit = EDIT_REMOVE_UNUSED_LAYERS;
    window.deserializeFailedToast = TOAST_DESERIALIZE_LAYERS_FAILED;
    window.flags = WINDOW_ADD | WINDOW_REMOVE_UNUSED | WINDOW_COPY | WINDOW_PASTE | WINDOW_PROPERTIES;
    window.storage_get = [](Document& document) -> Storage& { return document.layer; };
    window.row_label_get = [](Document&, const Element& layer)
    {
      return std::vformat(localize.get(FORMAT_LAYER), std::make_format_args(layer.id, layer.name, layer.spritesheetId));
    };
    window.tooltip_draw = [](Document&, Resources& resources, const Element& layer)
    {
      ImGui::PushFont(resources.fonts[resource::font::BOLD].get(), resource::font::SIZE);
      ImGui::TextUnformatted(layer.name.c_str());
      ImGui::PopFont();
      ImGui::TextUnformatted(std::vformat(localize.get(FORMAT_ID), std::make_format_args(layer.id)).c_str());
      ImGui::TextUnformatted(
          std::vformat(localize.get(FORMAT_SPRITESHEET_ID), std::make_format_args(layer.spritesheetId)).c_str());
    };
    window.properties_open = [](Manager& manager, int id) { manager.layer_properties_open(id); };
    window.popup_update = [](Window& window, Manager& manager, Settings&, Resources&, Clipboard&, Document& document)
    {
      auto& reference = document.layer.reference;
      auto& propertiesPopup = manager.layerPropertiesPopup;

      manager.layer_properties_trigger();

      if (ImGui::BeginPopupModal(propertiesPopup.label(), &propertiesPopup.isOpen, ImGuiWindowFlags_NoResize))
      {
        auto childSize = child_size_get(2);
        auto& layer = manager.editLayer;

        if (ImGui::BeginChild("##Child", childSize, ImGuiChildFlags_Borders))
        {
          if (propertiesPopup.isJustOpened) ImGui::SetKeyboardFocusHere();
          input_text_string(localize.get(BASIC_NAME), &layer.name);
          ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_ITEM_NAME));

          combo_id_mapped(localize.get(LABEL_SPRITESHEET), &layer.spritesheetId, document.spritesheet.ids,
                          document.spritesheet.labels);
          ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_LAYER_SPRITESHEET));
        }
        ImGui::EndChild();

        auto widgetSize = widget_size_with_row_get(2);

        shortcut(manager.chords[SHORTCUT_CONFIRM]);
        if (ImGui::Button(reference == -1 ? localize.get(BASIC_ADD) : localize.get(BASIC_CONFIRM), widgetSize))
        {
          auto editedLayer = layer;
          auto editedReference = reference;
          manager.command_push({manager.selected, [&window, editedLayer, editedReference](Manager&, Document& document)
                                {
                                  auto behavior = [&]()
                                  {
                                    auto layers = document.anm2.element_get(ElementType::LAYERS);
                                    if (!layers) return;
                                    auto changed = editedLayer;
                                    changed.type = ElementType::LAYER_ELEMENT;
                                    changed.tag = "Layer";

                                    if (editedReference == -1)
                                    {
                                      auto id = element_child_next_id_get(*layers, ElementType::LAYER_ELEMENT);
                                      changed.id = id;
                                      layers->children.push_back(changed);
                                      document.layer.selection = {id};
                                      document.layer.reference = id;
                                      window.newElementId = id;
                                      return;
                                    }

                                    auto target =
                                        element_child_id_get(*layers, ElementType::LAYER_ELEMENT, editedReference);
                                    if (!target) return;
                                    changed.id = editedReference;
                                    *target = changed;
                                    document.layer.selection = {editedReference};
                                  };

                                  window_edit(document, Document::LAYERS,
                                              localize.get(editedReference == -1 ? EDIT_ADD_LAYER
                                                                                 : EDIT_SET_LAYER_PROPERTIES),
                                              behavior);
                                }});

          manager.layer_properties_close();
        }

        ImGui::SameLine();

        shortcut(manager.chords[SHORTCUT_CANCEL]);
        if (ImGui::Button(localize.get(BASIC_CANCEL), widgetSize)) manager.layer_properties_close();

        manager.layer_properties_end();
        ImGui::EndPopup();
      }
    };
    return window;
  }

  Window nulls_window_register()
  {
    Window window{};
    window.title = LABEL_NULLS_WINDOW;
    window.isOpen = &Settings::windowIsNulls;
    window.changeType = Document::NULLS;
    window.containerType = ElementType::NULLS;
    window.elementType = ElementType::NULL_ELEMENT;
    window.childLabel = "##Nulls Child";
    window.addTooltip = TOOLTIP_ADD_NULL;
    window.removeUnusedTooltip = TOOLTIP_REMOVE_UNUSED_NULLS;
    window.pasteEdit = EDIT_PASTE_NULLS;
    window.removeUnusedEdit = EDIT_REMOVE_UNUSED_NULLS;
    window.deserializeFailedToast = TOAST_DESERIALIZE_NULLS_FAILED;
    window.flags =
        WINDOW_ADD | WINDOW_REMOVE_UNUSED | WINDOW_COPY | WINDOW_PASTE | WINDOW_PROPERTIES | WINDOW_REFERENCE_ITALIC;
    window.storage_get = [](Document& document) -> Storage& { return document.null; };
    window.row_label_get = [](Document&, const Element& null)
    { return std::vformat(localize.get(FORMAT_NULL), std::make_format_args(null.id, null.name)); };
    window.tooltip_draw = [](Document&, Resources& resources, const Element& element)
    {
      ImGui::PushFont(resources.fonts[resource::font::BOLD].get(), resource::font::SIZE);
      ImGui::TextUnformatted(element.name.c_str());
      ImGui::PopFont();
      ImGui::TextUnformatted(std::vformat(localize.get(FORMAT_ID), std::make_format_args(element.id)).c_str());
    };
    window.properties_open = [](Manager& manager, int id) { manager.null_properties_open(id); };
    window.popup_update = [](Window& window, Manager& manager, Settings&, Resources&, Clipboard&, Document& document)
    {
      auto& reference = document.null.reference;
      auto& propertiesPopup = manager.nullPropertiesPopup;

      manager.null_properties_trigger();

      if (ImGui::BeginPopupModal(propertiesPopup.label(), &propertiesPopup.isOpen, ImGuiWindowFlags_NoResize))
      {
        auto childSize = child_size_get(2);
        auto& null = manager.editNull;

        if (ImGui::BeginChild("##Child", childSize, ImGuiChildFlags_Borders))
        {
          if (propertiesPopup.isJustOpened) ImGui::SetKeyboardFocusHere();
          input_text_string(localize.get(BASIC_NAME), &null.name);
          ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_NULL_NAME));

          ImGui::Checkbox(localize.get(LABEL_RECT), &null.isShowRect);
          ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_NULL_RECT));
        }
        ImGui::EndChild();

        auto widgetSize = widget_size_with_row_get(2);

        shortcut(manager.chords[SHORTCUT_CONFIRM]);
        if (ImGui::Button(reference == -1 ? localize.get(BASIC_ADD) : localize.get(BASIC_CONFIRM), widgetSize))
        {
          auto editedNull = null;
          auto editedReference = reference;
          manager.command_push({manager.selected, [&window, editedNull, editedReference](Manager&, Document& document)
                                {
                                  auto behavior = [&]()
                                  {
                                    auto nulls = document.anm2.element_get(ElementType::NULLS);
                                    if (!nulls) return;
                                    auto changed = editedNull;
                                    changed.type = ElementType::NULL_ELEMENT;
                                    changed.tag = "Null";

                                    if (editedReference == -1)
                                    {
                                      auto id = element_child_next_id_get(*nulls, ElementType::NULL_ELEMENT);
                                      changed.id = id;
                                      nulls->children.push_back(changed);
                                      document.null.selection = {id};
                                      document.null.reference = id;
                                      window.newElementId = id;
                                      return;
                                    }

                                    auto target = element_child_id_get(*nulls, ElementType::NULL_ELEMENT, editedReference);
                                    if (!target) return;
                                    changed.id = editedReference;
                                    *target = changed;
                                    document.null.selection = {editedReference};
                                  };

                                  window_edit(document, Document::NULLS,
                                              localize.get(editedReference == -1 ? EDIT_ADD_NULL
                                                                                 : EDIT_SET_NULL_PROPERTIES),
                                              behavior);
                                }});

          manager.null_properties_close();
        }

        ImGui::SameLine();

        shortcut(manager.chords[SHORTCUT_CANCEL]);
        if (ImGui::Button(localize.get(BASIC_CANCEL), widgetSize)) manager.null_properties_close();

        ImGui::EndPopup();
      }

      manager.null_properties_end();
    };
    return window;
  }

  Window events_window_register()
  {
    Window window{};
    window.title = LABEL_EVENTS_WINDOW;
    window.isOpen = &Settings::windowIsEvents;
    window.changeType = Document::EVENTS;
    window.containerType = ElementType::EVENTS;
    window.elementType = ElementType::EVENT_ELEMENT;
    window.childLabel = "##Events Child";
    window.addTooltip = TOOLTIP_ADD_EVENT;
    window.removeUnusedTooltip = TOOLTIP_REMOVE_UNUSED_EVENTS;
    window.addEdit = EDIT_ADD_EVENT;
    window.renameEdit = EDIT_RENAME_EVENT;
    window.pasteEdit = EDIT_PASTE_EVENTS;
    window.removeUnusedEdit = EDIT_REMOVE_UNUSED_EVENTS;
    window.deserializeFailedToast = TOAST_DESERIALIZE_EVENTS_FAILED;
    window.flags = WINDOW_ADD | WINDOW_REMOVE_UNUSED | WINDOW_COPY | WINDOW_PASTE | WINDOW_RENAME;
    window.storage_get = [](Document& document) -> Storage& { return document.event; };
    window.tooltip_draw = [](Document&, Resources& resources, const Element& element)
    {
      ImGui::PushFont(resources.fonts[resource::font::BOLD].get(), resource::font::SIZE);
      ImGui::TextUnformatted(element.name.c_str());
      ImGui::PopFont();
      ImGui::TextUnformatted(std::vformat(localize.get(FORMAT_ID), std::make_format_args(element.id)).c_str());
    };
    window.add = [](Window& window, Manager&, Settings&, Document& document, Clipboard&)
    {
      auto behavior = [&]()
      {
        auto events = document.anm2.element_get(ElementType::EVENTS);
        if (!events) return;

        auto id = element_child_next_id_get(*events, ElementType::EVENT_ELEMENT);
        auto event = element_make(ElementType::EVENT_ELEMENT);
        event.id = id;
        event.name = localize.get(TEXT_NEW_EVENT);
        events->children.push_back(event);

        auto& storage = window.storage_get(document);
        storage.selection = {id};
        storage.reference = id;
        window.newElementId = id;
      };

      window_edit(window, document, localize.get(window.addEdit), behavior);
    };
    return window;
  }
}
