#include "animation_preview.hpp"

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

#include <glm/gtc/type_ptr.hpp>

#include "actions.hpp"
#include "log.hpp"
#include "math.hpp"
#include "model/draw.hpp"
#include "model/frames.hpp"
#include "path.hpp"
#include "strings.hpp"
#include "toast.hpp"
#include "tool.hpp"
#include "types.hpp"
#include "util/imgui/draw.hpp"
#include "util/imgui/input.hpp"
#include "util/imgui/layout.hpp"
#include "util/imgui/shortcut.hpp"
#include "window/timeline/timeline.hpp"

using namespace anm2ed::types;
using namespace anm2ed::util;
using namespace anm2ed::resource;
using namespace anm2ed::resource::image;
using namespace glm;

namespace anm2ed::imgui
{
  constexpr auto NULL_RECT_COLOR = vec4(0.0f, 0.0f, 1.0f, 0.90f);
  constexpr auto SELECTED_LAYER_BORDER_COLOR = vec4(1.0f, 0.1059f, 0.5882f, 1.0f);
  constexpr auto TARGET_SIZE = vec2(32, 32);
  constexpr auto POINT_SIZE = vec2(4, 4);
  constexpr auto TRIGGER_TEXT_COLOR_DARK = ImVec4(1.0f, 1.0f, 1.0f, 0.5f);
  constexpr auto TRIGGER_TEXT_COLOR_LIGHT = ImVec4(0.0f, 0.0f, 0.0f, 0.5f);
  struct OverlayAnimationOption
  {
    uint64_t documentId{};
    int animationIndex{-1};
    std::string label{};
  };

  std::vector<OverlayAnimationOption> overlay_animation_options_get(Manager& manager)
  {
    std::vector<OverlayAnimationOption> options{{.label = std::string(localize.get(BASIC_NONE))}};

    for (auto& document : manager.documents)
    {
      auto documentLabel = path::to_utf8(document.filename_get());
      for (auto [animationIndex, animation] : document.model.animations_get())
      {
        auto label = std::format("{} ({})", animation->name, documentLabel);
        options.push_back({.documentId = document.tabId, .animationIndex = animationIndex, .label = label});
      }
    }

    return options;
  }

  std::vector<const char*> overlay_animation_option_labels_get(std::vector<OverlayAnimationOption>& options)
  {
    std::vector<const char*> labels{};
    labels.reserve(options.size());
    for (auto& option : options)
      labels.push_back(option.label.c_str());
    return labels;
  }

  int overlay_animation_option_index_get(const Document& document, const std::vector<OverlayAnimationOption>& options)
  {
    if (document.overlayIndex == -1) return 0;
    auto documentId = document.overlayDocumentId ? document.overlayDocumentId : document.tabId;
    for (int i = 1; i < (int)options.size(); ++i)
      if (options[i].documentId == documentId && options[i].animationIndex == document.overlayIndex) return i;
    return -1;
  }

  Document* overlay_animation_document_get(Manager& manager, const Document& document)
  {
    if (document.overlayIndex == -1) return nullptr;
    auto documentId = document.overlayDocumentId ? document.overlayDocumentId : document.tabId;
    for (auto& candidate : manager.documents)
      if (candidate.tabId == documentId) return &candidate;
    return nullptr;
  }

  std::optional<vec4> animation_rects_merge(std::optional<vec4> rect, vec4 next)
  {
    if (next == vec4(-1.0f)) return rect;
    if (!rect) return next;

    auto minPoint = glm::min(vec2(rect->x, rect->y), vec2(next.x, next.y));
    auto maxPoint = glm::max(vec2(rect->x + rect->z, rect->y + rect->w), vec2(next.x + next.z, next.y + next.w));
    return vec4(minPoint, maxPoint - minPoint);
  }

  std::optional<vec4> animation_render_rect_get(Manager& manager, Document& document, const model::Animation* animation,
                                                bool isRootTransform)
  {
    std::optional<vec4> rect{};

    if (animation)
      rect = animation_rects_merge(rect, model::animation_rect(document.model, *animation, isRootTransform));

    if (auto overlayDocument = overlay_animation_document_get(manager, document))
      if (auto overlayAnimation = overlayDocument->model.animation_get(document.overlayIndex))
        rect = animation_rects_merge(rect,
                                     model::animation_rect(overlayDocument->model, *overlayAnimation, isRootTransform));

    return rect;
  }

  int trigger_sound_id_get(Document& document, const model::Animation* animation, float time,
                           int deterministicIndex = -1)
  {
    if (!animation || !animation->triggers.isVisible) return -1;

    auto trigger = model::frame_generate(animation->triggers, time);
    if (!trigger.isVisible || trigger.soundIds.empty()) return -1;

    size_t soundIndex{};
    if (trigger.soundIds.size() > 1)
      soundIndex = deterministicIndex >= 0 ? (size_t)deterministicIndex % trigger.soundIds.size()
                                           : (size_t)math::random_in_range(0.0f, (float)trigger.soundIds.size());
    soundIndex = std::min(soundIndex, trigger.soundIds.size() - 1);

    auto soundID = trigger.soundIds[soundIndex];
    return document.sound_get(soundID) ? soundID : -1;
  }

  void AnimationPreview::tick(Manager& manager, Settings& settings, float deltaSeconds)
  {
    auto& document = *manager.get();
    auto& model = document.model;
    auto& playback = document.playback;
    auto& frameTime = document.frameTime;

    auto stop_all_sounds = [&]()
    {
      for (auto& [id, _] : document.sounds)
        if (auto sound = document.sound_get(id)) audio::stop(*sound, mixer);
    };

    if (manager.isRecording)
    {
      if (recorder.is_done())
      {
        recorder.finish(document, audioStream);
        recording_stop(manager, document);
      }
      stop_all_sounds();
      wasPlaybackPlaying = false;
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
        playback.timing_reset();
      }
      else
      {
        if (!manager.isRecording && !document.sounds.empty() && isSound)
        {
          if (auto soundID = trigger_sound_id_get(document, animation, playback.time); soundID != -1)
            if (auto sound = document.sound_get(soundID)) audio::play(*sound, false, mixer);
        }

        auto fps = std::max(model.info.fps, 1);
        playback.tick(fps, animation->frameNum, (animation->isLoop || settings.playbackIsLoop) && !manager.isRecording,
                      deltaSeconds);

        frameTime = playback.time;
      }
    }

    if (wasPlaybackPlaying && !playback.isPlaying) stop_all_sounds();
    wasPlaybackPlaying = playback.isPlaying;
  }

  void AnimationPreview::recording_stop(Manager& manager, Document& document)
  {
    document.playback.isPlaying = false;
    document.playback.isFinished = false;
    manager.isRecording = false;
    manager.isRecordingStart = false;
    manager.progressPopup.close();
  }

  // Starts a render from the render settings; a bounded render frames the animation (and overlay) at the render scale.
  void AnimationPreview::recording_start(Manager& manager, Settings& settings, Document& document,
                                         const model::Animation* animation)
  {
    auto animationFps = std::max(document.model.info.fps, 1);
    render::RecorderOptions options{
        .type = (render::Type)settings.renderType,
        .path = settings.renderPath,
        .format = settings.renderFormat,
        .ffmpegPath = settings.renderFFmpegPath,
        .start = manager.recordingStart,
        .end = manager.recordingEnd,
        .animationFps = animationFps,
        .fps = render::fps_get(settings.renderFpsMode, animationFps, settings.playbackTickRate),
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
        recordSize = glm::vec2(rect->z, rect->w) * math::percent_to_unit(settings.renderScale);
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
      toast_log(Level::ERROR, TOAST_EXPORT_RENDERED_ANIMATION_FAILED, path::to_utf8(options.path));
      return recording_stop(manager, document);
    }
    document.playback.isPlaying = true;
    document.playback.timing_reset();
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
    auto& shaderLine = resources.shaders[shader::LINE];
    bool isLightTheme = settings.theme == theme::LIGHT;
    auto& shaderAxes = resources.shaders[shader::AXIS];
    auto& shaderGrid = resources.shaders[shader::GRID];
    auto& shaderTexture = resources.shaders[shader::TEXTURE];
    auto& shaderDashed = resources.shaders[shader::DASHED];
    auto center_view = [&]() { pan = vec2(); };

    auto fit_view = [&]()
    {
      if (animation) set_to_rect(zoom, pan, model::animation_rect(model, *animation, isRootTransform));
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
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_PREVIEW_ZOOM));

        view_buttons_draw(manager, settings, center_view, fit_view);

        auto readoutSize = ImVec2(row_widget_width_get(2), ImGui::GetTextLineHeightWithSpacing());
        auto mousePosInt = ivec2(mousePos);
        auto positionText =
            std::vformat(localize.get(FORMAT_POSITION_SPACED), std::make_format_args(mousePosInt.x, mousePosInt.y));
        if (ImGui::BeginChild("##Position Readout", readoutSize, false, ImGuiWindowFlags_NoScrollbar))
        {
          ImGui::TextUnformatted(positionText.c_str());
          if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", positionText.c_str());
        }
        ImGui::EndChild();

        ImGui::SameLine();

        auto sizeInt = ivec2(size);
        auto sizeText = std::vformat(localize.get(FORMAT_SIZE_SPACED), std::make_format_args(sizeInt.x, sizeInt.y));
        if (ImGui::BeginChild("##Size Readout", readoutSize, false, ImGuiWindowFlags_NoScrollbar))
        {
          ImGui::TextUnformatted(sizeText.c_str());
          if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", sizeText.c_str());
        }
        ImGui::EndChild();
      }
      ImGui::EndChild();

      ImGui::SameLine();

      if (ImGui::BeginChild("##Background Child", childSize, true, ImGuiWindowFlags_HorizontalScrollbar))
      {
        ImGui::BeginDisabled(isTransparent);
        {
          ImGui::ColorEdit3(localize.get(LABEL_BACKGROUND_COLOR), value_ptr(backgroundColor),
                            ImGuiColorEditFlags_NoInputs);
          ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_BACKGROUND_COLOR));
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::Checkbox(localize.get(LABEL_AXES), &isAxes);
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_AXES));
        ImGui::SameLine();
        ImGui::ColorEdit4(localize.get(BASIC_COLOR), value_ptr(axesColor), ImGuiColorEditFlags_NoInputs);
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_AXES_COLOR));

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
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_OVERLAY));

        overlayDrawOrder = glm::clamp(overlayDrawOrder, (int)overlay_draw_order::OVER, (int)overlay_draw_order::UNDER);
        overlayTransparency = glm::clamp(overlayTransparency, 0.0f, 100.0f);
        auto orderIconSize = icon_size_get();
        auto orderButtonWidth = orderIconSize.x + ImGui::GetStyle().FramePadding.x * 2.0f;
        auto alphaWidth = std::max(ImGui::GetContentRegionAvail().x - (orderButtonWidth * 2.0f) -
                                       (ImGui::GetStyle().ItemSpacing.x * 2.0f),
                                   ImGui::GetFrameHeight());
        auto overlayIconTint = isLightTheme ? ImVec4(0.0f, 0.0f, 0.0f, 1.0f) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
        ImGui::PushItemWidth(alphaWidth);
        ImGui::DragFloat("##Overlay Alpha", &overlayTransparency, DRAG_SPEED, 0, 100, "%.0f%%");
        ImGui::PopItemWidth();
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_OVERLAY_ALPHA));
        overlayTransparency = glm::clamp(overlayTransparency, 0.0f, 100.0f);

        ImGui::SameLine();
        radio_button_icon("##Overlay Over", &overlayDrawOrder, overlay_draw_order::OVER,
                          resources.icon_id_get(icon::OVER), orderIconSize, overlayIconTint);
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_OVERLAY_OVER));

        ImGui::SameLine();
        radio_button_icon("##Overlay Under", &overlayDrawOrder, overlay_draw_order::UNDER,
                          resources.icon_id_get(icon::UNDER), orderIconSize, overlayIconTint);
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_OVERLAY_UNDER));
      }
      ImGui::EndChild();

      ImGui::SameLine();

      if (ImGui::BeginChild("##Helpers Child", childSize, true, ImGuiWindowFlags_HorizontalScrollbar))
      {
        auto helpersChildSize = ImVec2(row_widget_width_get(2), ImGui::GetContentRegionAvail().y);

        if (ImGui::BeginChild("##Helpers Child 1", helpersChildSize))
        {
          ImGui::Checkbox(localize.get(LABEL_ROOT_TRANSFORM), &isRootTransform);
          ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_ROOT_TRANSFORM));
          ImGui::Checkbox(localize.get(LABEL_PIVOTS), &isPivots);
          ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_PIVOTS));
          ImGui::Checkbox(localize.get(LABEL_TRANSPARENT), &isTransparent);
          ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_PREVIEW_TRANSPARENT));
        }
        ImGui::EndChild();

        ImGui::SameLine();

        if (ImGui::BeginChild("##Helpers Child 2", helpersChildSize))
        {
          ImGui::Checkbox(localize.get(LABEL_ALT_ICONS), &isAltIcons);
          ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_ALT_ICONS));
          ImGui::Checkbox(localize.get(LABEL_BORDER), &isBorder);
          ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_BORDER));
        }
        ImGui::EndChild();
      }
      ImGui::EndChild();

      if (manager.isRecordingStart) recording_start(manager, settings, document, animation);
      auto isRecordingFrame = manager.isRecording && !recorder.is_done();
      if (isRecordingFrame)
      {
        playback.isPlaying = true;
        playback.isFinished = false;
        playback.time = recorder.time_get();
        document.frameTime = playback.time;
      }

      // A render of an isolated animation hides helpers, overlay and onion skin; a bounded one draws its own view.
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
        bool isCanvasResized = ivec2(nextSize) != ivec2(size);
        size_set(nextSize);
        if (isCanvasResized && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        {
          auto resizeSizeInt = ivec2(size);
          auto resizeSizeText =
              std::vformat(localize.get(FORMAT_SIZE_SPACED), std::make_format_args(resizeSizeInt.x, resizeSizeInt.y));
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
        std::erase_if(selectedFrameReferences, [](const Reference& frameReference)
                      { return frameReference.itemType == NONE || frameReference.itemType == TRIGGER; });
        auto editReference = reference;
        if ((referenceItemType == ItemType::TRIGGER || !selectedFrameReferences.contains(reference)) &&
            !selectedFrameReferences.empty())
          editReference = *selectedFrameReferences.begin();

        auto frame = selectedFrameReferences.empty() ? nullptr : model.frame_get(editReference);
        auto selectedNull =
            editReference.itemType == NULL_ ? model::item_get(model.content.nulls, editReference.itemID) : nullptr;
        bool isSelectedNullRect = selectedNull && selectedNull->isShowRect;
        auto null_rect_top_left = [](const model::Frame& frame) { return frame.position - (frame.scale * 0.5f); };

        auto useTool = (tool::Type)tool;
        if (input.isMiddleDown) useTool = tool::PAN;
        if (tool == tool::MOVE && input.isRightDown) useTool = tool::SCALE;
        if (tool == tool::SCALE && input.isRightDown) useTool = tool::MOVE;

        // A tool swapped in by the right button is driven by the right button.
        auto isSwapped = useTool != tool && useTool != tool::PAN;
        auto isToolClicked = isSwapped ? input.isRightClicked : input.isLeftClicked;
        auto isToolDown = isSwapped ? input.isRightDown : input.isLeftDown;
        auto isToolReleased = isSwapped ? input.isRightReleased : input.isLeftReleased;
        auto isBegin = isToolClicked || input.isArrowBegin;
        auto isDuring = isToolDown || input.isArrowDown;
        auto isEnd = isToolReleased || input.isArrowEnd;
        auto arrow = input.arrow * input.step;
        auto isArrow = arrow != vec2();

        // Changes apply to every selected frame; absolute values are applied as the difference from the edited frame.
        auto frames_change = [&](FrameChange change, ChangeType type = ChangeType::ADD)
        { frames_change_push(manager, selectedFrameReferences, change, type); };
        auto null_rect_set = [&](vec2 minPoint, vec2 maxPoint)
        {
          auto rectSize = maxPoint - minPoint;
          frames_change({.positionX = minPoint.x + rectSize.x * 0.5f - frame->position.x,
                         .positionY = minPoint.y + rectSize.y * 0.5f - frame->position.y,
                         .scaleX = rectSize.x - frame->scale.x,
                         .scaleY = rectSize.y - frame->scale.y});
        };

        tool_cursor_update(useTool, tool::ANIMATION_PREVIEW, frame != nullptr, true,
                           input.isLeftDown || input.isRightDown || input.isMiddleDown || input.isArrowDown,
                           TEXT_SELECT_FRAME);
        if (useTool != tool::MOVE) isMoveDragging = false;
        if (useTool == tool::PAN && (input.isLeftDown || input.isRightDown || input.isMiddleDown))
          pan += input.mouseDelta;

        if (frame && useTool == tool::MOVE)
        {
          if (isBegin) edit_begin_push(manager, EDIT_FRAME_POSITION);
          if (isToolClicked)
          {
            auto origin = isSelectedNullRect ? null_rect_top_left(*frame) : frame->position;
            moveOffset = settings.inputIsMoveToolSnapToMouse ? vec2() : mousePos - origin;
            isMoveDragging = true;
          }
          if (isToolDown && isMoveDragging)
          {
            auto position = vec2(ivec2(mousePos - moveOffset + (isSelectedNullRect ? frame->scale * 0.5f : vec2())));
            frames_change({.positionX = position.x - frame->position.x, .positionY = position.y - frame->position.y});
          }
          if (isArrow) frames_change({.positionX = arrow.x, .positionY = arrow.y});
          if (isToolReleased) isMoveDragging = false;
          if (isDuring) tooltip_lines_draw({localize_format(FORMAT_POSITION, frame->position.x, frame->position.y)});
          if (isEnd) document_change_push(manager);
        }

        if (frame && useTool == tool::SCALE)
        {
          if (isBegin) edit_begin_push(manager, EDIT_FRAME_SCALE);
          if (isToolClicked && isSelectedNullRect) nullRectScaleAnchor = null_rect_top_left(*frame);
          if (isToolDown && isSelectedNullRect)
          {
            auto rectSize = mousePos - nullRectScaleAnchor;
            if (input.isMod)
            {
              auto squareSize = std::max(std::abs(rectSize.x), std::abs(rectSize.y));
              rectSize = {std::copysign(squareSize, rectSize.x), std::copysign(squareSize, rectSize.y)};
            }
            auto [minPoint, maxPoint] = rect_snap(nullRectScaleAnchor, nullRectScaleAnchor + rectSize, false, {}, {});
            null_rect_set(minPoint, maxPoint);
          }
          else if (isToolDown)
            frames_change(
                {.scaleX = input.mouseDelta.x,
                 .scaleY = input.isMod ? frame->scale.x + input.mouseDelta.x - frame->scale.y : input.mouseDelta.y});
          if (isArrow)
            frames_change({.positionX = isSelectedNullRect ? arrow.x * 0.5f : 0.0f,
                           .positionY = isSelectedNullRect ? arrow.y * 0.5f : 0.0f,
                           .scaleX = arrow.x,
                           .scaleY = arrow.y});
          if (isDuring) tooltip_lines_draw({localize_format(FORMAT_SCALE, frame->scale.x, frame->scale.y)});
          if (isEnd && isSelectedNullRect)
          {
            auto topLeft = null_rect_top_left(*frame);
            auto [minPoint, maxPoint] = rect_snap(topLeft, topLeft + frame->scale, false, {}, {});
            null_rect_set(minPoint, maxPoint);
          }
          if (isEnd) document_change_push(manager);
        }

        if (frame && useTool == tool::ROTATE)
        {
          if (isBegin) edit_begin_push(manager, EDIT_FRAME_ROTATION);
          if (isToolDown) frames_change({.rotation = input.mouseDelta.x});
          if (auto turn = glm::sign(input.arrow.x - input.arrow.y); turn != 0.0f)
            frames_change({.rotation = turn * input.step});
          if (isDuring) tooltip_lines_draw({localize_format(FORMAT_ROTATION, frame->rotation)});
          if (isEnd) document_change_push(manager);
        }

        if (frame && useTool == tool::SHEAR)
        {
          if (isBegin) edit_begin_push(manager, EDIT_FRAME_SHEAR);
          if (isToolDown)
          {
            auto delta = input.mouseDelta;
            if (input.isMod) (std::abs(delta.x) >= std::abs(delta.y) ? delta.y : delta.x) = 0.0f;
            frames_change({.shearX = delta.x, .shearY = delta.y});
          }
          if (isArrow) frames_change({.shearX = arrow.x, .shearY = arrow.y});
          if (isDuring) tooltip_lines_draw({localize_format(FORMAT_SHEAR, frame->shear.x, frame->shear.y)});
          if (isEnd) document_change_push(manager);
        }

        if (input.wheel != 0 || input.isZoomIn || input.isZoomOut)
          zoom_step(zoom, pan, vec2(mousePos),
                    (input.wheel > 0 || input.isZoomIn) ? ZOOM_LEVEL_STEP : -ZOOM_LEVEL_STEP);
      }

      bind();
      viewport_set();
      clear(isViewTransparent ? vec4(0) : vec4(backgroundColor, 1.0f));

      if (isAxes && !isIsolated) axes_render(shaderAxes, viewZoom, viewPan, axesColor);
      if (isGrid && !isIsolated) grid_render(shaderGrid, viewZoom, viewPan, gridSize, gridOffset, gridColor);

      auto baseTransform = transform_get(viewZoom, viewPan);
      auto frameTime = document.frameTime > -1 && !playback.isPlaying ? document.frameTime : playback.time;

      model::DrawOptions drawOptions{.time = frameTime,
                                     .isRootTransform = isRootTransform,
                                     .isIndexSampled = settings.onionskinMode == (int)OnionskinMode::INDEX};
      std::vector<vec3> sampleColors{};
      std::vector<float> sampleAlphas{};
      auto samples_add = [&](int count, int direction, vec3 color)
      {
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

      auto is_layer_selected = [&](int id)
      {
        auto is_layer = [&](const Reference& itemReference)
        {
          return itemReference.animationIndex == reference.animationIndex && itemReference.itemType == LAYER &&
                 itemReference.itemID == id;
        };
        return (reference.animationIndex != -1 && is_layer(reference)) ||
               std::ranges::any_of(document.selected_get(SelectionKind::TRACKS), is_layer) ||
               std::ranges::any_of(document.frame_references_get(Document::FrameReferenceFallback::NONE), is_layer);
      };

      // The item the reference points at: the root (or a group's root), a layer or a null.
      auto is_draw_selected = [&](const model::Draw& draw)
      {
        switch (draw.type)
        {
          case model::DrawType::ROOT:
            return referenceItemType == ItemType::ROOT && reference.groupId == -1;
          case model::DrawType::GROUP_ROOT:
            return referenceItemType == ItemType::ROOT && reference.groupType == draw.groupType &&
                   reference.groupId == draw.id;
          case model::DrawType::LAYER:
            return referenceItemType == ItemType::LAYER && reference.itemID == draw.id;
          default:
            return referenceItemType == ItemType::NULL_ && reference.itemID == draw.id;
        }
      };
      auto isOnlySelected = settings.onionskinIsOnlySelected;

      // Draws an animation's draw list; onion-skin samples are tinted and faded, `alphaOffset` fades the overlay.
      auto render = [&](Document& sampleDocument, const model::Animation& sampleAnimation, float alphaOffset)
      {
        auto isActiveDocument = &sampleDocument == &document;
        auto& sampleModel = sampleDocument.model;
        auto targetIcon = resources.icon_id_get(isAltIcons && !isIsolated ? icon::TARGET_ALT : icon::TARGET);
        for (const auto& draw : model::animation_draws_get(sampleModel, sampleAnimation, drawOptions))
        {
          auto isOnion = draw.sample != -1;
          if (isOnion && isOnlySelected && !(isActiveDocument && is_draw_selected(draw))) continue;
          auto sampleColor = isOnion ? sampleColors[draw.sample] : vec3();
          auto sampleAlpha = isOnion ? sampleAlphas[draw.sample] : 0.0f;
          auto onionColor = vec4(sampleColor, 1.0f - sampleAlpha);
          auto& frame = draw.frame;
          auto transform = baseTransform * draw.parent;
          auto marker_model_get = [&](vec2 markerSize)
          {
            return math::quad_model_get(markerSize, frame.position, markerSize * 0.5f,
                                        math::percent_to_unit(frame.scale), frame.rotation,
                                        math::percent_to_unit(frame.shear));
          };

          if (draw.type == model::DrawType::ROOT || draw.type == model::DrawType::GROUP_ROOT)
          {
            if (isViewOnlyLayers) continue;
            auto isSelected = isActiveDocument && draw.type == model::DrawType::GROUP_ROOT &&
                              referenceItemType == ItemType::ROOT && reference.groupType == draw.groupType &&
                              reference.groupId == draw.id;
            auto color = isOnion                              ? vec4(sampleColor, sampleAlpha)
                         : draw.type == model::DrawType::ROOT ? color::GREEN
                         : isSelected                         ? color::RED
                                                              : ROOT_COLOR;
            auto markerModel = isRootTransform ? math::quad_model_get(TARGET_SIZE, {}, TARGET_SIZE * 0.5f)
                                               : marker_model_get(TARGET_SIZE);
            texture_render(shaderTexture, targetIcon, transform * markerModel, color);
          }
          else if (draw.type == model::DrawType::LAYER)
          {
            auto layer = model::item_get(sampleModel.content.layers, draw.id);
            auto layerTexture = sampleDocument.texture_get(layer->spritesheetId);
            if (!layerTexture || !layerTexture->is_valid() || layerTexture->size.x <= 0 || layerTexture->size.y <= 0)
              continue;
            auto textureSize = vec2(layerTexture->size);
            auto layerModel = model::draw_quad_model_get(draw);
            auto layerTransform = transform * layerModel;
            auto vertices = math::uv_vertices_get(frame.crop / textureSize, (frame.crop + frame.size) / textureSize);
            auto tint = frame.tint;
            tint.a = std::max(0.0f, tint.a - (alphaOffset + sampleAlpha));
            auto customShader = sampleDocument.shader_get(frame.shaderId);
            texture_render(customShader ? *customShader : shaderTexture, resource::texture::id_get(*layerTexture),
                           layerTransform, tint, frame.colorOffset + sampleColor, vertices.data(), textureSize,
                           draw.time);

            auto color = isOnion                                          ? onionColor
                         : isActiveDocument && is_layer_selected(draw.id) ? SELECTED_LAYER_BORDER_COLOR
                                                                          : color::RED;
            if (isBorder && !isIsolated) rect_render(shaderLine, layerTransform, layerModel, color);
            if (isPivots && !isIsolated)
              texture_render(shaderTexture, resources.icon_id_get(icon::PIVOT),
                             transform * marker_model_get(PIVOT_SIZE), color);
          }
          else if (!isViewOnlyLayers)
          {
            auto isShowRect = model::item_get(sampleModel.content.nulls, draw.id)->isShowRect;
            auto isSelected = isActiveDocument && draw.id == reference.itemID && referenceItemType == ItemType::NULL_;
            auto color = isOnion ? onionColor : isSelected ? color::RED : NULL_RECT_COLOR;
            auto markerSize = isShowRect ? POINT_SIZE : TARGET_SIZE;
            texture_render(shaderTexture, isShowRect ? resources.icon_id_get(icon::POINT) : targetIcon,
                           transform * marker_model_get(markerSize), color);
            if (!isShowRect) continue;
            auto rectModel = math::quad_model_get(frame.scale, frame.position, frame.scale * 0.5f, vec2(1.0f),
                                                  frame.rotation, math::percent_to_unit(frame.shear));
            rect_render(shaderLine, transform * rectModel, rectModel, color);
          }
        }
      };

      if (animation)
      {
        auto overlay_render = [&]()
        {
          if (auto overlayDocument = isIsolated ? nullptr : overlay_animation_document_get(manager, document))
            if (auto overlayAnimation = overlayDocument->model.animation_get(overlayIndex))
              render(*overlayDocument, *overlayAnimation, 1.0f - math::percent_to_unit(overlayTransparency));
        };

        if (overlayDrawOrder == overlay_draw_order::UNDER) overlay_render();
        render(document, *animation, 0.0f);
        if (overlayDrawOrder == overlay_draw_order::OVER) overlay_render();

        if (settings.onionskinIsEnabled && settings.onionskinIsPivotPath && !isIsolated)
        {
          auto key = std::tuple{document.hash, reference.animationIndex, isRootTransform};
          if (key != pivotPathsKey || pivotPaths.empty())
          {
            pivotPathsKey = key;
            pivotPaths.clear();
            for (int time = 0; time < animation->frameNum; ++time)
              for (const auto& draw : model::animation_draws_get(
                       model, *animation, {.time = (float)time, .isRootTransform = isRootTransform}))
              {
                auto path = std::ranges::find_if(pivotPaths,
                                                 [&](const PivotPath& path)
                                                 {
                                                   return path.item.type == draw.type && path.item.id == draw.id &&
                                                          path.item.groupType == draw.groupType;
                                                 });
                if (path == pivotPaths.end()) path = pivotPaths.insert(pivotPaths.end(), {.item = draw});
                path->points.push_back(vec2(draw.parent * vec4(draw.frame.position, 0.0f, 1.0f)));
              }
          }

          auto pointIcon = resources.icon_id_get(icon::POINT);
          for (const auto& path : pivotPaths)
          {
            if (isOnlySelected && !is_draw_selected(path.item)) continue;
            for (std::size_t i = 0; i < path.points.size(); ++i)
            {
              if (i > 0) line_render(shaderDashed, baseTransform, path.points[i - 1], path.points[i], color::RED);
              texture_render(shaderTexture, pointIcon,
                             baseTransform * math::quad_model_get(POINT_SIZE, path.points[i], POINT_SIZE * 0.5f),
                             color::RED);
            }
          }
        }
      }

      if (isRecordingFrame)
      {
        auto soundId = settings.timelineIsSound && animation && recorder.is_sound_frame()
                           ? trigger_sound_id_get(document, animation, frameTime, recorder.index)
                           : -1;
        if (!recorder.frame_capture(pixels_get(), size, soundId))
        {
          toast_log(Level::ERROR, TOAST_EXPORT_RENDERED_ANIMATION_FAILED, path::to_utf8(recorder.options.path));
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

      if (animation && animation->triggers.isVisible && !isOnlyShowLayers && !manager.isRecording)
      {
        if (auto trigger = model::frame_generate(animation->triggers, frameTime);
            trigger.isVisible && trigger.eventId > -1)
        {
          auto clipMin = ImGui::GetItemRectMin();
          auto clipMax = ImGui::GetItemRectMax();
          auto drawList = ImGui::GetWindowDrawList();
          auto textPos = to_imvec2(to_vec2(cursorScreenPos) + to_vec2(ImGui::GetStyle().WindowPadding));

          drawList->PushClipRect(clipMin, clipMax);
          ImGui::PushFont(resources.fonts[font::BOLD].get(), font::SIZE_LARGE);
          auto triggerTextColor = isLightTheme ? TRIGGER_TEXT_COLOR_LIGHT : TRIGGER_TEXT_COLOR_DARK;
          if (auto event = model::item_get(model.content.events, trigger.eventId))
            drawList->AddText(textPos, ImGui::GetColorU32(triggerTextColor), event->name.c_str());
          ImGui::PopFont();
          drawList->PopClipRect();
        }
      }
    }

    if (tool == tool::PAN)
      view_context_menu_draw("##Animation Preview Context Menu", manager, settings, document, zoom, pan, center_view,
                             fit_view, animation != nullptr);

    manager.progressPopup.trigger();

    if (ImGui::BeginPopupModal(manager.progressPopup.label(), &manager.progressPopup.isOpen, ImGuiWindowFlags_NoResize))
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
}
