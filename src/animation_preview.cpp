#include "animation_preview.h"

#include <glm/gtc/type_ptr.hpp>

#include "imgui.h"
#include "math.h"
#include "tool.h"
#include "types.h"

using namespace anm2ed::manager;
using namespace anm2ed::settings;
using namespace anm2ed::canvas;
using namespace anm2ed::playback;
using namespace anm2ed::resources;
using namespace anm2ed::types;
using namespace glm;

namespace anm2ed::animation_preview
{
  constexpr auto NULL_COLOR = vec4(0.0f, 0.0f, 1.0f, 0.90f);
  constexpr auto TARGET_SIZE = vec2(32, 32);
  constexpr auto POINT_SIZE = vec2(4, 4);
  constexpr auto NULL_RECT_SIZE = vec2(100);
  constexpr auto TRIGGER_TEXT_COLOR = ImVec4(1.0f, 1.0f, 1.0f, 0.5f);

  AnimationPreview::AnimationPreview() : Canvas(vec2())
  {
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
    auto& isIcons = settings.previewIsIcons;
    auto& isAltIcons = settings.previewIsAltIcons;
    auto& isBorder = settings.previewIsBorder;
    auto& tool = settings.tool;
    auto& shaderLine = resources.shaders[shader::LINE];
    auto& shaderAxes = resources.shaders[shader::AXIS];
    auto& shaderGrid = resources.shaders[shader::GRID];
    auto& shaderTexture = resources.shaders[shader::TEXTURE];

    if (ImGui::Begin("Animation Preview", &settings.windowIsAnimationPreview))
    {

      auto childSize = ImVec2(imgui::row_widget_width_get(4),
                              (ImGui::GetTextLineHeightWithSpacing() * 4) + (ImGui::GetStyle().WindowPadding.y * 2));

      if (ImGui::BeginChild("##Grid Child", childSize, true, ImGuiWindowFlags_HorizontalScrollbar))
      {

        ImGui::Checkbox("Grid", &isGrid);
        ImGui::SameLine();
        ImGui::ColorEdit4("Color", value_ptr(gridColor), ImGuiColorEditFlags_NoInputs);
        ImGui::InputInt2("Size", value_ptr(gridSize));
        ImGui::InputInt2("Offset", value_ptr(gridOffset));
      }
      ImGui::EndChild();

      ImGui::SameLine();

      if (ImGui::BeginChild("##View Child", childSize, true, ImGuiWindowFlags_HorizontalScrollbar))
      {
        ImGui::InputFloat("Zoom", &zoom, zoomStep, zoomStep, "%.0f%%");

        auto widgetSize = imgui::widget_size_with_row_get(2);

        imgui::shortcut(settings.shortcutCenterView);
        if (ImGui::Button("Center View", widgetSize)) pan = vec2();
        imgui::set_item_tooltip_shortcut("Centers the view.", settings.shortcutCenterView);

        ImGui::SameLine();

        imgui::shortcut(settings.shortcutFit);
        if (ImGui::Button("Fit", widgetSize))
          if (animation) set_to_rect(zoom, pan, animation->rect(isRootTransform));
        imgui::set_item_tooltip_shortcut("Set the view to match the extent of the animation.", settings.shortcutFit);

        ImGui::TextUnformatted(std::format(POSITION_FORMAT, (int)mousePos.x, (int)mousePos.y).c_str());
      }
      ImGui::EndChild();

      ImGui::SameLine();

      if (ImGui::BeginChild("##Background Child", childSize, true, ImGuiWindowFlags_HorizontalScrollbar))
      {
        ImGui::ColorEdit4("Background", value_ptr(backgroundColor), ImGuiColorEditFlags_NoInputs);
        ImGui::SameLine();
        ImGui::Checkbox("Axes", &isAxes);
        ImGui::SameLine();
        ImGui::ColorEdit4("Color", value_ptr(axesColor), ImGuiColorEditFlags_NoInputs);

        std::vector<std::string> animationNames{};
        animationNames.emplace_back("None");
        for (auto& animation : anm2.animations.items)
          animationNames.emplace_back(animation.name);

        imgui::combo_strings("Overlay", &overlayIndex, animationNames);

        ImGui::InputFloat("Alpha", &overlayTransparency, 0, 0, "%.0f");
      }
      ImGui::EndChild();

      ImGui::SameLine();

      if (ImGui::BeginChild("##Helpers Child", childSize, true, ImGuiWindowFlags_HorizontalScrollbar))
      {
        auto helpersChildSize = ImVec2(imgui::row_widget_width_get(2), ImGui::GetContentRegionAvail().y);

        if (ImGui::BeginChild("##Helpers Child 1", helpersChildSize))
        {
          ImGui::Checkbox("Root Transform", &isRootTransform);
          ImGui::Checkbox("Pivots", &isPivots);
          ImGui::Checkbox("Icons", &isIcons);
        }
        ImGui::EndChild();

        ImGui::SameLine();

        if (ImGui::BeginChild("##Helpers Child 2", helpersChildSize))
        {
          ImGui::Checkbox("Alt Icons", &isAltIcons);
          ImGui::Checkbox("Border", &isBorder);
        }
        ImGui::EndChild();
      }
      ImGui::EndChild();

      auto cursorScreenPos = ImGui::GetCursorScreenPos();

      size_set(to_vec2(ImGui::GetContentRegionAvail()));
      bind();
      viewport_set();
      clear(backgroundColor);
      if (isAxes) axes_render(shaderAxes, zoom, pan, axesColor);
      if (isGrid) grid_render(shaderGrid, zoom, pan, gridSize, gridOffset, gridColor);

      auto render = [&](float time, vec3 colorOffset = {}, float alphaOffset = {}, bool isOnionskin = false)
      {
        auto transform = transform_get(zoom, pan);
        auto root = animation->rootAnimation.frame_generate(time, anm2::ROOT);

        if (isRootTransform)
          transform *= math::quad_model_parent_get(root.position, {}, math::percent_to_unit(root.scale), root.rotation);

        if (isIcons && root.isVisible && animation->rootAnimation.isVisible)
        {
          auto rootTransform = transform * math::quad_model_get(TARGET_SIZE, root.position, TARGET_SIZE * 0.5f,
                                                                math::percent_to_unit(root.scale), root.rotation);

          vec4 color = isOnionskin ? vec4(colorOffset, alphaOffset) : color::GREEN;

          texture_render(shaderTexture, resources.icons[icon::TARGET].id, rootTransform, color);
        }

        for (auto& id : animation->layerOrder)
        {
          auto& layerAnimation = animation->layerAnimations.at(id);
          if (!layerAnimation.isVisible) continue;

          auto& layer = anm2.content.layers.at(id);

          if (auto frame = layerAnimation.frame_generate(time, anm2::LAYER); frame.isVisible)
          {
            auto spritesheet = anm2.spritesheet_get(layer.spritesheetID);
            if (!spritesheet) continue;

            auto& texture = spritesheet->texture;
            if (!texture.is_valid()) continue;

            auto layerTransform = transform * math::quad_model_get(frame.size, frame.position, frame.pivot,
                                                                   math::percent_to_unit(frame.scale), frame.rotation);

            auto uvMin = frame.crop / vec2(texture.size);
            auto uvMax = (frame.crop + frame.size) / vec2(texture.size);
            auto vertices = math::uv_vertices_get(uvMin, uvMax);
            vec3 frameColorOffset = frame.offset + colorOffset;
            vec4 frameTint = frame.tint;
            frameTint.a = std::max(0.0f, frameTint.a - alphaOffset);

            texture_render(shaderTexture, texture.id, layerTransform, frameTint, frameColorOffset, vertices.data());

            auto color = isOnionskin ? vec4(colorOffset, 1.0f - alphaOffset) : color::RED;

            if (isBorder) rect_render(shaderLine, layerTransform, color);

            if (isPivots)
            {
              auto pivotTransform =
                  transform * math::quad_model_get(PIVOT_SIZE, frame.position, PIVOT_SIZE * 0.5f,
                                                   math::percent_to_unit(frame.scale), frame.rotation);

              texture_render(shaderTexture, resources.icons[icon::PIVOT].id, pivotTransform, color);
            }
          }
        }

        if (isIcons)
        {
          for (auto& [id, nullAnimation] : animation->nullAnimations)
          {
            if (!nullAnimation.isVisible) continue;

            auto& isShowRect = anm2.content.nulls[id].isShowRect;

            if (auto frame = nullAnimation.frame_generate(time, anm2::NULL_); frame.isVisible)
            {
              auto icon = isShowRect ? icon::POINT : icon::TARGET;
              auto& size = isShowRect ? POINT_SIZE : TARGET_SIZE;
              auto color = isOnionskin ? vec4(colorOffset, 1.0f - alphaOffset)
                           : id == reference.itemID && reference.itemType == anm2::NULL_ ? color::RED
                                                                                         : NULL_COLOR;

              auto nullTransform = transform * math::quad_model_get(size, frame.position, size * 0.5f,
                                                                    math::percent_to_unit(frame.scale), frame.rotation);

              texture_render(shaderTexture, resources.icons[icon].id, nullTransform, color);

              if (isShowRect)
              {
                auto rectTransform =
                    transform * math::quad_model_get(NULL_RECT_SIZE, frame.position, NULL_RECT_SIZE * 0.5f,
                                                     math::percent_to_unit(frame.scale), frame.rotation);

                rect_render(shaderLine, rectTransform, color);
              }
            }
          }
        }
      };

      auto onionskin_render = [&](float time, int count, int direction, vec3 color)
      {
        for (int i = 1; i <= count; i++)
        {
          float useTime = time + (float)(direction * i);
          float alphaOffset = (1.0f / (count + 1)) * i;
          render(useTime, color, alphaOffset, true);
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
        render(frameTime);
        if (drawOrder == draw_order::ABOVE && isEnabled) onionskins_render(frameTime);
      }

      unbind();

      ImGui::Image(texture, to_imvec2(size));

      isPreviewHovered = ImGui::IsItemHovered();

      if (animation && animation->triggers.isVisible)
      {
        if (auto trigger = animation->triggers.frame_generate(frameTime, anm2::TRIGGER); trigger.isVisible)
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
        ImGui::SetKeyboardFocusHere(-1);

        mousePos = position_translate(zoom, pan, to_vec2(ImGui::GetMousePos()) - to_vec2(cursorScreenPos));

        auto isMouseClick = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
        auto isMouseReleased = ImGui::IsMouseReleased(ImGuiMouseButton_Left);
        auto isMouseDown = ImGui::IsMouseDown(ImGuiMouseButton_Left);
        auto isMouseMiddleDown = ImGui::IsMouseDown(ImGuiMouseButton_Middle);
        auto isLeftPressed = ImGui::IsKeyPressed(ImGuiKey_LeftArrow, false);
        auto isRightPressed = ImGui::IsKeyPressed(ImGuiKey_RightArrow, false);
        auto isUpPressed = ImGui::IsKeyPressed(ImGuiKey_UpArrow, false);
        auto isDownPressed = ImGui::IsKeyPressed(ImGuiKey_DownArrow, false);
        auto isLeftReleased = ImGui::IsKeyReleased(ImGuiKey_LeftArrow);
        auto isRightReleased = ImGui::IsKeyReleased(ImGuiKey_RightArrow);
        auto isUpReleased = ImGui::IsKeyReleased(ImGuiKey_UpArrow);
        auto isDownReleased = ImGui::IsKeyReleased(ImGuiKey_DownArrow);
        auto isLeft = imgui::chord_repeating(ImGuiKey_LeftArrow);
        auto isRight = imgui::chord_repeating(ImGuiKey_RightArrow);
        auto isUp = imgui::chord_repeating(ImGuiKey_UpArrow);
        auto isDown = imgui::chord_repeating(ImGuiKey_DownArrow);
        auto isMouseRightDown = ImGui::IsMouseDown(ImGuiMouseButton_Right);
        auto mouseDelta = to_ivec2(ImGui::GetIO().MouseDelta);
        auto mouseWheel = ImGui::GetIO().MouseWheel;
        auto isZoomIn = imgui::chord_repeating(imgui::string_to_chord(settings.shortcutZoomIn));
        auto isZoomOut = imgui::chord_repeating(imgui::string_to_chord(settings.shortcutZoomOut));
        auto isMod = ImGui::IsKeyDown(ImGuiMod_Shift);
        auto frame = document.frame_get();
        auto useTool = tool;
        auto step = isMod ? step::FAST : step::NORMAL;
        auto isKeyPressed = isLeftPressed || isRightPressed || isUpPressed || isDownPressed;
        auto isKeyReleased = isLeftReleased || isRightReleased || isUpReleased || isDownReleased;
        auto isBegin = isMouseClick || isKeyPressed;
        auto isEnd = isMouseReleased || isKeyReleased;

        if (isMouseMiddleDown) useTool = tool::PAN;
        if (tool == tool::MOVE && isMouseRightDown) useTool = tool::SCALE;
        if (tool == tool::SCALE && isMouseRightDown) useTool = tool::MOVE;

        switch (useTool)
        {
          case tool::PAN:
            if (isMouseDown || isMouseMiddleDown) pan += mouseDelta;
            break;
          case tool::MOVE:
            if (!frame) break;
            if (isBegin) document.snapshot("Frame Position");
            if (isMouseDown) frame->position = mousePos;
            if (isLeft) frame->position.x -= step;
            if (isRight) frame->position.x += step;
            if (isUp) frame->position.y -= step;
            if (isDown) frame->position.y += step;
            if (isEnd) document.change(change::FRAMES);
            break;
          case tool::SCALE:
            if (!frame) break;
            if (isBegin) document.snapshot("Frame Scale");
            if (isMouseDown) frame->scale += mouseDelta;
            if (isLeft) frame->scale.x -= step;
            if (isRight) frame->scale.x += step;
            if (isUp) frame->scale.y -= step;
            if (isDown) frame->scale.y += step;
            if (isEnd) document.change(change::FRAMES);
            break;
          case tool::ROTATE:
            if (!frame) break;
            if (isBegin) document.snapshot("Frame Rotation");
            if (isMouseDown) frame->rotation += mouseDelta.y;
            if (isLeft || isDown) frame->rotation -= step;
            if (isUp || isRight) frame->rotation += step;
            if (isEnd) document.change(change::FRAMES);
            break;
          default:
            break;
        }

        if (mouseWheel != 0 || isZoomIn || isZoomOut)
          zoom_set(zoom, pan, vec2(mousePos), (mouseWheel > 0 || isZoomIn) ? zoomStep : -zoomStep);
      }
    }
    ImGui::End();
  }
}
