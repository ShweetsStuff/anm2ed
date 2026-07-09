#include "actions.hpp"

#include <utility>

#include "util/imgui/shortcut.hpp"
#include "util/imgui/tooltip.hpp"

namespace anm2ed::imgui
{
  bool is_action_shortcut_valid(int shortcutType) { return shortcutType >= 0 && shortcutType < SHORTCUT_COUNT; }

  std::string action_shortcut_text_get(const Action& action, Settings& settings)
  {
    if (!is_action_shortcut_valid(action.shortcut)) return {};
    return settings.*SHORTCUT_MEMBERS[action.shortcut];
  }

  ImGuiKeyChord action_chord_get(const Action& action, Manager& manager)
  {
    if (!is_action_shortcut_valid(action.shortcut)) return ImGuiKey_None;
    return manager.chords[action.shortcut];
  }

  bool is_action_enabled(const Action& action) { return !action.isEnabled || action.isEnabled(); }

  Action action_make(ActionType type, std::function<bool()> isEnabled, std::function<void()> run, StringType tooltip,
                     int shortcut)
  {
    auto info = ACTION_INFOS[type];
    return {.type = type,
            .label = info.label,
            .tooltip = tooltip,
            .shortcut = shortcut == ACTION_COUNT ? info.shortcut : shortcut,
            .isEnabled = std::move(isEnabled),
            .run = std::move(run)};
  }

  void Actions::add(Action action) { items.push_back(std::move(action)); }

  void Actions::add(ActionType type, std::function<bool()> isEnabled, std::function<void()> run, StringType tooltip,
                    int shortcut)
  {
    add(action_make(type, std::move(isEnabled), std::move(run), tooltip, shortcut));
  }

  void Actions::separator() { items.push_back({.isSeparator = true}); }

  void actions_undo_redo_add(Actions& actions, Manager& manager, Document& document)
  {
    actions.add(ACTION_UNDO, [&document]() { return document.is_able_to_undo(); },
                [&manager]()
                { manager.command_push({manager.selected, [](Manager&, Document& document) { document.undo(); }}); });
    actions.add(ACTION_REDO, [&document]() { return document.is_able_to_redo(); },
                [&manager]()
                { manager.command_push({manager.selected, [](Manager&, Document& document) { document.redo(); }}); });
  }

  void actions_menu_draw(Actions& actions, Settings& settings)
  {
    bool isPreviousSeparator{};
    for (auto& action : actions.items)
    {
      if (action.isSeparator)
      {
        if (!isPreviousSeparator) ImGui::Separator();
        isPreviousSeparator = true;
        continue;
      }

      auto shortcutText = action_shortcut_text_get(action, settings);
      auto shortcutLabel = shortcutText.empty() ? nullptr : shortcutText.c_str();
      if (ImGui::MenuItem(localize.get(action.label), shortcutLabel, false, is_action_enabled(action)) && action.run)
        action.run();
      if (action.tooltip != STRING_UNDEFINED && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        ImGui::SetItemTooltip("%s", localize.get(action.tooltip));
      isPreviousSeparator = false;
    }
  }

  bool actions_context_window_draw(const char* label, Actions& actions, Settings& settings, ImGuiPopupFlags flags)
  {
    auto mouseButton = flags & ImGuiPopupFlags_MouseButtonMask_;
    if (ImGui::IsMouseReleased(mouseButton) && ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup))
      if (!(flags & ImGuiPopupFlags_NoOpenOverItems) || !ImGui::IsAnyItemHovered()) ImGui::OpenPopup(label, flags);

    if (!ImGui::BeginPopup(label, ImGuiWindowFlags_NoMove)) return false;
    actions_menu_draw(actions, settings);
    ImGui::EndPopup();
    return true;
  }

  bool actions_popup_draw(const char* label, Actions& actions, Settings& settings)
  {
    if (!ImGui::BeginPopup(label, ImGuiWindowFlags_NoMove)) return false;
    actions_menu_draw(actions, settings);
    ImGui::EndPopup();
    return true;
  }

  void actions_shortcuts_update(Actions& actions, Manager& manager, types::shortcut::Type shortcutType)
  {
    for (auto& action : actions.items)
    {
      if (action.isSeparator || !action.run || !is_action_enabled(action)) continue;
      auto chord = action_chord_get(action, manager);
      if (!shortcut(chord, shortcutType)) continue;
      action.run();
      break;
    }
  }

  void action_button_draw(Action& action, Manager& manager, Settings& settings, ImVec2 widgetSize, bool& isSameLine)
  {
    if (isSameLine) ImGui::SameLine();
    ImGui::BeginDisabled(!is_action_enabled(action));
    shortcut(action_chord_get(action, manager));
    if (ImGui::Button(localize.get(action.label), widgetSize) && action.run) action.run();
    ImGui::EndDisabled();
    if (action.tooltip != STRING_UNDEFINED)
    {
      auto shortcutText = action_shortcut_text_get(action, settings);
      if (shortcutText.empty())
        ImGui::SetItemTooltip("%s", localize.get(action.tooltip));
      else
        set_item_tooltip_shortcut(localize.get(action.tooltip), shortcutText);
    }
    isSameLine = true;
  }
}
