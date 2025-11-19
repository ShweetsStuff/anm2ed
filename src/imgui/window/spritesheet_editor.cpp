#include "spritesheet_editor.h"

#include <cmath>
#include <format>
#include <utility>

#include "imgui_.h"
#include "imgui_internal.h"
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
  constexpr auto BORDER_DASH_LENGTH = 1.0f;
  constexpr auto BORDER_DASH_GAP = 0.5f;
  constexpr auto BORDER_DASH_OFFSET = 0.0f;

  constexpr auto PIVOT_COLOR = color::PINK;

  SpritesheetEditor::SpritesheetEditor() : Canvas(vec2()) {}

  void SpritesheetEditor::update(Manager& manager, Settings& settings, Resources& resources)
  {
    auto& document = *manager.get();
    auto& anm2 = document.anm2;
    auto& reference = document.reference;
    auto& referenceSpritesheet = document.spritesheet.reference;
    auto& pan = document.editorPan;
    auto& zoom = document.editorZoom;
    auto& backgroundColor = settings.editorBackgroundColor;
    auto& gridColor = settings.editorGridColor;
    auto& gridSize = settings.editorGridSize;
    auto& gridOffset = settings.editorGridOffset;
    auto& toolColor = settings.toolColor;
    auto& isGrid = settings.editorIsGrid;
    auto& isGridSnap = settings.editorIsGridSnap;
    auto& zoomStep = settings.inputZoomStep;
    auto& isBorder = settings.editorIsBorder;
    auto& isTransparent = settings.editorIsTransparent;
    auto spritesheet = document.spritesheet_get();
    auto& tool = settings.tool;
    auto& shaderGrid = resources.shaders[shader::GRID];
    auto& shaderTexture = resources.shaders[shader::TEXTURE];
    auto& dashedShader = resources.shaders[shader::DASHED];

    auto reset_checker_pan = [&]()
    {
      checkerPan = pan;
      checkerSyncPan = pan;
      checkerSyncZoom = zoom;
      isCheckerPanInitialized = true;
      hasPendingZoomPanAdjust = false;
    };

    auto sync_checker_pan = [&]()
    {
      if (!isCheckerPanInitialized)
      {
        reset_checker_pan();
        return;
      }

      if (pan != checkerSyncPan || zoom != checkerSyncZoom)
      {
        bool ignorePanDelta = hasPendingZoomPanAdjust && zoom != checkerSyncZoom;
        if (!ignorePanDelta) checkerPan += pan - checkerSyncPan;
        checkerSyncPan = pan;
        checkerSyncZoom = zoom;
        if (ignorePanDelta) hasPendingZoomPanAdjust = false;
      }
    };

    auto center_view = [&]() { pan = -size * 0.5f; };

    if (ImGui::Begin("Spritesheet Editor", &settings.windowIsSpritesheetEditor))
    {

      auto childSize = ImVec2(imgui::row_widget_width_get(3),
                              (ImGui::GetTextLineHeightWithSpacing() * 4) + (ImGui::GetStyle().WindowPadding.y * 2));

      if (ImGui::BeginChild("##Grid Child", childSize, true, ImGuiWindowFlags_HorizontalScrollbar))
      {
        ImGui::Checkbox("Grid", &isGrid);
        ImGui::SetItemTooltip("Toggle the visibility of the grid.");
        ImGui::SameLine();
        ImGui::Checkbox("Snap", &isGridSnap);
        ImGui::SetItemTooltip("Cropping will snap points to the grid.");
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
        ImGui::SetItemTooltip("Change the zoom of the editor.");

        auto widgetSize = ImVec2(imgui::row_widget_width_get(2), 0);

        imgui::shortcut(manager.chords[SHORTCUT_CENTER_VIEW]);
        if (ImGui::Button("Center View", widgetSize)) center_view();
        imgui::set_item_tooltip_shortcut("Centers the view.", settings.shortcutCenterView);

        ImGui::SameLine();

        imgui::shortcut(manager.chords[SHORTCUT_FIT]);
        if (ImGui::Button("Fit", widgetSize))
          if (spritesheet) set_to_rect(zoom, pan, {0, 0, spritesheet->texture.size.x, spritesheet->texture.size.y});
        imgui::set_item_tooltip_shortcut("Set the view to match the extent of the spritesheet.", settings.shortcutFit);

        ImGui::TextUnformatted(std::format(POSITION_FORMAT, (int)mousePos.x, (int)mousePos.y).c_str());
      }
      ImGui::EndChild();

      ImGui::SameLine();

      if (ImGui::BeginChild("##Background Child", childSize, true, ImGuiWindowFlags_HorizontalScrollbar))
      {
        auto subChildSize = ImVec2(row_widget_width_get(2), ImGui::GetContentRegionAvail().y);

        if (ImGui::BeginChild("##Background Child 1", subChildSize))
        {
          ImGui::ColorEdit3("Background", value_ptr(backgroundColor), ImGuiColorEditFlags_NoInputs);
          ImGui::SetItemTooltip("Change the background color.");

          ImGui::Checkbox("Border", &isBorder);
          ImGui::SetItemTooltip("Toggle a border appearing around the spritesheet.");
        }
        ImGui::EndChild();

        ImGui::SameLine();

        if (ImGui::BeginChild("##Background Child 2", subChildSize))
        {
          ImGui::Checkbox("Transparent", &isTransparent);
          ImGui::SetItemTooltip("Toggle the spritesheet editor being transparent.");
        }

        ImGui::EndChild();
      }
      ImGui::EndChild();

      auto drawList = ImGui::GetCurrentWindow()->DrawList;
      auto cursorScreenPos = ImGui::GetCursorScreenPos();
      auto min = ImGui::GetCursorScreenPos();
      auto max = to_imvec2(to_vec2(min) + size);

      size_set(to_vec2(ImGui::GetContentRegionAvail()));

      bind();
      viewport_set();
      clear(isTransparent ? vec4() : vec4(backgroundColor, 1.0f));

      auto frame = document.frame_get();

      if (spritesheet && spritesheet->texture.is_valid())
      {
        auto& texture = spritesheet->texture;
        auto transform = transform_get(zoom, pan);

        auto spritesheetModel = math::quad_model_get(texture.size);
        auto spritesheetTransform = transform * spritesheetModel;

        texture_render(shaderTexture, texture.id, spritesheetTransform);

        if (isGrid) grid_render(shaderGrid, zoom, pan, gridSize, gridOffset, gridColor);

        if (isBorder)
          rect_render(dashedShader, spritesheetTransform, spritesheetModel, color::WHITE, BORDER_DASH_LENGTH,
                      BORDER_DASH_GAP, BORDER_DASH_OFFSET);

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

      unbind();

      sync_checker_pan();
      render_checker_background(drawList, min, max, -size * 0.5f - checkerPan, CHECKER_SIZE);
      if (!isTransparent) drawList->AddRectFilled(min, max, ImGui::GetColorU32(to_imvec4(vec4(backgroundColor, 1.0f))));
      drawList->AddImage(texture, min, max);
      ImGui::InvisibleButton("##Spritesheet Editor", to_imvec2(size));

      if (ImGui::IsItemHovered())
      {
        auto isMouseLeftClicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
        auto isMouseLeftReleased = ImGui::IsMouseReleased(ImGuiMouseButton_Left);
        auto isMouseLeftDown = ImGui::IsMouseDown(ImGuiMouseButton_Left);
        auto isMouseMiddleDown = ImGui::IsMouseDown(ImGuiMouseButton_Middle);
        auto isMouseRightClicked = ImGui::IsMouseClicked(ImGuiMouseButton_Right);
        auto isMouseRightDown = ImGui::IsMouseDown(ImGuiMouseButton_Right);
        auto isMouseRightReleased = ImGui::IsMouseReleased(ImGuiMouseButton_Right);
        auto mouseDelta = to_ivec2(ImGui::GetIO().MouseDelta);
        auto mouseWheel = ImGui::GetIO().MouseWheel;
        auto isMouseClicked = isMouseLeftClicked || isMouseRightClicked;
        auto isMouseDown = isMouseLeftDown || isMouseRightDown;
        auto isMouseReleased = isMouseLeftReleased || isMouseRightReleased;

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
        auto stepX = isGridSnap ? step * gridSize.x : step;
        auto stepY = isGridSnap ? step * gridSize.y : step;
        previousMousePos = mousePos;
        mousePos = position_translate(zoom, pan, to_vec2(ImGui::GetMousePos()) - to_vec2(cursorScreenPos));

        auto snap_rect = [&](glm::vec2 minPoint, glm::vec2 maxPoint)
        {
          if (isGridSnap)
          {
            if (gridSize.x != 0)
            {
              auto offsetX = (float)(gridOffset.x);
              auto sizeX = (float)(gridSize.x);
              minPoint.x = std::floor((minPoint.x - offsetX) / sizeX) * sizeX + offsetX;
              maxPoint.x = std::ceil((maxPoint.x - offsetX) / sizeX) * sizeX + offsetX;
            }
            if (gridSize.y != 0)
            {
              auto offsetY = (float)(gridOffset.y);
              auto sizeY = (float)(gridSize.y);
              minPoint.y = std::floor((minPoint.y - offsetY) / sizeY) * sizeY + offsetY;
              maxPoint.y = std::ceil((maxPoint.y - offsetY) / sizeY) * sizeY + offsetY;
            }
          }
          return std::pair{minPoint, maxPoint};
        };

        if (isMouseMiddleDown) useTool = tool::PAN;
        if (tool == tool::MOVE && isMouseRightDown) useTool = tool::CROP;
        if (tool == tool::CROP && isMouseRightDown) useTool = tool::MOVE;
        if (tool == tool::DRAW && isMouseRightDown) useTool = tool::ERASE;
        if (tool == tool::ERASE && isMouseRightDown) useTool = tool::DRAW;

        auto& areaType = tool::INFO[useTool].areaType;
        auto cursor = areaType == tool::SPRITESHEET_EDITOR || areaType == tool::ALL ? tool::INFO[useTool].cursor
                                                                                    : ImGuiMouseCursor_NotAllowed;
        ImGui::SetMouseCursor(cursor);
        ImGui::SetKeyboardFocusHere();

        switch (useTool)
        {
          case tool::PAN:
            if (isMouseDown || isMouseMiddleDown) pan += mouseDelta;
            break;
          case tool::MOVE:
            if (!frame) break;
            if (isBegin) document.snapshot("Frame Pivot");
            if (isMouseDown) frame->pivot = vec2(ivec2(mousePos - frame->crop));
            if (isLeftPressed) frame->pivot.x -= step;
            if (isRightPressed) frame->pivot.x += step;
            if (isUpPressed) frame->pivot.y -= step;
            if (isDownPressed) frame->pivot.y += step;
            frame->pivot = vec2(ivec2(frame->pivot));
            if (isDuring)
            {
              if (ImGui::BeginTooltip())
              {
                auto pivotFormat = math::vec2_format_get(frame->pivot);
                auto pivotString = std::format("Pivot: ({}, {})", pivotFormat, pivotFormat);
                ImGui::Text(pivotString.c_str(), frame->pivot.x, frame->pivot.y);
                ImGui::EndTooltip();
              }
            }

            if (isEnd) document.change(Document::FRAMES);
            break;
          case tool::CROP:
            if (!frame) break;
            if (isBegin) document.snapshot("Frame Crop");

            if (isMouseClicked)
            {
              cropAnchor = mousePos;
              frame->crop = vec2(ivec2(cropAnchor));
              frame->size = vec2();
            }
            if (isMouseDown)
            {
              auto [minPoint, maxPoint] = snap_rect(glm::min(cropAnchor, mousePos), glm::max(cropAnchor, mousePos));
              frame->crop = vec2(ivec2(minPoint));
              frame->size = vec2(ivec2(maxPoint - minPoint));
            }
            if (isLeftPressed) frame->crop.x -= stepX;
            if (isRightPressed) frame->crop.x += stepX;
            if (isUpPressed) frame->crop.y -= stepY;
            if (isDownPressed) frame->crop.y += stepY;
            frame->crop = vec2(ivec2(frame->crop));
            frame->size = vec2(ivec2(frame->size));
            if (isDuring)
            {
              if (!isMouseDown)
              {
                auto minPoint = glm::min(frame->crop, frame->crop + frame->size);
                auto maxPoint = glm::max(frame->crop, frame->crop + frame->size);
                frame->crop = vec2(ivec2(minPoint));
                frame->size = vec2(ivec2(maxPoint - minPoint));
                if (isGridSnap)
                {
                  auto [snapMin, snapMax] = snap_rect(frame->crop, frame->crop + frame->size);
                  frame->crop = snapMin;
                  frame->size = snapMax - snapMin;
                }
              }
              if (ImGui::BeginTooltip())
              {
                auto cropFormat = math::vec2_format_get(frame->crop);
                auto sizeFormat = math::vec2_format_get(frame->size);
                auto cropString = std::format("Crop: ({}, {})", cropFormat, cropFormat);
                auto sizeString = std::format("Size: ({}, {})", sizeFormat, sizeFormat);
                ImGui::Text(cropString.c_str(), frame->crop.x, frame->crop.y);
                ImGui::Text(sizeString.c_str(), frame->size.x, frame->size.y);
                ImGui::EndTooltip();
              }
            }
            if (isEnd) document.change(Document::FRAMES);
            break;
          case tool::DRAW:
          case tool::ERASE:
          {
            if (!spritesheet) break;
            auto color = useTool == tool::DRAW ? toolColor : vec4();
            if (isMouseClicked) document.snapshot(useTool == tool::DRAW ? "Draw" : "Erase");
            if (isMouseDown) spritesheet->texture.pixel_line(ivec2(previousMousePos), ivec2(mousePos), color);
            if (isMouseReleased) document.change(Document::SPRITESHEETS);
            break;
          }
          case tool::COLOR_PICKER:
          {
            if (spritesheet && isDuring)
            {
              toolColor = spritesheet->texture.pixel_read(mousePos);
              if (ImGui::BeginTooltip())
              {
                ImGui::ColorButton("##Color Picker Button", to_imvec4(toolColor));
                ImGui::SameLine();
                auto rgba8 = glm::clamp(ivec4(toolColor * 255.0f + 0.5f), ivec4(0), ivec4(255));
                auto hex = std::format("#{:02X}{:02X}{:02X}{:02X}", rgba8.r, rgba8.g, rgba8.b, rgba8.a);
                ImGui::TextUnformatted(hex.c_str());
                ImGui::SameLine();
                ImGui::Text("(%d, %d, %d, %d)", rgba8.r, rgba8.g, rgba8.b, rgba8.a);
                ImGui::EndTooltip();
              }
            }
            break;
          }
          default:
            break;
        }

        if (mouseWheel != 0 || isZoomIn || isZoomOut)
        {
          auto focus = mouseWheel != 0 ? vec2(mousePos) : vec2();
          if (auto spritesheet = document.spritesheet_get(); spritesheet && mouseWheel == 0)
            focus = spritesheet->texture.size / 2;

          auto previousZoom = zoom;
          zoom_set(zoom, pan, focus, (mouseWheel > 0 || isZoomIn) ? zoomStep : -zoomStep);
          if (zoom != previousZoom) hasPendingZoomPanAdjust = true;
        }
      }
    }
    ImGui::End();

    if (!document.isSpritesheetEditorSet)
    {
      size = settings.editorSize;
      zoom = settings.editorStartZoom;
      set();
      center_view();
      reset_checker_pan();
      document.isSpritesheetEditorSet = true;
    }

    settings.editorSize = size;
    settings.editorStartZoom = zoom;
  }
}
