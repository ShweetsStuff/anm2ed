#include "animation_preview.h"

#include <ranges>

#include <glm/gtc/type_ptr.hpp>

#include "log.h"
#include "math_.h"
#include "toast.h"
#include "tool.h"
#include "types.h"

using namespace anm2ed::canvas;
using namespace anm2ed::types;
using namespace anm2ed::util;
using namespace anm2ed::resource;
using namespace glm;

namespace anm2ed::imgui
{
  constexpr auto NULL_COLOR = vec4(0.0f, 0.0f, 1.0f, 0.90f);
  constexpr auto TARGET_SIZE = vec2(32, 32);
  constexpr auto POINT_SIZE = vec2(4, 4);
  constexpr auto NULL_RECT_SIZE = vec2(100);
  constexpr auto TRIGGER_TEXT_COLOR = ImVec4(1.0f, 1.0f, 1.0f, 0.5f);

  AnimationPreview::AnimationPreview() : Canvas(vec2())
  {
  }

  void AnimationPreview::tick(Manager& manager, Document& document, Settings& settings)
  {
    auto& anm2 = document.anm2;
    auto& playback = document.playback;
    auto& zoom = document.previewZoom;
    auto& pan = document.previewPan;
    auto& isRootTransform = settings.previewIsRootTransform;
    auto& scale = settings.renderScale;

    if (playback.isPlaying)
    {
      auto& isSound = settings.timelineIsSound;
      auto& isOnlyShowLayers = settings.timelineIsOnlyShowLayers;

      if (isSound && !anm2.content.sounds.empty())
        if (auto animation = document.animation_get(); animation)
          if (animation->triggers.isVisible && !isOnlyShowLayers)
            if (auto trigger = animation->triggers.frame_generate(playback.time, anm2::TRIGGER);
                trigger.is_visible(anm2::TRIGGER))
              if (anm2.content.sounds.contains(trigger.soundID)) anm2.content.sounds[trigger.soundID].audio.play();

      document.reference.frameTime = playback.time;
    }

    if (manager.isRecording)
    {
      if (manager.isRecordingStart)
      {
        if (settings.renderIsRawAnimation)
        {
          savedSettings = settings;
          settings.previewBackgroundColor = vec4();
          settings.previewIsGrid = false;
          settings.previewIsAxes = false;
          settings.timelineIsOnlyShowLayers = true;

          savedZoom = zoom;
          savedPan = pan;

          if (auto animation = document.animation_get())
          {
            auto rect = animation->rect(isRootTransform);
            size = vec2(rect.z, rect.w) * scale;
            set_to_rect(zoom, pan, rect);
          }

          isSizeTrySet = false;

          bind();
          viewport_set();
          clear(settings.previewBackgroundColor);
          unbind();
        }

        manager.isRecordingStart = false;

        return; // Need to wait an additional frame. Kind of hacky, but oh well.
      }

      auto pixels = pixels_get();
      renderFrames.push_back(Texture(pixels.data(), size));

      if (playback.time > manager.recordingEnd || playback.isFinished)
      {
        auto& ffmpegPath = settings.renderFFmpegPath;
        auto& path = settings.renderPath;
        auto& type = settings.renderType;

        if (type == render::PNGS)
        {
          auto& format = settings.renderFormat;
          bool isSuccess{true};
          for (auto [i, frame] : std::views::enumerate(renderFrames))
          {
            std::filesystem::path outputPath =
                std::filesystem::path(path) / std::vformat(format, std::make_format_args(i));

            if (!frame.write_png(outputPath))
            {
              isSuccess = false;
              break;
            }
            logger.info(std::format("Saved frame to: {}", outputPath.string()));
          }

          if (isSuccess)
            toasts.info(std::format("Exported rendered frames to: {}", path));
          else
            toasts.warning(std::format("Could not export frames to: {}", path));
        }
        else
        {
          if (animation_render(ffmpegPath, path, renderFrames, (render::Type)type, size, anm2.info.fps))
            toasts.info(std::format("Exported rendered animation to: {}", path));
          else
            toasts.warning(std::format("Could not output rendered animation: {}", path));
        }

        renderFrames.clear();

        pan = savedPan;
        zoom = savedZoom;
        settings = savedSettings;
        isSizeTrySet = true;

        playback.isPlaying = false;
        playback.isFinished = false;
        manager.isRecording = false;
        manager.progressPopup.close();
      }
    }
  }

  void AnimationPreview::update(Manager& manager, Settings& settings, Resources& resources)
  {
    auto& document = *manager.get();
    auto& anm2 = document.anm2;
    auto& playback = document.playback;
    auto& reference = document.reference;
    auto animation = document.animation_get();
    auto& pan = document.previewPan;
    auto& zoom = document.previewZoom;
    auto& backgroundColor = settings.previewBackgroundColor;
    auto& axesColor = settings.previewAxesColor;
    auto& gridColor = settings.previewGridColor;
    auto& gridSize = settings.previewGridSize;
    auto& gridOffset = settings.previewGridOffset;
    auto& zoomStep = settings.viewZoomStep;
    auto& isGrid = settings.previewIsGrid;
    auto& overlayTransparency = settings.previewOverlayTransparency;
    auto& overlayIndex = document.overlayIndex;
    auto& isRootTransform = settings.previewIsRootTransform;
    auto& isPivots = settings.previewIsPivots;
    auto& isAxes = settings.previewIsAxes;
    auto& isAltIcons = settings.previewIsAltIcons;
    auto& isBorder = settings.previewIsBorder;
    auto& tool = settings.tool;
    auto& isOnlyShowLayers = settings.timelineIsOnlyShowLayers;
    auto& shaderLine = resources.shaders[shader::LINE];
    auto& shaderAxes = resources.shaders[shader::AXIS];
    auto& shaderGrid = resources.shaders[shader::GRID];
    auto& shaderTexture = resources.shaders[shader::TEXTURE];

    auto center_view = [&]() { pan = vec2(); };

    if (ImGui::Begin("Animation Preview", &settings.windowIsAnimationPreview))
    {
      auto childSize = ImVec2(row_widget_width_get(4),
                              (ImGui::GetTextLineHeightWithSpacing() * 4) + (ImGui::GetStyle().WindowPadding.y * 2));

      if (ImGui::BeginChild("##Grid Child", childSize, true, ImGuiWindowFlags_HorizontalScrollbar))
      {
        ImGui::Checkbox("Grid", &isGrid);
        ImGui::SetItemTooltip("Toggle the visibility of the grid.");
        ImGui::SameLine();
        ImGui::ColorEdit4("Color", value_ptr(gridColor), ImGuiColorEditFlags_NoInputs);
        ImGui::SetItemTooltip("Change the grid's color.");

        input_int2_range("Size", gridSize, ivec2(GRID_SIZE_MIN), ivec2(GRID_SIZE_MAX));
        ImGui::SetItemTooltip("Change the size of all cells in the grid.");

        input_int2_range("Offset", gridOffset, ivec2(GRID_OFFSET_MIN), ivec2(GRID_OFFSET_MAX));
        ImGui::SetItemTooltip("Change the offset of the grid.");
      }
      ImGui::EndChild();

      ImGui::SameLine();

      if (ImGui::BeginChild("##View Child", childSize, true, ImGuiWindowFlags_HorizontalScrollbar))
      {
        ImGui::InputFloat("Zoom", &zoom, zoomStep, zoomStep, "%.0f%%");
        ImGui::SetItemTooltip("Change the zoom of the preview.");

        auto widgetSize = widget_size_with_row_get(2);

        shortcut(settings.shortcutCenterView);
        if (ImGui::Button("Center View", widgetSize)) pan = vec2();
        set_item_tooltip_shortcut("Centers the view.", settings.shortcutCenterView);

        ImGui::SameLine();

        shortcut(settings.shortcutFit);
        if (ImGui::Button("Fit", widgetSize))
          if (animation) set_to_rect(zoom, pan, animation->rect(isRootTransform));
        set_item_tooltip_shortcut("Set the view to match the extent of the animation.", settings.shortcutFit);

        ImGui::TextUnformatted(std::format(POSITION_FORMAT, (int)mousePos.x, (int)mousePos.y).c_str());
      }
      ImGui::EndChild();

      ImGui::SameLine();

      if (ImGui::BeginChild("##Background Child", childSize, true, ImGuiWindowFlags_HorizontalScrollbar))
      {
        ImGui::ColorEdit4("Background", value_ptr(backgroundColor), ImGuiColorEditFlags_NoInputs);
        ImGui::SetItemTooltip("Change the background color.");
        ImGui::SameLine();
        ImGui::Checkbox("Axes", &isAxes);
        ImGui::SetItemTooltip("Toggle the axes' visbility.");
        ImGui::SameLine();
        ImGui::ColorEdit4("Color", value_ptr(axesColor), ImGuiColorEditFlags_NoInputs);
        ImGui::SetItemTooltip("Set the color of the axes.");

        combo_negative_one_indexed("Overlay", &overlayIndex, document.animation.labels);
        ImGui::SetItemTooltip("Set an animation to be drawn over the current animation.");

        ImGui::InputFloat("Alpha", &overlayTransparency, 0, 0, "%.0f");
        ImGui::SetItemTooltip("Set the alpha of the overlayed animation.");
      }
      ImGui::EndChild();

      ImGui::SameLine();

      if (ImGui::BeginChild("##Helpers Child", childSize, true, ImGuiWindowFlags_HorizontalScrollbar))
      {
        auto helpersChildSize = ImVec2(row_widget_width_get(2), ImGui::GetContentRegionAvail().y);

        if (ImGui::BeginChild("##Helpers Child 1", helpersChildSize))
        {
          ImGui::Checkbox("Root Transform", &isRootTransform);
          ImGui::SetItemTooltip("Root frames will transform the rest of the animation.");
          ImGui::Checkbox("Pivots", &isPivots);
          ImGui::SetItemTooltip("Toggle the visibility of the animation's pivots.");
        }
        ImGui::EndChild();

        ImGui::SameLine();

        if (ImGui::BeginChild("##Helpers Child 2", helpersChildSize))
        {
          ImGui::Checkbox("Alt Icons", &isAltIcons);
          ImGui::SetItemTooltip("Toggle a different appearance of the target icons.");
          ImGui::Checkbox("Border", &isBorder);
          ImGui::SetItemTooltip("Toggle the visibility of borders around layers.");
        }
        ImGui::EndChild();
      }
      ImGui::EndChild();

      auto cursorScreenPos = ImGui::GetCursorScreenPos();

      if (isSizeTrySet) size_set(to_vec2(ImGui::GetContentRegionAvail()));
      bind();
      viewport_set();
      clear(backgroundColor);
      if (isAxes) axes_render(shaderAxes, zoom, pan, axesColor);
      if (isGrid) grid_render(shaderGrid, zoom, pan, gridSize, gridOffset, gridColor);

      auto render = [&](anm2::Animation* animation, float time, vec3 colorOffset = {}, float alphaOffset = {},
                        bool isOnionskin = false)
      {
        auto transform = transform_get(zoom, pan);
        auto root = animation->rootAnimation.frame_generate(time, anm2::ROOT);

        if (isRootTransform)
          transform *= math::quad_model_parent_get(root.position, {}, math::percent_to_unit(root.scale), root.rotation);

        if (!isOnlyShowLayers && root.isVisible && animation->rootAnimation.isVisible)
        {
          auto rootTransform = transform * math::quad_model_get(TARGET_SIZE, root.position, TARGET_SIZE * 0.5f,
                                                                math::percent_to_unit(root.scale), root.rotation);

          vec4 color = isOnionskin ? vec4(colorOffset, alphaOffset) : color::GREEN;

          auto icon = isAltIcons ? icon::TARGET_ALT : icon::TARGET;
          texture_render(shaderTexture, resources.icons[icon].id, rootTransform, color);
        }

        for (auto& id : animation->layerOrder)
        {
          auto& layerAnimation = animation->layerAnimations[id];
          if (!layerAnimation.isVisible) continue;

          auto& layer = anm2.content.layers.at(id);

          if (auto frame = layerAnimation.frame_generate(time, anm2::LAYER); frame.is_visible())
          {
            auto spritesheet = anm2.spritesheet_get(layer.spritesheetID);
            if (!spritesheet || !spritesheet->is_valid()) continue;

            auto& texture = spritesheet->texture;

            auto layerModel = math::quad_model_get(frame.size, frame.position, frame.pivot,
                                                   math::percent_to_unit(frame.scale), frame.rotation);
            auto layerTransform = transform * layerModel;

            auto uvMin = frame.crop / vec2(texture.size);
            auto uvMax = (frame.crop + frame.size) / vec2(texture.size);
            auto vertices = math::uv_vertices_get(uvMin, uvMax);
            vec3 frameColorOffset = frame.colorOffset + colorOffset;
            vec4 frameTint = frame.tint;
            frameTint.a = std::max(0.0f, frameTint.a - alphaOffset);

            texture_render(shaderTexture, texture.id, layerTransform, frameTint, frameColorOffset, vertices.data());

            auto color = isOnionskin ? vec4(colorOffset, 1.0f - alphaOffset) : color::RED;

            if (isBorder) rect_render(shaderLine, layerTransform, layerModel, color);

            if (isPivots)
            {
              auto pivotModel = math::quad_model_get(PIVOT_SIZE, frame.position, PIVOT_SIZE * 0.5f,
                                                     math::percent_to_unit(frame.scale), frame.rotation);
              auto pivotTransform = transform * pivotModel;

              texture_render(shaderTexture, resources.icons[icon::PIVOT].id, pivotTransform, color);
            }
          }
        }

        for (auto& [id, nullAnimation] : animation->nullAnimations)
        {
          if (!nullAnimation.isVisible || isOnlyShowLayers) continue;

          auto& isShowRect = anm2.content.nulls[id].isShowRect;

          if (auto frame = nullAnimation.frame_generate(time, anm2::NULL_); frame.isVisible)
          {
            auto icon = isShowRect ? icon::POINT : isAltIcons ? icon::TARGET_ALT : icon::TARGET;

            auto& size = isShowRect ? POINT_SIZE : TARGET_SIZE;
            auto color = isOnionskin ? vec4(colorOffset, 1.0f - alphaOffset)
                         : id == reference.itemID && reference.itemType == anm2::NULL_ ? color::RED
                                                                                       : NULL_COLOR;

            auto nullModel = math::quad_model_get(size, frame.position, size * 0.5f, math::percent_to_unit(frame.scale),
                                                  frame.rotation);
            auto nullTransform = transform * nullModel;

            texture_render(shaderTexture, resources.icons[icon].id, nullTransform, color);

            if (isShowRect)
            {
              auto rectModel = math::quad_model_get(NULL_RECT_SIZE, frame.position, NULL_RECT_SIZE * 0.5f,
                                                    math::percent_to_unit(frame.scale), frame.rotation);
              auto rectTransform = transform * rectModel;

              rect_render(shaderLine, rectTransform, rectModel, color);
            }
          }
        }
      };

      auto onionskin_render = [&](float time, int count, int direction, vec3 color)
      {
        for (int i = 1; i <= count; i++)
        {
          float useTime = time + (float)(direction * i);
          if (useTime < 0.0f || useTime > animation->frameNum) continue;

          float alphaOffset = (1.0f / (count + 1)) * i;
          render(animation, useTime, color, alphaOffset, true);
        }
      };

      auto onionskins_render = [&](float time)
      {
        onionskin_render(time, settings.onionskinBeforeCount, -1, settings.onionskinBeforeColor);
        onionskin_render(time, settings.onionskinAfterCount, 1, settings.onionskinAfterColor);
      };

      auto frameTime = reference.frameTime > -1 && !playback.isPlaying ? reference.frameTime : playback.time;

      if (animation)
      {
        auto& drawOrder = settings.onionskinDrawOrder;
        auto& isEnabled = settings.onionskinIsEnabled;

        if (drawOrder == draw_order::BELOW && isEnabled) onionskins_render(frameTime);

        render(animation, frameTime);

        if (auto overlayAnimation = anm2.animation_get({overlayIndex}))
          render(overlayAnimation, frameTime, {}, 1.0f - math::uint8_to_float(overlayTransparency));

        if (drawOrder == draw_order::ABOVE && isEnabled) onionskins_render(frameTime);
      }

      unbind();

      ImGui::Image(texture, to_imvec2(size));

      isPreviewHovered = ImGui::IsItemHovered();

      if (animation && animation->triggers.isVisible && !isOnlyShowLayers)
      {
        if (auto trigger = animation->triggers.frame_generate(frameTime, anm2::TRIGGER);
            trigger.isVisible && trigger.eventID > -1)
        {
          auto clipMin = ImGui::GetItemRectMin();
          auto clipMax = ImGui::GetItemRectMax();
          auto drawList = ImGui::GetWindowDrawList();
          auto textPos = to_imvec2(to_vec2(cursorScreenPos) + to_vec2(ImGui::GetStyle().WindowPadding));

          drawList->PushClipRect(clipMin, clipMax);
          ImGui::PushFont(resources.fonts[font::BOLD].get(), font::SIZE_LARGE);
          drawList->AddText(textPos, ImGui::GetColorU32(TRIGGER_TEXT_COLOR),
                            anm2.content.events.at(trigger.eventID).name.c_str());
          ImGui::PopFont();
          drawList->PopClipRect();
        }
      }

      if (isPreviewHovered)
      {
        auto isMouseClicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
        auto isMouseReleased = ImGui::IsMouseReleased(ImGuiMouseButton_Left);
        auto isMouseDown = ImGui::IsMouseDown(ImGuiMouseButton_Left);
        auto isMouseMiddleDown = ImGui::IsMouseDown(ImGuiMouseButton_Middle);
        auto isMouseRightDown = ImGui::IsMouseDown(ImGuiMouseButton_Right);
        auto mouseDelta = to_ivec2(ImGui::GetIO().MouseDelta);
        auto mouseWheel = ImGui::GetIO().MouseWheel;

        auto isLeftJustPressed = ImGui::IsKeyPressed(ImGuiKey_LeftArrow, false);
        auto isRightJustPressed = ImGui::IsKeyPressed(ImGuiKey_RightArrow, false);
        auto isUpJustPressed = ImGui::IsKeyPressed(ImGuiKey_UpArrow, false);
        auto isDownJustPressed = ImGui::IsKeyPressed(ImGuiKey_DownArrow, false);
        auto isLeftPressed = ImGui::IsKeyPressed(ImGuiKey_LeftArrow);
        auto isRightPressed = ImGui::IsKeyPressed(ImGuiKey_RightArrow);
        auto isUpPressed = ImGui::IsKeyPressed(ImGuiKey_UpArrow);
        auto isDownPressed = ImGui::IsKeyPressed(ImGuiKey_DownArrow);
        auto isLeftDown = ImGui::IsKeyDown(ImGuiKey_LeftArrow);
        auto isRightDown = ImGui::IsKeyDown(ImGuiKey_RightArrow);
        auto isUpDown = ImGui::IsKeyDown(ImGuiKey_UpArrow);
        auto isDownDown = ImGui::IsKeyDown(ImGuiKey_DownArrow);
        auto isLeftReleased = ImGui::IsKeyReleased(ImGuiKey_LeftArrow);
        auto isRightReleased = ImGui::IsKeyReleased(ImGuiKey_RightArrow);
        auto isUpReleased = ImGui::IsKeyReleased(ImGuiKey_UpArrow);
        auto isDownReleased = ImGui::IsKeyReleased(ImGuiKey_DownArrow);
        auto isKeyJustPressed = isLeftJustPressed || isRightJustPressed || isUpJustPressed || isDownJustPressed;
        auto isKeyDown = isLeftDown || isRightDown || isUpDown || isDownDown;
        auto isKeyReleased = isLeftReleased || isRightReleased || isUpReleased || isDownReleased;

        auto isZoomIn = chord_repeating(string_to_chord(settings.shortcutZoomIn));
        auto isZoomOut = chord_repeating(string_to_chord(settings.shortcutZoomOut));

        auto isBegin = isMouseClicked || isKeyJustPressed;
        auto isDuring = isMouseDown || isKeyDown;
        auto isEnd = isMouseReleased || isKeyReleased;

        auto isMod = ImGui::IsKeyDown(ImGuiMod_Shift);

        auto frame = document.frame_get();
        auto useTool = tool;
        auto step = isMod ? canvas::STEP_FAST : canvas::STEP;
        mousePos = position_translate(zoom, pan, to_vec2(ImGui::GetMousePos()) - to_vec2(cursorScreenPos));

        if (isMouseMiddleDown) useTool = tool::PAN;
        if (tool == tool::MOVE && isMouseRightDown) useTool = tool::SCALE;
        if (tool == tool::SCALE && isMouseRightDown) useTool = tool::MOVE;

        auto& areaType = tool::INFO[useTool].areaType;
        auto cursor = areaType == tool::ANIMATION_PREVIEW || areaType == tool::ALL ? tool::INFO[useTool].cursor
                                                                                   : ImGuiMouseCursor_NotAllowed;
        ImGui::SetMouseCursor(cursor);
        ImGui::SetKeyboardFocusHere(-1);

        switch (useTool)
        {
          case tool::PAN:
            if (isMouseDown || isMouseMiddleDown) pan += mouseDelta;
            break;
          case tool::MOVE:
            if (!frame) break;
            if (isBegin) document.snapshot("Frame Position");
            if (isMouseDown) frame->position = mousePos;
            if (isLeftPressed) frame->position.x -= step;
            if (isRightPressed) frame->position.x += step;
            if (isUpPressed) frame->position.y -= step;
            if (isDownPressed) frame->position.y += step;
            if (isEnd) document.change(Document::FRAMES);
            if (isDuring)
            {
              if (ImGui::BeginTooltip())
              {
                auto positionFormat = math::vec2_format_get(frame->position);
                auto positionString = std::format("Position: ({}, {})", positionFormat, positionFormat);
                ImGui::Text(positionString.c_str(), frame->position.x, frame->position.y);
                ImGui::EndTooltip();
              }
            }
            break;
          case tool::SCALE:
            if (!frame) break;
            if (isBegin) document.snapshot("Frame Scale");
            if (isMouseDown) frame->scale += mouseDelta;
            if (isLeftPressed) frame->scale.x -= step;
            if (isRightPressed) frame->scale.x += step;
            if (isUpPressed) frame->scale.y -= step;
            if (isDownPressed) frame->scale.y += step;

            if (isDuring)
            {
              if (ImGui::BeginTooltip())
              {
                auto scaleFormat = math::vec2_format_get(frame->scale);
                auto scaleString = std::format("Scale: ({}, {})", scaleFormat, scaleFormat);
                ImGui::Text(scaleString.c_str(), frame->scale.x, frame->scale.y);
                ImGui::EndTooltip();
              }
            }

            if (isEnd) document.change(Document::FRAMES);
            break;
          case tool::ROTATE:
            if (!frame) break;
            if (isBegin) document.snapshot("Frame Rotation");
            if (isMouseDown) frame->rotation += mouseDelta.y;
            if (isLeftPressed || isDownPressed) frame->rotation -= step;
            if (isUpPressed || isRightPressed) frame->rotation += step;

            if (isDuring)
            {
              if (ImGui::BeginTooltip())
              {
                auto rotationFormat = math::float_format_get(frame->rotation);
                auto rotationString = std::format("Rotation: {}", rotationFormat);
                ImGui::Text(rotationString.c_str(), frame->rotation);
                ImGui::EndTooltip();
              }
            }

            if (isEnd) document.change(Document::FRAMES);
            break;
          default:
            break;
        }

        if (mouseWheel != 0 || isZoomIn || isZoomOut)
          zoom_set(zoom, pan, mouseWheel != 0 ? vec2(mousePos) : vec2(),
                   (mouseWheel > 0 || isZoomIn) ? zoomStep : -zoomStep);
      }
    }
    ImGui::End();

    manager.progressPopup.trigger();

    if (ImGui::BeginPopupModal(manager.progressPopup.label, &manager.progressPopup.isOpen, ImGuiWindowFlags_NoResize))
    {
      if (!animation) return;

      auto& start = manager.recordingStart;
      auto& end = manager.recordingEnd;
      auto progress = (playback.time - start) / (end - start);

      ImGui::ProgressBar(progress);

      if (ImGui::Button("Cancel", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
      {
        playback.isPlaying = false;
        manager.isRecording = false;
        manager.progressPopup.close();
      }

      ImGui::EndPopup();
    }

    if (!document.isAnimationPreviewSet)
    {
      center_view();
      zoom = settings.previewStartZoom;
      document.isAnimationPreviewSet = true;
    }

    settings.previewStartZoom = zoom;
  }
}
