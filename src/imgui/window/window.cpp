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
      destination.children.push_back(group);

      for (const auto& child : sourceItem.children)
        window_track_shell_push(destination, copiedTrackIds, child, trackType, group.id);
    }
    return destination;
  }

  void window_edit(Window& window, Document& document, const std::string& message, auto behavior)
  {
    document.snapshot(message);
    behavior();
    document.anm2_change(window.changeType);
  }

  void window_edit(Document& document, Document::ChangeType changeType, const std::string& message, auto behavior)
  {
    document.snapshot(message);
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

                            document.snapshot(localize.get(renameEdit));
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
        document.snapshot(localize.get(window.pasteEdit));
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
    auto loadPath = path::lower_case_backslash_handle(path);
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

  int window_animations_merge(Document& document, const AnimationMergeOptions& options)
  {
    auto& anm2 = document.anm2;
    auto& selection = document.animation.selection;
    auto& reference = document.reference;
    auto& overlayIndex = document.overlayIndex;
    int merged{-1};

    document.snapshot(localize.get(EDIT_MERGE_ANIMATIONS));
    if (options.selection.empty())
    {
      if (selection.contains(overlayIndex)) overlayIndex = -1;

      if (selection.size() > 1)
        merged = anm2.animations_merge(*selection.begin(), selection);
      else if (selection.size() == 1 && *selection.begin() != window_animation_count_get(anm2) - 1)
      {
        auto start = *selection.begin();
        auto next = *selection.begin() + 1;
        std::set<int> animationSet{};
        animationSet.insert(start);
        animationSet.insert(next);
        merged = anm2.animations_merge(start, animationSet);
      }
      else
        return -1;
    }
    else
    {
      if (options.selection.contains(overlayIndex)) overlayIndex = -1;
      auto mergeSelection = options.selection;
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

    document.snapshot(localize.get(EDIT_MERGE_SPRITESHEETS));
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

    document.snapshot(localize.get(EDIT_PACK_SPRITESHEET));
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
      actions.add(ACTION_DUPLICATE, [&]() { return !selection.empty(); },
                  [&]() { window_command_run(window, manager, settings, document, clipboard, window.duplicate); },
                  window.duplicateTooltip);

    if (window_flag_has(window.flags, WINDOW_MERGE))
      actions.add(ACTION_MERGE, [&]() { return selection.size() == 1; },
                  [&]()
                  {
                    window_command_run(window, manager, settings, document, clipboard,
                                       window.merge_open ? window.merge_open : window.merge);
                  },
                  window.mergeTooltip);

    if (window_flag_has(window.flags, WINDOW_REMOVE))
      actions.add(ACTION_REMOVE, [&]() { return !selection.empty(); },
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
      actions.add(ACTION_RENAME, [&]() { return selection.size() == 1; },
                  [&]() { window.renameQueued = *selection.begin(); });
    if (window_flag_has(window.flags, WINDOW_PROPERTIES))
      actions.add(ACTION_PROPERTIES, [&]() { return selection.size() == 1; },
                  [&]() { window_properties(window, manager, *selection.begin()); });
    if (window_flag_has(window.flags, WINDOW_ADD))
      actions.add(ACTION_ADD, []() { return true; },
                  [&]() { window_add(window, manager, settings, document, clipboard); });
    if (window_flag_has(window.flags, WINDOW_DUPLICATE))
      actions.add(ACTION_DUPLICATE, [&]() { return !selection.empty(); },
                  [&]() { window_command_run(window, manager, settings, document, clipboard, window.duplicate); });
    if (window_flag_has(window.flags, WINDOW_MERGE))
      actions.add(ACTION_MERGE, [&]() { return !selection.empty(); },
                  [&]() { window_command_run(window, manager, settings, document, clipboard, window.merge); });
    if (window_flag_has(window.flags, WINDOW_REMOVE))
      actions.add(ACTION_REMOVE, [&]() { return !selection.empty(); },
                  [&]() { window_command_run(window, manager, settings, document, clipboard, window.remove); });
    if (window_flag_has(window.flags, WINDOW_REMOVE_UNUSED))
      actions.add(ACTION_REMOVE_UNUSED, []() { return true; },
                  [&]() { window_remove_unused(window, manager, settings, document, clipboard); });
    if (window_flag_has(window.flags, WINDOW_DEFAULT))
      actions.add(ACTION_DEFAULT, [&]() { return selection.size() == 1; },
                  [&]() { window_command_run(window, manager, settings, document, clipboard, window.default_set); });

    actions.separator();

    if (window_flag_has(window.flags, WINDOW_CUT))
      actions.add(ACTION_CUT, [&]() { return !selection.empty(); },
                  [&]() { window_command_run(window, manager, settings, document, clipboard, window.cut); });
    if (window_flag_has(window.flags, WINDOW_COPY))
      actions.add(ACTION_COPY, [&]() { return !selection.empty(); },
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
    auto count = window_element_count_get(window, container);
    int index{};

    selection.start(count);

    if (container)
    {
      for (auto& element : container->children)
      {
        if (element.type != window.elementType) continue;
        auto key = window.element_key_get ? window.element_key_get(element, index) : element.id;
        auto isSelected = selection.contains(key);
        auto isReferenced = window_flag_has(window.flags, WINDOW_REFERENCE_ITALIC) && storage.reference == key;
        auto font = window.row_font_get ? window.row_font_get(document, element, key)
                    : isReferenced      ? resource::font::ITALICS
                                        : resource::font::REGULAR;
        auto isFontPushed = font != resource::font::REGULAR;

        ImGui::PushID(key);
        ImGui::SetNextItemSelectionUserData(key);
        auto label = window.row_label_get ? window.row_label_get(document, element) : element.name;
        if (isFontPushed) ImGui::PushFont(resources.fonts[font].get(), resource::font::SIZE);

        if (window_flag_has(window.flags, WINDOW_RENAME))
        {
          if (window.newElementId == key || window.renameQueued == key)
          {
            window.renameState = RENAME_FORCE_EDIT;
            window.renameQueued = -1;
          }

          auto isRenaming = window.renameId == key;
          auto& name = isRenaming ? window.renameText : element.name;
          if (selectable_input_text(label, window_rename_format_get(window, manager, key), name, isSelected,
                                    ImGuiSelectableFlags_None, window.renameState))
          {
            if (window.row_select) window.row_select(window, document, key);
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
        }
        else
        {
          if (ImGui::Selectable(label.c_str(), isSelected))
            if (window.row_select) window.row_select(window, document, key);
        }

        if (isFontPushed) ImGui::PopFont();

        if (window.newElementId == key || window.scrollQueued == key)
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
    window.flags = WINDOW_ADD | WINDOW_DUPLICATE | WINDOW_MERGE | WINDOW_REMOVE | WINDOW_DEFAULT | WINDOW_CUT |
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
            manager.command_push({manager.selected, [&window, indices, targetIndex](Manager&, Document& document) mutable
                                  {
                                    auto move = [&]()
                                    {
                                      auto items = window_container_get(window, document.anm2);
                                      if (items)
                                        document.animation.selection =
                                            anm2ed::util::vector::move_indices_to_position(items->children, indices,
                                                                                           targetIndex);
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
        if (count == 0) items->defaultAnimation = animation.name;

        items->children.insert(items->children.begin() + index, animation);
        selection = {index};
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

      auto behavior = [&]()
      {
        auto items = document.anm2.element_get(ElementType::ANIMATIONS);
        if (!items || selection.empty()) return;
        for (auto it = selection.rbegin(); it != selection.rend(); ++it)
        {
          auto i = *it;
          if (i < 0 || i >= (int)items->children.size()) continue;
          if (overlayIndex == i) overlayIndex = -1;
          if (reference.animationIndex == i) reference.animationIndex = -1;
          items->children.erase(items->children.begin() + i);
        }
        selection.clear();
      };

      window_edit(window, document, localize.get(EDIT_REMOVE_ANIMATIONS), behavior);
    };
    window.duplicate = [](Window& window, Manager&, Settings&, Document& document, Clipboard&)
    {
      auto& selection = document.animation.selection;

      auto behavior = [&]()
      {
        auto items = document.anm2.element_get(ElementType::ANIMATIONS);
        if (!items || selection.empty()) return;
        auto duplicated = selection;
        auto end = *duplicated.rbegin();
        for (auto& id : duplicated)
        {
          if (id < 0 || id >= (int)items->children.size()) continue;
          items->children.insert(items->children.begin() + end, items->children[id]);
          selection.insert(++end);
          selection.erase(id);
        }
      };

      window_edit(window, document, localize.get(EDIT_DUPLICATE_ANIMATIONS), behavior);
    };
    window.merge = [](Window& window, Manager&, Settings& settings, Document& document, Clipboard&)
    {
      auto& mergeSelection = document.merge.selection;
      auto& mergeReference = document.merge.reference;
      auto merged = window_animations_merge(document, {.selection = mergeSelection,
                                                       .reference = mergeReference,
                                                       .type = (merge::Type)settings.mergeType,
                                                       .isDeleteAnimationsAfter = settings.mergeIsDeleteAnimationsAfter});
      if (merged != -1) window.scrollQueued = merged;
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
    window.copy = [](Window&, Manager&, Settings&, Document& document, Clipboard& clipboard)
    {
      auto& selection = document.animation.selection;
      if (selection.empty()) return;

      std::string clipboardText{};
      for (auto& i : selection)
        if (auto animation = document.anm2.element_get(ElementType::ANIMATION, i))
          clipboardText += element_to_string(*animation);
      clipboard.set(clipboardText);
    };
    window.cut = [](Window& window, Manager& manager, Settings& settings, Document& document, Clipboard& clipboard)
    {
      if (window.copy) window.copy(window, manager, settings, document, clipboard);

      auto& selection = document.animation.selection;
      auto behavior = [&]()
      {
        auto items = document.anm2.element_get(ElementType::ANIMATIONS);
        if (!items) return;
        for (auto it = selection.rbegin(); it != selection.rend(); ++it)
        {
          auto i = *it;
          if (i < 0 || i >= (int)items->children.size()) continue;
          items->children.erase(items->children.begin() + i);
        }
        selection.clear();
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
        std::set<int> indices{};
        std::string errorString{};
        if (anm2.animations_deserialize(clipboardText, start, indices, &errorString))
        {
          if (!indices.empty())
          {
            auto index = *indices.rbegin();
            selection = {index};
            reference = {index};
            window.newElementId = index;
            window.scrollQueued = index;
          }
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
      auto animation_count = [&]() { return window_element_count_get(window, animations()); };
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

        auto footerSize = footer_size_get();
        auto optionsSize = child_size_get(2);
        auto deleteAfterSize = child_size_get();
        auto animationsSize =
            ImVec2(0, ImGui::GetContentRegionAvail().y -
                          (optionsSize.y + deleteAfterSize.y + footerSize.y + ImGui::GetStyle().ItemSpacing.y * 3));

        if (ImGui::BeginChild(localize.get(LABEL_ANIMATIONS_CHILD), animationsSize, ImGuiChildFlags_Borders))
        {
          auto items = animations();
          mergeSelection.start(animation_count());

          if (items)
          {
            for (int i = 0; i < (int)items->children.size(); i++)
            {
              if (i == mergeReference) continue;

              auto& animation = items->children[i];
              if (animation.type != ElementType::ANIMATION) continue;

              ImGui::PushID(i);
              ImGui::SetNextItemSelectionUserData(i);
              ImGui::Selectable(animation.name.c_str(), mergeSelection.contains(i));
              ImGui::PopID();
            }
          }

          mergeSelection.finish();
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

        ImGui::BeginDisabled(mergeSelection.empty());
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

      document.snapshot(localize.get(EDIT_TRIM_REGIONS));
      behavior();
      document.change(Document::SPRITESHEETS);
    };
    window.copy = [](Window&, Manager&, Settings&, Document& document, Clipboard& clipboard)
    {
      auto spritesheet = document.anm2.element_get(ElementType::SPRITESHEET, document.spritesheet.reference);
      auto& selection = document.region.selection;
      if (!spritesheet || selection.empty()) return;

      std::string clipboardText{};
      for (auto& id : selection)
        if (auto region = element_child_id_get(*spritesheet, ElementType::REGION, id))
          clipboardText += element_to_string(*region);
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
      auto pasted = anm2;
      std::string errorString{};
      if (pasted.regions_deserialize(spritesheetReference, clipboard.get(), true, &errorString))
      {
        document.snapshot(localize.get(EDIT_PASTE_REGIONS));
        anm2 = std::move(pasted);
        if (auto pastedSpritesheet = anm2.element_get(ElementType::SPRITESHEET, spritesheetReference))
        {
          auto maxRegionIdAfter = element_child_max_id_get(*pastedSpritesheet, ElementType::REGION);
          if (maxRegionIdAfter > maxRegionIdBefore)
          {
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
      if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
          (ImGui::IsKeyPressed(ImGuiKey_UpArrow, true) || ImGui::IsKeyPressed(ImGuiKey_DownArrow, true)))
      {
        auto nextId = window_arrow_selection_get(window.order, reference, selection);
        if (nextId != -1)
        {
          selection = {nextId};
          reference = nextId;
          document.reference = {document.reference.animationIndex};
          frame.reference = -1;
          frame.selection.clear();
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
        auto isSelected = selection.contains(id);
        auto isReferenced = id == reference;

        ImGui::PushID(id);

        window_scroll_to_item(regionChildSize.y, scrollTargetId == id);

        if (ImGui::BeginChild("##Region Child", regionChildSize, ImGuiChildFlags_Borders))
        {
          auto cursorPos = ImGui::GetCursorPos();
          auto regionChildMin = ImGui::GetWindowPos();
          auto regionChildMax = ImVec2(regionChildMin.x + ImGui::GetWindowSize().x,
                                       regionChildMin.y + ImGui::GetWindowSize().y);

          ImGui::SetNextItemSelectionUserData(i);
          ImGui::SetNextItemStorageID(id);
          if (ImGui::Selectable("##Region Selectable", isSelected, 0, regionChildSize))
          {
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

                ImGui::TextUnformatted(std::vformat(localize.get(FORMAT_ID), std::make_format_args(id)).c_str());
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

          if (ImGui::BeginDragDropTarget())
          {
            if (auto payload = ImGui::AcceptDragDropPayload(
                    "Region Drag Drop",
                    ImGuiDragDropFlags_AcceptBeforeDelivery | ImGuiDragDropFlags_AcceptNoDrawDefaultRect))
            {
              auto isDropAfter = is_drop_after(regionRowMin, regionRowMax);
              drop_line_draw(ImGui::GetWindowDrawList(), regionChildMin, regionChildMax, isDropAfter);

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
                auto targetIndex = i + (isDropAfter ? 1 : 0);
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
            region.pivot = {(int)(region.size.x / 2.0f), (int)(region.size.y / 2.0f)};
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
                                      spritesheet->children.push_back(added);
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
        document.snapshot(localize.get(EDIT_PASTE_SPRITESHEETS));
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
        [](Window& window, Manager&, Settings&, Resources& resources, Clipboard&, Document& document, ImVec2)
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
      if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
          (ImGui::IsKeyPressed(ImGuiKey_UpArrow, true) || ImGui::IsKeyPressed(ImGuiKey_DownArrow, true)))
      {
        auto nextId = window_arrow_selection_get(ids, reference, selection);
        if (nextId != -1)
        {
          selection = {nextId};
          reference = nextId;
          region.reference = -1;
          region.selection.clear();
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

          if (ImGui::BeginChild("##Spritesheet Child", spritesheetChildSize, ImGuiChildFlags_Borders))
          {
            auto isSelected = selection.contains(id);
            auto isReferenced = id == reference;
            auto cursorPos = ImGui::GetCursorPos();
            auto textureInfo = document.texture_get(id);
            bool isValid = textureInfo && textureInfo->is_valid();
            auto& texture = isValid ? *textureInfo : resources.icons[icon::NONE];
            auto tintColor = !isValid ? ImVec4(1.0f, 0.25f, 0.25f, 1.0f) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
            auto pathString = path::to_utf8(spritesheet.path);
            auto pathCStr = pathString.c_str();

            ImGui::SetNextItemSelectionUserData(id);
            ImGui::SetNextItemStorageID(id);
            if (ImGui::Selectable("##Spritesheet Selectable", isSelected, 0, spritesheetChildSize))
            {
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
                                    if (!spritesheet) return;
                                    spritesheet->path = window_asset_path_get(document, dialogPath);
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
        document.snapshot(localize.get(TOAST_SOUNDS_PASTE));
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
      if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
          (ImGui::IsKeyPressed(ImGuiKey_UpArrow, true) || ImGui::IsKeyPressed(ImGuiKey_DownArrow, true)))
      {
        auto nextId = window_arrow_selection_get(ids, reference, selection);
        if (nextId != -1)
        {
          selection = {nextId};
          reference = nextId;
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

          if (ImGui::BeginChild("##Sound Child", soundChildSize, ImGuiChildFlags_Borders))
          {
            auto isSelected = selection.contains(id);
            auto cursorPos = ImGui::GetCursorPos();
            auto audio = document.sound_get(id);
            bool isValid = audio && audio->is_valid();
            auto& soundIcon = isValid ? resources.icons[icon::SOUND] : resources.icons[icon::NONE];
            auto tintColor = !isValid ? ImVec4(1.0f, 0.25f, 0.25f, 1.0f) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
            auto pathString = path::to_utf8(sound.path);

            ImGui::SetNextItemSelectionUserData(id);
            ImGui::SetNextItemStorageID(id);
            if (ImGui::Selectable("##Sound Selectable", isSelected, 0, soundChildSize))
            {
              reference = id;
              if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) play(id);
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
                ImGui::Text("%s", localize.get(TEXT_SOUND_PLAY));
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
