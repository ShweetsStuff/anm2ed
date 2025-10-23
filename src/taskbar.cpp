#include "taskbar.h"

#include <imgui/imgui.h>
#include <ranges>

#include "imgui.h"

using namespace anm2ed::settings;
using namespace anm2ed::dialog;
using namespace anm2ed::document_manager;
using namespace anm2ed::types;

namespace anm2ed::taskbar
{
  void Taskbar::update(Settings& settings, Dialog& dialog, DocumentManager& manager, bool& isQuit)
  {
    auto document = manager.get();
    auto animation = document ? document->animation_get() : nullptr;

    if (ImGui::BeginMainMenuBar())
    {
      height = ImGui::GetWindowSize().y;

      if (ImGui::BeginMenu("File"))
      {
        if (ImGui::MenuItem("New", settings.shortcutNew.c_str())) dialog.anm2_new();

        if (ImGui::MenuItem("Open", settings.shortcutOpen.c_str())) dialog.anm2_open();

        ImGui::BeginDisabled(!document);
        {
          if (ImGui::MenuItem("Save", settings.shortcutSave.c_str())) manager.save();
          if (ImGui::MenuItem("Save As", settings.shortcutSaveAs.c_str())) dialog.anm2_save();
          if (ImGui::MenuItem("Explore XML Location")) dialog.file_explorer_open(document->directory_get());
        }
        ImGui::EndDisabled();

        ImGui::Separator();
        if (ImGui::MenuItem("Exit", settings.shortcutExit.c_str())) isQuit = true;
        ImGui::EndMenu();
      }
      if (dialog.is_selected_file(dialog::ANM2_NEW))
      {
        manager.new_(dialog.path);
        dialog.reset();
      }

      if (dialog.is_selected_file(dialog::ANM2_OPEN))
      {
        manager.open(dialog.path);
        dialog.reset();
      }

      if (dialog.is_selected_file(dialog::ANM2_SAVE))
      {
        manager.save(dialog.path);
        dialog.reset();
      }

      if (ImGui::BeginMenu("Wizard"))
      {
        ImGui::BeginDisabled(!animation);
        {
          ImGui::MenuItem("Generate Animation From Grid");
          ImGui::MenuItem("Change All Frame Properties");
        }
        ImGui::EndDisabled();
        ImGui::EndMenu();
      }

      if (ImGui::BeginMenu("Playback"))
      {
        ImGui::MenuItem("Always Loop", nullptr, &settings.playbackIsLoop);
        ImGui::SetItemTooltip("%s", "Animations will always loop during playback, even if looping isn't set.");

        ImGui::MenuItem("Clamp Playhead", nullptr, &settings.playbackIsClampPlayhead);
        ImGui::SetItemTooltip("%s", "The playhead will always clamp to the animation's length.");

        ImGui::EndMenu();
      }

      if (ImGui::BeginMenu("Window"))
      {
        for (auto [i, member] : std::views::enumerate(WINDOW_MEMBERS))
          ImGui::MenuItem(WINDOW_STRINGS[i], nullptr, &(settings.*member));

        ImGui::EndMenu();
      }

      if (ImGui::BeginMenu("Settings"))
      {
        if (ImGui::MenuItem("Configure")) configurePopup.open();

        ImGui::EndMenu();
      }

      if (ImGui::BeginMenu("Help"))
      {
        if (ImGui::MenuItem("About")) aboutPopup.open();
        ImGui::EndMenu();
      }

      ImGui::EndMainMenuBar();
    }

    configurePopup.trigger();

    if (ImGui::BeginPopupModal(configurePopup.label, &configurePopup.isOpen, ImGuiWindowFlags_NoResize))
    {
      auto childSize = imgui::size_without_footer_get(2);

      if (ImGui::BeginTabBar("##Configure Tabs"))
      {
        if (ImGui::BeginTabItem("View"))
        {
          if (ImGui::BeginChild("##Tab Child", childSize, true))
          {
            ImGui::InputFloat("Zoom Step", &editSettings.viewZoomStep, 10.0f, 10.0f, "%.2f");
            ImGui::SetItemTooltip("%s", "When zooming in/out with mouse or shortcut, this value will be used.");
            editSettings.viewZoomStep = glm::clamp(editSettings.viewZoomStep, 1.0f, 250.0f);
          }
          ImGui::EndChild();
          ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Video"))
        {
          if (ImGui::BeginChild("##Tab Child", childSize, true))
          {
            ImGui::InputFloat("Display Scale", &editSettings.displayScale, 0.25f, 0.25f, "%.2f");
            ImGui::SetItemTooltip("%s", "Change the scale of the display.");
            editSettings.displayScale = glm::clamp(editSettings.displayScale, 0.5f, 2.0f);

            ImGui::Checkbox("Vsync", &editSettings.isVsync);
            ImGui::SetItemTooltip("%s",
                                  "Toggle vertical sync; synchronizes program update rate with monitor refresh rate.");
          }
          ImGui::EndChild();

          ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Shortcuts"))
        {

          ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2());

          if (ImGui::BeginChild("##Tab Child", childSize, true))
          {

            if (ImGui::BeginTable("Shortcuts", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY))
            {
              ImGui::TableSetupScrollFreeze(0, 1);
              ImGui::TableSetupColumn("Shortcut");
              ImGui::TableSetupColumn("Value");
              ImGui::TableHeadersRow();

              for (int i = 0; i < SHORTCUT_COUNT; ++i)
              {
                bool isSelected = selectedShortcut == i;

                ShortcutMember member = SHORTCUT_MEMBERS[i];
                std::string* settingString = &(editSettings.*member);
                std::string chordString = isSelected ? "" : *settingString;

                ImGui::PushID(i);
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(SHORTCUT_STRINGS[i]);
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

                  for (auto& key : imgui::KEY_MAP | std::views::values)
                  {
                    if (ImGui::IsKeyPressed(key))
                    {
                      chord |= key;
                      *settingString = imgui::chord_to_string(chord);
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

      auto widgetSize = imgui::widget_size_with_row_get(3);

      if (ImGui::Button("Save", widgetSize))
      {
        settings = editSettings;
        configurePopup.close();
      }
      ImGui::SetItemTooltip("Use the configured settings.");

      ImGui::SameLine();

      if (ImGui::Button("Use Default Settings", widgetSize)) editSettings = Settings();
      ImGui::SetItemTooltip("Reset the settings to their defaults.");

      ImGui::SameLine();

      if (ImGui::Button("Close", widgetSize)) ImGui::CloseCurrentPopup();
      ImGui::SetItemTooltip("Close without updating settings.");

      ImGui::EndPopup();
    }

    aboutPopup.trigger();

    if (ImGui::BeginPopupModal(aboutPopup.label, &aboutPopup.isOpen, ImGuiWindowFlags_NoResize))
    {
      if (ImGui::Button("Close")) aboutPopup.close();
      ImGui::EndPopup();
    }

    if (ImGui::Shortcut(imgui::string_to_chord(settings.shortcutNew), ImGuiInputFlags_RouteGlobal)) dialog.anm2_new();
    if (ImGui::Shortcut(imgui::string_to_chord(settings.shortcutOpen), ImGuiInputFlags_RouteGlobal)) dialog.anm2_open();
    if (ImGui::Shortcut(imgui::string_to_chord(settings.shortcutSave), ImGuiInputFlags_RouteGlobal)) manager.save();
    if (ImGui::Shortcut(imgui::string_to_chord(settings.shortcutSaveAs), ImGuiInputFlags_RouteGlobal))
      dialog.anm2_save();
    if (ImGui::Shortcut(imgui::string_to_chord(settings.shortcutExit), ImGuiInputFlags_RouteGlobal)) isQuit = true;
  }
}
