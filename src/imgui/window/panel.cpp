#include "panel.hpp"

#include <algorithm>
#include <filesystem>
#include <format>

#include "path.hpp"
#include "strings.hpp"
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

  bool is_drag_drop_active() { return ImGui::GetDragDropPayload() != nullptr; }

  void command_push(const Panel& panel, std::function<void(Document&)> run)
  {
    panel.manager.command_push(
        {panel.manager.selected, [run = std::move(run)](Manager&, Document& document) { run(document); }});
  }

  void row_select(std::set<int>&, int& focus, const ListRows& rows, int key)
  {
    focus = key;
    if (rows.select) rows.select(key);
  }

  void row_scroll_update(PanelState& state, int key, int arrowKey)
  {
    if ((state.newId != key && state.scrollId != key && arrowKey != key) || is_drag_drop_active()) return;
    ImGui::SetScrollHereY(SCROLL_CENTER);
    if (state.scrollId == key) state.scrollId = -1;
  }

  void row_tooltip_update(const ListRows& rows, int key)
  {
    if (!rows.tooltip_draw) return;
    if (ImGui::BeginItemTooltip())
    {
      rows.tooltip_draw(key);
      ImGui::EndTooltip();
    }
  }

  // Arrow keys move a single selection through the keys.
  int arrow_key_get(const std::vector<int>& keys, const std::set<int>& selection, int focus)
  {
    if (keys.empty() || !ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) ||
        !(ImGui::IsKeyPressed(ImGuiKey_UpArrow, true) || ImGui::IsKeyPressed(ImGuiKey_DownArrow, true)))
      return -1;
    auto current = focus == -1 && !selection.empty() ? *selection.begin() : focus;
    auto it = std::ranges::find(keys, current);
    auto index = it == keys.end() ? 0 : (int)std::distance(keys.begin(), it);
    auto delta = ImGui::IsKeyPressed(ImGuiKey_UpArrow, true) ? -1 : 1;
    return keys[std::clamp(index + delta, 0, (int)keys.size() - 1)];
  }

  // Multi-select over `keys` (row index -> key); returns the key the arrow keys moved to, or -1.
  int list_select_begin(Panel& panel, std::set<int>& selection, int& focus, const ListRows& rows,
                        const std::vector<int>& arrowKeys)
  {
    auto& storage = panel.state.selection;
    storage = selection;
    storage.set_index_map(&panel.state.keys);
    storage.start(panel.state.keys.size());
    selection = storage;
    if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_A, ImGuiInputFlags_RouteFocused))
      selection = storage = std::set<int>(panel.state.keys.begin(), panel.state.keys.end());
    if (ImGui::Shortcut(ImGuiKey_Escape, ImGuiInputFlags_RouteFocused))
    {
      storage.clear();
      selection.clear();
      row_select(selection, focus, rows, -1);
    }
    return arrow_key_get(arrowKeys, selection, focus);
  }

  void list_select_end(Panel& panel, std::set<int>& selection, int& focus, const ListRows& rows, int arrowKey)
  {
    auto& storage = panel.state.selection;
    storage.finish();
    storage.set_index_map(nullptr);
    selection = storage;
    if (arrowKey != -1)
    {
      selection = {arrowKey};
      row_select(selection, focus, rows, arrowKey);
    }
    if (shortcut(panel.manager.chords[SHORTCUT_CONFIRM], shortcut::FOCUSED) && selection.size() == 1 && rows.activate)
      rows.activate(*selection.begin());
  }

  bool row_draw(Panel& panel, std::set<int>& selection, int& focus, const ListRows& rows, int key, int index,
                int arrowKey)
  {
    auto& state = panel.state;
    auto isSelected = selection.contains(key) || arrowKey == key;
    auto isFocused = rows.isFocusItalic && (focus == key || arrowKey == key);
    auto font = rows.font_get ? rows.font_get(key) : isFocused ? font::ITALICS : font::REGULAR;
    auto label = rows.label_get(key);

    ImGui::PushID(key);
    ImGui::SetNextItemSelectionUserData(index);
    if (font != font::REGULAR) ImGui::PushFont(panel.resources.fonts[font].get(), font::SIZE);
    if (arrowKey == key) ImGui::SetKeyboardFocusHere();

    bool isActivated{};
    if (rows.rename)
    {
      if (state.newId == key)
      {
        state.renameState = RENAME_FORCE_EDIT;
        state.newId = -1;
      }
      auto isRenaming = state.renameId == key;
      auto name = rows.name_get(key);
      auto renameLabel = std::format("###Document #{} Row #{}", panel.manager.selected, key);
      isActivated = selectable_input_text(label, renameLabel, isRenaming ? state.renameText : name, isSelected,
                                          ImGuiSelectableFlags_None, state.renameState);
      if (isActivated && state.renameState == RENAME_BEGIN)
      {
        state.renameId = key;
        state.renameText = name;
      }
      else if (isActivated && state.renameState == RENAME_FINISHED)
      {
        if (isRenaming && state.renameText != name) rows.rename(key, state.renameText);
        state.renameId = -1;
        state.renameText.clear();
      }
    }
    else
      isActivated = ImGui::Selectable(label.c_str(), isSelected);

    if (isActivated || ImGui::IsItemClicked(ImGuiMouseButton_Left) || ImGui::IsItemClicked(ImGuiMouseButton_Right))
      row_select(selection, focus, rows, key);
    if (font != font::REGULAR) ImGui::PopFont();
    row_scroll_update(state, key, arrowKey);
    if (state.newId == key) state.newId = -1;

    if (rows.activate && ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
      rows.activate(key);

    auto isBreak = rows.drag_drop_update && rows.drag_drop_update(key, index);
    row_tooltip_update(rows, key);
    ImGui::PopID();
    return isBreak;
  }

  void card_draw(Panel& panel, std::set<int>& selection, int& focus, const ListRows& rows, int key, int index,
                 int arrowKey, bool& isBreak)
  {
    auto cardSize = ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetTextLineHeightWithSpacing() * rows.cardLines);
    auto style = ImGui::GetStyle();

    ImGui::PushID(key);
    if (arrowKey == key)
    {
      auto targetTop = ImGui::GetCursorPosY();
      if (targetTop < ImGui::GetScrollY())
        ImGui::SetScrollY(targetTop);
      else if (targetTop + cardSize.y > ImGui::GetScrollY() + ImGui::GetWindowHeight())
        ImGui::SetScrollY(targetTop + cardSize.y - ImGui::GetWindowHeight());
      ImGui::SetNextWindowFocus();
    }

    if (ImGui::BeginChild("##Card Child", cardSize, ImGuiChildFlags_Borders))
    {
      auto cursorPos = ImGui::GetCursorPos();
      auto isSelected = selection.contains(key) || arrowKey == key;
      auto isFocused = key == focus || arrowKey == key;
      auto image = rows.image_get(key);
      auto label = rows.label_get(key);

      ImGui::SetNextItemSelectionUserData(index);
      ImGui::SetNextItemStorageID(key);
      if (arrowKey == key) ImGui::SetKeyboardFocusHere();
      if (ImGui::Selectable("##Card Selectable", isSelected, 0, cardSize) ||
          ImGui::IsItemClicked(ImGuiMouseButton_Left) || ImGui::IsItemClicked(ImGuiMouseButton_Right))
        row_select(selection, focus, rows, key);
      if (rows.activate && ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        rows.activate(key);

      if (rows.tooltip_draw && ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip) &&
          !ImGui::IsMouseDragging(ImGuiMouseButton_Left))
      {
        ImGui::SetNextWindowSize(ImVec2(ImGui::CalcTextSize(label.c_str()).x + style.ItemSpacing.x +
                                            style.WindowPadding.x * TOOLTIP_PADDING_MULTIPLIER,
                                        0),
                                 ImGuiCond_Appearing);
        row_tooltip_update(rows, key);
      }

      isBreak = rows.drag_drop_update && rows.drag_drop_update(key, index);

      auto aspectRatio = image.size.y != 0.0f ? image.size.x / image.size.y : 1.0f;
      auto imageSize = ImVec2(cardSize.y, cardSize.y);
      if (imageSize.x / imageSize.y > aspectRatio)
        imageSize.x = imageSize.y * aspectRatio;
      else
        imageSize.y = imageSize.x / aspectRatio;
      ImGui::SetCursorPos(cursorPos);
      ImGui::ImageWithBg(resource::texture::id_get(*image.texture), imageSize, to_imvec2(image.uvMin),
                         to_imvec2(image.uvMax), ImVec4(), image.isValid ? CARD_TINT_VALID : CARD_TINT_INVALID);

      ImGui::SetCursorPos(
          ImVec2(cardSize.y + style.ItemSpacing.x, cardSize.y - cardSize.y / 2 - ImGui::GetTextLineHeight() / 2));
      if (isFocused) ImGui::PushFont(panel.resources.fonts[font::ITALICS].get(), font::SIZE);
      ImGui::TextUnformatted(label.c_str());
      if (isFocused) ImGui::PopFont();
    }
    ImGui::EndChild();

    row_scroll_update(panel.state, key, -1);
    if (panel.state.newId == key) panel.state.newId = -1;
    ImGui::PopID();
  }

  // A selectable list (or cards) of keys; `selection` and `focus` are read and written.
  void list_draw(Panel& panel, const std::vector<int>& keys, std::set<int>& selection, int& focus, const ListRows& rows)
  {
    panel.state.keys = keys;
    auto isCards = rows.image_get != nullptr;
    if (isCards) ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2());
    auto arrowKey = list_select_begin(panel, selection, focus, rows, keys);
    for (int i = 0; i < (int)keys.size(); ++i)
    {
      bool isBreak{};
      if (isCards)
        card_draw(panel, selection, focus, rows, keys[i], i, arrowKey, isBreak);
      else
        isBreak = row_draw(panel, selection, focus, rows, keys[i], i, arrowKey);
      if (isBreak) break;
    }
    if (isCards) ImGui::PopStyleVar();
    list_select_end(panel, selection, focus, rows, arrowKey);
  }

  void content_list_draw(Panel& panel, SelectionKind kind, const std::vector<int>& keys, const ListRows& rows)
  {
    auto& document = panel.document;
    auto selection = document.selected_ids_get(kind);
    auto focus = document.focused_id_get(kind);
    list_draw(panel, keys, selection, focus, rows);
    document.selected_ids_set(kind, selection);
    document.focused_id_set(kind, focus);
  }

  // The list in a bordered child with its context menu and shortcuts, then the footer buttons.
  void list_panel_draw(Panel& panel, Actions& actions, const Footer& footer, const std::function<void()>& list,
                       Actions* footerActions)
  {
    auto& [manager, settings, resources, dialog, clipboard, document, state] = panel;
    if (ImGui::BeginChild("##List Child", size_without_footer_get((int)footer.size()), ImGuiChildFlags_Borders))
    {
      list();
      actions_shortcuts_update(actions, manager);
    }
    ImGui::EndChild();

    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenOverlappedByWindow) &&
        ImGui::IsMouseReleased(ImGuiMouseButton_Right))
      ImGui::OpenPopup(CONTEXT_MENU_LABEL);
    Actions menu{};
    actions_undo_redo_add(menu, manager, document);
    menu.separator();
    menu.items.insert(menu.items.end(), actions.items.begin(), actions.items.end());
    actions_popup_draw(CONTEXT_MENU_LABEL, menu, settings);

    auto& buttons = footerActions ? *footerActions : actions;
    for (const auto& row : footer)
    {
      auto widgetSize = widget_size_with_row_get((int)row.size());
      bool isSameLine{};
      for (auto type : row)
        if (auto action = std::ranges::find(buttons.items, type, &Action::type); action != buttons.items.end())
          action_button_draw(*action, manager, settings, widgetSize, isSameLine);
    }
  }

  void tooltip_name_draw(Resources& resources, const std::string& name)
  {
    ImGui::PushFont(resources.fonts[font::BOLD].get(), font::SIZE);
    ImGui::TextUnformatted(name.c_str());
    ImGui::PopFont();
  }

  void tooltip_image_draw(const CardImage& image, const std::function<void()>& info_draw)
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

  std::filesystem::path asset_path_get(Document& document, const std::filesystem::path& path)
  {
    return path::backslash_replace(path::make_relative(path::backslash_handle(path), document.directory_get()));
  }

  void directory_open(Dialog& dialog, Document& document, const std::filesystem::path& path)
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

  PopupButton popup_buttons_draw(Manager& manager, const char* confirmLabel, bool isConfirmEnabled,
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
}
