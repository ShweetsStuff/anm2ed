#include "taskbar.h"

#include <filesystem>
#include <format>
#include <ranges>
#include <vector>

#include <imgui/imgui.h>

#include "path_.h"
#include "strings.h"
#include "types.h"

using namespace anm2ed::resource;
using namespace anm2ed::types;
using namespace anm2ed::util;
using namespace glm;

namespace anm2ed::imgui
{
  void Taskbar::update(Manager& manager, Settings& settings, Resources& resources, Dialog& dialog, bool& isQuitting)
  {
    auto document = manager.get();
    auto animation = document ? document->animation_get() : nullptr;
    auto item = document ? document->item_get() : nullptr;
    auto frames = document ? &document->frames : nullptr;

    if (ImGui::BeginMainMenuBar())
    {
      height = ImGui::GetWindowSize().y;

      if (ImGui::BeginMenu(localize.get(LABEL_FILE_MENU)))
      {
        if (ImGui::MenuItem(localize.get(BASIC_NEW), settings.shortcutNew.c_str())) dialog.file_save(Dialog::ANM2_NEW);
        if (ImGui::MenuItem(localize.get(BASIC_OPEN), settings.shortcutOpen.c_str()))
          dialog.file_open(Dialog::ANM2_OPEN);

        auto recentFiles = manager.recent_files_ordered();
        if (ImGui::BeginMenu(localize.get(LABEL_OPEN_RECENT), !recentFiles.empty()))
        {
          for (auto [i, file] : std::views::enumerate(recentFiles))
          {
            ImGui::PushID((int)i);
            auto fileNameUtf8 = path::to_utf8(file.filename());
            auto filePathUtf8 = path::to_utf8(file);
            auto label = std::format(FILE_LABEL_FORMAT, fileNameUtf8, filePathUtf8);
            if (ImGui::MenuItem(label.c_str())) manager.open(file);
            ImGui::PopID();
          }

          if (!recentFiles.empty())
            if (ImGui::MenuItem(localize.get(LABEL_CLEAR_LIST))) manager.recent_files_clear();

          ImGui::EndMenu();
        }

        if (ImGui::MenuItem(localize.get(BASIC_SAVE), settings.shortcutSave.c_str(), false, document))
        {
          if (settings.fileIsWarnOverwrite)
            overwritePopup.open();
          else
            manager.save(document->path);
        }

        if (ImGui::MenuItem(localize.get(LABEL_SAVE_AS), settings.shortcutSaveAs.c_str(), false, document))
          dialog.file_save(Dialog::ANM2_SAVE);
        if (ImGui::MenuItem(localize.get(LABEL_EXPLORE_XML_LOCATION), nullptr, false, document))
          dialog.file_explorer_open(document->directory_get());

        ImGui::Separator();
        if (ImGui::MenuItem(localize.get(LABEL_EXIT), settings.shortcutExit.c_str())) isQuitting = true;
        ImGui::EndMenu();
      }
      if (dialog.is_selected(Dialog::ANM2_NEW))
      {
        manager.new_(dialog.path);
        dialog.reset();
      }

      if (dialog.is_selected(Dialog::ANM2_OPEN))
      {
        manager.open(dialog.path);
        dialog.reset();
      }

      if (dialog.is_selected(Dialog::ANM2_SAVE))
      {
        manager.save(dialog.path);
        dialog.reset();
      }

      if (ImGui::BeginMenu(localize.get(LABEL_WIZARD_MENU)))
      {
        if (ImGui::MenuItem(localize.get(LABEL_TASKBAR_GENERATE_ANIMATION_FROM_GRID), nullptr, false,
                            item && document->reference.itemType == anm2::LAYER))
          generatePopup.open();

        if (ImGui::MenuItem(localize.get(LABEL_CHANGE_ALL_FRAME_PROPERTIES), nullptr, false,
                            frames && !frames->selection.empty() && document->reference.itemType != anm2::TRIGGER))
          changePopup.open();

        ImGui::Separator();

        if (ImGui::MenuItem(localize.get(LABEL_TASKBAR_RENDER_ANIMATION), nullptr, false,
                            animation && manager.isAbleToRecord))
          renderPopup.open();
        ImGui::EndMenu();
      }

      if (ImGui::BeginMenu(localize.get(LABEL_PLAYBACK_MENU)))
      {
        ImGui::MenuItem(localize.get(LABEL_PLAYBACK_ALWAYS_LOOP), nullptr, &settings.playbackIsLoop);
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_PLAYBACK_ALWAYS_LOOP));

        ImGui::MenuItem(localize.get(LABEL_CLAMP), nullptr, &settings.playbackIsClamp);
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_PLAYBACK_CLAMP));

        ImGui::EndMenu();
      }

      if (ImGui::BeginMenu(localize.get(LABEL_WINDOW_MENU)))
      {
        for (int i = 0; i < WINDOW_COUNT; i++)
        {
          auto member = WINDOW_MEMBERS[i];
          ImGui::MenuItem(localize.get(::anm2ed::WINDOW_STRING_TYPES[i]), nullptr, &(settings.*member));
        }

        ImGui::EndMenu();
      }

      if (ImGui::BeginMenu(localize.get(LABEL_SETTINGS_MENU)))
      {
        if (ImGui::MenuItem(localize.get(LABEL_TASKBAR_CONFIGURE))) configurePopup.open();
        ImGui::EndMenu();
      }

      if (ImGui::BeginMenu(localize.get(LABEL_HELP_MENU)))
      {
        if (ImGui::MenuItem(localize.get(LABEL_TASKBAR_ABOUT))) aboutPopup.open();
        ImGui::EndMenu();
      }

      ImGui::EndMainMenuBar();
    }

    generatePopup.trigger();

    if (ImGui::BeginPopupModal(generatePopup.label(), &generatePopup.isOpen, ImGuiWindowFlags_NoResize))
    {
      if (document)
      {
        generateAnimationFromGrid.update(*document, resources, settings);
        if (generateAnimationFromGrid.isEnd) generatePopup.close();
      }
      ImGui::EndPopup();
    }

    changePopup.trigger();

    if (ImGui::BeginPopupModal(changePopup.label(), &changePopup.isOpen, ImGuiWindowFlags_NoResize))
    {
      if (document)
      {
        changeAllFrameProperties.update(*document, settings);
        if (changeAllFrameProperties.isChanged) changePopup.close();
      }
      ImGui::EndPopup();
    }

    configurePopup.trigger();
    if (ImGui::BeginPopupModal(configurePopup.label(), &configurePopup.isOpen, ImGuiWindowFlags_NoResize))
    {
      if (configurePopup.isJustOpened) configure.reset(settings);
      configure.update(manager, settings);
      if (configure.isSet) configurePopup.close();
      ImGui::EndPopup();
    }
    configurePopup.end();

    renderPopup.trigger();
    if (ImGui::BeginPopupModal(renderPopup.label(), &renderPopup.isOpen, ImGuiWindowFlags_NoResize))
    {
      if (document)
      {
        if (renderPopup.isJustOpened) renderAnimation.reset(manager, *document, settings);
        renderAnimation.update(manager, *document, resources, settings, dialog);
      }
      if (renderAnimation.isEnd) renderPopup.close();
      ImGui::EndPopup();
    }
    renderPopup.end();

    aboutPopup.trigger();
    if (ImGui::BeginPopupModal(aboutPopup.label(), &aboutPopup.isOpen, ImGuiWindowFlags_NoResize))
    {
      if (aboutPopup.isJustOpened) about.reset(resources);
      about.update(resources);
      ImGui::EndPopup();
    }

    if (auto* music = resources.music_track_if_loaded())
      if (music->is_playing() && !aboutPopup.isOpen) music->stop();

    overwritePopup.trigger();

    if (ImGui::BeginPopupModal(overwritePopup.label(), &overwritePopup.isOpen, ImGuiWindowFlags_NoResize))
    {
      ImGui::TextUnformatted(localize.get(LABEL_OVERWRITE_CONFIRMATION));

      auto widgetSize = widget_size_with_row_get(2);

      if (ImGui::Button(localize.get(BASIC_YES), widgetSize))
      {
        manager.save();
        overwritePopup.close();
      }

      ImGui::SameLine();

      if (ImGui::Button(localize.get(BASIC_NO), widgetSize)) overwritePopup.close();

      ImGui::EndPopup();
    }

    aboutPopup.end();

    if (shortcut(manager.chords[SHORTCUT_NEW], shortcut::GLOBAL)) dialog.file_save(Dialog::ANM2_NEW);
    if (shortcut(manager.chords[SHORTCUT_OPEN], shortcut::GLOBAL)) dialog.file_open(Dialog::ANM2_OPEN);
    if (shortcut(manager.chords[SHORTCUT_SAVE], shortcut::GLOBAL))
    {
      if (settings.fileIsWarnOverwrite)
        overwritePopup.open();
      else
        manager.save();
    }
    if (shortcut(manager.chords[SHORTCUT_SAVE_AS], shortcut::GLOBAL)) dialog.file_save(Dialog::ANM2_SAVE);
    if (shortcut(manager.chords[SHORTCUT_EXIT], shortcut::GLOBAL)) isQuitting = true;
  }
}
