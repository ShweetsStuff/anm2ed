#include "configure.h"

#include "imgui_.h"

using namespace anm2ed::types;

namespace anm2ed::imgui::wizard
{
  void Configure::reset(Settings& settings) { temporary = settings; }

  void Configure::update(Manager& manager, Settings& settings)
  {
    isSet = false;

    auto childSize = size_without_footer_get(2);

    if (ImGui::BeginTabBar("##Configure Tabs"))
    {
      if (ImGui::BeginTabItem(localize.get(LABEL_DISPLAY)))
      {
        if (ImGui::BeginChild("##Tab Child", childSize, true))
        {
          ImGui::SeparatorText(localize.get(LABEL_WINDOW_MENU));
          input_float_range(localize.get(LABEL_UI_SCALE), temporary.uiScale, 0.5f, 2.0f, 0.25f, 0.25f, "%.2f");
          ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_UI_SCALE));
          ImGui::Checkbox(localize.get(LABEL_VSYNC), &temporary.isVsync);
          ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_VSYNC));

          ImGui::SeparatorText(localize.get(LABEL_LOCALIZATION));
          ImGui::Combo(localize.get(LABEL_LANGUAGE), &temporary.language, LANGUAGE_STRINGS, LANGUAGE_COUNT);
          ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_LANGUAGE));

          ImGui::SeparatorText(localize.get(LABEL_THEME));

          for (int i = 0; i < theme::COUNT; i++)
          {
            ImGui::RadioButton(localize.get(theme::STRINGS[i]), &temporary.theme, i);
            ImGui::SameLine();
          }
        }
        ImGui::EndChild();

        ImGui::EndTabItem();
      }

      if (ImGui::BeginTabItem(localize.get(LABEL_FILE_MENU)))
      {
        if (ImGui::BeginChild("##Tab Child", childSize, true))
        {
          ImGui::SeparatorText(localize.get(LABEL_AUTOSAVE));

          ImGui::Checkbox(localize.get(BASIC_ENABLED), &temporary.fileIsAutosave);
          ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_AUTOSAVE_ENABLED));

          ImGui::BeginDisabled(!temporary.fileIsAutosave);
          input_int_range(localize.get(LABEL_TIME_MINUTES), temporary.fileAutosaveTime, 0, 10);
          ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_AUTOSAVE_INTERVAL));
          ImGui::EndDisabled();

          ImGui::SeparatorText(localize.get(LABEL_SNAPSHOTS));
          input_int_range(localize.get(LABEL_STACK_SIZE), temporary.fileSnapshotStackSize, 0, 100);
          ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_STACK_SIZE));

          ImGui::SeparatorText(localize.get(LABEL_OPTIONS));
          ImGui::Checkbox(localize.get(LABEL_OVERWRITE_WARNING), &temporary.fileIsWarnOverwrite);
          ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_OVERWRITE_WARNING));
        }
        ImGui::EndChild();

        ImGui::EndTabItem();
      }

      if (ImGui::BeginTabItem(localize.get(LABEL_INPUT)))
      {
        if (ImGui::BeginChild("##Tab Child", childSize, true))
        {
          ImGui::SeparatorText(localize.get(LABEL_KEYBOARD));

          input_float_range(localize.get(LABEL_REPEAT_DELAY), temporary.keyboardRepeatDelay, 0.05f, 1.0f, 0.05f, 0.05f,
                            "%.2f");
          ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_REPEAT_DELAY));

          input_float_range(localize.get(LABEL_REPEAT_RATE), temporary.keyboardRepeatRate, 0.005f, 1.0f, 0.005f, 0.005f,
                            "%.3f");
          ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_REPEAT_DELAY));

          ImGui::SeparatorText(localize.get(LABEL_ZOOM));

          input_float_range(localize.get(LABEL_ZOOM_STEP), temporary.inputZoomStep, 10.0f, 250.0f, 10.0f, 10.0f,
                            "%.0f%%");
          ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_ZOOM_STEP));

          ImGui::SeparatorText(localize.get(LABEL_TOOL));

          ImGui::Checkbox(localize.get(LABEL_MOVE_TOOL_SNAP), &temporary.inputIsMoveToolSnapToMouse);
          ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_MOVE_TOOL_SNAP));
        }
        ImGui::EndChild();

        ImGui::EndTabItem();
      }

      if (ImGui::BeginTabItem(localize.get(LABEL_SHORTCUTS_TAB)))
      {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2());

        if (ImGui::BeginChild("##Tab Child", childSize, true))
        {
          if (ImGui::BeginTable(localize.get(LABEL_SHORTCUTS_TAB), 2,
                                ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY))
          {
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableSetupColumn(localize.get(LABEL_SHORTCUT_COLUMN));
            ImGui::TableSetupColumn(localize.get(LABEL_VALUE_COLUMN));
            ImGui::TableHeadersRow();

            for (int i = 0; i < SHORTCUT_COUNT; ++i)
            {
              bool isSelected = selectedShortcut == i;

              ShortcutMember member = SHORTCUT_MEMBERS[i];
              std::string* settingString = &(temporary.*member);
              std::string chordString = isSelected ? "" : *settingString;

              ImGui::PushID(i);
              ImGui::TableNextRow();
              ImGui::TableSetColumnIndex(0);
              ImGui::TextUnformatted(localize.get(::anm2ed::SHORTCUT_STRING_TYPES[i]));
              ImGui::TableSetColumnIndex(1);

              if (ImGui::Selectable(chordString.c_str(), isSelected)) selectedShortcut = i;

              ImGui::PopID();

              if (isSelected)
              {
                ImGuiKeyChord chord{ImGuiKey_None};

                if (ImGui::IsKeyDown(ImGuiMod_Ctrl)) chord |= ImGuiMod_Ctrl;
                if (ImGui::IsKeyDown(ImGuiMod_Shift)) chord |= ImGuiMod_Shift;
                if (ImGui::IsKeyDown(ImGuiMod_Alt)) chord |= ImGuiMod_Alt;
                if (ImGui::IsKeyDown(ImGuiMod_Super)) chord |= ImGuiMod_Super;

                for (const auto& entry : KEY_MAP)
                {
                  auto key = entry.second;
                  if (ImGui::IsKeyPressed(key))
                  {
                    chord |= key;
                    *settingString = chord_to_string(chord);
                    selectedShortcut = -1;
                    break;
                  }
                }
              }
            }
            ImGui::EndTable();
          }
          ImGui::EndChild();

          ImGui::PopStyleVar();

          ImGui::EndTabItem();
        }
      }

      ImGui::EndTabBar();
    }

    auto widgetSize = widget_size_with_row_get(3);

    if (ImGui::Button(localize.get(BASIC_SAVE), widgetSize))
    {
      settings = temporary;

      ImGui::GetIO().KeyRepeatDelay = settings.keyboardRepeatDelay;
      ImGui::GetIO().KeyRepeatRate = settings.keyboardRepeatRate;
      ImGui::GetStyle().FontScaleMain = settings.uiScale;
      SnapshotStack::max_size_set(settings.fileSnapshotStackSize);
      imgui::theme_set((theme::Type)settings.theme);
      localize.language = (Language)settings.language;
      manager.chords_set(settings);

      for (auto& document : manager.documents)
        document.snapshots.apply_limit();

      isSet = true;
    }
    ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_SETTINGS_SAVE));

    ImGui::SameLine();

    if (ImGui::Button(localize.get(LABEL_USE_DEFAULT_SETTINGS), widgetSize)) temporary = Settings();
    ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_USE_DEFAULT_SETTINGS));

    ImGui::SameLine();

    if (ImGui::Button(localize.get(LABEL_CLOSE), widgetSize)) isSet = true;
    ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_CLOSE_SETTINGS));
  }
}