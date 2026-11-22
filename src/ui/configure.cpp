#include "ui/configure.h"

#include "math_util.h"
#include "sdl.h"
#include "ui/input.h"
#include "ui/layout.h"
#include "ui/shortcut.h"
#include "ui/theme.h"

void Configure::reset(Settings& settings)
{
    temporary = settings;
}

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
                input_float_range(localize.get(LABEL_UI_SCALE), temporary.uiScale, UI_SCALE_MIN, UI_SCALE_MAX,
                                  UI_SCALE_STEP, UI_SCALE_STEP, "%.0f%%");
                tooltip_set(TOOLTIP_UI_SCALE);
                ImGui::Checkbox(localize.get(LABEL_VSYNC), &temporary.isVsync);
                tooltip_set(TOOLTIP_VSYNC);
                ImGui::SameLine();
                ImGui::Checkbox(localize.get(LABEL_DETACHABLE_WINDOWS), &temporary.isDetachableWindows);
                tooltip_set(TOOLTIP_DETACHABLE_WINDOWS);

                ImGui::SeparatorText(localize.get(LABEL_PLAYBACK_MENU));
                ImGui::RadioButton("30 Hz", &temporary.playbackTickRate, 30);
                tooltip_set(TOOLTIP_PLAYBACK_TICK_RATE);
                ImGui::SameLine();
                ImGui::RadioButton("60 Hz", &temporary.playbackTickRate, 60);
                tooltip_set(TOOLTIP_PLAYBACK_TICK_RATE);

                ImGui::SeparatorText(localize.get(LABEL_LOCALIZATION));
                ImGui::Combo(localize.get(LABEL_LANGUAGE), &temporary.language, LANGUAGE_STRINGS, LANGUAGE_COUNT);
                tooltip_set(TOOLTIP_LANGUAGE);

                ImGui::SeparatorText(localize.get(LABEL_THEME));

                for (int i = 0; i < THEME_COUNT; i++)
                {
                    ImGui::RadioButton(localize.get(THEME_STRINGS[i]), &temporary.theme, i);
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
                tooltip_set(TOOLTIP_AUTOSAVE_ENABLED);

                ImGui::SeparatorText(localize.get(LABEL_AUTO_RESOURCE_RELOAD));
                ImGui::PushID(LABEL_AUTO_RESOURCE_RELOAD);
                ImGui::Checkbox(localize.get(BASIC_ENABLED), &temporary.fileIsAutoReloadResources);
                tooltip_set(TOOLTIP_AUTO_RESOURCE_RELOAD);
                ImGui::PopID();

                ImGui::SeparatorText(localize.get(LABEL_SNAPSHOTS));
                input_int_range(localize.get(LABEL_STACK_SIZE), temporary.fileSnapshotStackSize, 0, 100);
                tooltip_set(TOOLTIP_STACK_SIZE);

                ImGui::SeparatorText(localize.get(LABEL_OPTIONS));
                ImGui::Checkbox(localize.get(LABEL_OVERWRITE_WARNING), &temporary.fileIsWarnOverwrite);
                tooltip_set(TOOLTIP_OVERWRITE_WARNING);
                ImGui::SameLine();
                ImGui::Checkbox(localize.get(LABEL_ISAAC_COMPATIBILITY_WARNING), &temporary.fileIsWarnIsaac);
                tooltip_set(TOOLTIP_ISAAC_COMPATIBILITY_WARNING);

                ImGui::SeparatorText(localize.get(LABEL_SERIALIZATION));
                ImGui::Checkbox(localize.get(LABEL_SAVE_AS_JSON), &temporary.fileIsJson);
                tooltip_set(TOOLTIP_SAVE_AS_JSON);
            }
            ImGui::EndChild();

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem(localize.get(LABEL_INPUT)))
        {
            if (ImGui::BeginChild("##Tab Child", childSize, true))
            {
                ImGui::SeparatorText(localize.get(LABEL_KEYBOARD));

                input_float_range(localize.get(LABEL_REPEAT_DELAY), temporary.keyboardRepeatDelay, 0.05f, 1.0f, 0.05f,
                                  0.05f, "%.2f");
                tooltip_set(TOOLTIP_REPEAT_DELAY);

                input_float_range(localize.get(LABEL_REPEAT_RATE), temporary.keyboardRepeatRate, 0.005f, 1.0f, 0.005f,
                                  0.005f, "%.3f");
                tooltip_set(TOOLTIP_REPEAT_DELAY);

                ImGui::SeparatorText(localize.get(LABEL_TOOL));

                ImGui::Checkbox(localize.get(LABEL_MOVE_TOOL_SNAP), &temporary.inputIsMoveToolSnapToMouse);
                tooltip_set(TOOLTIP_MOVE_TOOL_SNAP);
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
                        ImGui::TextUnformatted(localize.get(SHORTCUT_STRING_TYPES[i]));
                        ImGui::TableSetColumnIndex(1);

                        if (ImGui::Selectable(chordString.c_str(), isSelected))
                            selectedShortcut = i;

                        ImGui::PopID();

                        if (isSelected)
                        {
                            ImGuiKeyChord chord = ImGuiKey_None;

                            if (ImGui::IsKeyDown(ImGuiMod_Ctrl))
                                chord |= ImGuiMod_Ctrl;
                            if (ImGui::IsKeyDown(ImGuiMod_Shift))
                                chord |= ImGuiMod_Shift;
                            if (ImGui::IsKeyDown(ImGuiMod_Alt))
                                chord |= ImGuiMod_Alt;
                            if (ImGui::IsKeyDown(ImGuiMod_Super))
                                chord |= ImGuiMod_Super;

                            for (auto& entry : KEY_MAP)
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

    shortcut(manager.chords[SHORTCUT_CONFIRM]);
    if (ImGui::Button(localize.get(BASIC_SAVE), widgetSize))
    {
        auto settingsPath = preferences_directory_get() / "settings.ini";
        settings = temporary;

        ImGui::GetIO().KeyRepeatDelay = settings.keyboardRepeatDelay;
        ImGui::GetIO().KeyRepeatRate = settings.keyboardRepeatRate;
        ImGui::GetStyle().FontScaleMain = percent_to_unit(settings.uiScale);
        if (settings.isDetachableWindows)
            ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
        else
            ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_ViewportsEnable;
        SnapshotStack::max_size_set(settings.fileSnapshotStackSize);
        theme_set((Theme)settings.theme);
        localize.language = (Language)settings.language;
        manager.settings_apply(settings);

        for (auto& document : manager.documents)
            document.snapshots.apply_limit();

        settings.save(settingsPath, ImGui::SaveIniSettingsToMemory(nullptr));

        isSet = true;
    }
    tooltip_set(TOOLTIP_SETTINGS_SAVE);

    ImGui::SameLine();

    if (ImGui::Button(localize.get(LABEL_USE_DEFAULT_SETTINGS), widgetSize))
        temporary = Settings();
    tooltip_set(TOOLTIP_USE_DEFAULT_SETTINGS);

    ImGui::SameLine();

    shortcut(manager.chords[SHORTCUT_CLOSE]);
    if (ImGui::Button(localize.get(LABEL_CLOSE), widgetSize))
        isSet = true;
    tooltip_set(TOOLTIP_CLOSE_SETTINGS);
}
