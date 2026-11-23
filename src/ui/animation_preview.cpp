#include "ui/animation_preview.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <format>
#include <map>
#include <optional>
#include <string>
#include <system_error>
#include <vector>

#include "vec.h"

#include "ui/actions.h"
#include "log.h"
#include "math_util.h"
#include "model/draw.h"
#include "model/frames.h"
#include "path.h"
#include "localization.h"
#include "ui/toast.h"
#include "tool.h"
#include "types.h"
#include "ui/draw.h"
#include "ui/input.h"
#include "ui/layout.h"
#include "ui/shortcut.h"
#include "ui/timeline.h"

constexpr auto NULL_RECT_COLOR = Vec4(0.0f, 0.0f, 1.0f, 0.90f);
constexpr auto SELECTED_LAYER_BORDER_COLOR = Vec4(1.0f, 0.1059f, 0.5882f, 1.0f);
constexpr auto TARGET_SIZE = Vec2(32, 32);
constexpr auto PARENT_DIM_ALPHA = 0.65f; // how much a stepped-into child's parent fades
constexpr auto POINT_SIZE = Vec2(4, 4);
constexpr auto TRIGGER_TEXT_COLOR_DARK = ImVec4(1.0f, 1.0f, 1.0f, 0.5f);
constexpr auto TRIGGER_TEXT_COLOR_LIGHT = ImVec4(0.0f, 0.0f, 0.0f, 0.5f);
struct OverlayAnimationOption
{
    uint64_t documentId = 0;
    int animationIndex = -1;
    std::string label;
};

static std::vector<OverlayAnimationOption> overlay_animation_options_get(Manager& manager)
{
    std::vector<OverlayAnimationOption> options{{.label = std::string(localize.get(BASIC_NONE))}};

    for (auto& document : manager.documents)
    {
        auto documentLabel = path_to_utf8(document.filename_get());
        for (auto [animationIndex, animation] : document.model.animations_get())
        {
            auto label = std::format("{} ({})", animation->name, documentLabel);
            options.push_back({.documentId = document.tabId, .animationIndex = animationIndex, .label = label});
        }
    }

    return options;
}

static std::vector<const char*> overlay_animation_option_labels_get(std::vector<OverlayAnimationOption>& options)
{
    std::vector<const char*> labels;
    labels.reserve(options.size());
    for (auto& option : options)
        labels.push_back(option.label.c_str());
    return labels;
}

static int overlay_animation_option_index_get(const Document& document,
                                              const std::vector<OverlayAnimationOption>& options)
{
    if (document.overlayIndex == -1)
        return 0;
    auto documentId = document.overlayDocumentId ? document.overlayDocumentId : document.tabId;
    for (int i = 1; i < (int)options.size(); ++i)
        if (options[i].documentId == documentId && options[i].animationIndex == document.overlayIndex)
            return i;
    return -1;
}

static Document* overlay_animation_document_get(Manager& manager, const Document& document)
{
    if (document.overlayIndex == -1)
        return nullptr;
    auto documentId = document.overlayDocumentId ? document.overlayDocumentId : document.tabId;
    for (auto& candidate : manager.documents)
        if (candidate.tabId == documentId)
            return &candidate;
    return nullptr;
}

static std::optional<Vec4> animation_rects_merge(std::optional<Vec4> rect, Vec4 next)
{
    if (next == Vec4(-1.0f))
        return rect;
    if (!rect)
        return next;

    auto minPoint = vec_min(Vec2(rect->x, rect->y), Vec2(next.x, next.y));
    auto maxPoint = vec_max(Vec2(rect->x + rect->z, rect->y + rect->w), Vec2(next.x + next.z, next.y + next.w));
    return Vec4(minPoint, maxPoint - minPoint);
}

static std::optional<Vec4> animation_render_rect_get(Manager& manager, Document& document, const Animation* animation,
                                                     bool isRootTransform)
{
    std::optional<Vec4> rect{};

    if (animation)
        rect = animation_rects_merge(
            rect, animation_rect(document.model, *animation, isRootTransform, document.library_resolver_get()));

    if (auto overlayDocument = overlay_animation_document_get(manager, document))
        if (auto overlayAnimation = overlayDocument->model.animation_get(document.overlayIndex))
            rect =
                animation_rects_merge(rect, animation_rect(overlayDocument->model, *overlayAnimation, isRootTransform,
                                                           overlayDocument->library_resolver_get()));

    return rect;
}

void AnimationPreview::tick(Manager& manager, Settings& settings, float deltaSeconds)
{
    auto& document = *manager.get();
    auto& model = document.model;
    auto& playback = document.playback;
    auto& frameTime = document.frameTime;

    auto stop_all_sounds = [&]() {
        for (auto sound : document.sounds_get())
            audio_stop(*sound);
    };

    if (manager.isRecording)
    {
        if (recorder.is_done())
        {
            recorder.finish();
            recording_stop(manager, document);
        }
        stop_all_sounds();
        isPlaybackPlayingBefore = false;
        return;
    }

    if (playback.isPlaying)
    {
        auto animation = model.animation_get(document.reference_get().animationIndex);
        auto& isSound = settings.timelineIsSound;

        if (!animation)
        {
            playback.isPlaying = false;
            playback.isFinished = false;
        }
        else
        {
            auto fps = std::max(model.info.fps, 1);
            auto range = animation_play_range_get(*animation);
            if (!isPlaybackPlayingBefore)
                soundFrame = (int)std::floor(playback.time) - 1;
            auto isLoop = (animation->isLoop || settings.playbackIsLoop) && !manager.isRecording;
            playback.tick(fps, range.x, range.y, isLoop ? PLAYBACK_LOOP : PLAYBACK_ONCE, deltaSeconds);

            // Each frame playback reaches plays its trigger's sound once, however many ticks it lasts or skips.
            auto frame = (int)std::floor(playback.time);
            for (int step = 0; soundFrame != frame && step < range.y - range.x; ++step)
            {
                soundFrame = soundFrame + 1 >= range.y || soundFrame + 1 < range.x ? range.x : soundFrame + 1;
                if (manager.isRecording || !isSound)
                    continue;
                for (auto sound : document.trigger_sounds_get(*animation, soundFrame))
                    audio_play(*sound);
            }

            frameTime = playback.time;
        }
    }

    if (isPlaybackPlayingBefore && !playback.isPlaying)
        stop_all_sounds();
    isPlaybackPlayingBefore = playback.isPlaying;
}

void AnimationPreview::recording_stop(Manager& manager, Document& document)
{
    document.playback.isPlaying = false;
    document.playback.isFinished = false;
    manager.isRecording = false;
    manager.isRecordingStart = false;
    manager.progressPopup.close();
}

// Starts a render from the render settings. A bounded render frames the animation (and overlay) at the render scale.
void AnimationPreview::recording_start(Manager& manager, Settings& settings, Document& document,
                                       const Animation* animation)
{
    auto animationFps = std::max(document.model.info.fps, 1);
    RecorderOptions options{.type = (RenderType)settings.renderType,
                            .path = settings.renderPath,
                            .format = settings.renderFormat,
                            .ffmpegPath = settings.renderFFmpegPath,
                            .start = manager.recordingStart,
                            .end = manager.recordingEnd,
                            .animationFps = animationFps,
                            .fps = fps_get(settings.renderFpsMode, animationFps, settings.playbackTickRate),
                            .rows = settings.renderRows,
                            .columns = settings.renderColumns,
                            .isSound = settings.timelineIsSound,
                            .isIsolated = settings.renderIsUseIsolatedAnimation,
                            .isBounded = settings.renderIsUseAnimationBounds};
    if (options.isBounded)
    {
        recordSize = size;
        recordZoom = document.previewZoom;
        recordPan = document.previewPan;
        if (auto rect = animation_render_rect_get(manager, document, animation, settings.previewIsRootTransform))
        {
            recordSize = Vec2(rect->z, rect->w) * percent_to_unit(settings.renderScale);
            auto previousSize = size;
            size = recordSize;
            set_to_rect(recordZoom, recordPan, *rect);
            size = previousSize;
        }
    }

    manager.isRecordingStart = false;
    manager.isRecording = true;
    if (!recorder.start(options))
    {
        toast_log(Level::ERROR, TOAST_EXPORT_RENDERED_ANIMATION_FAILED, path_to_utf8(options.path));
        return recording_stop(manager, document);
    }
    document.playback.isPlaying = true;
    document.playback.time = recorder.time_get();
    document.frameTime = document.playback.time;
}

void AnimationPreview::update(Manager& manager, Settings& settings, Resources& resources)
{
    isFocused = false;

    auto& document = *manager.get();
    auto& model = document.model;
    auto& playback = document.playback;
    auto reference = document.reference_get();
    auto animation = model.animation_get(reference.animationIndex);
    auto& pan = document.previewPan;
    auto& zoom = document.previewZoom;
    auto& backgroundColor = settings.previewBackgroundColor;
    auto& isTransparent = settings.animationPreviewTransparent;
    auto& axesColor = settings.previewAxesColor;
    auto& gridColor = settings.previewGridColor;
    auto& gridSize = settings.previewGridSize;
    auto& gridOffset = settings.previewGridOffset;
    auto& isGrid = settings.previewIsGrid;
    auto& overlayTransparency = settings.previewOverlayTransparency;
    auto& overlayDrawOrder = settings.previewOverlayDrawOrder;
    auto& overlayIndex = document.overlayIndex;
    auto& overlayDocumentId = document.overlayDocumentId;
    auto& isRootTransform = settings.previewIsRootTransform;
    auto& isPivots = settings.previewIsPivots;
    auto& isAxes = settings.previewIsAxes;
    auto& isAltIcons = settings.previewIsAltIcons;
    auto& isBorder = settings.previewIsBorder;
    auto& tool = settings.tool;
    auto& isOnlyShowLayers = settings.timelineIsOnlyShowLayers;
    auto& shaderLine = resources.shaders[SHADER_LINE];
    bool isLightTheme = settings.theme == THEME_LIGHT;
    auto& shaderAxes = resources.shaders[SHADER_AXIS];
    auto& shaderGrid = resources.shaders[SHADER_GRID];
    auto& shaderTexture = resources.shaders[SHADER_TEXTURE];
    auto& shaderDashed = resources.shaders[SHADER_DASHED];
    auto center_view = [&]() {
        pan = Vec2();
    };

    auto fit_view = [&]() {
        if (animation)
            set_to_rect(zoom, pan, animation_rect(model, *animation, isRootTransform, document.library_resolver_get()));
    };

    manager.isAbleToRecord = false;

    if (ImGui::Begin(localize.get(LABEL_ANIMATION_PREVIEW_WINDOW), &settings.windowIsAnimationPreview))
    {
        isFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
        manager.isAbleToRecord = true;

        auto childSize = ImVec2(row_widget_width_get(4),
                                (ImGui::GetTextLineHeightWithSpacing() * 4) + (ImGui::GetStyle().WindowPadding.y * 2));

        grid_child_draw(childSize, isGrid, gridColor, gridSize, gridOffset);

        ImGui::SameLine();

        if (ImGui::BeginChild("##View Child", childSize, true, ImGuiWindowFlags_HorizontalScrollbar))
        {
            ImGui::InputFloat(localize.get(BASIC_ZOOM), &zoom, 0.0f, 0.0f, "%.0f%%");
            tooltip_set(TOOLTIP_PREVIEW_ZOOM);

            view_buttons_draw(manager, settings, center_view, fit_view);

            auto readoutSize = ImVec2(row_widget_width_get(2), ImGui::GetTextLineHeightWithSpacing());
            auto mousePosInt = IVec2(mousePos);
            auto positionText = localize_format(FORMAT_POSITION_SPACED, mousePosInt.x, mousePosInt.y);
            if (ImGui::BeginChild("##Position Readout", readoutSize, false, ImGuiWindowFlags_NoScrollbar))
            {
                ImGui::TextUnformatted(positionText.c_str());
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", positionText.c_str());
            }
            ImGui::EndChild();

            ImGui::SameLine();

            auto sizeInt = IVec2(size);
            auto sizeText = localize_format(FORMAT_SIZE_SPACED, sizeInt.x, sizeInt.y);
            if (ImGui::BeginChild("##Size Readout", readoutSize, false, ImGuiWindowFlags_NoScrollbar))
            {
                ImGui::TextUnformatted(sizeText.c_str());
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", sizeText.c_str());
            }
            ImGui::EndChild();
        }
        ImGui::EndChild();

        ImGui::SameLine();

        if (ImGui::BeginChild("##Background Child", childSize, true, ImGuiWindowFlags_HorizontalScrollbar))
        {
            ImGui::BeginDisabled(isTransparent);
            {
                ImGui::ColorEdit3(localize.get(LABEL_BACKGROUND_COLOR), values_get(backgroundColor),
                                  ImGuiColorEditFlags_NoInputs);
                tooltip_set(TOOLTIP_BACKGROUND_COLOR);
            }
            ImGui::EndDisabled();
            ImGui::SameLine();
            checkbox_draw(LABEL_AXES, isAxes, TOOLTIP_AXES);
            ImGui::SameLine();
            ImGui::ColorEdit4(localize.get(BASIC_COLOR), values_get(axesColor), ImGuiColorEditFlags_NoInputs);
            tooltip_set(TOOLTIP_AXES_COLOR);

            auto overlayOptions = overlay_animation_options_get(manager);
            auto overlayLabels = overlay_animation_option_labels_get(overlayOptions);
            auto overlayOptionIndex = overlay_animation_option_index_get(document, overlayOptions);
            if (overlayOptionIndex == -1)
            {
                overlayIndex = -1;
                overlayDocumentId = 0;
                overlayOptionIndex = 0;
            }
            if (ImGui::Combo(localize.get(LABEL_OVERLAY), &overlayOptionIndex, overlayLabels.data(),
                             (int)overlayLabels.size()))
            {
                auto& option = overlayOptions[overlayOptionIndex];
                overlayIndex = option.animationIndex;
                overlayDocumentId = overlayIndex == -1 ? 0 : option.documentId;
            }
            tooltip_set(TOOLTIP_OVERLAY);

            overlayDrawOrder = vec_clamp(overlayDrawOrder, (int)OVERLAY_OVER, (int)OVERLAY_UNDER);
            overlayTransparency = vec_clamp(overlayTransparency, 0.0f, 100.0f);
            auto orderIconSize = icon_size_get();
            auto orderButtonWidth = orderIconSize.x + ImGui::GetStyle().FramePadding.x * 2.0f;
            auto alphaWidth = std::max(ImGui::GetContentRegionAvail().x - (orderButtonWidth * 2.0f) -
                                           (ImGui::GetStyle().ItemSpacing.x * 2.0f),
                                       ImGui::GetFrameHeight());
            auto overlayIconTint = isLightTheme ? ImVec4(0.0f, 0.0f, 0.0f, 1.0f) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
            ImGui::PushItemWidth(alphaWidth);
            ImGui::DragFloat("##Overlay Alpha", &overlayTransparency, DRAG_SPEED, 0, 100, "%.0f%%");
            ImGui::PopItemWidth();
            tooltip_set(TOOLTIP_OVERLAY_ALPHA);
            overlayTransparency = vec_clamp(overlayTransparency, 0.0f, 100.0f);

            ImGui::SameLine();
            radio_button_icon("##Overlay Over", &overlayDrawOrder, OVERLAY_OVER, resources.icon_id_get(ICON_OVER),
                              orderIconSize, overlayIconTint);
            tooltip_set(TOOLTIP_OVERLAY_OVER);

            ImGui::SameLine();
            radio_button_icon("##Overlay Under", &overlayDrawOrder, OVERLAY_UNDER, resources.icon_id_get(ICON_UNDER),
                              orderIconSize, overlayIconTint);
            tooltip_set(TOOLTIP_OVERLAY_UNDER);
        }
        ImGui::EndChild();

        ImGui::SameLine();

        if (ImGui::BeginChild("##Helpers Child", childSize, true, ImGuiWindowFlags_HorizontalScrollbar))
        {
            auto helpersChildSize = ImVec2(row_widget_width_get(2), ImGui::GetContentRegionAvail().y);

            if (ImGui::BeginChild("##Helpers Child 1", helpersChildSize))
            {
                checkbox_draw(LABEL_ROOT_TRANSFORM, isRootTransform, TOOLTIP_ROOT_TRANSFORM);
                checkbox_draw(LABEL_PIVOTS, isPivots, TOOLTIP_PIVOTS);
                checkbox_draw(LABEL_TRANSPARENT, isTransparent, TOOLTIP_PREVIEW_TRANSPARENT);
            }
            ImGui::EndChild();

            ImGui::SameLine();

            if (ImGui::BeginChild("##Helpers Child 2", helpersChildSize))
            {
                checkbox_draw(LABEL_ALT_ICONS, isAltIcons, TOOLTIP_ALT_ICONS);
                checkbox_draw(LABEL_BORDER, isBorder, TOOLTIP_BORDER);
            }
            ImGui::EndChild();
        }
        ImGui::EndChild();

        if (manager.isRecordingStart)
            recording_start(manager, settings, document, animation);
        auto isRecordingFrame = manager.isRecording && !recorder.is_done();
        if (isRecordingFrame)
        {
            playback.isPlaying = true;
            playback.isFinished = false;
            playback.time = recorder.time_get();
            document.frameTime = playback.time;
        }

        // A render of an isolated animation hides helpers, overlay and onion skin. A bounded one draws its own view.
        auto isIsolated = manager.isRecording && recorder.options.isIsolated;
        auto isBounded = manager.isRecording && recorder.options.isBounded;
        auto viewZoom = isBounded ? recordZoom : zoom;
        auto viewPan = isBounded ? recordPan : pan;
        auto isViewTransparent = isTransparent || isIsolated;
        auto isViewOnlyLayers = isOnlyShowLayers || isIsolated;

        if (isBounded)
            size_set(recordSize);
        else
        {
            auto nextSize = to_vec2(ImGui::GetContentRegionAvail());
            bool isCanvasResized = IVec2(nextSize) != IVec2(size);
            size_set(nextSize);
            if (isCanvasResized && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
            {
                auto resizeSizeInt = IVec2(size);
                auto resizeSizeText = std::vformat(localize.get(FORMAT_SIZE_SPACED),
                                                   std::make_format_args(resizeSizeInt.x, resizeSizeInt.y));
                ImGui::SetTooltip("%s", resizeSizeText.c_str());
            }
        }

        auto cursorScreenPos = ImGui::GetCursorScreenPos();
        auto min = cursorScreenPos;
        auto max = to_imvec2(to_vec2(min) + size);
        auto referenceItemType = static_cast<ItemType>(reference.itemType);

        if (is_canvas_hovered(min, max))
        {
            auto input = canvas_input_get(manager, isFocused);
            mousePos = position_translate(zoom, pan, to_vec2(ImGui::GetMousePos()) - to_vec2(cursorScreenPos));
            auto selectedFrameReferences = document.frame_references_get();
            std::erase_if(selectedFrameReferences, [](const Reference& frameReference) {
                return frameReference.itemType == NONE || frameReference.itemType == TRIGGER;
            });
            auto editReference = reference;
            if ((referenceItemType == ItemType::TRIGGER || !selectedFrameReferences.contains(reference)) &&
                !selectedFrameReferences.empty())
                editReference = *selectedFrameReferences.begin();

            auto frame = selectedFrameReferences.empty() ? nullptr : model.frame_get(editReference);
            auto selectedNull =
                editReference.itemType == NULL_ ? item_get(model.content.nulls, editReference.itemID) : nullptr;
            bool isSelectedNullRect = selectedNull && selectedNull->isShowRect;
            auto null_rect_top_left = [](const Frame& frame) {
                return frame.position - (frame.scale * 0.5f);
            };

            auto useTool = (Tool)tool;
            if (input.isMiddleDown)
                useTool = TOOL_PAN;
            if (tool == TOOL_MOVE && input.isRightDown)
                useTool = TOOL_SCALE;
            if (tool == TOOL_SCALE && input.isRightDown)
                useTool = TOOL_MOVE;

            // A tool swapped in by the right button is driven by the right button.
            auto isSwapped = useTool != tool && useTool != TOOL_PAN;
            auto isToolClicked = isSwapped ? input.isRightClicked : input.isLeftClicked;
            auto isToolDown = isSwapped ? input.isRightDown : input.isLeftDown;
            auto isToolReleased = isSwapped ? input.isRightReleased : input.isLeftReleased;
            auto isBegin = isToolClicked || input.isArrowBegin;
            auto isDuring = isToolDown || input.isArrowDown;
            auto isEnd = isToolReleased || input.isArrowEnd;
            auto arrow = input.arrow * input.step;
            auto isArrow = arrow != Vec2();

            // Changes apply to every selected frame. Absolute values are applied as the difference from the edited
            // frame.
            auto frames_change = [&](FrameChange change, ChangeType type = ChangeType::ADD) {
                frames_change_push(manager, selectedFrameReferences, change, type);
            };
            auto null_rect_set = [&](Vec2 minPoint, Vec2 maxPoint) {
                auto rectSize = maxPoint - minPoint;
                frames_change({.positionX = minPoint.x + rectSize.x * 0.5f - frame->position.x,
                               .positionY = minPoint.y + rectSize.y * 0.5f - frame->position.y,
                               .scaleX = rectSize.x - frame->scale.x,
                               .scaleY = rectSize.y - frame->scale.y});
            };

            tool_cursor_update(useTool, TOOL_AREA_ANIMATION_PREVIEW, frame != nullptr, true,
                               input.isLeftDown || input.isRightDown || input.isMiddleDown || input.isArrowDown,
                               TEXT_SELECT_FRAME);
            if (useTool != TOOL_MOVE)
                isMoveDragging = false;
            if (useTool == TOOL_PAN && (input.isLeftDown || input.isRightDown || input.isMiddleDown))
                pan += input.mouseDelta;

            if (frame && useTool == TOOL_MOVE)
            {
                if (isBegin)
                    edit_begin_push(manager, EDIT_FRAME_POSITION);
                if (isToolClicked)
                {
                    auto origin = isSelectedNullRect ? null_rect_top_left(*frame) : frame->position;
                    moveOffset = settings.inputIsMoveToolSnapToMouse ? Vec2() : mousePos - origin;
                    isMoveDragging = true;
                }
                if (isToolDown && isMoveDragging)
                {
                    auto position =
                        Vec2(IVec2(mousePos - moveOffset + (isSelectedNullRect ? frame->scale * 0.5f : Vec2())));
                    frames_change(
                        {.positionX = position.x - frame->position.x, .positionY = position.y - frame->position.y});
                }
                if (isArrow)
                    frames_change({.positionX = arrow.x, .positionY = arrow.y});
                if (isToolReleased)
                    isMoveDragging = false;
                if (isDuring)
                    tooltip_lines_draw({localize_format(FORMAT_POSITION, frame->position.x, frame->position.y)});
                if (isEnd)
                    document_change_push(manager);
            }

            if (frame && useTool == TOOL_SCALE)
            {
                if (isBegin)
                    edit_begin_push(manager, EDIT_FRAME_SCALE);
                if (isToolClicked && isSelectedNullRect)
                    nullRectScaleAnchor = null_rect_top_left(*frame);
                if (isToolDown && isSelectedNullRect)
                {
                    auto rectSize = mousePos - nullRectScaleAnchor;
                    if (input.isMod)
                    {
                        auto squareSize = std::max(std::abs(rectSize.x), std::abs(rectSize.y));
                        rectSize = {std::copysign(squareSize, rectSize.x), std::copysign(squareSize, rectSize.y)};
                    }
                    auto [minPoint, maxPoint] =
                        rect_snap(nullRectScaleAnchor, nullRectScaleAnchor + rectSize, false, {}, {});
                    null_rect_set(minPoint, maxPoint);
                }
                else if (isToolDown)
                    frames_change({.scaleX = input.mouseDelta.x,
                                   .scaleY = input.isMod ? frame->scale.x + input.mouseDelta.x - frame->scale.y
                                                         : input.mouseDelta.y});
                if (isArrow)
                    frames_change({.positionX = isSelectedNullRect ? arrow.x * 0.5f : 0.0f,
                                   .positionY = isSelectedNullRect ? arrow.y * 0.5f : 0.0f,
                                   .scaleX = arrow.x,
                                   .scaleY = arrow.y});
                if (isDuring)
                    tooltip_lines_draw({localize_format(FORMAT_SCALE, frame->scale.x, frame->scale.y)});
                if (isEnd && isSelectedNullRect)
                {
                    auto topLeft = null_rect_top_left(*frame);
                    auto [minPoint, maxPoint] = rect_snap(topLeft, topLeft + frame->scale, false, {}, {});
                    null_rect_set(minPoint, maxPoint);
                }
                if (isEnd)
                    document_change_push(manager);
            }

            if (frame && useTool == TOOL_ROTATE)
            {
                if (isBegin)
                    edit_begin_push(manager, EDIT_FRAME_ROTATION);
                if (isToolDown)
                    frames_change({.rotation = input.mouseDelta.x});
                if (auto turn = vec_sign(input.arrow.x - input.arrow.y); turn != 0.0f)
                    frames_change({.rotation = turn * input.step});
                if (isDuring)
                    tooltip_lines_draw({localize_format(FORMAT_ROTATION, frame->rotation)});
                if (isEnd)
                    document_change_push(manager);
            }

            if (frame && useTool == TOOL_SKEW)
            {
                if (isBegin)
                    edit_begin_push(manager, EDIT_FRAME_SKEW);
                if (isToolDown)
                {
                    auto delta = input.mouseDelta;
                    if (input.isMod)
                        (std::abs(delta.x) >= std::abs(delta.y) ? delta.y : delta.x) = 0.0f;
                    frames_change({.skewX = delta.x, .skewY = delta.y});
                }
                if (isArrow)
                    frames_change({.skewX = arrow.x, .skewY = arrow.y});
                if (isDuring)
                    tooltip_lines_draw({localize_format(FORMAT_SKEW, frame->skew.x, frame->skew.y)});
                if (isEnd)
                    document_change_push(manager);
            }

            if (input.wheel != 0 || input.isZoomIn || input.isZoomOut)
                zoom_step(zoom, pan, Vec2(mousePos),
                          (input.wheel > 0 || input.isZoomIn) ? ZOOM_LEVEL_STEP : -ZOOM_LEVEL_STEP);
        }

        bind();
        viewport_set();
        clear(isViewTransparent ? Vec4(0) : Vec4(backgroundColor, 1.0f));

        if (isAxes && !isIsolated)
            axes_render(shaderAxes, viewZoom, viewPan, axesColor);
        if (isGrid && !isIsolated)
            grid_render(shaderGrid, viewZoom, viewPan, gridSize, gridOffset, gridColor);

        auto baseTransform = transform_get(viewZoom, viewPan);
        auto frameTime = document.frameTime > -1 && !playback.isPlaying ? document.frameTime : playback.time;

        DrawOptions drawOptions{.time = frameTime,
                                .isRootTransform = isRootTransform,
                                .isIndexSampled = settings.onionskinMode == (int)OnionskinMode::INDEX};
        std::vector<Vec3> sampleColors{};
        std::vector<float> sampleAlphas;
        auto samples_add = [&](int count, int direction, Vec3 color) {
            for (int i = 1; i <= count; ++i)
            {
                drawOptions.samples.push_back({.timeOffset = (float)(direction * i), .indexOffset = direction * i});
                sampleColors.push_back(color);
                sampleAlphas.push_back((1.0f / (count + 1)) * i);
            }
        };
        if (settings.onionskinIsEnabled && !isIsolated)
        {
            samples_add(settings.onionskinBeforeCount, -1, settings.onionskinBeforeColor);
            samples_add(settings.onionskinAfterCount, 1, settings.onionskinAfterColor);
        }

        auto is_layer_selected = [&](int id) {
            auto is_layer = [&](const Reference& itemReference) {
                return itemReference.animationIndex == reference.animationIndex && itemReference.itemType == LAYER &&
                       itemReference.itemID == id;
            };
            return (reference.animationIndex != -1 && is_layer(reference)) ||
                   std::ranges::any_of(document.selected_get(SelectionKind::TRACKS), is_layer) ||
                   std::ranges::any_of(document.frame_references_get(Document::FrameReferenceFallback::NONE), is_layer);
        };

        // The item the reference points at: the root (or a group's root), a layer or a null.
        auto is_draw_selected = [&](const Draw& draw) {
            switch (draw.type)
            {
                case DrawType::ROOT:
                    return referenceItemType == ItemType::ROOT && reference.groupId == -1;
                case DrawType::GROUP_ROOT:
                    return referenceItemType == ItemType::ROOT && reference.groupType == draw.groupType &&
                           reference.groupId == draw.id;
                case DrawType::LAYER:
                    return referenceItemType == ItemType::LAYER && reference.itemID == draw.id;
                default:
                    return referenceItemType == ItemType::NULL_ && reference.itemID == draw.id;
            }
        };
        auto isOnlySelected = settings.onionskinIsOnlySelected;

        // Draws an animation's draw list. Onion-skin samples are tinted and faded, `alphaOffset` fades the overlay. A
        // parent around a stepped-into child is drawn in the child's space (`scene`), its layers only, without the
        // animation layer playing the child (`skipLayerId`).
        auto render = [&](Document& sampleDocument, const Animation& sampleAnimation, float alphaOffset,
                          const DrawOptions& options, const Mat4& scene = Mat4(1.0f), int skipLayerId = -1) {
            auto isActiveDocument = &sampleDocument == &document && skipLayerId == -1;
            auto isParent = skipLayerId != -1;
            auto& sampleModel = sampleDocument.model;
            auto targetIcon = resources.icon_id_get(isAltIcons && !isIsolated ? ICON_TARGET_ALT : ICON_TARGET);
            auto sampleOptions = options;
            sampleOptions.library_get = sampleDocument.library_resolver_get();
            for (auto& draw : animation_draws_get(sampleModel, sampleAnimation, sampleOptions))
            {
                if (isParent && (draw.type != DrawType::LAYER || draw.id == skipLayerId))
                    continue;
                auto isOnion = draw.sample != -1;
                if (isOnion && isOnlySelected && !(isActiveDocument && is_draw_selected(draw)))
                    continue;
                auto sampleColor = isOnion ? sampleColors[draw.sample] : Vec3();
                auto sampleAlpha = isOnion ? sampleAlphas[draw.sample] : 0.0f;
                auto onionColor = Vec4(sampleColor, 1.0f - sampleAlpha);
                auto& frame = draw.frame;
                auto transform = baseTransform * scene * draw.parent;
                auto marker_model_get = [&](Vec2 markerSize) {
                    return quad_model_get(markerSize, frame.position, markerSize * 0.5f, percent_to_unit(frame.scale),
                                          frame.rotation, percent_to_unit(frame.skew));
                };

                if (draw.type == DrawType::ROOT || draw.type == DrawType::GROUP_ROOT)
                {
                    if (isViewOnlyLayers)
                        continue;
                    auto isSelected = isActiveDocument && draw.type == DrawType::GROUP_ROOT &&
                                      referenceItemType == ItemType::ROOT && reference.groupType == draw.groupType &&
                                      reference.groupId == draw.id;
                    auto color = isOnion                       ? Vec4(sampleColor, sampleAlpha)
                                 : draw.type == DrawType::ROOT ? COLOR_GREEN
                                 : isSelected                  ? COLOR_RED
                                                               : ROOT_COLOR;
                    auto markerModel = isRootTransform ? quad_model_get(TARGET_SIZE, {}, TARGET_SIZE * 0.5f)
                                                       : marker_model_get(TARGET_SIZE);
                    texture_render(shaderTexture, targetIcon, transform * markerModel, color);
                }
                else if (draw.type == DrawType::LAYER)
                {
                    auto layerTexture =
                        sampleDocument.draw_image_get(draw, percent_to_unit(viewZoom) * draw_scale_get(draw));
                    if (!layerTexture || !layerTexture->is_valid() || layerTexture->size.x <= 0 ||
                        layerTexture->size.y <= 0)
                        continue;
                    auto textureSize = layerTexture->unit_size_get();
                    auto layerModel = draw_quad_model_get(draw);
                    auto layerTransform = transform * layerModel;
                    auto tint = frame.tint;
                    tint.a = std::max(0.0f, tint.a - (alphaOffset + sampleAlpha));
                    auto customShader = sampleDocument.shader_get(frame.shaderId);
                    layer_render(customShader ? *customShader : shaderTexture, texture_id_get(*layerTexture),
                                 textureSize, baseTransform * scene, draw, tint,
                                 frame.colorOffset + Vec4(sampleColor, 0.0f));
                    if (isParent)
                        continue;

                    auto color = isOnion                                          ? onionColor
                                 : isActiveDocument && is_layer_selected(draw.id) ? SELECTED_LAYER_BORDER_COLOR
                                                                                  : COLOR_RED;
                    if (isBorder && !isIsolated)
                        rect_render(shaderLine, layerTransform, layerModel, color);
                    if (isPivots && !isIsolated)
                        texture_render(shaderTexture, resources.icon_id_get(ICON_PIVOT),
                                       transform * marker_model_get(PIVOT_SIZE), color);
                }
                else if (!isViewOnlyLayers)
                {
                    auto isShowRect = item_get(sampleModel.content.nulls, draw.id)->isShowRect;
                    auto isSelected =
                        isActiveDocument && draw.id == reference.itemID && referenceItemType == ItemType::NULL_;
                    auto color = isOnion ? onionColor : isSelected ? COLOR_RED : NULL_RECT_COLOR;
                    auto markerSize = isShowRect ? POINT_SIZE : TARGET_SIZE;
                    texture_render(shaderTexture, isShowRect ? resources.icon_id_get(ICON_POINT) : targetIcon,
                                   transform * marker_model_get(markerSize), color);
                    if (!isShowRect)
                        continue;
                    auto rectModel = quad_model_get(frame.scale, frame.position, frame.scale * 0.5f, Vec2(1.0f),
                                                    frame.rotation, percent_to_unit(frame.skew));
                    rect_render(shaderLine, transform * rectModel, rectModel, color);
                }
            }
        };

        if (animation)
        {
            auto overlay_render = [&]() {
                if (auto overlayDocument = isIsolated ? nullptr : overlay_animation_document_get(manager, document))
                    if (auto overlayAnimation = overlayDocument->model.animation_get(overlayIndex))
                        render(*overlayDocument, *overlayAnimation, 1.0f - percent_to_unit(overlayTransparency),
                               drawOptions);
            };

            // Editing a stepped-into child: its parent, dimmed and frozen where it was stepped in, around it.
            if (auto& steps = document.childSteps;
                !isIsolated && !steps.empty() && steps.back().childAnimationIndex == reference.animationIndex)
            {
                auto& step = steps.back();
                auto parentIndex = manager.tab_index_get(step.tabId);
                auto parentDocument = parentIndex == -1 ? nullptr : manager.get(parentIndex);
                auto parentAnimation =
                    parentDocument ? parentDocument->model.animation_get(step.animationIndex) : nullptr;
                auto placement = parentAnimation ? layer_child_transform_get(*parentAnimation, step.layer.itemID,
                                                                             step.frameTime, isRootTransform)
                                                 : std::nullopt;
                if (placement && mat4_determinant(*placement) != 0.0f)
                    render(*parentDocument, *parentAnimation, PARENT_DIM_ALPHA,
                           {.time = step.frameTime, .isRootTransform = isRootTransform}, mat4_inverse(*placement),
                           step.layer.itemID);
            }

            if (overlayDrawOrder == OVERLAY_UNDER)
                overlay_render();
            render(document, *animation, 0.0f, drawOptions);
            if (overlayDrawOrder == OVERLAY_OVER)
                overlay_render();

            if (settings.onionskinIsEnabled && settings.onionskinIsPivotPath && !isIsolated)
            {
                auto key =
                    std::tuple{document.hash, document.librariesRevision, reference.animationIndex, isRootTransform};
                if (key != pivotPathsKey || pivotPaths.empty())
                {
                    pivotPathsKey = key;
                    pivotPaths.clear();
                    for (int time = 0; time < animation->frameNum; ++time)
                        for (auto& draw : animation_draws_get(model, *animation,
                                                              {.time = (float)time,
                                                               .isRootTransform = isRootTransform,
                                                               .library_get = document.library_resolver_get()}))
                        {
                            auto path = std::ranges::find_if(pivotPaths, [&](const PivotPath& path) {
                                return path.item.type == draw.type && path.item.id == draw.id &&
                                       path.item.groupType == draw.groupType;
                            });
                            if (path == pivotPaths.end())
                                path = pivotPaths.insert(pivotPaths.end(), {.item = draw});
                            path->points.push_back(Vec2(draw.parent * Vec4(draw.frame.position, 0.0f, 1.0f)));
                        }
                }

                auto pointIcon = resources.icon_id_get(ICON_POINT);
                for (auto& path : pivotPaths)
                {
                    if (isOnlySelected && !is_draw_selected(path.item))
                        continue;
                    for (std::size_t i = 0; i < path.points.size(); ++i)
                    {
                        if (i > 0)
                            line_render(shaderDashed, baseTransform, path.points[i - 1], path.points[i], COLOR_RED);
                        texture_render(shaderTexture, pointIcon,
                                       baseTransform * quad_model_get(POINT_SIZE, path.points[i], POINT_SIZE * 0.5f),
                                       COLOR_RED);
                    }
                }
            }
        }

        if (isRecordingFrame)
        {
            auto sounds = settings.timelineIsSound && animation && recorder.is_sound_frame()
                              ? document.trigger_sounds_get(*animation, (int)frameTime, recorder.index)
                              : std::vector<const AudioData*>{};
            if (!recorder.frame_capture(pixels_get(), size, sounds))
            {
                toast_log(Level::ERROR, TOAST_EXPORT_RENDERED_ANIMATION_FAILED, path_to_utf8(recorder.options.path));
                recorder.cancel();
                recording_stop(manager, document);
            }
        }

        unbind();

        if (isViewTransparent)
        {
            checker_pan_sync(zoom, pan);
            render_checker_background(ImGui::GetWindowDrawList(), min, max, -size - checkerPan, CHECKER_SIZE);
        }
        image_premultiplied_draw(texture, to_imvec2(size));

        // The events triggers (the animation's and its children's) fire on this frame.
        std::string names;
        if (animation && !isOnlyShowLayers && !manager.isRecording)
            for (auto& [source, trigger, layerId] :
                 animation_triggers_get(model, *animation, (int)frameTime, document.library_resolver_get()))
                for (auto eventId : trigger.eventIds)
                    if (auto event = item_get(source->content.events, eventId))
                        names += (names.empty() ? "" : ", ") + event->name;
        if (!names.empty())
        {
            auto clipMin = ImGui::GetItemRectMin();
            auto clipMax = ImGui::GetItemRectMax();
            auto drawList = ImGui::GetWindowDrawList();
            auto textPos = to_imvec2(to_vec2(cursorScreenPos) + to_vec2(ImGui::GetStyle().WindowPadding));

            drawList->PushClipRect(clipMin, clipMax);
            ImGui::PushFont(resources.fonts[FONT_BOLD], FONT_SIZE_LARGE);
            auto triggerTextColor = isLightTheme ? TRIGGER_TEXT_COLOR_LIGHT : TRIGGER_TEXT_COLOR_DARK;
            drawList->AddText(textPos, ImGui::GetColorU32(triggerTextColor), names.c_str());
            ImGui::PopFont();
            drawList->PopClipRect();
        }
    }

    if (tool == TOOL_PAN)
        view_context_menu_draw(
            "##Animation Preview Context Menu", manager, settings, document,
            {.zoom = zoom, .pan = pan, .center = center_view, .fit = fit_view, .isFitEnabled = animation != nullptr});

    if (manager.progressPopup.begin())
    {
        ImGui::ProgressBar(recorder.count > 0 ? (float)recorder.index / (float)recorder.count : 0.0f);
        ImGui::TextUnformatted(localize.get(TEXT_RECORDING_PROGRESS));

        shortcut(manager.chords[SHORTCUT_CANCEL]);
        if (ImGui::Button(localize.get(BASIC_CANCEL), ImVec2(ImGui::GetContentRegionAvail().x, 0)))
        {
            recorder.cancel();
            recording_stop(manager, document);
        }

        ImGui::EndPopup();
    }

    if (!document.isAnimationPreviewSet)
    {
        center_view();
        zoom = settings.previewStartZoom;
        checker_pan_reset(zoom, pan);
        document.isAnimationPreviewSet = true;
    }

    settings.previewStartZoom = zoom;
    ImGui::End();
}
