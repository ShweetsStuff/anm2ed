#include "spritesheet_editor.h"

#include "math_.h"
#include "tool.h"
#include "types.h"

using namespace anm2ed::canvas;
using namespace anm2ed::types;
using namespace anm2ed::resource;
using namespace anm2ed::util;
using namespace glm;

namespace anm2ed::imgui
{
  constexpr auto PIVOT_COLOR = color::PINK;

  SpritesheetEditor::SpritesheetEditor() : Canvas(vec2())
  {
  }

  void SpritesheetEditor::update(Manager& manager, Settings& settings, Resources& resources)
  {
    auto& document = *manager.get();
    auto& anm2 = document.anm2;
    auto& reference = document.reference;
    auto& referenceSpritesheet = document.referenceSpritesheet;
    auto& pan = document.editorPan;
    auto& zoom = document.editorZoom;
    auto& backgroundColor = settings.editorBackgroundColor;
    auto& gridColor = settings.editorGridColor;
    auto& gridSize = settings.editorGridSize;
    auto& gridOffset = settings.editorGridOffset;
    auto& isGrid = settings.editorIsGrid;
    auto& zoomStep = settings.viewZoomStep;
    auto& isBorder = settings.editorIsBorder;
    auto spritesheet = document.spritesheet_get();
    auto& tool = settings.tool;
    auto& shaderGrid = resources.shaders[shader::GRID];
    auto& shaderTexture = resources.shaders[shader::TEXTURE];
    auto& dashedShader = resources.shaders[shader::DASHED];

    if (ImGui::Begin("Spritesheet Editor", &settings.windowIsSpritesheetEditor))
    {
      auto childSize = ImVec2(imgui::row_widget_width_get(3),
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

        auto widgetSize = ImVec2(imgui::row_widget_width_get(2), 0);

        imgui::shortcut(settings.shortcutCenterView);
        if (ImGui::Button("Center View", widgetSize)) pan = -size * 0.5f;
        imgui::set_item_tooltip_shortcut("Centers the view.", settings.shortcutCenterView);

        ImGui::SameLine();

        imgui::shortcut(settings.shortcutFit);
        if (ImGui::Button("Fit", widgetSize))
          if (spritesheet) set_to_rect(zoom, pan, {0, 0, spritesheet->texture.size.x, spritesheet->texture.size.y});
        imgui::set_item_tooltip_shortcut("Set the view to match the extent of the spritesheet.", settings.shortcutFit);

        ImGui::TextUnformatted(std::format(POSITION_FORMAT, (int)mousePos.x, (int)mousePos.y).c_str());
      }
      ImGui::EndChild();

      ImGui::SameLine();

      if (ImGui::BeginChild("##Background Child", childSize, true, ImGuiWindowFlags_HorizontalScrollbar))
      {
        ImGui::ColorEdit4("Background", value_ptr(backgroundColor), ImGuiColorEditFlags_NoInputs);

        ImGui::Checkbox("Border", &isBorder);
      }
      ImGui::EndChild();

      auto cursorScreenPos = ImGui::GetCursorScreenPos();

      size_set(to_vec2(ImGui::GetContentRegionAvail()));
      bind();
      viewport_set();
      clear(backgroundColor);

      auto frame = document.frame_get();

      if (spritesheet && spritesheet->texture.is_valid())
      {
        auto& texture = spritesheet->texture;
        auto transform = transform_get(zoom, pan);

        auto spritesheetModel = math::quad_model_get(texture.size);
        auto spritesheetTransform = transform * spritesheetModel;
        texture_render(shaderTexture, texture.id, spritesheetTransform);
        if (isBorder) rect_render(dashedShader, spritesheetTransform, spritesheetModel);

        if (frame && reference.itemID > -1 &&
            anm2.content.layers.at(reference.itemID).spritesheetID == referenceSpritesheet)
        {
          auto cropModel = math::quad_model_get(frame->size, frame->crop);
          auto cropTransform = transform * cropModel;
          rect_render(dashedShader, cropTransform, cropModel, color::RED);

          auto pivotTransform =
              transform * math::quad_model_get(canvas::PIVOT_SIZE, frame->crop + frame->pivot, PIVOT_SIZE * 0.5f);
          texture_render(shaderTexture, resources.icons[icon::PIVOT].id, pivotTransform, PIVOT_COLOR);
        }
      }

      if (isGrid) grid_render(shaderGrid, zoom, pan, gridSize, gridOffset, gridColor);

      unbind();

      ImGui::Image(texture, to_imvec2(size));

      if (ImGui::IsItemHovered())
      {
        ImGui::SetKeyboardFocusHere(-1);

        previousMousePos = mousePos;
        mousePos = position_translate(zoom, pan, to_vec2(ImGui::GetMousePos()) - to_vec2(cursorScreenPos));

        auto isMouseClicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
        auto isMouseDown = ImGui::IsMouseDown(ImGuiMouseButton_Left);
        auto isMouseMiddleDown = ImGui::IsMouseDown(ImGuiMouseButton_Middle);
        auto mouseDelta = to_ivec2(ImGui::GetIO().MouseDelta);
        auto mouseWheel = ImGui::GetIO().MouseWheel;
        auto& toolColor = settings.toolColor;
        auto isZoomIn = imgui::chord_repeating(imgui::string_to_chord(settings.shortcutZoomIn));
        auto isZoomOut = imgui::chord_repeating(imgui::string_to_chord(settings.shortcutZoomOut));
        auto isLeft = imgui::chord_repeating(ImGuiKey_LeftArrow);
        auto isRight = imgui::chord_repeating(ImGuiKey_RightArrow);
        auto isUp = imgui::chord_repeating(ImGuiKey_UpArrow);
        auto isDown = imgui::chord_repeating(ImGuiKey_DownArrow);
        auto isMod = ImGui::IsKeyDown(ImGuiMod_Shift);
        auto step = isMod ? canvas::STEP_FAST : canvas::STEP;
        auto useTool = tool;
        auto isMouseClick = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
        auto isMouseReleased = ImGui::IsMouseReleased(ImGuiMouseButton_Left);
        auto isLeftPressed = ImGui::IsKeyPressed(ImGuiKey_LeftArrow, false);
        auto isRightPressed = ImGui::IsKeyPressed(ImGuiKey_RightArrow, false);
        auto isUpPressed = ImGui::IsKeyPressed(ImGuiKey_UpArrow, false);
        auto isDownPressed = ImGui::IsKeyPressed(ImGuiKey_DownArrow, false);
        auto isLeftReleased = ImGui::IsKeyReleased(ImGuiKey_LeftArrow);
        auto isRightReleased = ImGui::IsKeyReleased(ImGuiKey_RightArrow);
        auto isUpReleased = ImGui::IsKeyReleased(ImGuiKey_UpArrow);
        auto isDownReleased = ImGui::IsKeyReleased(ImGuiKey_DownArrow);
        auto frame = document.frame_get();
        auto isKeyPressed = isLeftPressed || isRightPressed || isUpPressed || isDownPressed;
        auto isKeyReleased = isLeftReleased || isRightReleased || isUpReleased || isDownReleased;
        auto isBegin = isMouseClick || isKeyPressed;
        auto isEnd = isMouseReleased || isKeyReleased;

        if (isMouseMiddleDown) useTool = tool::PAN;

        ImGui::SetMouseCursor(tool::INFO[useTool].cursor);

        switch (useTool)
        {
          case tool::PAN:
            if (isMouseDown || isMouseMiddleDown) pan += mouseDelta;
            break;
          case tool::MOVE:
            if (!frame) break;
            if (isBegin) document.snapshot("Frame Pivot");
            if (isMouseDown) frame->pivot = ivec2(mousePos - frame->crop);
            if (isLeft) frame->pivot.x -= step;
            if (isRight) frame->pivot.x += step;
            if (isUp) frame->pivot.y -= step;
            if (isDown) frame->pivot.y += step;
            if (isEnd) document.change(Document::FRAMES);
            break;
          case tool::CROP:
            if (!frame) break;
            if (isBegin) document.snapshot(isMod ? "Frame Size" : "Frame Crop");
            if (isMouseClicked) frame->crop = ivec2(mousePos);
            if (isMouseDown) frame->size = ivec2(mousePos - frame->crop);
            if (isLeft) isMod ? frame->size.x -= step : frame->crop.x -= step;
            if (isRight) isMod ? frame->size.x += step : frame->crop.x += step;
            if (isUp) isMod ? frame->size.y -= step : frame->crop.y -= step;
            if (isDown) isMod ? frame->size.y += step : frame->crop.y += step;
            if (isEnd) document.change(Document::FRAMES);
            break;
          case tool::DRAW:
          case tool::ERASE:
          {
            if (!spritesheet) break;
            auto color = tool == tool::DRAW ? toolColor : vec4();
            if (isMouseClicked) document.snapshot(tool == tool::DRAW ? "Draw" : "Erase");
            if (isMouseDown) spritesheet->texture.pixel_line(ivec2(previousMousePos), ivec2(mousePos), color);
            if (isMouseReleased) document.change(Document::FRAMES);
            break;
          }
          case tool::COLOR_PICKER:
          {
            if (isMouseDown)
            {
              auto position = to_vec2(ImGui::GetMousePos());
              toolColor = pixel_read(position, {settings.windowSize.x, settings.windowSize.y});
              if (ImGui::BeginTooltip())
              {
                ImGui::ColorButton("##Color Picker Button", to_imvec4(toolColor));
                ImGui::EndTooltip();
              }
            }
            break;
          }
          default:
            break;
        }

        if (mouseWheel != 0 || isZoomIn || isZoomOut)
          zoom_set(zoom, pan, mousePos, (mouseWheel > 0 || isZoomIn) ? zoomStep : -zoomStep);
      }
    }
    ImGui::End();
  }
}
