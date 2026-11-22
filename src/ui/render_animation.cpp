#include "ui/render_animation.h"

#include <algorithm>
#include <ranges>
#include <set>
#include <string>

#include "log.h"
#include "path.h"
#include "subprocess.h"
#include "render.h"
#include "ui/toast.h"
#include "ui/input.h"
#include "ui/layout.h"
#include "ui/shortcut.h"

void RenderAnimation::range_to_animation_set(Manager& manager, Document& document)
{
    if (auto animation = document.model.animation_get(document.reference_get().animationIndex))
    {
        manager.recordingStart = 0;
        manager.recordingEnd = animation->frameNum - 1;
    }
}

void RenderAnimation::range_to_frames_set(Manager& manager, Document& document)
{
    auto frameReferences = document.frame_references_get(Document::FrameReferenceFallback::NONE);
    auto reference = document.reference_get();
    std::erase_if(frameReferences, [&](const Reference& frameReference) {
        return frameReference.animationIndex != reference.animationIndex ||
               frameReference.itemType != reference.itemType || frameReference.itemID != reference.itemID ||
               frameReference.groupType != reference.groupType || frameReference.groupId != reference.groupId;
    });
    if (!frameReferences.empty())
    {
        if (auto item = document.model.track_get(reference))
        {
            int duration = 0;
            int frameIndex = 0;
            for (auto& frame : item->frames)
            {
                if (frameIndex == frameReferences.begin()->frameIndex)
                    manager.recordingStart = duration;
                if (frameIndex == frameReferences.rbegin()->frameIndex)
                    manager.recordingEnd = duration + frame.duration - 1;

                duration += frame.duration;
                ++frameIndex;
            }
        }
    }
}

void RenderAnimation::reset(Manager& manager, Document& document, Settings& settings)
{
    if (!manager.isRecordingRange)
        range_to_animation_set(manager, document);
    settings.renderPath.replace_extension(RENDER_EXTENSIONS[settings.renderType]);
}

void RenderAnimation::update(Manager& manager, Document& document, Resources& resources, Settings& settings,
                             Dialog& dialog)
{
    isEnd = false;

    auto animation = document.model.animation_get(document.reference_get().animationIndex);
    if (!animation)
        return;

    auto& ffmpegPath = settings.renderFFmpegPath;
    auto& path = settings.renderPath;
    auto& format = settings.renderFormat;
    auto& scale = settings.renderScale;
    auto& renderFpsMode = settings.renderFpsMode;
    auto& isUseAnimationBounds = settings.renderIsUseAnimationBounds;
    auto& isUseIsolatedAnimation = settings.renderIsUseIsolatedAnimation;
    auto& type = settings.renderType;
    auto& start = manager.recordingStart;
    auto& end = manager.recordingEnd;
    auto& rows = settings.renderRows;
    auto& columns = settings.renderColumns;
    auto& isRange = manager.isRecordingRange;
    auto reference = document.reference_get();
    auto& frameNum = animation->frameNum;
    auto animationFrameNum = std::max(frameNum, 1);
    auto frameMax = animationFrameNum - 1;
    start = std::clamp(start, 0, frameMax);
    end = std::clamp(end, start, frameMax);
    auto animationFps = std::max(document.model.info.fps, 1);
    if (renderFpsMode < FPS_ANIMATION || renderFpsMode >= FPS_COUNT)
        renderFpsMode = FPS_ANIMATION;
    auto renderFps = fps_get(renderFpsMode, animationFps, settings.playbackTickRate);
    auto render_frame_count_get = [&]() {
        if (isRange)
            return frame_count_get(start, end, animationFps, renderFps);
        return frame_count_get(0, frameMax, animationFps, renderFps);
    };
    auto renderFrameNum = render_frame_count_get();

    auto widgetSize = widget_size_with_row_get(2);

    Dialog::Type dialogType = Dialog::NONE;
    switch (type)
    {
        case RENDER_PNGS:
            dialogType = Dialog::PNG_DIRECTORY_SET;
            break;
        case RENDER_SPRITESHEET:
            dialogType = Dialog::PNG_PATH_SET;
            break;
        case RENDER_GIF:
            dialogType = Dialog::GIF_PATH_SET;
            break;
        case RENDER_WEBM:
            dialogType = Dialog::WEBM_PATH_SET;
            break;
        case RENDER_OGV:
            dialogType = Dialog::OGV_PATH_SET;
            break;
        case RENDER_MP4:
            dialogType = Dialog::MP4_PATH_SET;
            break;
        default:
            dialogType = Dialog::NONE;
            break;
    }

    if (ImGui::ImageButton("##FFmpeg Path Set", resources.icon_id_get(ICON_FOLDER), icon_size_get()))
        dialog.file_open(Dialog::FFMPEG_PATH_SET);
    ImGui::SameLine();
    input_text_path(localize.get(LABEL_FFMPEG_PATH), &ffmpegPath);
    tooltip_set(TOOLTIP_FFMPEG_PATH);

    if (dialog.is_selected(Dialog::FFMPEG_PATH_SET))
    {
        ffmpegPath = dialog.path;
        dialog.reset();
    }

    if (ImGui::ImageButton("##Path Set", resources.icon_id_get(ICON_FOLDER), icon_size_get()))
    {
        if (dialogType == Dialog::PNG_DIRECTORY_SET)
            dialog.folder_open(dialogType);
        else
            dialog.file_save(dialogType);
    }

    ImGui::SameLine();
    auto pathLabel = type == RENDER_PNGS ? LABEL_OUTPUT_DIRECTORY : LABEL_OUTPUT_PATH;
    input_text_path(localize.get(pathLabel), &path);
    tooltip_set(TOOLTIP_OUTPUT_PATH);

    if (dialog.is_selected(dialogType))
    {
        path = dialog.path;
        dialog.reset();
    }

    if (ImGui::Combo(localize.get(LABEL_TYPE), &type, RENDER_STRINGS, RENDER_COUNT))
        path.replace_extension(RENDER_EXTENSIONS[type]);

    tooltip_set(TOOLTIP_RENDER_TYPE);

    if (type == RENDER_PNGS || type == RENDER_SPRITESHEET)
        ImGui::Separator();

    if (type == RENDER_PNGS)
    {
        input_text_path(localize.get(LABEL_FORMAT), &format);
        tooltip_set(TOOLTIP_FORMAT);
    }
    else if (type == RENDER_SPRITESHEET)
    {
        input_int_range(localize.get(LABEL_GENERATE_ROWS), rows, 1, renderFrameNum);
        tooltip_set(TOOLTIP_ROWS);

        input_int_range(localize.get(LABEL_GENERATE_COLUMNS), columns, 1, renderFrameNum);
        tooltip_set(TOOLTIP_COLUMNS);
    }

    ImGui::Separator();

    ImGui::Checkbox(localize.get(LABEL_CUSTOM_RANGE), &isRange);
    tooltip_set(TOOLTIP_CUSTOM_RANGE);

    ImGui::SameLine();

    ImGui::BeginDisabled(document.frame_references_get(Document::FrameReferenceFallback::NONE).empty() ||
                         reference.itemType == TRIGGER);
    if (ImGui::Button(localize.get(LABEL_TO_SELECTED_FRAMES)))
        range_to_frames_set(manager, document);
    tooltip_set(TOOLTIP_TO_SELECTED_FRAMES);
    ImGui::EndDisabled();

    ImGui::SameLine();

    if (ImGui::Button(localize.get(LABEL_TO_ANIMATION_RANGE)))
        range_to_animation_set(manager, document);
    tooltip_set(TOOLTIP_TO_ANIMATION_RANGE);

    ImGui::BeginDisabled(!isRange);
    {
        input_int_range(localize.get(LABEL_START), start, 0, frameMax);
        tooltip_set(TOOLTIP_START);
        input_int_range(localize.get(LABEL_END), end, start, frameMax);
        tooltip_set(TOOLTIP_END);
    }
    ImGui::EndDisabled();
    renderFrameNum = render_frame_count_get();

    if (type == RENDER_SPRITESHEET)
    {
        auto layout = spritesheet_layout_get(rows, columns, renderFrameNum);
        rows = layout.rows;
        columns = layout.columns;
    }

    ImGui::Separator();

    ImGui::Checkbox(localize.get(LABEL_USE_ANIMATION_BOUNDS), &isUseAnimationBounds);
    tooltip_set(TOOLTIP_USE_ANIMATION_BOUNDS);

    ImGui::SameLine();

    ImGui::Checkbox(localize.get(LABEL_USE_ISOLATED_ANIMATION), &isUseIsolatedAnimation);
    tooltip_set(TOOLTIP_USE_ISOLATED_ANIMATION);

    ImGui::BeginDisabled(!isUseAnimationBounds);
    {
        input_float_range(localize.get(BASIC_SCALE), scale, RENDER_SCALE_MIN, RENDER_SCALE_MAX, RENDER_SCALE_STEP,
                          RENDER_SCALE_STEP, "%.0f%%");
        tooltip_set(TOOLTIP_SCALE_OUTPUT);
    }
    ImGui::EndDisabled();

    ImGui::Separator();

    const char* renderFpsOptions[] = {localize.get(LABEL_ANIMATION_FPS), localize.get(LABEL_PLAYBACK_RATE)};
    ImGui::Combo(localize.get(LABEL_RENDER_FPS), &renderFpsMode, renderFpsOptions, IM_ARRAYSIZE(renderFpsOptions));
    tooltip_set(TOOLTIP_RENDER_FPS);

    renderFps = fps_get(renderFpsMode, animationFps, settings.playbackTickRate);
    renderFrameNum = render_frame_count_get();
    if (type == RENDER_SPRITESHEET)
    {
        auto layout = spritesheet_layout_get(rows, columns, renderFrameNum);
        rows = layout.rows;
        columns = layout.columns;
    }

    ImGui::Separator();

    ImGui::Checkbox(localize.get(LABEL_SOUND), &settings.timelineIsSound);
    tooltip_set(TOOLTIP_SOUND);

    ImGui::Separator();

    shortcut(manager.chords[SHORTCUT_CONFIRM]);
    if (ImGui::Button(localize.get(LABEL_RENDER), widgetSize))
    {
        path.replace_extension(RENDER_EXTENSIONS[type]);

        auto render_is_valid = [&]() {
            auto ffmpeg_is_valid = [&]() {
                if (!is_path_executable(ffmpegPath))
                    return false;

                auto testCommand = std::string("\"") + path_to_utf8(ffmpegPath) + "\" -version";
                Process process(testCommand.c_str(), "r");
                auto result = process.output_get_and_close();

                if (!result.contains("ffmpeg version"))
                    return false;

                return true;
            };

            auto ffmpeg_valid_check = [&]() {
                if (!ffmpeg_is_valid())
                {
                    toast_log(Level::ERROR, TOAST_INVALID_FFMPEG);
                    return false;
                }
                return true;
            };

            auto png_format_valid_check = [&]() {
                auto formatString = path_to_utf8(format);
                if (!formatString.contains("{}"))
                {
                    toast_log(Level::ERROR, TOAST_PNG_FORMAT_INVALID);
                    return false;
                }
                return true;
            };

            auto path_valid_check = [&]() {
                if (path.empty())
                {
                    toast_log(Level::ERROR, TOAST_RENDER_PATH_EMPTY);
                    return false;
                }
                return true;
            };

            auto png_directory_valid_check = [&]() {
                if (!path_ensure_directory(path))
                {
                    toast_log(Level::ERROR, TOAST_PNG_DIRECTORY_INVALID);
                    return false;
                }
                return true;
            };

            auto spritesheet_valid_check = [&]() {
                if (rows <= 0 && columns <= 0)
                {
                    toast_log(Level::ERROR, TOAST_RENDER_PATH_EMPTY);
                    return false;
                }
                return true;
            };

            if (!path_valid_check())
                return false;

            switch (type)
            {
                case RENDER_PNGS:
                    if (!png_format_valid_check())
                        return false;
                    if (!png_directory_valid_check())
                        return false;
                    format.replace_extension(RENDER_EXTENSIONS[RENDER_SPRITESHEET]);
                    break;
                case RENDER_SPRITESHEET:
                    if (!spritesheet_valid_check())
                        return false;
                    break;
                case RENDER_GIF:
                case RENDER_WEBM:
                case RENDER_OGV:
                case RENDER_MP4:
                    if (!ffmpeg_valid_check())
                        return false;
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

    tooltip_set(TOOLTIP_RENDER_BUTTON);

    ImGui::SameLine();

    shortcut(manager.chords[SHORTCUT_CANCEL]);
    if (ImGui::Button(localize.get(BASIC_CANCEL), widgetSize))
        isEnd = true;
}
