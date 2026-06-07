#include "taskbar.hpp"

#include <filesystem>
#include <format>
#include <ranges>
#include <vector>

#include <imgui/imgui.h>

#include "document.hpp"
#include "log.hpp"
#include "path.hpp"
#include "strings.hpp"
#include "toast.hpp"
#include "types.hpp"
#include "util/imgui/layout.hpp"
#include "util/imgui/shortcut.hpp"

using namespace anm2ed::resource;
using namespace anm2ed::types;
using namespace anm2ed::util;
using namespace glm;

namespace anm2ed::imgui
{
  Options save_options_get(Settings& settings)
  {
    Flags flags{};
    if (settings.fileIsSerializeGroups) flags |= SERIALIZE_GROUPS;
    if (settings.fileIsSerializeRegions) flags |= SERIALIZE_REGIONS;
    if (settings.fileIsSerializeSounds) flags |= SERIALIZE_SOUNDS;
    if (settings.fileIsKeepRedundantFrameRegionValues) flags |= SERIALIZE_REDUNDANT_FRAME_REGION_VALUES;
    if (settings.fileIsBakeSpecialInterpolatedFrames) flags |= SERIALIZE_BAKE_SPECIAL_INTERPOLATED_FRAMES;
    return {.flags = flags};
  }

  bool Taskbar::save_execute(Manager& manager, Settings& settings, const PendingSave& request)
  {
    return manager.save(request.index, request.path, save_options_get(settings));
  }

  void Taskbar::save_enqueue(Manager& manager, Settings& settings, const PendingSave& request)
  {
    auto index = request.index;
    auto path = request.path;
    auto options = save_options_get(settings);

    manager.command_push({.runManager =
                              [=](Manager& manager)
                              { manager.save(index, path, options); }});
  }

  bool Taskbar::save_request(Manager& manager, Settings& settings, int index, const std::filesystem::path& path,
                             bool isQueued)
  {
    auto* document = manager.get(index);
    if (!document) return false;

    PendingSave request{.index = index, .path = path};
    if (isQueued)
    {
      save_enqueue(manager, settings, request);
      return true;
    }
    return save_execute(manager, settings, request);
  }

  bool Taskbar::save_manual(Manager& manager, Settings& settings, int index, const std::filesystem::path& path)
  {
    return save_request(manager, settings, index, path);
  }

  void Taskbar::update(Manager& manager, Settings& settings, Resources& resources, Dialog& dialog, bool& isQuitting)
  {
    auto document = manager.get();
    auto itemType = document ? (ItemType)document->reference.itemType : ItemType::NONE;
    auto animation =
        document ? document->anm2.element_get(ElementType::ANIMATION, document->reference.animationIndex) : nullptr;
    auto frames = document ? &document->frames : nullptr;
    auto layerReferences = document ? document->layer_references_get() : std::vector<Reference>{};
    bool isGenerateAnimationFromGridAvailable = !layerReferences.empty();
    bool hasRegions = false;
    if (document)
    {
      if (auto spritesheets = document->anm2.element_get(ElementType::SPRITESHEETS))
      {
        for (auto& spritesheet : spritesheets->children)
          for (auto& child : spritesheet.children)
            if (spritesheet.type == ElementType::SPRITESHEET && child.type == ElementType::REGION)
            {
              hasRegions = true;
              break;
            }
      }
    }

    if (ImGui::BeginMainMenuBar())
    {
      height = ImGui::GetWindowSize().y;

      if (ImGui::BeginMenu(localize.get(LABEL_FILE_MENU)))
      {
        if (ImGui::MenuItem(localize.get(BASIC_NEW), settings.shortcutNew.c_str()))
          dialog.file_save(Dialog::ANM2_CREATE);
        if (ImGui::MenuItem(localize.get(BASIC_OPEN), settings.shortcutOpen.c_str()))
          dialog.file_open(Dialog::ANM2_OPEN, true);

        auto recentFiles = manager.recent_files_ordered();
        if (ImGui::BeginMenu(localize.get(LABEL_OPEN_RECENT), !recentFiles.empty()))
        {
          for (auto [i, file] : std::views::enumerate(recentFiles))
          {
            ImGui::PushID((int)i);
            auto fileNameUtf8 = path::to_utf8(file.filename());
            auto filePathUtf8 = path::to_utf8(file);
            auto label = std::format(FILE_LABEL_FORMAT, fileNameUtf8, filePathUtf8);
            if (ImGui::MenuItem(label.c_str()))
              manager.command_push({.runManager = [file](Manager& manager) { manager.open(file); }});
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
            save_request(manager, settings, manager.selected, document->path, true);
        }

        if (ImGui::MenuItem(localize.get(LABEL_SAVE_AS), settings.shortcutSaveAs.c_str(), false, document))
          dialog.file_save(Dialog::ANM2_SAVE);
        if (ImGui::MenuItem(localize.get(LABEL_EXPLORE_XML_LOCATION), nullptr, false, document))
          dialog.file_explorer_open(document->directory_get());

        ImGui::Separator();
        if (ImGui::MenuItem(localize.get(LABEL_EXIT), settings.shortcutExit.c_str())) isQuitting = true;
        ImGui::EndMenu();
      }
      if (dialog.is_selected(Dialog::ANM2_CREATE))
      {
        auto path = dialog.path;
        manager.command_push({.runManager = [path](Manager& manager) { manager.new_(path); }});
        dialog.reset();
      }

      if (dialog.is_selected(Dialog::ANM2_OPEN))
      {
        auto paths = dialog.paths;
        manager.command_push({.runManager =
                                  [paths](Manager& manager)
                                  {
                                    for (auto& path : paths) manager.open(path);
                                  }});
        dialog.reset();
      }

      if (dialog.is_selected(Dialog::ANM2_SAVE))
      {
        save_request(manager, settings, manager.selected, dialog.path, true);
        dialog.reset();
      }

      if (ImGui::BeginMenu(localize.get(LABEL_WIZARD_MENU)))
      {
        if (ImGui::MenuItem(localize.get(LABEL_TASKBAR_GENERATE_ANIMATION_FROM_GRID), nullptr, false,
                            isGenerateAnimationFromGridAvailable))
          generatePopup.open();
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_WIZARD_GENERATE_ANIMATION_FROM_GRID));

        bool isChangeAllFramesAvailable = frames && !frames->selection.empty() && itemType != ItemType::TRIGGER;
        bool isChangeAllAnimationsAvailable = document && !document->animation.selection.empty();
        if (ImGui::MenuItem(localize.get(LABEL_CHANGE_ALL_FRAME_PROPERTIES), nullptr, false,
                            isChangeAllFramesAvailable || isChangeAllAnimationsAvailable))
          changePopup.open();
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_WIZARD_CHANGE_ALL_FRAME_PROPERTIES));

        if (ImGui::MenuItem(localize.get(LABEL_SCAN_AND_SET_REGIONS), nullptr, false, document && hasRegions))
        {
          manager.command_push({manager.selected,
                                [](Manager&, Document& document)
                                {
                                  document.snapshot(localize.get(EDIT_SCAN_AND_SET_REGIONS));
                                  document.scan_and_set_regions();
                                  document.change(Document::FRAMES);
                                }});
          toasts.push(localize.get(TOAST_SCAN_AND_SET_REGIONS));
          logger.info(localize.get(TOAST_SCAN_AND_SET_REGIONS, anm2ed::ENGLISH));
        }
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_WIZARD_SCAN_AND_SET_REGIONS));

        ImGui::Separator();

        if (ImGui::MenuItem(localize.get(LABEL_TASKBAR_RENDER_ANIMATION), nullptr, false,
                            animation && manager.isAbleToRecord))
          renderPopup.open();
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_WIZARD_RENDER_ANIMATION));

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
        generateAnimationFromGrid.update(manager, *document, resources, settings);
        if (generateAnimationFromGrid.isEnd) generatePopup.close();
      }
      ImGui::EndPopup();
    }

    changePopup.trigger();

    if (ImGui::BeginPopupModal(changePopup.label(), &changePopup.isOpen, ImGuiWindowFlags_NoResize))
    {
      if (document)
      {
        changeAllFrameProperties.update(manager, *document, settings, true);
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
        save_request(manager, settings, manager.selected, {}, true);
        overwritePopup.close();
      }

      ImGui::SameLine();

      if (ImGui::Button(localize.get(BASIC_NO), widgetSize)) overwritePopup.close();

      ImGui::EndPopup();
    }

    aboutPopup.end();

    if (shortcut(manager.chords[SHORTCUT_NEW], shortcut::GLOBAL)) dialog.file_save(Dialog::ANM2_CREATE);
    if (shortcut(manager.chords[SHORTCUT_OPEN], shortcut::GLOBAL)) dialog.file_open(Dialog::ANM2_OPEN, true);
    if (shortcut(manager.chords[SHORTCUT_SAVE], shortcut::GLOBAL))
    {
      if (settings.fileIsWarnOverwrite)
        overwritePopup.open();
      else
        save_request(manager, settings, manager.selected, {}, true);
    }
    if (shortcut(manager.chords[SHORTCUT_SAVE_AS], shortcut::GLOBAL)) dialog.file_save(Dialog::ANM2_SAVE);
    if (shortcut(manager.chords[SHORTCUT_EXIT], shortcut::GLOBAL)) isQuitting = true;
  }
}
