#include "render_animation.h"

#include <ranges>
#include <string>

#include "log.h"
#include "path_.h"
#include "process_.h"
#include "toast.h"

using namespace anm2ed::resource;
using namespace anm2ed::util;

namespace anm2ed::imgui::wizard
{
  void RenderAnimation::range_to_animation_set(Manager& manager, Document& document)
  {
    if (auto animation = document.animation_get())
    {
      manager.recordingStart = 0;
      manager.recordingEnd = animation->frameNum - 1;
    }
  }

  void RenderAnimation::range_to_frames_set(Manager& manager, Document& document)
  {
    auto& frames = document.frames.selection;
    if (!frames.empty())
    {
      if (auto item = document.item_get())
      {
        int duration{};
        for (auto [i, frame] : std::views::enumerate(item->frames))
        {
          if ((int)i == *frames.begin()) manager.recordingStart = duration;
          if ((int)i == *frames.rbegin()) manager.recordingEnd = duration + frame.duration - 1;

          duration += frame.duration;
        }
      }
    }
  }

  void RenderAnimation::reset(Manager& manager, Document& document, Settings& settings)
  {
    if (!manager.isRecordingRange) range_to_animation_set(manager, document);
    settings.renderPath.replace_extension(render::EXTENSIONS[settings.renderType]);
  }

  void RenderAnimation::update(Manager& manager, Document& document, Resources& resources, Settings& settings,
                               Dialog& dialog)
  {
    isEnd = false;

    auto animation = document.animation_get();
    if (!animation) return;

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
    auto& frames = document.frames.selection;
    auto& reference = document.reference;
    auto& frameNum = animation->frameNum;

    auto widgetSize = widget_size_with_row_get(2);
    auto dialogType = type == render::PNGS          ? Dialog::PNG_DIRECTORY_SET
                      : type == render::SPRITESHEET ? Dialog::PNG_PATH_SET
                      : type == render::GIF         ? Dialog::GIF_PATH_SET
                      : type == render::WEBM        ? Dialog::WEBM_PATH_SET
                                                    : Dialog::NONE;

    if (ImGui::ImageButton("##FFmpeg Path Set", resources.icons[icon::FOLDER].id, icon_size_get()))
      dialog.file_open(Dialog::FFMPEG_PATH_SET);
    ImGui::SameLine();
    input_text_path(localize.get(LABEL_FFMPEG_PATH), &ffmpegPath);
    ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_FFMPEG_PATH));

    if (dialog.is_selected(Dialog::FFMPEG_PATH_SET))
    {
      ffmpegPath = dialog.path;
      dialog.reset();
    }

    if (ImGui::ImageButton("##Path Set", resources.icons[icon::FOLDER].id, icon_size_get()))
    {
      if (dialogType == Dialog::PNG_DIRECTORY_SET)
        dialog.folder_open(dialogType);
      else
        dialog.file_save(dialogType);
    }
    ImGui::SameLine();
    auto pathLabel = type == render::PNGS ? LABEL_OUTPUT_DIRECTORY : LABEL_OUTPUT_PATH;
    input_text_path(localize.get(pathLabel), &path);
    ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_OUTPUT_PATH));

    if (dialog.is_selected(dialogType))
    {
      path = dialog.path;
      dialog.reset();
    }

    if (ImGui::Combo(localize.get(LABEL_TYPE), &type, render::STRINGS, render::COUNT))
      path.replace_extension(render::EXTENSIONS[type]);

    ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_RENDER_TYPE));

    if (type == render::PNGS || type == render::SPRITESHEET) ImGui::Separator();

    if (type == render::PNGS)
    {
      input_text_path(localize.get(LABEL_FORMAT), &format);
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_FORMAT));
    }
    else if (type == render::SPRITESHEET)
    {
      input_int_range(localize.get(LABEL_GENERATE_ROWS), rows, 1, frameNum - 1);
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_ROWS));

      input_int_range(localize.get(LABEL_GENERATE_COLUMNS), columns, 1, frameNum - 1);
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_COLUMNS));
    }

    ImGui::Separator();

    ImGui::Checkbox(localize.get(LABEL_CUSTOM_RANGE), &isRange);
    ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_CUSTOM_RANGE));

    ImGui::SameLine();

    ImGui::BeginDisabled(frames.empty() || reference.itemID == anm2::TRIGGER);
    if (ImGui::Button(localize.get(LABEL_TO_SELECTED_FRAMES))) range_to_frames_set(manager, document);
    ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_TO_SELECTED_FRAMES));
    ImGui::EndDisabled();

    ImGui::SameLine();

    if (ImGui::Button(localize.get(LABEL_TO_ANIMATION_RANGE))) range_to_animation_set(manager, document);
    ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_TO_ANIMATION_RANGE));

    ImGui::BeginDisabled(!isRange);
    {
      input_int_range(localize.get(LABEL_START), start, 0, frameNum - 1);
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_START));
      input_int_range(localize.get(LABEL_END), end, start, frameNum - 1);
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
      path.replace_extension(render::EXTENSIONS[type]);

      auto render_is_valid = [&]()
      {
        auto ffmpeg_is_valid = [&]()
        {
          if (!path::is_executable(ffmpegPath)) return false;

          auto testCommand = std::string("\"") + path::to_utf8(ffmpegPath) + "\" -version";
          Process process(testCommand.c_str(), "r");
          auto result = process.output_get_and_close();

          if (!result.contains("ffmpeg version")) return false;

          return true;
        };

        auto ffmpeg_valid_check = [&]()
        {
          if (!ffmpeg_is_valid())
          {
            toasts.push(localize.get(TOAST_INVALID_FFMPEG));
            logger.error(localize.get(TOAST_INVALID_FFMPEG, anm2ed::ENGLISH));
            return false;
          }
          return true;
        };

        auto png_format_valid_check = [&]()
        {
          auto formatString = path::to_utf8(format);
          if (!formatString.contains("{}"))
          {
            toasts.push(localize.get(TOAST_PNG_FORMAT_INVALID));
            logger.error(localize.get(TOAST_PNG_FORMAT_INVALID, anm2ed::ENGLISH));
          }
          return true;
        };

        auto path_valid_check = [&]()
        {
          if (path.empty())
          {
            toasts.push(localize.get(TOAST_RENDER_PATH_EMPTY));
            logger.error(localize.get(TOAST_RENDER_PATH_EMPTY, anm2ed::ENGLISH));
            return false;
          }
          return true;
        };

        auto png_directory_valid_check = [&]()
        {
          if (!path::ensure_directory(path))
          {
            toasts.push(localize.get(TOAST_PNG_DIRECTORY_INVALID));
            logger.error(localize.get(TOAST_PNG_DIRECTORY_INVALID, anm2ed::ENGLISH));
            return false;
          }
          return true;
        };

        auto spritesheet_valid_check = [&]()
        {
          if (rows <= 0 && columns <= 0)
          {
            toasts.push(localize.get(TOAST_RENDER_PATH_EMPTY));
            logger.error(localize.get(TOAST_RENDER_PATH_EMPTY, anm2ed::ENGLISH));
            return false;
          }
          return true;
        };

        if (!path_valid_check()) return false;

        switch (type)
        {
          case render::PNGS:
            if (!png_directory_valid_check()) return false;
            if (!png_format_valid_check()) return false;
            format.replace_extension(render::EXTENSIONS[render::SPRITESHEET]);
            break;
          case render::SPRITESHEET:
            if (!spritesheet_valid_check()) return false;
            break;
          case render::GIF:
          case render::WEBM:
          case render::MP4:
            if (!ffmpeg_valid_check()) return false;
            break;
          default:
            return false;
            break;
        }

        return true;
      };

      if (render_is_valid())
      {
        manager.isRecordingStart = true;
        manager.progressPopup.open();
      }

      isEnd = true;
    }

    ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_RENDER_BUTTON));

    ImGui::SameLine();

    if (ImGui::Button(localize.get(BASIC_CANCEL), widgetSize)) isEnd = true;
  }
}
