#include "taskbar.h"

#include <algorithm>
#include <array>
#include <cfloat>
#include <cmath>
#include <cstddef>
#include <filesystem>
#include <format>
#include <system_error>
#include <tuple>
#include <vector>

#include <imgui/imgui.h>

#include "log.h"
#include "math_.h"
#include "render.h"
#include "shader.h"
#include "snapshots.h"
#include "types.h"

#include "icon.h"
#include "toast.h"

#include "strings.h"

using namespace anm2ed::resource;
using namespace anm2ed::types;
using namespace anm2ed::canvas;
using namespace anm2ed::util;
using namespace glm;

namespace anm2ed::imgui
{
  namespace
  {
#ifndef _WIN32
    constexpr auto EXEC_PERMS =
        std::filesystem::perms::owner_exec | std::filesystem::perms::group_exec | std::filesystem::perms::others_exec;
#endif

    bool ffmpeg_is_executable(const std::string& pathString)
    {
      if (pathString.empty()) return false;

      std::error_code ec{};
      auto status = std::filesystem::status(pathString, ec);
      if (ec || !std::filesystem::is_regular_file(status)) return false;

#ifndef _WIN32
      if ((status.permissions() & EXEC_PERMS) == std::filesystem::perms::none) return false;
#endif
      return true;
    }

    bool png_directory_ensure(const std::string& directory)
    {
      if (directory.empty())
      {
        toasts.push(localize.get(TOAST_PNG_DIRECTORY_NOT_SET));
        logger.warning(localize.get(TOAST_PNG_DIRECTORY_NOT_SET, anm2ed::ENGLISH));
        return false;
      }

      std::error_code ec{};
      auto pathValue = std::filesystem::path(directory);
      auto exists = std::filesystem::exists(pathValue, ec);

      if (ec)
      {
        auto errorMessage = ec.message();
        toasts.push(std::vformat(localize.get(TOAST_PNG_DIRECTORY_ACCESS_ERROR),
                                 std::make_format_args(directory, errorMessage)));
        logger.error(std::vformat(localize.get(TOAST_PNG_DIRECTORY_ACCESS_ERROR, anm2ed::ENGLISH),
                                  std::make_format_args(directory, errorMessage)));
        return false;
      }

      if (exists)
      {
        if (!std::filesystem::is_directory(pathValue, ec) || ec)
        {
          toasts.push(std::vformat(localize.get(TOAST_PNG_DIRECTORY_NOT_DIRECTORY), std::make_format_args(directory)));
          logger.warning(std::vformat(localize.get(TOAST_PNG_DIRECTORY_NOT_DIRECTORY, anm2ed::ENGLISH),
                                      std::make_format_args(directory)));
          return false;
        }
        return true;
      }

      if (!std::filesystem::create_directories(pathValue, ec) || ec)
      {
        auto errorMessage = ec.message();
        toasts.push(std::vformat(localize.get(TOAST_PNG_DIRECTORY_CREATE_ERROR),
                                 std::make_format_args(directory, errorMessage)));
        logger.error(std::vformat(localize.get(TOAST_PNG_DIRECTORY_CREATE_ERROR, anm2ed::ENGLISH),
                                  std::make_format_args(directory, errorMessage)));
        return false;
      }

      return true;
    }
  }

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
      {"Localization", font::BOLD},
      {"ExtremeThreat (Russian)"},
      {"CxRedix (Chinese)"},
      {"sawalk/사왈이 (Korean)"},
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

      if (ImGui::BeginMenu(localize.get(LABEL_FILE_MENU)))
      {
        if (ImGui::MenuItem(localize.get(BASIC_NEW), settings.shortcutNew.c_str())) dialog.file_save(dialog::ANM2_NEW);
        if (ImGui::MenuItem(localize.get(BASIC_OPEN), settings.shortcutOpen.c_str()))
          dialog.file_open(dialog::ANM2_OPEN);

        auto recentFiles = manager.recent_files_ordered();
        if (ImGui::BeginMenu(localize.get(LABEL_OPEN_RECENT), !recentFiles.empty()))
        {
          for (std::size_t index = 0; index < recentFiles.size(); ++index)
          {
            const auto& file = recentFiles[index];
            auto label = std::format(FILE_LABEL_FORMAT, file.filename().string(), file.string());

            ImGui::PushID((int)index);
            if (ImGui::MenuItem(label.c_str())) manager.open(file.string());
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
          dialog.file_save(dialog::ANM2_SAVE);
        if (ImGui::MenuItem(localize.get(LABEL_EXPLORE_XML_LOCATION), nullptr, false, document))
          dialog.file_explorer_open(document->directory_get().string());

        ImGui::Separator();
        if (ImGui::MenuItem(localize.get(LABEL_EXIT), settings.shortcutExit.c_str())) isQuitting = true;
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

      if (ImGui::BeginMenu(localize.get(LABEL_WIZARD_MENU)))
      {
        if (ImGui::MenuItem(localize.get(LABEL_TASKBAR_GENERATE_ANIMATION_FROM_GRID), nullptr, false,
                            item && document->reference.itemType == anm2::LAYER))
          generatePopup.open();

        ImGui::Separator();
        if (ImGui::MenuItem(localize.get(LABEL_TASKBAR_RENDER_ANIMATION), nullptr, false, animation))
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
        for (std::size_t index = 0; index < WINDOW_COUNT; ++index)
        {
          auto member = WINDOW_MEMBERS[index];
          ImGui::MenuItem(localize.get(::anm2ed::WINDOW_STRING_TYPES[index]), nullptr, &(settings.*member));
        }

        ImGui::EndMenu();
      }

      if (ImGui::BeginMenu(localize.get(LABEL_SETTINGS_MENU)))
      {
        if (ImGui::MenuItem(localize.get(LABEL_TASKBAR_CONFIGURE)))
        {
          editSettings = settings;
          configurePopup.open();
        }

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
      auto& startPosition = settings.generateStartPosition;
      auto& size = settings.generateSize;
      auto& pivot = settings.generatePivot;
      auto& rows = settings.generateRows;
      auto& columns = settings.generateColumns;
      auto& count = settings.generateCount;
      auto& delay = settings.generateDuration;
      auto& zoom = settings.generateZoom;
      auto& zoomStep = settings.inputZoomStep;

      auto childSize = ImVec2(row_widget_width_get(2), size_without_footer_get().y);

      if (ImGui::BeginChild("##Options Child", childSize, ImGuiChildFlags_Borders))
      {
        ImGui::InputInt2(localize.get(LABEL_GENERATE_START_POSITION), value_ptr(startPosition));
        ImGui::InputInt2(localize.get(LABEL_GENERATE_FRAME_SIZE), value_ptr(size));
        ImGui::InputInt2(localize.get(BASIC_PIVOT), value_ptr(pivot));
        ImGui::InputInt(localize.get(LABEL_GENERATE_ROWS), &rows, STEP, STEP_FAST);
        ImGui::InputInt(localize.get(LABEL_GENERATE_COLUMNS), &columns, STEP, STEP_FAST);

        input_int_range(localize.get(LABEL_GENERATE_COUNT), count, anm2::FRAME_NUM_MIN, rows * columns);

        ImGui::InputInt(localize.get(BASIC_DURATION), &delay, STEP, STEP_FAST);
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
        generate.clear(vec4(backgroundColor, 1.0f));

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

      if (ImGui::Button(localize.get(LABEL_GENERATE), widgetSize))
      {
        auto generate_from_grid = [&]()
        {
          item->frames_generate_from_grid(startPosition, size, pivot, columns, count, delay);
          animation->frameNum = animation->length();
        };

        DOCUMENT_EDIT_PTR(document, localize.get(EDIT_GENERATE_ANIMATION_FROM_GRID), Document::FRAMES,
                          generate_from_grid());

        generatePopup.close();
      }

      ImGui::SameLine();

      if (ImGui::Button(localize.get(BASIC_CANCEL), widgetSize)) generatePopup.close();

      ImGui::EndPopup();
    }

    configurePopup.trigger();

    if (ImGui::BeginPopupModal(configurePopup.label(), &configurePopup.isOpen, ImGuiWindowFlags_NoResize))
    {
      auto childSize = size_without_footer_get(2);

      if (ImGui::BeginTabBar("##Configure Tabs"))
      {
        if (ImGui::BeginTabItem(localize.get(LABEL_DISPLAY)))
        {
          if (ImGui::BeginChild("##Tab Child", childSize, true))
          {
            ImGui::SeparatorText(localize.get(LABEL_WINDOW_MENU));
            input_float_range(localize.get(LABEL_UI_SCALE), editSettings.uiScale, 0.5f, 2.0f, 0.25f, 0.25f, "%.2f");
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_UI_SCALE));
            ImGui::Checkbox(localize.get(LABEL_VSYNC), &editSettings.isVsync);
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_VSYNC));

            ImGui::SeparatorText(localize.get(LABEL_LOCALIZATION));
            ImGui::Combo(localize.get(LABEL_LANGUAGE), &editSettings.language, LANGUAGE_STRINGS, LANGUAGE_COUNT);
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_LANGUAGE));

            ImGui::SeparatorText(localize.get(LABEL_THEME));

            for (int i = 0; i < theme::COUNT; i++)
            {
              ImGui::RadioButton(localize.get(theme::STRINGS[i]), &editSettings.theme, i);
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

            ImGui::Checkbox(localize.get(BASIC_ENABLED), &editSettings.fileIsAutosave);
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_AUTOSAVE_ENABLED));

            ImGui::BeginDisabled(!editSettings.fileIsAutosave);
            input_int_range(localize.get(LABEL_TIME_MINUTES), editSettings.fileAutosaveTime, 0, 10);
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_AUTOSAVE_INTERVAL));
            ImGui::EndDisabled();

            ImGui::SeparatorText(localize.get(LABEL_SNAPSHOTS));
            input_int_range(localize.get(LABEL_STACK_SIZE), editSettings.fileSnapshotStackSize, 0, 100);
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_STACK_SIZE));

            ImGui::SeparatorText(localize.get(LABEL_OPTIONS));
            ImGui::Checkbox(localize.get(LABEL_OVERWRITE_WARNING), &editSettings.fileIsWarnOverwrite);
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

            input_float_range(localize.get(LABEL_REPEAT_DELAY), editSettings.keyboardRepeatDelay, 0.05f, 1.0f, 0.05f,
                              0.05f, "%.2f");
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_REPEAT_DELAY));

            input_float_range(localize.get(LABEL_REPEAT_RATE), editSettings.keyboardRepeatRate, 0.005f, 1.0f, 0.005f,
                              0.005f, "%.3f");
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_REPEAT_DELAY));

            ImGui::SeparatorText(localize.get(LABEL_ZOOM));

            input_float_range(localize.get(LABEL_ZOOM_STEP), editSettings.inputZoomStep, 10.0f, 250.0f, 10.0f, 10.0f,
                              "%.0f%%");
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_ZOOM_STEP));

            ImGui::SeparatorText(localize.get(LABEL_TOOL));

            ImGui::Checkbox(localize.get(LABEL_MOVE_TOOL_SNAP), &editSettings.inputIsMoveToolSnapToMouse);
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
                std::string* settingString = &(editSettings.*member);
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
        settings = editSettings;

        ImGui::GetIO().KeyRepeatDelay = settings.keyboardRepeatDelay;
        ImGui::GetIO().KeyRepeatRate = settings.keyboardRepeatRate;
        ImGui::GetStyle().FontScaleMain = settings.uiScale;
        SnapshotStack::max_size_set(settings.fileSnapshotStackSize);
        imgui::theme_set((theme::Type)settings.theme);
        localize.language = (Language)settings.language;
        manager.chords_set(settings);

        for (auto& document : manager.documents)
          document.snapshots.apply_limit();

        configurePopup.close();
      }
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_SETTINGS_SAVE));

      ImGui::SameLine();

      if (ImGui::Button(localize.get(LABEL_USE_DEFAULT_SETTINGS), widgetSize)) editSettings = Settings();
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_USE_DEFAULT_SETTINGS));

      ImGui::SameLine();

      if (ImGui::Button(localize.get(LABEL_CLOSE), widgetSize)) configurePopup.close();
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_CLOSE_SETTINGS));

      ImGui::EndPopup();
    }

    renderPopup.trigger();

    if (ImGui::BeginPopupModal(renderPopup.label(), &renderPopup.isOpen, ImGuiWindowFlags_NoResize))
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

      auto range_to_frames_set = [&]()
      {
        if (auto item = document->item_get())
        {
          int duration{};
          for (std::size_t index = 0; index < item->frames.size(); ++index)
          {
            const auto& frame = item->frames[index];

            if ((int)index == *frames.begin())
              start = duration;
            else if ((int)index == *frames.rbegin())
            {
              end = duration;
              break;
            }

            duration += frame.duration;
          }
        }
      };

      auto range_to_animation_set = [&]()
      {
        start = 0;
        end = animation->frameNum - 1;
      };

      auto range_set = [&]()
      {
        if (!isRange) range_to_animation_set();
        length = std::max(1, end - (start + 1));
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
      input_text_string(localize.get(LABEL_FFMPEG_PATH), &ffmpegPath);
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_FFMPEG_PATH));
      dialog.set_string_to_selected_path(ffmpegPath, dialog::FFMPEG_PATH_SET);

      if (ImGui::ImageButton("##Path Set", resources.icons[icon::FOLDER].id, icon_size_get()))
      {
        if (dialogType == dialog::PNG_DIRECTORY_SET)
          dialog.folder_open(dialogType);
        else
          dialog.file_save(dialogType);
      }
      ImGui::SameLine();
      auto pathLabel = type == render::PNGS ? LABEL_OUTPUT_DIRECTORY : LABEL_OUTPUT_PATH;
      input_text_string(localize.get(pathLabel), &path);
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_OUTPUT_PATH));
      dialog.set_string_to_selected_path(path, dialogType);

      if (ImGui::Combo(localize.get(LABEL_TYPE), &type, render::STRINGS, render::COUNT)) render_set();
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_RENDER_TYPE));

      if (type == render::PNGS || type == render::SPRITESHEET) ImGui::Separator();

      if (type == render::PNGS)
      {
        if (input_text_string(localize.get(LABEL_FORMAT), &format))
          format = std::filesystem::path(format).replace_extension(".png").string();
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_FORMAT));
      }
      else if (type == render::SPRITESHEET)
      {
        input_int_range(localize.get(LABEL_GENERATE_ROWS), rows, 1, length);
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_ROWS));

        input_int_range(localize.get(LABEL_GENERATE_COLUMNS), columns, 1, length);
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_COLUMNS));

        if (ImGui::Button(localize.get(LABEL_SET_TO_RECOMMENDED))) rows_columns_set();
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_SET_TO_RECOMMENDED));
      }

      ImGui::Separator();

      if (ImGui::Checkbox(localize.get(LABEL_CUSTOM_RANGE), &isRange))
      {
        range_set();
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_CUSTOM_RANGE));
      }

      ImGui::SameLine();

      ImGui::BeginDisabled(frames.empty());
      if (ImGui::Button(localize.get(LABEL_TO_SELECTED_FRAMES))) range_to_frames_set();
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_TO_SELECTED_FRAMES));
      ImGui::EndDisabled();

      ImGui::SameLine();

      if (ImGui::Button(localize.get(LABEL_TO_ANIMATION_RANGE))) range_to_animation_set();
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_TO_ANIMATION_RANGE));

      ImGui::BeginDisabled(!isRange);
      {
        input_int_range(localize.get(LABEL_START), start, 0, animation->frameNum);
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_START));
        input_int_range(localize.get(LABEL_END), end, start, animation->frameNum);
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_END));
      }
      ImGui::EndDisabled();

      ImGui::Separator();

      ImGui::Checkbox(localize.get(LABEL_RAW), &isRaw);
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_RAW));

      ImGui::BeginDisabled(!isRaw);
      {
        input_float_range(localize.get(BASIC_SCALE), scale, 1.0f, 100.0f, STEP, STEP_FAST, "%.1fx");
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_SCALE_OUTPUT));
      }
      ImGui::EndDisabled();

      ImGui::Separator();

      ImGui::Checkbox(localize.get(LABEL_SOUND), &settings.timelineIsSound);
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_SOUND));

      ImGui::Separator();

      if (ImGui::Button(localize.get(LABEL_RENDER), widgetSize))
      {
        bool isRender = true;
        if (!ffmpeg_is_executable(ffmpegPath))
        {
          toasts.push(localize.get(TOAST_INVALID_FFMPEG_PATH));
          logger.error(localize.get(TOAST_INVALID_FFMPEG_PATH, anm2ed::ENGLISH));
          isRender = false;
        }

        if (isRender && type == render::PNGS) isRender = png_directory_ensure(path);

        if (isRender)
        {
          manager.isRecordingStart = true;
          manager.progressPopup.open();
        }

        renderPopup.close();
      }
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_RENDER_BUTTON));

      ImGui::SameLine();

      if (ImGui::Button(localize.get(BASIC_CANCEL), widgetSize)) renderPopup.close();

      ImGui::EndPopup();
    }

    renderPopup.end();

    aboutPopup.trigger();

    if (ImGui::BeginPopupModal(aboutPopup.label(), &aboutPopup.isOpen, ImGuiWindowFlags_NoResize))
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
      auto applicationLabel = localize.get(LABEL_APPLICATION_NAME);
      auto versionLabel = localize.get(LABEL_APPLICATION_VERSION);

      ImGui::PushFont(resources.fonts[font::BOLD].get(), font::SIZE_LARGE);

      ImGui::SetCursorPosX((size.x - ImGui::CalcTextSize(applicationLabel).x) / 2);
      ImGui::TextUnformatted(applicationLabel);

      ImGui::SetCursorPosX((size.x - ImGui::CalcTextSize(versionLabel).x) / 2);
      ImGui::TextUnformatted(versionLabel);

      ImGui::PopFont();

      auto creditRegionPos = ImGui::GetCursorScreenPos();
      auto creditRegionSize = ImGui::GetContentRegionAvail();

      if (creditRegionSize.y > 0.0f && creditRegionSize.x > 0.0f)
      {
        auto fontSize = ImGui::GetFontSize();
        auto drawList = ImGui::GetWindowDrawList();
        auto clipMax = ImVec2(creditRegionPos.x + creditRegionSize.x, creditRegionPos.y + creditRegionSize.y);
        drawList->PushClipRect(creditRegionPos, clipMax, true);

        auto delta = ImGui::GetIO().DeltaTime;
        creditsState.spawnTimer -= delta;
        auto maxVisible = std::max(1, (int)std::floor(creditRegionSize.y / (float)fontSize));

        while (creditsState.active.size() < (size_t)maxVisible && creditsState.spawnTimer <= 0.0f)
        {
          creditsState.active.push_back({creditsState.nextIndex, 0.0f});
          creditsState.nextIndex = (creditsState.nextIndex + 1) % CREDIT_COUNT;
          creditsState.spawnTimer += CREDIT_DELAY;
        }

        auto baseY = clipMax.y - (float)fontSize;
        const auto& baseColor = ImGui::GetStyleColorVec4(ImGuiCol_Text);
        auto fadeSpan = (float)fontSize * 2.0f;

        for (auto it = creditsState.active.begin(); it != creditsState.active.end();)
        {
          it->offset += CREDIT_SCROLL_SPEED * delta;
          auto yPos = baseY - it->offset;
          if (yPos + fontSize < creditRegionPos.y)
          {
            it = creditsState.active.erase(it);
            continue;
          }

          const auto& credit = CREDITS[it->index];
          auto fontPtr = resources.fonts[credit.font].get();
          auto textSize = fontPtr->CalcTextSizeA((float)fontSize, FLT_MAX, 0.0f, credit.string);
          auto xPos = creditRegionPos.x + (creditRegionSize.x - textSize.x) * 0.5f;

          auto alpha = 1.0f;
          auto topDist = yPos - creditRegionPos.y;
          if (topDist < fadeSpan) alpha *= std::clamp(topDist / fadeSpan, 0.0f, 1.0f);
          auto bottomDist = (creditRegionPos.y + creditRegionSize.y) - (yPos + fontSize);
          if (bottomDist < fadeSpan) alpha *= std::clamp(bottomDist / fadeSpan, 0.0f, 1.0f);
          if (alpha <= 0.0f)
          {
            ++it;
            continue;
          }

          auto color = baseColor;
          color.w *= alpha;

          drawList->AddText(fontPtr, fontSize, ImVec2(xPos, yPos), ImGui::GetColorU32(color), credit.string);
          ++it;
        }

        drawList->PopClipRect();
      }

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

    if (shortcut(manager.chords[SHORTCUT_NEW], shortcut::GLOBAL)) dialog.file_save(dialog::ANM2_NEW);
    if (shortcut(manager.chords[SHORTCUT_OPEN], shortcut::GLOBAL)) dialog.file_open(dialog::ANM2_OPEN);
    if (shortcut(manager.chords[SHORTCUT_SAVE], shortcut::GLOBAL))
    {
      if (settings.fileIsWarnOverwrite)
        overwritePopup.open();
      else
        manager.save();
    }
    if (shortcut(manager.chords[SHORTCUT_SAVE_AS], shortcut::GLOBAL)) dialog.file_save(dialog::ANM2_SAVE);
    if (shortcut(manager.chords[SHORTCUT_EXIT], shortcut::GLOBAL)) isQuitting = true;
  }
}
