#include "window.hpp"

#include <algorithm>
#include <filesystem>
#include <format>

#include "actions.hpp"
#include "path.hpp"
#include "strings.hpp"
#include "toast.hpp"
#include "util/imgui/layout.hpp"
#include "util/imgui/shortcut.hpp"

using namespace anm2ed::resource;
using namespace anm2ed::types;
using namespace anm2ed::util;
using namespace glm;

namespace anm2ed::imgui
{
  constexpr float SCROLL_CENTER = 0.5f;
  constexpr float TOOLTIP_PADDING_MULTIPLIER = 4.0f;
  constexpr float PREVIEW_VIEWPORT_FRACTION = 0.5f;
  constexpr const char* CONTEXT_MENU_LABEL = "##Context Menu";

  struct WindowFlagInfo
  {
    WindowFlag flag;
    ActionType action;
    Window::Command Window::* command;
  };

  constexpr WindowFlagInfo WINDOW_FLAG_INFOS[] = {
#define X(symbol, action, command) {WINDOW_##symbol, action, &Window::command},
      WINDOW_FLAGS
#undef X
  };

  bool is_drag_drop_active() { return ImGui::GetDragDropPayload() != nullptr; }

  void window_command_push(Window& window, Manager& manager, Settings& settings, Clipboard& clipboard,
                           const Window::Command& command)
  {
    if (!command) return;
    manager.command_push({manager.selected,
                          [&window, &settings, &clipboard, command](Manager& manager, Document& document) mutable
                          { command(window, manager, settings, document, clipboard); }});
  }

  void window_rename_finish(Window& window, Manager& manager, int key, const std::string& name)
  {
    manager.command_push({manager.selected, [&window, key, name](Manager&, Document& document)
                          {
                            if (!window.rename_apply || window.name_get(document, key) == name) return;
                            document.edit_apply(window.renameEdit, [&](model::Model& model)
                                                { window.rename_apply(document, model, key, name); });
                          }});
  }

  void window_copy(Window& window, Document& document, Clipboard& clipboard)
  {
    std::string clipboardText{};
    for (auto key : window.storage_get(document).selection)
      clipboardText += window.copy_get(document, key);
    if (!clipboardText.empty()) clipboard.set(clipboardText);
  }

  void window_paste(Window& window, Manager&, Settings&, Document& document, Clipboard& clipboard)
  {
    if (clipboard.is_empty() || !window.paste_apply) return;
    std::string errorString{};
    auto key = -1;
    document.edit_apply(window.pasteEdit, [&](model::Model& model)
                        { key = window.paste_apply(document, model, clipboard.get(), &errorString); });
    if (key == -1)
    {
      if (!errorString.empty()) toast_log(Level::ERROR, window.deserializeFailedToast, errorString);
      return;
    }
    auto& storage = window.storage_get(document);
    window.newElementId = key;
    storage.selection = {key};
    storage.reference = key;
    if (window.row_select) window.row_select(window, document, key);
  }

  void window_remove_unused(Window& window, Manager&, Settings&, Document& document, Clipboard&)
  {
    if (!window.unused_remove) return;
    document.edit_apply(window.removeUnusedEdit, [&](model::Model& model) { window.unused_remove(document, model); });
  }

  bool is_window_group_selected(const Window& window)
  {
    return window.elementType == ElementType::ANIMATION && !window.selection.empty();
  }

  bool is_window_item_selected(Window& window, Document& document)
  {
    return !window.storage_get(document).selection.empty() || is_window_group_selected(window);
  }

  void window_properties(Window& window, Manager& manager, Settings& settings, Clipboard& clipboard, int id)
  {
    if (window.properties)
      window_command_push(window, manager, settings, clipboard, window.properties);
    else if (window.properties_open)
      window.properties_open(manager, id);
  }

  void window_activate(Window& window, Manager& manager, Settings& settings, Clipboard& clipboard, int id)
  {
    if (window_flag_has(window.flags, WINDOW_PROPERTIES)) window_properties(window, manager, settings, clipboard, id);
    if (window_flag_has(window.flags, WINDOW_OPEN_DIRECTORY))
      window_command_push(window, manager, settings, clipboard, window.open);
  }

  Action window_action_make(Window& window, Manager& manager, Settings& settings, Document& document,
                            Clipboard& clipboard, const WindowFlagInfo& info, bool isFooter)
  {
    auto selection = &window.storage_get(document).selection;
    auto command = window.*(info.command);
    auto window_push = [&window, &manager, &settings, &clipboard](Window::Command command)
    {
      return [&window, &manager, &settings, &clipboard, command]()
      { window_command_push(window, manager, settings, clipboard, command); };
    };

    std::function<bool()> isEnabled = [selection]() { return selection->size() == 1; };
    std::function<void()> run = window_push(command);
    auto isItemSelected = [&window, &document]() { return is_window_item_selected(window, document); };
    auto isAny = [selection]() { return !selection->empty(); };

    switch (info.flag)
    {
      case WINDOW_RENAME:
        isEnabled = [&window, selection]()
        {
          return selection->size() == 1 ||
                 (window.elementType == ElementType::ANIMATION && selection->empty() && window.selection.size() == 1);
        };
        run = [&window, selection]()
        {
          window.renameQueued =
              selection->size() == 1 ? *selection->begin() : window_group_key_get(*window.selection.begin());
        };
        break;
      case WINDOW_PROPERTIES:
        run = [&window, &manager, &settings, &clipboard, selection]()
        { window_properties(window, manager, settings, clipboard, *selection->begin()); };
        break;
      case WINDOW_ADD:
        isEnabled = []() { return true; };
        if (!command && window.properties_open) run = [&window, &manager]() { window.properties_open(manager, -1); };
        break;
      case WINDOW_REMOVE_UNUSED:
        isEnabled = []() { return true; };
        if (!command) run = window_push(window_remove_unused);
        break;
      case WINDOW_COPY:
        isEnabled = isItemSelected;
        if (!command) run = [&window, &document, &clipboard]() { window_copy(window, document, clipboard); };
        break;
      case WINDOW_PASTE:
        isEnabled = [&clipboard]() { return !clipboard.is_empty(); };
        if (!command) run = window_push(window_paste);
        break;
      case WINDOW_DUPLICATE:
      case WINDOW_CUT:
      case WINDOW_REMOVE:
        isEnabled = isItemSelected;
        break;
      case WINDOW_RELOAD:
      case WINDOW_TRIM:
      case WINDOW_SAVE:
      case WINDOW_GROUP:
        isEnabled = isAny;
        break;
      case WINDOW_MERGE:
        isEnabled = [selection]() { return selection->size() > 1; };
        if (window.merge_open && (isFooter || window.elementType != ElementType::ANIMATION) &&
            !is_window_group_selected(window))
          run = window_push(window.merge_open);
        break;
      default:
        break;
    }

    if (auto enabled = window.enabled.find(info.flag); enabled != window.enabled.end())
      isEnabled = [&window, &document, is_enabled = enabled->second]() { return is_enabled(window, document); };

    auto tooltip = window.tooltips.find(info.flag);
    return action_make(info.action, std::move(isEnabled), std::move(run),
                       tooltip == window.tooltips.end() ? STRING_UNDEFINED : tooltip->second,
                       info.flag == WINDOW_PLAY ? -1 : ACTION_COUNT);
  }

  Actions window_context_actions_get(Window& window, Manager& manager, Settings& settings, Document& document,
                                     Clipboard& clipboard, bool isUndoRedoIncluded)
  {
    Actions actions{};
    if (isUndoRedoIncluded)
    {
      actions_undo_redo_add(actions, manager, document);
      actions.separator();
    }

    for (const auto& info : WINDOW_FLAG_INFOS)
    {
      if (info.flag == WINDOW_CUT) actions.separator();
      if (window_flag_has(window.flags, info.flag))
        actions.add(window_action_make(window, manager, settings, document, clipboard, info, false));
    }
    return actions;
  }

  void window_footer_draw(Window& window, Manager& manager, Settings& settings, Document& document,
                          Clipboard& clipboard)
  {
    for (const auto& row : window.footer)
    {
      auto widgetSize = widget_size_with_row_get((int)row.size());
      bool isSameLine{};
      for (auto flag : row)
      {
        auto info = std::ranges::find(WINDOW_FLAG_INFOS, flag, &WindowFlagInfo::flag);
        auto action = window_action_make(window, manager, settings, document, clipboard, *info, true);
        action_button_draw(action, manager, settings, widgetSize, isSameLine);
      }
    }
  }

  int window_arrow_selection_get(const std::vector<int>& ids, const Storage& storage)
  {
    if (ids.empty() || !ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) ||
        !(ImGui::IsKeyPressed(ImGuiKey_UpArrow, true) || ImGui::IsKeyPressed(ImGuiKey_DownArrow, true)))
      return -1;
    auto current =
        storage.reference == -1 && !storage.selection.empty() ? *storage.selection.begin() : storage.reference;
    auto it = std::ranges::find(ids, current);
    auto index = it == ids.end() ? 0 : (int)std::distance(ids.begin(), it);
    auto delta = ImGui::IsKeyPressed(ImGuiKey_UpArrow, true) ? -1 : 1;
    return ids[std::clamp(index + delta, 0, (int)ids.size() - 1)];
  }

  int window_selection_start(Window& window, Document& document, const std::vector<int>& arrowIds)
  {
    auto& storage = window.storage_get(document);
    storage.selection.set_index_map(&window.order);
    storage.selection.start(window.order.size());
    if (ImGui::Shortcut(ImGuiKey_Escape, ImGuiInputFlags_RouteFocused))
    {
      storage.selection.clear();
      storage.reference = -1;
      if (window.row_select) window.row_select(window, document, -1);
    }
    return window_arrow_selection_get(arrowIds, storage);
  }

  void window_selection_finish(Window& window, Manager& manager, Settings& settings, Clipboard& clipboard,
                               Document& document, int arrowSelectionId)
  {
    auto& storage = window.storage_get(document);
    storage.selection.finish();
    storage.selection.set_index_map(nullptr);
    if (arrowSelectionId != -1)
    {
      storage.selection = {arrowSelectionId};
      storage.reference = arrowSelectionId;
      if (window.row_select) window.row_select(window, document, arrowSelectionId);
    }
    if (shortcut(manager.chords[SHORTCUT_CONFIRM], shortcut::FOCUSED) && storage.selection.size() == 1)
      window_activate(window, manager, settings, clipboard, *storage.selection.begin());
  }

  void window_row_select(Window& window, Document& document, int key)
  {
    window.storage_get(document).reference = key;
    if (window.row_select) window.row_select(window, document, key);
  }

  void window_scroll_update(Window& window, int key, int scrollTargetId)
  {
    if ((window.newElementId != key && window.scrollQueued != key && scrollTargetId != key) || is_drag_drop_active())
      return;
    ImGui::SetScrollHereY(SCROLL_CENTER);
    if (window.newElementId == key) window.newElementId = -1;
    if (window.scrollQueued == key) window.scrollQueued = -1;
  }

  void window_tooltip_update(Window& window, Resources& resources, Document& document, int key)
  {
    if (!window.tooltip_draw) return;
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, window.tooltipItemSpacing);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, window.tooltipWindowPadding);
    if (ImGui::BeginItemTooltip())
    {
      window.tooltip_draw(document, resources, key);
      ImGui::EndTooltip();
    }
    ImGui::PopStyleVar(2);
  }

  bool window_row_draw(Window& window, Manager& manager, Resources& resources, Document& document, int key, int index,
                       int arrowSelectionId)
  {
    auto& storage = window.storage_get(document);
    auto isSelected = storage.selection.contains(key) || arrowSelectionId == key;
    auto isReferenced =
        window_flag_has(window.flags, WINDOW_REFERENCE_ITALIC) && (storage.reference == key || arrowSelectionId == key);
    auto font = window.row_font_get ? window.row_font_get(document, key)
                : isReferenced      ? resource::font::ITALICS
                                    : resource::font::REGULAR;
    auto isFontPushed = font != resource::font::REGULAR;
    auto name = window.name_get ? window.name_get(document, key) : std::string{};
    auto label = window.row_label_get ? window.row_label_get(document, key) : name;

    ImGui::PushID(key);
    ImGui::SetNextItemSelectionUserData(index);
    if (isFontPushed) ImGui::PushFont(resources.fonts[font].get(), resource::font::SIZE);
    if (arrowSelectionId == key) ImGui::SetKeyboardFocusHere();

    bool isActivated{};
    if (window_flag_has(window.flags, WINDOW_RENAME))
    {
      if (window.newElementId == key || window.renameQueued == key)
      {
        window.renameState = RENAME_FORCE_EDIT;
        window.renameQueued = -1;
      }

      auto isRenaming = window.renameId == key;
      auto renameLabel =
          std::format("###Document #{} Window #{} Element #{}", manager.selected, (int)window.elementType, key);
      isActivated = selectable_input_text(label, renameLabel, isRenaming ? window.renameText : name, isSelected,
                                          ImGuiSelectableFlags_None, window.renameState);
      if (isActivated && window.renameState == RENAME_BEGIN)
      {
        window.renameId = key;
        window.renameText = name;
      }
      else if (isActivated && window.renameState == RENAME_FINISHED)
      {
        if (isRenaming) window_rename_finish(window, manager, key, window.renameText);
        window.renameId = -1;
        window.renameText.clear();
      }
    }
    else
      isActivated = ImGui::Selectable(label.c_str(), isSelected);

    if (isActivated || ImGui::IsItemClicked(ImGuiMouseButton_Left) || ImGui::IsItemClicked(ImGuiMouseButton_Right))
      window_row_select(window, document, key);
    if (isFontPushed) ImGui::PopFont();
    window_scroll_update(window, key, arrowSelectionId);

    if (window_flag_has(window.flags, WINDOW_PROPERTIES) && ImGui::IsItemHovered() &&
        ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && window.properties_open)
      window.properties_open(manager, key);

    auto isBreak = window.row_drag_drop_update && window.row_drag_drop_update(window, manager, document, key, index);
    window_tooltip_update(window, resources, document, key);
    ImGui::PopID();
    return isBreak;
  }

  void window_tooltip_name_draw(Resources& resources, const std::string& name)
  {
    ImGui::PushFont(resources.fonts[font::BOLD].get(), font::SIZE);
    ImGui::TextUnformatted(name.c_str());
    ImGui::PopFont();
  }

  void window_tooltip_image_draw(const WindowCardImage& image, const std::function<void()>& info_draw)
  {
    auto maxPreviewSize = to_vec2(ImGui::GetMainViewport()->Size) * PREVIEW_VIEWPORT_FRACTION;
    auto previewSize = glm::max(image.size, vec2(1.0f));
    previewSize *= glm::min(1.0f, glm::min(maxPreviewSize.x / previewSize.x, maxPreviewSize.y / previewSize.y));

    auto childFlags = ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY;
    auto noScrollFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2());
    if (ImGui::BeginChild("##Tooltip Image Child", to_imvec2(previewSize), childFlags, noScrollFlags))
      ImGui::ImageWithBg(resource::texture::id_get(*image.texture), to_imvec2(previewSize), to_imvec2(image.uvMin),
                         to_imvec2(image.uvMax), ImVec4(), image.isValid ? CARD_TINT_VALID : CARD_TINT_INVALID);
    ImGui::EndChild();
    ImGui::PopStyleVar();

    ImGui::SameLine();
    if (ImGui::BeginChild("##Tooltip Info Child", ImVec2(), ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY,
                          noScrollFlags))
      info_draw();
    ImGui::EndChild();
  }

  void window_cards_draw(Window& window, Manager& manager, Settings& settings, Resources& resources,
                         Clipboard& clipboard, Document& document)
  {
    auto& storage = window.storage_get(document);
    auto cardSize = ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetTextLineHeightWithSpacing() * window.cardLines);
    auto style = ImGui::GetStyle();

    window.order = window.keys_get(document);

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2());
    auto arrowSelectionId = window_selection_start(window, document, window.order);

    for (int i = 0; i < (int)window.order.size(); i++)
    {
      auto id = window.order[i];

      ImGui::PushID(id);
      if (arrowSelectionId == id)
      {
        auto targetTop = ImGui::GetCursorPosY();
        if (targetTop < ImGui::GetScrollY())
          ImGui::SetScrollY(targetTop);
        else if (targetTop + cardSize.y > ImGui::GetScrollY() + ImGui::GetWindowHeight())
          ImGui::SetScrollY(targetTop + cardSize.y - ImGui::GetWindowHeight());
        ImGui::SetNextWindowFocus();
      }

      auto isBreak = false;
      if (ImGui::BeginChild("##Card Child", cardSize, ImGuiChildFlags_Borders))
      {
        auto cursorPos = ImGui::GetCursorPos();
        auto isSelected = storage.selection.contains(id) || arrowSelectionId == id;
        auto isReferenced = id == storage.reference || arrowSelectionId == id;
        auto image = window.card_image_get(document, resources, id);
        auto tint = image.isValid ? CARD_TINT_VALID : CARD_TINT_INVALID;
        auto label = window.row_label_get ? window.row_label_get(document, id) : window.name_get(document, id);

        ImGui::SetNextItemSelectionUserData(i);
        ImGui::SetNextItemStorageID(id);
        if (arrowSelectionId == id) ImGui::SetKeyboardFocusHere();
        if (ImGui::Selectable("##Card Selectable", isSelected, 0, cardSize) ||
            ImGui::IsItemClicked(ImGuiMouseButton_Left) || ImGui::IsItemClicked(ImGuiMouseButton_Right))
          window_row_select(window, document, id);
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
          window_activate(window, manager, settings, clipboard, id);

        if (window.tooltip_draw && ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip) &&
            !ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        {
          auto textWidth = ImGui::CalcTextSize(label.c_str()).x;
          ImGui::SetNextWindowSize(ImVec2(textWidth + window.tooltipItemSpacing.x +
                                              window.tooltipWindowPadding.x * TOOLTIP_PADDING_MULTIPLIER,
                                          0),
                                   ImGuiCond_Appearing);
          window_tooltip_update(window, resources, document, id);
        }

        isBreak = window.row_drag_drop_update && window.row_drag_drop_update(window, manager, document, id, i);

        auto aspectRatio = image.size.y != 0.0f ? image.size.x / image.size.y : 1.0f;
        auto imageSize = ImVec2(cardSize.y, cardSize.y);
        if (imageSize.x / imageSize.y > aspectRatio)
          imageSize.x = imageSize.y * aspectRatio;
        else
          imageSize.y = imageSize.x / aspectRatio;
        ImGui::SetCursorPos(cursorPos);
        ImGui::ImageWithBg(resource::texture::id_get(*image.texture), imageSize, to_imvec2(image.uvMin),
                           to_imvec2(image.uvMax), ImVec4(), tint);

        ImGui::SetCursorPos(
            ImVec2(cardSize.y + style.ItemSpacing.x, cardSize.y - cardSize.y / 2 - ImGui::GetTextLineHeight() / 2));
        if (isReferenced) ImGui::PushFont(resources.fonts[font::ITALICS].get(), font::SIZE);
        ImGui::TextUnformatted(label.c_str());
        if (isReferenced) ImGui::PopFont();
      }
      ImGui::EndChild();

      window_scroll_update(window, id, -1);
      ImGui::PopID();
      if (isBreak) break;
    }

    ImGui::PopStyleVar();
    window_selection_finish(window, manager, settings, clipboard, document, arrowSelectionId);
  }

  void window_list_draw(Window& window, Manager& manager, Settings& settings, Resources& resources,
                        Clipboard& clipboard, Document& document)
  {
    window.order = window.keys_get(document);
    auto arrowSelectionId = window_selection_start(window, document, window.order);
    for (int i = 0; i < (int)window.order.size(); ++i)
      if (window_row_draw(window, manager, resources, document, window.order[i], i, arrowSelectionId)) break;
    window_selection_finish(window, manager, settings, clipboard, document, arrowSelectionId);
  }

  std::filesystem::path window_asset_path_get(Document& document, const std::filesystem::path& path)
  {
    return path::backslash_replace(path::make_relative(path::backslash_handle(path), document.directory_get()));
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

  PopupButton window_popup_buttons_draw(Manager& manager, const char* confirmLabel, bool isConfirmEnabled,
                                        StringType cancelLabel)
  {
    auto widgetSize = widget_size_with_row_get(2);
    auto result = PopupButton::NONE;

    ImGui::BeginDisabled(!isConfirmEnabled);
    shortcut(manager.chords[SHORTCUT_CONFIRM]);
    if (ImGui::Button(confirmLabel, widgetSize)) result = PopupButton::CONFIRM;
    ImGui::EndDisabled();

    ImGui::SameLine();
    shortcut(manager.chords[SHORTCUT_CANCEL]);
    if (ImGui::Button(localize.get(cancelLabel), widgetSize)) result = PopupButton::CANCEL;
    return result;
  }

  void window_update(Window& window, Manager& manager, Settings& settings, Resources& resources, Dialog& dialog,
                     Clipboard& clipboard)
  {
    auto document = manager.get();
    if (!document || !window.isOpen || !window.storage_get) return;
    window.dialog = &dialog;

    if (ImGui::Begin(localize.get(window.title), &(settings.*window.isOpen)))
    {
      if (window.is_available && !window.is_available(window, *document))
      {
        if (window.unavailableText != STRING_UNDEFINED) ImGui::TextUnformatted(localize.get(window.unavailableText));
      }
      else
      {
        auto childSize = size_without_footer_get((int)window.footer.size());
        auto style = ImGui::GetStyle();
        window.tooltipWindowPadding = style.WindowPadding;
        window.tooltipItemSpacing = style.ItemSpacing;

        auto isCards = window.cardLines > 0;
        if (isCards) ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2());
        if (ImGui::BeginChild(window.childLabel, childSize, true))
        {
          if (window.rows_update)
            window.rows_update(window, manager, settings, resources, clipboard, *document);
          else if (isCards)
            window_cards_draw(window, manager, settings, resources, clipboard, *document);
          else
            window_list_draw(window, manager, settings, resources, clipboard, *document);

          auto actions = window_context_actions_get(window, manager, settings, *document, clipboard, false);
          actions_shortcuts_update(actions, manager);
        }
        ImGui::EndChild();
        if (isCards) ImGui::PopStyleVar();

        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenOverlappedByWindow) &&
            ImGui::IsMouseReleased(ImGuiMouseButton_Right))
          ImGui::OpenPopup(CONTEXT_MENU_LABEL);
        auto actions = window_context_actions_get(window, manager, settings, *document, clipboard, true);
        actions_popup_draw(CONTEXT_MENU_LABEL, actions, settings);

        window_footer_draw(window, manager, settings, *document, clipboard);
        if (window.body_update) window.body_update(window, manager, settings, resources, clipboard, *document);
      }
    }
    ImGui::End();

    if (window.popup_update) window.popup_update(window, manager, settings, resources, clipboard, *document);
    if (window.post_update) window.post_update(window, manager, settings, resources, clipboard, *document);
  }
}
