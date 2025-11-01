#include "taskbar.h"

#include <imgui/imgui.h>
#include <ranges>

#include "math_.h"
#include "render.h"
#include "shader.h"
#include "types.h"

using namespace anm2ed::resource;
using namespace anm2ed::types;
using namespace anm2ed::canvas;
using namespace anm2ed::util;
using namespace glm;

namespace anm2ed::imgui
{
  Taskbar::Taskbar() : generate(vec2())
  {
  }

  void Taskbar::update(Manager& manager, Settings& settings, Resources& resources, Dialog& dialog, bool& isQuitting)
  {
    auto document = manager.get();

    if (ImGui::BeginMainMenuBar())
    {
      height = ImGui::GetWindowSize().y;

      if (ImGui::BeginMenu("File"))
      {
        if (ImGui::MenuItem("New", settings.shortcutNew.c_str())) dialog.file_open(dialog::ANM2_NEW);

        if (ImGui::MenuItem("Open", settings.shortcutOpen.c_str())) dialog.file_open(dialog::ANM2_NEW);

        if (manager.recentFiles.empty())
        {
          ImGui::BeginDisabled();
          ImGui::MenuItem("Open Recent");
          ImGui::EndDisabled();
        }
        else
        {
          if (ImGui::BeginMenu("Open Recent"))
          {
            for (auto [i, file] : std::views::enumerate(manager.recentFiles))
            {
              auto label = std::format(FILE_LABEL_FORMAT, file.filename().string(), file.string());

              ImGui::PushID(i);
              if (ImGui::MenuItem(label.c_str())) manager.open(file);
              ImGui::PopID();
            }

            if (!manager.recentFiles.empty())
              if (ImGui::MenuItem("Clear List")) manager.recent_files_clear();

            ImGui::EndMenu();
          }
        }

        ImGui::BeginDisabled(!document);
        {
          if (ImGui::MenuItem("Save", settings.shortcutSave.c_str())) manager.save();
          if (ImGui::MenuItem("Save As", settings.shortcutSaveAs.c_str())) dialog.file_save(dialog::ANM2_SAVE);
          if (ImGui::MenuItem("Explore XML Location")) dialog.file_explorer_open(document->directory_get());
        }
        ImGui::EndDisabled();

        ImGui::Separator();
        if (ImGui::MenuItem("Exit", settings.shortcutExit.c_str())) isQuitting = true;
        ImGui::EndMenu();
      }
      if (dialog.is_selected(dialog::ANM2_NEW))
      {
        manager.new_(dialog.path);
        dialog.reset();
      }

      if (dialog.is_selected(dialog::ANM2_OPEN))
      {
        manager.open(dialog.path);
        dialog.reset();
      }

      if (dialog.is_selected(dialog::ANM2_SAVE))
      {
        manager.save(dialog.path);
        dialog.reset();
      }

      if (ImGui::BeginMenu("Wizard"))
      {
        auto animation = document ? document->animation_get() : nullptr;
        auto item = document ? document->item_get() : nullptr;
        ImGui::BeginDisabled(!item || document->reference.itemType != anm2::LAYER);
        if (ImGui::MenuItem("Generate Animation From Grid")) generatePopup.open();
        if (ImGui::MenuItem("Change All Frame Properties")) changePopup.open();
        ImGui::EndDisabled();
        ImGui::Separator();
        ImGui::BeginDisabled(!animation);
        if (ImGui::MenuItem("Render Animation")) renderPopup.open();
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
        if (ImGui::MenuItem("Configure"))
        {
          editSettings = settings;
          configurePopup.open();
        }

        ImGui::EndMenu();
      }

      if (ImGui::BeginMenu("Help"))
      {
        if (ImGui::MenuItem("About")) aboutPopup.open();
        ImGui::EndMenu();
      }

      ImGui::EndMainMenuBar();
    }

    generatePopup.trigger();

    if (ImGui::BeginPopupModal(generatePopup.label, &generatePopup.isOpen, ImGuiWindowFlags_NoResize))
    {
      auto& startPosition = settings.generateStartPosition;
      auto& size = settings.generateSize;
      auto& pivot = settings.generatePivot;
      auto& rows = settings.generateRows;
      auto& columns = settings.generateColumns;
      auto& count = settings.generateCount;
      auto& delay = settings.generateDelay;
      auto& zoom = settings.generateZoom;
      auto& zoomStep = settings.viewZoomStep;

      auto childSize = ImVec2(imgui::row_widget_width_get(2), imgui::size_without_footer_get().y);

      if (ImGui::BeginChild("##Options Child", childSize, ImGuiChildFlags_Borders))
      {
        ImGui::InputInt2("Start Position", value_ptr(startPosition));
        ImGui::InputInt2("Frame Size", value_ptr(size));
        ImGui::InputInt2("Pivot", value_ptr(pivot));
        ImGui::InputInt("Rows", &rows, imgui::STEP, imgui::STEP_FAST);
        ImGui::InputInt("Columns", &columns, imgui::STEP, imgui::STEP_FAST);

        ImGui::InputInt("Count", &count, imgui::STEP, imgui::STEP_FAST);
        count = glm::min(count, rows * columns);

        ImGui::InputInt("Delay", &delay, imgui::STEP, imgui::STEP_FAST);
      }
      ImGui::EndChild();

      ImGui::SameLine();

      if (ImGui::BeginChild("##Preview Child", childSize, ImGuiChildFlags_Borders))
      {
        auto& backgroundColor = settings.previewBackgroundColor;
        auto& time = generateTime;
        auto& shaderTexture = resources.shaders[resource::shader::TEXTURE];

        auto previewSize = ImVec2(ImGui::GetContentRegionAvail().x, imgui::size_without_footer_get(2).y);

        generate.size_set(to_vec2(previewSize));
        generate.bind();
        generate.viewport_set();
        generate.clear(backgroundColor);

        if (document && document->reference.itemType == anm2::LAYER)
        {
          auto& texture = document->anm2.content
                              .spritesheets[document->anm2.content.layers[document->reference.itemID].spritesheetID]
                              .texture;

          auto index = std::clamp((int)(time * count - 1), 0, count - 1);
          auto row = index / columns;
          auto column = index % columns;
          auto crop = startPosition + ivec2(size.x * column, size.y * row);
          auto uvMin = (vec2(crop) + vec2(0.5f)) / vec2(texture.size);
          auto uvMax = (vec2(crop) + vec2(size) - vec2(0.5f)) / vec2(texture.size);

          mat4 transform = generate.transform_get(zoom) * math::quad_model_get(size, {}, pivot);

          generate.texture_render(shaderTexture, texture.id, transform, vec4(1.0f), {},
                                  math::uv_vertices_get(uvMin, uvMax).data());
        }
        generate.unbind();

        ImGui::Image(generate.texture, previewSize);

        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::SliderFloat("##Time", &time, 0.0f, 1.0f, "");

        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::InputFloat("##Zoom", &zoom, zoomStep, zoomStep, "%.0f%%");
        zoom = glm::clamp(zoom, ZOOM_MIN, ZOOM_MAX);
      }

      ImGui::EndChild();

      auto widgetSize = imgui::widget_size_with_row_get(2);

      if (ImGui::Button("Generate", widgetSize))
      {
        document->generate_animation_from_grid(startPosition, size, pivot, columns, count, delay);
        generatePopup.close();
      }

      ImGui::SameLine();

      if (ImGui::Button("Cancel", widgetSize)) generatePopup.close();

      ImGui::EndPopup();
    }

    changePopup.trigger();

    if (ImGui::BeginPopupModal(changePopup.label, &changePopup.isOpen, ImGuiWindowFlags_NoResize))
    {
      auto& isCrop = settings.changeIsCrop;
      auto& isSize = settings.changeIsSize;
      auto& isPosition = settings.changeIsPosition;
      auto& isPivot = settings.changeIsPivot;
      auto& isScale = settings.changeIsScale;
      auto& isRotation = settings.changeIsRotation;
      auto& isDelay = settings.changeIsDelay;
      auto& isTint = settings.changeIsTint;
      auto& isColorOffset = settings.changeIsColorOffset;
      auto& isVisibleSet = settings.changeIsVisibleSet;
      auto& isInterpolatedSet = settings.changeIsInterpolatedSet;
      auto& crop = settings.changeCrop;
      auto& size = settings.changeSize;
      auto& position = settings.changePosition;
      auto& pivot = settings.changePivot;
      auto& scale = settings.changeScale;
      auto& rotation = settings.changeRotation;
      auto& delay = settings.changeDelay;
      auto& tint = settings.changeTint;
      auto& colorOffset = settings.changeColorOffset;
      auto& isVisible = settings.changeIsVisible;
      auto& isInterpolated = settings.changeIsInterpolated;

      auto& isFromSelectedFrame = settings.changeIsFromSelectedFrame;
      auto& numberFrames = settings.changeNumberFrames;

      auto propertiesSize = imgui::child_size_get(10);

      if (ImGui::BeginChild("##Properties", propertiesSize, ImGuiChildFlags_Borders))
      {
        auto start = [&](const char* checkboxLabel, bool& isEnabled)
        {
          ImGui::Checkbox(checkboxLabel, &isEnabled);
          ImGui::SameLine();
          ImGui::BeginDisabled(!isEnabled);
        };
        auto end = [&]() { ImGui::EndDisabled(); };

        auto bool_value = [&](const char* checkboxLabel, const char* valueLabel, bool& isEnabled, bool& value)
        {
          start(checkboxLabel, isEnabled);
          ImGui::Checkbox(valueLabel, &value);
          end();
        };

        auto color3_value = [&](const char* checkboxLabel, const char* valueLabel, bool& isEnabled, vec3& value)
        {
          start(checkboxLabel, isEnabled);
          ImGui::ColorEdit3(valueLabel, value_ptr(value));
          end();
        };

        auto color4_value = [&](const char* checkboxLabel, const char* valueLabel, bool& isEnabled, vec4& value)
        {
          start(checkboxLabel, isEnabled);
          ImGui::ColorEdit4(valueLabel, value_ptr(value));
          end();
        };

        auto float2_value = [&](const char* checkboxLabel, const char* valueLabel, bool& isEnabled, vec2& value)
        {
          start(checkboxLabel, isEnabled);
          ImGui::InputFloat2(valueLabel, value_ptr(value), math::vec2_format_get(value));
          end();
        };

        auto float_value = [&](const char* checkboxLabel, const char* valueLabel, bool& isEnabled, float& value)
        {
          start(checkboxLabel, isEnabled);
          ImGui::InputFloat(valueLabel, &value, imgui::STEP, imgui::STEP_FAST, math::float_format_get(value));
          end();
        };

        auto int_value = [&](const char* checkboxLabel, const char* valueLabel, bool& isEnabled, int& value)
        {
          start(checkboxLabel, isEnabled);
          ImGui::InputInt(valueLabel, &value, imgui::STEP, imgui::STEP_FAST);
          end();
        };

        float2_value("##Is Crop", "Crop", isCrop, crop);
        float2_value("##Is Size", "Size", isSize, size);
        float2_value("##Is Position", "Position", isPosition, position);
        float2_value("##Is Pivot", "Pivot", isPivot, pivot);
        float2_value("##Is Scale", "Scale", isScale, scale);
        float_value("##Is Rotation", "Rotation", isRotation, rotation);
        int_value("##Is Delay", "Delay", isDelay, delay);
        color4_value("##Is Tint", "Tint", isTint, tint);
        color3_value("##Is Color Offset", "Color Offset", isColorOffset, colorOffset);
        bool_value("##Is Visible", "Visible", isVisibleSet, isVisible);
        ImGui::SameLine();
        bool_value("##Is Interpolated", "Interpolated", isInterpolatedSet, isInterpolated);
      }
      ImGui::EndChild();

      auto settingsSize = imgui::child_size_get(2);

      if (ImGui::BeginChild("##Settings", settingsSize, ImGuiChildFlags_Borders))
      {
        ImGui::Checkbox("From Selected Frame", &isFromSelectedFrame);
        ImGui::SetItemTooltip("The frames after the currently referenced frame will be changed with these values.\nIf"
                              "off, will use all frames.");

        ImGui::BeginDisabled(!isFromSelectedFrame);
        ImGui::InputInt("Number of Frames", &numberFrames, imgui::STEP, imgui::STEP_FAST);
        numberFrames = glm::clamp(numberFrames, anm2::FRAME_NUM_MIN,
                                  (int)document->item_get()->frames.size() - document->reference.frameIndex);
        ImGui::SetItemTooltip("Set the number of frames that will be changed.");
        ImGui::EndDisabled();
      }
      ImGui::EndChild();

      auto widgetSize = imgui::widget_size_with_row_get(4);

      auto frame_change = [&](anm2::ChangeType type)
      {
        anm2::FrameChange frameChange;
        frameChange.crop = isCrop ? std::make_optional(crop) : std::nullopt;
        frameChange.size = isSize ? std::make_optional(size) : std::nullopt;
        frameChange.position = isPosition ? std::make_optional(position) : std::nullopt;
        frameChange.pivot = isPivot ? std::make_optional(pivot) : std::nullopt;
        frameChange.scale = isScale ? std::make_optional(scale) : std::nullopt;
        frameChange.rotation = isRotation ? std::make_optional(rotation) : std::nullopt;
        frameChange.delay = isDelay ? std::make_optional(delay) : std::nullopt;
        frameChange.tint = isTint ? std::make_optional(tint) : std::nullopt;
        frameChange.colorOffset = isColorOffset ? std::make_optional(colorOffset) : std::nullopt;
        frameChange.isVisible = isVisibleSet ? std::make_optional(isVisible) : std::nullopt;
        frameChange.isInterpolated = isInterpolatedSet ? std::make_optional(isInterpolated) : std::nullopt;

        document->frames_change(frameChange, type, isFromSelectedFrame, numberFrames);
      };

      if (ImGui::Button("Add", widgetSize))
      {
        frame_change(anm2::ADD);
        changePopup.close();
      }

      ImGui::SameLine();

      if (ImGui::Button("Subtract", widgetSize))
      {
        frame_change(anm2::SUBTRACT);
        changePopup.close();
      }

      ImGui::SameLine();

      if (ImGui::Button("Adjust", widgetSize))
      {
        frame_change(anm2::ADJUST);
        changePopup.close();
      }

      ImGui::SameLine();

      if (ImGui::Button("Cancel", widgetSize)) changePopup.close();

      ImGui::EndPopup();
    }

    configurePopup.trigger();

    if (ImGui::BeginPopupModal(configurePopup.label, &configurePopup.isOpen, ImGuiWindowFlags_NoResize))
    {
      auto childSize = imgui::size_without_footer_get(2);

      if (ImGui::BeginTabBar("##Configure Tabs"))
      {
        if (ImGui::BeginTabItem("General"))
        {
          if (ImGui::BeginChild("##Tab Child", childSize, true))
          {
            ImGui::SeparatorText("File");

            ImGui::Checkbox("Autosaving", &editSettings.fileIsAutosave);
            ImGui::SetItemTooltip("Enables autosaving of documents.");

            ImGui::BeginDisabled(!editSettings.fileIsAutosave);
            ImGui::InputInt("Autosave Time (minutes", &editSettings.fileAutosaveTime, imgui::STEP, imgui::STEP_FAST);
            editSettings.fileAutosaveTime = glm::clamp(editSettings.fileAutosaveTime, 0, 10);
            ImGui::SetItemTooltip("If changed, will autosave documents using this interval.");
            ImGui::EndDisabled();

            ImGui::SeparatorText("View");

            ImGui::InputFloat("Display Scale", &editSettings.displayScale, 0.25f, 0.25f, "%.2f");
            ImGui::SetItemTooltip("Change the scale of the display.");
            editSettings.displayScale = glm::clamp(editSettings.displayScale, 0.5f, 2.0f);

            ImGui::InputFloat("Zoom Step", &editSettings.viewZoomStep, 10.0f, 10.0f, "%.2f");
            ImGui::SetItemTooltip("When zooming in/out with mouse or shortcut, this value will be used.");
            editSettings.viewZoomStep = glm::clamp(editSettings.viewZoomStep, 1.0f, 250.0f);

            ImGui::Checkbox("Vsync", &editSettings.isVsync);
            ImGui::SetItemTooltip("Toggle vertical sync; synchronizes program update rate with monitor refresh rate.");
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

      if (ImGui::Button("Close", widgetSize)) configurePopup.close();
      ImGui::SetItemTooltip("Close without updating settings.");

      ImGui::EndPopup();
    }

    renderPopup.trigger();

    if (ImGui::BeginPopupModal(renderPopup.label, &renderPopup.isOpen, ImGuiWindowFlags_NoResize))
    {
      auto animation = document ? document->animation_get() : nullptr;
      if (!animation) renderPopup.close();

      auto& playback = document->playback;
      auto& ffmpegPath = settings.renderFFmpegPath;
      auto& path = settings.renderPath;
      auto& format = settings.renderFormat;
      auto& type = settings.renderType;
      auto& start = manager.recordingStart;
      auto& end = manager.recordingEnd;
      auto& isRange = settings.renderIsRange;
      auto widgetSize = imgui::widget_size_with_row_get(2);
      auto dialogType = type == render::PNGS   ? dialog::PNG_DIRECTORY_SET
                        : type == render::GIF  ? dialog::GIF_PATH_SET
                        : type == render::WEBM ? dialog::WEBM_PATH_SET
                                               : dialog::NONE;

      if (ImGui::ImageButton("##FFmpeg Path Set", resources.icons[icon::FOLDER].id, imgui::icon_size_get()))
        dialog.file_open(dialog::FFMPEG_PATH_SET);
      ImGui::SameLine();
      imgui::input_text_string("FFmpeg Path", &ffmpegPath);
      dialog.set_string_to_selected_path(ffmpegPath, dialog::FFMPEG_PATH_SET);

      if (ImGui::ImageButton("##Path Set", resources.icons[icon::FOLDER].id, imgui::icon_size_get()))
      {
        if (dialogType == dialog::PNG_DIRECTORY_SET)
          dialog.folder_open(dialogType);
        else
          dialog.file_open(dialogType);
      }
      ImGui::SameLine();
      imgui::input_text_string(type == render::PNGS ? "Directory" : "Path", &path);
      dialog.set_string_to_selected_path(path, dialogType);

      ImGui::Combo("Type", &type, render::STRINGS, render::COUNT);

      ImGui::BeginDisabled(type != render::PNGS);
      imgui::input_text_string("Format", &format);
      ImGui::EndDisabled();

      ImGui::BeginDisabled(!isRange);
      imgui::input_int_range("Start", start, 0, animation->frameNum - 1);
      ImGui::InputInt("End", &end, start, animation->frameNum);
      ImGui::EndDisabled();

      ImGui::Checkbox("Custom Range", &isRange);

      if (ImGui::Button("Render", widgetSize))
      {
        manager.isRecording = true;
        playback.time = start;
        playback.isPlaying = true;
        renderPopup.close();
        manager.progressPopup.open();
      }

      ImGui::SameLine();

      if (ImGui::Button("Cancel", widgetSize)) renderPopup.close();

      ImGui::EndPopup();
    }

    aboutPopup.trigger();

    if (ImGui::BeginPopupModal(aboutPopup.label, &aboutPopup.isOpen, ImGuiWindowFlags_NoResize))
    {
      if (ImGui::Button("Close")) aboutPopup.close();
      ImGui::EndPopup();
    }

    if (imgui::shortcut(settings.shortcutNew, shortcut::GLOBAL)) dialog.file_open(dialog::ANM2_NEW);
    if (imgui::shortcut(settings.shortcutOpen, shortcut::GLOBAL)) dialog.file_open(dialog::ANM2_OPEN);
    if (imgui::shortcut(settings.shortcutSave, shortcut::GLOBAL)) document->save();
    if (imgui::shortcut(settings.shortcutSaveAs, shortcut::GLOBAL)) dialog.file_save(dialog::ANM2_SAVE);
    if (imgui::shortcut(settings.shortcutExit, shortcut::GLOBAL)) isQuitting = true;
  }
}
