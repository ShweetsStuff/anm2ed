#include "taskbar.h"

#include <algorithm>
#include <array>
#include <cfloat>
#include <cmath>
#include <filesystem>
#include <format>
#include <ranges>
#include <tuple>
#include <vector>

#include <imgui/imgui.h>

#include "math_.h"
#include "render.h"
#include "shader.h"
#include "types.h"

#include "icon.h"

using namespace anm2ed::resource;
using namespace anm2ed::types;
using namespace anm2ed::canvas;
using namespace anm2ed::util;
using namespace glm;

namespace anm2ed::imgui
{
  static constexpr auto ANM2ED_LABEL = "Anm2Ed";
  static constexpr auto VERSION_LABEL = "Version 2.0";
  static constexpr auto CREDIT_DELAY = 1.0f;
  static constexpr auto CREDIT_SCROLL_SPEED = 25.0f;

  struct Credit
  {
    const char* string{};
    font::Type font{font::REGULAR};
  };

  struct ScrollingCredit
  {
    int index{};
    float offset{};
  };

  struct CreditsState
  {
    std::vector<ScrollingCredit> active{};
    float spawnTimer{1.0f};
    int nextIndex{};
  };

  static constexpr Credit CREDITS[] = {
      {"Anm2Ed", font::BOLD},
      {"License: GPLv3"},
      {""},
      {"Designer", font::BOLD},
      {"Shweet"},
      {""},
      {"Additional Help", font::BOLD},
      {"im-tem"},
      {""},
      {"Based on the work of:", font::BOLD},
      {"Adrian Gavrilita"},
      {"Simon Parzer"},
      {"Matt Kapuszczak"},
      {""},
      {"XM Music", font::BOLD},
      {"Drozerix"},
      {"\"Keygen Wraith\""},
      {"https://modarchive.org/module.php?207854"},
      {"License: CC0"},
      {""},
      {"Libraries", font::BOLD},
      {"Dear ImGui"},
      {"https://github.com/ocornut/imgui"},
      {"License: MIT"},
      {""},
      {"SDL"},
      {"https://github.com/libsdl-org/SDL"},
      {"License: zlib"},
      {""},
      {"SDL_mixer"},
      {"https://github.com/libsdl-org/SDL_mixer"},
      {"License: zlib"},
      {""},
      {"tinyxml2"},
      {"https://github.com/leethomason/tinyxml2"},
      {"License: zlib"},
      {""},
      {"glm"},
      {"https://github.com/g-truc/glm"},
      {"License: MIT"},
      {""},
      {"lunasvg"},
      {"https://github.com/sammycage/lunasvg"},
      {"License: MIT"},
      {""},
      {"Icons", font::BOLD},
      {"Remix Icons"},
      {"remixicon.com"},
      {"License: Apache"},
      {""},
      {"Font", font::BOLD},
      {"Noto Sans"},
      {"https://fonts.google.com/noto/specimen/Noto+Sans"},
      {"License: OFL"},
      {""},
      {"Special Thanks", font::BOLD},
      {"Edmund McMillen"},
      {"Florian Himsl"},
      {"Tyrone Rodriguez"},
      {"The-Vinh Truong (_kilburn)"},
      {"Everyone who waited patiently for this to be finished"},
      {"Everyone else who has worked on The Binding of Isaac!"},
      {""},
      {""},
      {""},
      {""},
      {""},
      {"enjoy the jams :)"},
      {""},
      {""},
      {""},
      {""},
      {""},
  };
  static constexpr auto CREDIT_COUNT = (int)(sizeof(CREDITS) / sizeof(Credit));

  Taskbar::Taskbar() : generate(vec2()) {}

  void Taskbar::update(Manager& manager, Settings& settings, Resources& resources, Dialog& dialog, bool& isQuitting)
  {
    auto document = manager.get();
    auto animation = document ? document->animation_get() : nullptr;
    auto item = document ? document->item_get() : nullptr;

    if (ImGui::BeginMainMenuBar())
    {
      height = ImGui::GetWindowSize().y;

      if (ImGui::BeginMenu("File"))
      {
        if (ImGui::MenuItem("New", settings.shortcutNew.c_str())) dialog.file_save(dialog::ANM2_NEW);
        if (ImGui::MenuItem("Open", settings.shortcutOpen.c_str())) dialog.file_open(dialog::ANM2_OPEN);

        auto recentFiles = manager.recent_files_ordered();
        if (ImGui::BeginMenu("Open Recent", !recentFiles.empty()))
        {
          for (auto [i, file] : std::views::enumerate(recentFiles))
          {
            auto label = std::format(FILE_LABEL_FORMAT, file.filename().string(), file.string());

            ImGui::PushID(i);
            if (ImGui::MenuItem(label.c_str())) manager.open(file.string());
            ImGui::PopID();
          }

          if (!recentFiles.empty())
            if (ImGui::MenuItem("Clear List")) manager.recent_files_clear();

          ImGui::EndMenu();
        }

        if (ImGui::MenuItem("Save", settings.shortcutSave.c_str(), false, document))
          if (settings.fileIsWarnOverwrite) overwritePopup.open();

        if (ImGui::MenuItem("Save As", settings.shortcutSaveAs.c_str(), false, document))
          dialog.file_save(dialog::ANM2_SAVE);
        if (ImGui::MenuItem("Explore XML Location", nullptr, false, document))
          dialog.file_explorer_open(document->directory_get().string());

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
        if (ImGui::MenuItem("Generate Animation From Grid", nullptr, false,
                            item && document->reference.itemType == anm2::LAYER))
          generatePopup.open();

        ImGui::Separator();
        if (ImGui::MenuItem("Render Animation", nullptr, false, animation)) renderPopup.open();
        ImGui::EndMenu();
      }

      if (ImGui::BeginMenu("Playback"))
      {
        ImGui::MenuItem("Always Loop", nullptr, &settings.playbackIsLoop);
        ImGui::SetItemTooltip("%s", "Animations will always loop during playback, even if looping isn't set.");

        ImGui::MenuItem("Clamp", nullptr, &settings.playbackIsClamp);
        ImGui::SetItemTooltip("%s", "Operations will always be clamped to within the animation's bounds.\nFor example, "
                                    "dragging the playhead, or triggers.");

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

      auto childSize = ImVec2(row_widget_width_get(2), size_without_footer_get().y);

      if (ImGui::BeginChild("##Options Child", childSize, ImGuiChildFlags_Borders))
      {
        ImGui::InputInt2("Start Position", value_ptr(startPosition));
        ImGui::InputInt2("Frame Size", value_ptr(size));
        ImGui::InputInt2("Pivot", value_ptr(pivot));
        ImGui::InputInt("Rows", &rows, STEP, STEP_FAST);
        ImGui::InputInt("Columns", &columns, STEP, STEP_FAST);

        input_int_range("Count", count, anm2::FRAME_NUM_MIN, rows * columns);

        ImGui::InputInt("Delay", &delay, STEP, STEP_FAST);
      }
      ImGui::EndChild();

      ImGui::SameLine();

      if (ImGui::BeginChild("##Preview Child", childSize, ImGuiChildFlags_Borders))
      {
        auto& backgroundColor = settings.previewBackgroundColor;
        auto& time = generateTime;
        auto& shaderTexture = resources.shaders[resource::shader::TEXTURE];

        auto previewSize = ImVec2(ImGui::GetContentRegionAvail().x, size_without_footer_get(2).y);

        generate.size_set(to_vec2(previewSize));
        generate.bind();
        generate.viewport_set();
        generate.clear(backgroundColor);

        if (document && document->reference.itemType == anm2::LAYER)
        {
          auto& texture = document->anm2.content
                              .spritesheets[document->anm2.content.layers[document->reference.itemID].spritesheetID]
                              .texture;

          auto index = std::clamp((int)(time * (count - 1)), 0, (count - 1));
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

      auto widgetSize = widget_size_with_row_get(2);

      if (ImGui::Button("Generate", widgetSize))
      {
        auto generate_from_grid = [&]()
        {
          item->frames_generate_from_grid(startPosition, size, pivot, columns, count, delay);
          animation->frameNum = animation->length();
        };

        DOCUMENT_EDIT_PTR(document, "Generate Animation from Grid", Document::FRAMES, generate_from_grid());

        generatePopup.close();
      }

      ImGui::SameLine();

      if (ImGui::Button("Cancel", widgetSize)) generatePopup.close();

      ImGui::EndPopup();
    }

    configurePopup.trigger();

    if (ImGui::BeginPopupModal(configurePopup.label, &configurePopup.isOpen, ImGuiWindowFlags_NoResize))
    {
      auto childSize = size_without_footer_get(2);

      if (ImGui::BeginTabBar("##Configure Tabs"))
      {
        if (ImGui::BeginTabItem("Display"))
        {
          if (ImGui::BeginChild("##Tab Child", childSize, true))
          {
            input_float_range("UI Scale", editSettings.uiScale, 0.5f, 2.0f, 0.25f, 0.25f, "%.2f");
            ImGui::SetItemTooltip("Change the scale of the UI.");

            ImGui::Checkbox("Vsync", &editSettings.isVsync);
            ImGui::SetItemTooltip("Toggle vertical sync; synchronizes program update rate with monitor refresh rate.");
          }
          ImGui::EndChild();

          ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("File"))
        {
          if (ImGui::BeginChild("##Tab Child", childSize, true))
          {
            ImGui::SeparatorText("Autosave");

            ImGui::Checkbox("Enabled", &editSettings.fileIsAutosave);
            ImGui::SetItemTooltip("Enables autosaving of documents.");

            ImGui::BeginDisabled(!editSettings.fileIsAutosave);
            input_int_range("Time (minutes)", editSettings.fileAutosaveTime, 0, 10);
            ImGui::SetItemTooltip("If changed, will autosave documents using this interval.");
            ImGui::EndDisabled();

            ImGui::SeparatorText("Options");

            ImGui::Checkbox("Overwrite Warning", &editSettings.fileIsWarnOverwrite);
            ImGui::SetItemTooltip("A warning will be shown when saving a file.");
          }
          ImGui::EndChild();

          ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Input"))
        {
          if (ImGui::BeginChild("##Tab Child", childSize, true))
          {
            ImGui::SeparatorText("Keyboard");

            input_float_range("Repeat Delay (seconds)", editSettings.keyboardRepeatDelay, 0.05f, 1.0f, 0.05f, 0.05f,
                              "%.2f");
            ImGui::SetItemTooltip("Set how often, after repeating begins, key inputs will be fired.");

            input_float_range("Repeat Rate (seconds)", editSettings.keyboardRepeatRate, 0.005f, 1.0f, 0.005f, 0.005f,
                              "%.3f");
            ImGui::SetItemTooltip("Set how often, after repeating begins, key inputs will be fired.");

            ImGui::SeparatorText("Zoom");

            input_float_range("Step", editSettings.viewZoomStep, 10.0f, 250.0f, 10.0f, 10.0f, "%.0f%%");
            ImGui::SetItemTooltip("When zooming in/out with mouse or shortcut, this value will be used.");
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

                  for (auto& key : KEY_MAP | std::views::values)
                  {
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

      if (ImGui::Button("Save", widgetSize))
      {
        settings = editSettings;
        manager.chords_set(settings);
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
      auto& ffmpegPath = settings.renderFFmpegPath;
      auto& path = settings.renderPath;
      auto& format = settings.renderFormat;
      auto& scale = settings.renderScale;
      auto& isRaw = settings.renderIsRawAnimation;
      auto& type = settings.renderType;
      auto& start = manager.recordingStart;
      auto& end = manager.recordingEnd;
      auto& rows = settings.renderRows;
      auto& columns = settings.renderColumns;
      auto& isRange = manager.isRecordingRange;
      auto& frames = document->frames.selection;
      int length = std::max(1, end - start + 1);

      auto range_set = [&]()
      {
        if (!frames.empty())
        {
          if (auto item = document->item_get())
          {
            int duration{};
            for (auto [i, frame] : std::views::enumerate(item->frames))
            {
              if (i == *frames.begin())
                start = duration;
              else if (i == *frames.rbegin())
              {
                end = duration;
                break;
              }

              duration += frame.duration;
            }
          }
        }
        else if (!isRange)
        {
          start = 0;
          end = animation->frameNum - 1;
        }

        length = std::max(1, end - start + 1);
      };

      auto rows_columns_set = [&]()
      {
        auto framesNeeded = std::max(1, length);
        int bestRows = 1;
        int bestColumns = framesNeeded;

        auto bestScore = std::make_tuple(bestColumns - bestRows, bestColumns * bestRows - framesNeeded, -bestColumns);

        for (int candidateRows = 1; candidateRows <= framesNeeded; ++candidateRows)
        {
          int candidateColumns = (framesNeeded + candidateRows - 1) / candidateRows;
          if (candidateColumns < candidateRows) break;

          auto candidateScore = std::make_tuple(candidateColumns - candidateRows,
                                                candidateColumns * candidateRows - framesNeeded, -candidateColumns);

          if (candidateScore < bestScore)
          {
            bestScore = candidateScore;
            bestRows = candidateRows;
            bestColumns = candidateColumns;
          }
        }

        rows = bestRows;
        columns = bestColumns;
      };

      auto replace_extension = [&]()
      { path = std::filesystem::path(path).replace_extension(render::EXTENSIONS[type]).string(); };

      auto render_set = [&]()
      {
        replace_extension();
        range_set();
        rows_columns_set();
      };

      auto widgetSize = widget_size_with_row_get(2);
      auto dialogType = type == render::PNGS          ? dialog::PNG_DIRECTORY_SET
                        : type == render::SPRITESHEET ? dialog::PNG_PATH_SET
                        : type == render::GIF         ? dialog::GIF_PATH_SET
                        : type == render::WEBM        ? dialog::WEBM_PATH_SET
                                                      : dialog::NONE;

      if (renderPopup.isJustOpened) render_set();

      if (ImGui::ImageButton("##FFmpeg Path Set", resources.icons[icon::FOLDER].id, icon_size_get()))
        dialog.file_open(dialog::FFMPEG_PATH_SET);
      ImGui::SameLine();
      input_text_string("FFmpeg Path", &ffmpegPath);
      ImGui::SetItemTooltip("Set the path where the FFmpeg installation is located.\nFFmpeg is required to render "
                            "animations.\nhttps://ffmpeg.org");
      dialog.set_string_to_selected_path(ffmpegPath, dialog::FFMPEG_PATH_SET);

      if (ImGui::ImageButton("##Path Set", resources.icons[icon::FOLDER].id, icon_size_get()))
      {
        if (dialogType == dialog::PNG_DIRECTORY_SET)
          dialog.folder_open(dialogType);
        else
          dialog.file_save(dialogType);
      }
      ImGui::SameLine();
      input_text_string(type == render::PNGS ? "Directory" : "Path", &path);
      ImGui::SetItemTooltip("Set the output path or directory for the animation.");
      dialog.set_string_to_selected_path(path, dialogType);

      if (ImGui::Combo("Type", &type, render::STRINGS, render::COUNT)) render_set();
      ImGui::SetItemTooltip("Set the type of the output.");

      if (type == render::PNGS || type == render::SPRITESHEET) ImGui::Separator();

      if (type == render::PNGS)
      {
        if (input_text_string("Format", &format)) format = std::filesystem::path(format).replace_extension(".png");
        ImGui::SetItemTooltip(
            "For outputted images, each image will use this format.\n{} represents the index of each image.");
      }
      else if (type == render::SPRITESHEET)
      {
        input_int_range("Rows", rows, 1, length);
        ImGui::SetItemTooltip("Set how many rows the spritesheet will have.");

        input_int_range("Columns", columns, 1, length);
        ImGui::SetItemTooltip("Set how many columns the spritesheet will have.");

        if (ImGui::Button("Set to Recommended")) rows_columns_set();
        ImGui::SetItemTooltip("Use a recommended value for rows/columns.");
      }

      ImGui::Separator();

      if (ImGui::Checkbox("Custom Range", &isRange))
      {
        range_set();
        ImGui::SetItemTooltip("Toggle using a custom range for the animation.");
      }

      ImGui::SameLine();

      ImGui::BeginDisabled(frames.empty());
      if (ImGui::Button("To Selected Frames")) range_set();
      ImGui::SetItemTooltip("If frames are selected, use that range for the rendered animation.");
      ImGui::EndDisabled();

      ImGui::BeginDisabled(!isRange);
      {
        input_int_range("Start", start, 0, animation->frameNum - 1);
        ImGui::SetItemTooltip("Set the starting time of the animation.");
        input_int_range("End", end, start + 1, animation->frameNum);
        ImGui::SetItemTooltip("Set the ending time of the animation.");
      }
      ImGui::EndDisabled();

      ImGui::Separator();

      ImGui::Checkbox("Raw", &isRaw);
      ImGui::SetItemTooltip("Record only the raw animation; i.e., only its layers, to its bounds.");

      ImGui::BeginDisabled(!isRaw);
      {
        input_float_range("Scale", scale, 1.0f, 100.0f, STEP, STEP_FAST, "%.1fx");
        ImGui::SetItemTooltip("Set the output scale of the animation.");
      }
      ImGui::EndDisabled();

      ImGui::Separator();

      ImGui::Checkbox("Sound", &settings.timelineIsSound);
      ImGui::SetItemTooltip("Toggle sounds playing with triggers.\nBind sounds to events in the Events window.\nThe "
                            "output animation will use the played sounds.");

      ImGui::Separator();

      if (ImGui::Button("Render", widgetSize))
      {
        manager.isRecordingStart = true;
        renderPopup.close();
        manager.progressPopup.open();
      }
      ImGui::SetItemTooltip("Render the animation using the current settings.");

      ImGui::SameLine();

      if (ImGui::Button("Cancel", widgetSize)) renderPopup.close();

      ImGui::EndPopup();
    }

    renderPopup.end();

    aboutPopup.trigger();

    if (ImGui::BeginPopupModal(aboutPopup.label, &aboutPopup.isOpen, ImGuiWindowFlags_NoResize))
    {
      static CreditsState creditsState{};

      auto credits_reset = [&]()
      {
        resources.music_track().play(true);
        creditsState = {};
        creditsState.spawnTimer = CREDIT_DELAY;
      };

      if (aboutPopup.isJustOpened) credits_reset();

      auto size = ImGui::GetContentRegionAvail();

      ImGui::PushFont(resources.fonts[font::BOLD].get(), font::SIZE_LARGE);

      ImGui::SetCursorPosX((size.x - ImGui::CalcTextSize(ANM2ED_LABEL).x) / 2);
      ImGui::Text(ANM2ED_LABEL);

      ImGui::SetCursorPosX((size.x - ImGui::CalcTextSize(VERSION_LABEL).x) / 2);
      ImGui::Text(VERSION_LABEL);

      ImGui::PopFont();

      auto creditRegionPos = ImGui::GetCursorScreenPos();
      auto creditRegionSize = ImGui::GetContentRegionAvail();

      if (creditRegionSize.y > 0.0f && creditRegionSize.x > 0.0f)
      {
        auto drawList = ImGui::GetWindowDrawList();
        auto clipMax = ImVec2(creditRegionPos.x + creditRegionSize.x, creditRegionPos.y + creditRegionSize.y);
        drawList->PushClipRect(creditRegionPos, clipMax, true);

        auto delta = ImGui::GetIO().DeltaTime;
        creditsState.spawnTimer -= delta;
        auto maxVisible = std::max(1, (int)std::floor(creditRegionSize.y / (float)font::SIZE));

        while (creditsState.active.size() < (size_t)maxVisible && creditsState.spawnTimer <= 0.0f)
        {
          creditsState.active.push_back({creditsState.nextIndex, 0.0f});
          creditsState.nextIndex = (creditsState.nextIndex + 1) % CREDIT_COUNT;
          creditsState.spawnTimer += CREDIT_DELAY;
        }

        auto baseY = clipMax.y - (float)font::SIZE;
        const auto& baseColor = ImGui::GetStyleColorVec4(ImGuiCol_Text);
        auto fadeSpan = (float)font::SIZE * 2.0f;

        for (auto it = creditsState.active.begin(); it != creditsState.active.end();)
        {
          it->offset += CREDIT_SCROLL_SPEED * delta;
          auto yPos = baseY - it->offset;
          if (yPos + font::SIZE < creditRegionPos.y)
          {
            it = creditsState.active.erase(it);
            continue;
          }

          const auto& credit = CREDITS[it->index];
          auto fontPtr = resources.fonts[credit.font].get();
          auto textSize = fontPtr->CalcTextSizeA((float)font::SIZE, FLT_MAX, 0.0f, credit.string);
          auto xPos = creditRegionPos.x + (creditRegionSize.x - textSize.x) * 0.5f;

          auto alpha = 1.0f;
          auto topDist = yPos - creditRegionPos.y;
          if (topDist < fadeSpan) alpha *= std::clamp(topDist / fadeSpan, 0.0f, 1.0f);
          auto bottomDist = (creditRegionPos.y + creditRegionSize.y) - (yPos + font::SIZE);
          if (bottomDist < fadeSpan) alpha *= std::clamp(bottomDist / fadeSpan, 0.0f, 1.0f);
          if (alpha <= 0.0f)
          {
            ++it;
            continue;
          }

          auto color = baseColor;
          color.w *= alpha;

          drawList->AddText(fontPtr, (float)font::SIZE, ImVec2(xPos, yPos), ImGui::GetColorU32(color), credit.string);
          ++it;
        }

        drawList->PopClipRect();
      }

      ImGui::EndPopup();
    }

    if (auto* music = resources.music_track_if_loaded())
      if (music->is_playing() && !aboutPopup.isOpen) music->stop();

    overwritePopup.trigger();

    if (ImGui::BeginPopupModal(overwritePopup.label, &overwritePopup.isOpen, ImGuiWindowFlags_NoResize))
    {
      ImGui::Text("Are you sure? This will overwrite the existing file.");

      auto widgetSize = widget_size_with_row_get(2);

      if (ImGui::Button("Yes", widgetSize))
      {
        manager.save();
        overwritePopup.close();
      }

      ImGui::SameLine();

      if (ImGui::Button("No", widgetSize)) overwritePopup.close();

      ImGui::EndPopup();
    }

    aboutPopup.end();

    if (shortcut(manager.chords[SHORTCUT_NEW], shortcut::GLOBAL)) dialog.file_save(dialog::ANM2_NEW);
    if (shortcut(manager.chords[SHORTCUT_OPEN], shortcut::GLOBAL)) dialog.file_open(dialog::ANM2_OPEN);
    if (shortcut(manager.chords[SHORTCUT_SAVE], shortcut::GLOBAL))
    {
      if (settings.fileIsWarnOverwrite) overwritePopup.open();
      manager.save();
    }
    if (shortcut(manager.chords[SHORTCUT_SAVE_AS], shortcut::GLOBAL)) dialog.file_save(dialog::ANM2_SAVE);
    if (shortcut(manager.chords[SHORTCUT_EXIT], shortcut::GLOBAL)) isQuitting = true;
  }
}
#ifndef ANM2ED_USE_LIBXM
  #define ANM2ED_USE_LIBXM 1
#endif
