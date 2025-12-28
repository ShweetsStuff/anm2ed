#include "spritesheet_editor.h"

#include <cmath>
#include <format>
#include <utility>

#include "imgui_.h"
#include "imgui_internal.h"
#include "math_.h"
#include "strings.h"
#include "tool.h"
#include "types.h"

using namespace anm2ed::types;
using namespace anm2ed::resource;
using namespace anm2ed::util;
using namespace glm;

namespace anm2ed::imgui
{
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
    auto& frames = document.frames.selection;

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

    auto fit_view = [&]()
    {
      if (spritesheet && spritesheet->texture.is_valid())
        set_to_rect(zoom, pan, {0, 0, (float)spritesheet->texture.size.x, (float)spritesheet->texture.size.y});
    };

    auto zoom_adjust = [&](float delta)
    {
      auto focus = position_translate(zoom, pan, size * 0.5f);
      auto previousZoom = zoom;
      zoom_set(zoom, pan, focus, delta);
      if (zoom != previousZoom) hasPendingZoomPanAdjust = true;
    };

    auto zoom_in = [&]() { zoom_adjust(zoomStep); };
    auto zoom_out = [&]() { zoom_adjust(-zoomStep); };

    if (ImGui::Begin(localize.get(LABEL_SPRITESHEET_EDITOR_WINDOW), &settings.windowIsSpritesheetEditor))
    {

      auto childSize = ImVec2(imgui::row_widget_width_get(3),
                              (ImGui::GetTextLineHeightWithSpacing() * 4) + (ImGui::GetStyle().WindowPadding.y * 2));

      if (ImGui::BeginChild("##Grid Child", childSize, true, ImGuiWindowFlags_HorizontalScrollbar))
      {
        ImGui::Checkbox(localize.get(BASIC_GRID), &isGrid);
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_GRID_VISIBILITY));
        ImGui::SameLine();
        ImGui::Checkbox(localize.get(LABEL_SNAP), &isGridSnap);
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_GRID_SNAP));
        ImGui::SameLine();
        ImGui::ColorEdit4(localize.get(BASIC_COLOR), value_ptr(gridColor), ImGuiColorEditFlags_NoInputs);
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_GRID_COLOR));

        input_int2_range(localize.get(BASIC_SIZE), gridSize, ivec2(GRID_SIZE_MIN), ivec2(GRID_SIZE_MAX));
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_GRID_SIZE));

        input_int2_range(localize.get(BASIC_OFFSET), gridOffset, ivec2(GRID_OFFSET_MIN), ivec2(GRID_OFFSET_MAX));
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_GRID_OFFSET));
      }
      ImGui::EndChild();

      ImGui::SameLine();

      if (ImGui::BeginChild("##View Child", childSize, true, ImGuiWindowFlags_HorizontalScrollbar))
      {
        ImGui::InputFloat(localize.get(BASIC_ZOOM), &zoom, zoomStep, zoomStep, "%.0f%%");
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_EDITOR_ZOOM));

        auto widgetSize = ImVec2(imgui::row_widget_width_get(2), 0);

        imgui::shortcut(manager.chords[SHORTCUT_CENTER_VIEW]);
        if (ImGui::Button(localize.get(LABEL_CENTER_VIEW), widgetSize)) center_view();
        imgui::set_item_tooltip_shortcut(localize.get(TOOLTIP_CENTER_VIEW), settings.shortcutCenterView);

        ImGui::SameLine();

        imgui::shortcut(manager.chords[SHORTCUT_FIT]);
        if (ImGui::Button(localize.get(LABEL_FIT), widgetSize)) fit_view();
        imgui::set_item_tooltip_shortcut(localize.get(TOOLTIP_FIT), settings.shortcutFit);

        auto mousePosInt = ivec2(mousePos);
        ImGui::TextUnformatted(
            std::vformat(localize.get(FORMAT_POSITION_SPACED), std::make_format_args(mousePosInt.x, mousePosInt.y))
                .c_str());
      }
      ImGui::EndChild();

      ImGui::SameLine();

      if (ImGui::BeginChild("##Background Child", childSize, true, ImGuiWindowFlags_HorizontalScrollbar))
      {
        auto subChildSize = ImVec2(row_widget_width_get(2), ImGui::GetContentRegionAvail().y);

        if (ImGui::BeginChild("##Background Child 1", subChildSize))
        {
          ImGui::BeginDisabled(isTransparent);
          {
            ImGui::ColorEdit3(localize.get(LABEL_BACKGROUND_COLOR), value_ptr(backgroundColor),
                              ImGuiColorEditFlags_NoInputs);
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_BACKGROUND_COLOR));
          }
          ImGui::EndDisabled();

          ImGui::Checkbox(localize.get(LABEL_BORDER), &isBorder);
          ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_SPRITESHEET_BORDER));
        }
        ImGui::EndChild();

        ImGui::SameLine();

        if (ImGui::BeginChild("##Background Child 2", subChildSize))
        {
          ImGui::Checkbox(localize.get(LABEL_TRANSPARENT), &isTransparent);
          ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_TRANSPARENT));
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
      clear(isTransparent ? vec4(0) : vec4(backgroundColor, 1.0f));

      auto frame = document.frame_get();
      auto item = document.item_get();

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
              transform * math::quad_model_get(PIVOT_SIZE, frame->crop + frame->pivot, PIVOT_SIZE * 0.5f);
          texture_render(shaderTexture, resources.icons[icon::PIVOT].id, pivotTransform, PIVOT_COLOR);
        }
      }

      unbind();

      sync_checker_pan();
      if (isTransparent)
        render_checker_background(drawList, min, max, -size * 0.5f - checkerPan, CHECKER_SIZE);
      else
        drawList->AddRectFilled(min, max, ImGui::GetColorU32(to_imvec4(vec4(backgroundColor, 1.0f))));
      ImGui::Image(texture, to_imvec2(size));

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

        auto isZoomIn = shortcut(manager.chords[SHORTCUT_ZOOM_IN], shortcut::GLOBAL);
        auto isZoomOut = shortcut(manager.chords[SHORTCUT_ZOOM_OUT], shortcut::GLOBAL);

        auto isBegin = isMouseClicked || isKeyJustPressed;
        auto isDuring = isMouseDown || isKeyDown;
        auto isEnd = isMouseReleased || isKeyReleased;

        auto isMod = ImGui::IsKeyDown(ImGuiMod_Shift);

        auto frame = document.frame_get();
        auto useTool = tool;
        auto step = isMod ? STEP_FAST : STEP;
        auto stepX = isGridSnap ? step * gridSize.x : step;
        auto stepY = isGridSnap ? step * gridSize.y : step;
        previousMousePos = mousePos;
        mousePos = position_translate(zoom, pan, to_ivec2(ImGui::GetMousePos()) - to_ivec2(cursorScreenPos));

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

        auto frame_change_apply = [&](anm2::FrameChange frameChange, anm2::ChangeType changeType = anm2::ADJUST)
        { item->frames_change(frameChange, changeType, frames); };

        if (isMouseMiddleDown) useTool = tool::PAN;
        if (tool == tool::MOVE && isMouseRightDown) useTool = tool::CROP;
        if (tool == tool::CROP && isMouseRightDown) useTool = tool::MOVE;
        if (tool == tool::DRAW && isMouseRightDown) useTool = tool::ERASE;
        if (tool == tool::ERASE && isMouseRightDown) useTool = tool::DRAW;

        auto& toolInfo = tool::INFO[useTool];
        auto& areaType = toolInfo.areaType;
        bool isAreaAllowed = areaType == tool::ALL || areaType == tool::SPRITESHEET_EDITOR;
        bool isFrameRequired =
            !(useTool == tool::PAN || useTool == tool::DRAW || useTool == tool::ERASE || useTool == tool::COLOR_PICKER);
        bool isFrameAvailable = !isFrameRequired || frame;
        bool isSpritesheetRequired = useTool == tool::DRAW || useTool == tool::ERASE || useTool == tool::COLOR_PICKER;
        bool isSpritesheetAvailable = !isSpritesheetRequired || (spritesheet && spritesheet->texture.is_valid());
        auto cursor = (isAreaAllowed && isFrameAvailable && isSpritesheetAvailable) ? toolInfo.cursor
                                                                                    : ImGuiMouseCursor_NotAllowed;
        ImGui::SetMouseCursor(cursor);
        ImGui::SetKeyboardFocusHere();

        switch (useTool)
        {
          case tool::PAN:
            if (isMouseDown || isMouseMiddleDown) pan += mouseDelta;
            break;
          case tool::MOVE:
            if (!item || frames.empty()) break;
            if (isBegin) document.snapshot(localize.get(EDIT_FRAME_PIVOT));
            if (isMouseDown)
            {
              frame->crop = ivec2(frame->crop);
              frame_change_apply(
                  {.pivotX = (int)(mousePos.x - frame->crop.x), .pivotY = (int)(mousePos.y - frame->crop.y)});
            }
            if (isLeftPressed) frame_change_apply({.pivotX = step}, anm2::SUBTRACT);
            if (isRightPressed) frame_change_apply({.pivotX = step}, anm2::ADD);
            if (isUpPressed) frame_change_apply({.pivotY = step}, anm2::SUBTRACT);
            if (isDownPressed) frame_change_apply({.pivotY = step}, anm2::ADD);

            frame_change_apply({.pivotX = (int)frame->pivot.x, .pivotY = (int)frame->pivot.y});

            if (isDuring)
            {
              if (ImGui::BeginTooltip())
              {
                ImGui::TextUnformatted(
                    std::vformat(localize.get(FORMAT_PIVOT), std::make_format_args(frame->pivot.x, frame->pivot.y))
                        .c_str());
                ImGui::EndTooltip();
              }
            }

            if (isEnd) document.change(Document::FRAMES);
            break;
          case tool::CROP:
            if (!item || frames.empty()) break;
            if (isBegin) document.snapshot(localize.get(EDIT_FRAME_CROP));

            if (isMouseClicked)
            {
              cropAnchor = mousePos;
              frame_change_apply({.cropX = (int)cropAnchor.x, .cropY = (int)cropAnchor.y, .sizeX = {}, .sizeY = {}});
            }
            if (isMouseDown)
            {
              auto [minPoint, maxPoint] = snap_rect(glm::min(cropAnchor, mousePos), glm::max(cropAnchor, mousePos));
              frame_change_apply({.cropX = minPoint.x,
                                  .cropY = minPoint.y,
                                  .sizeX = maxPoint.x - minPoint.x,
                                  .sizeY = maxPoint.y - minPoint.y});
            }
            if (isLeftPressed) frame_change_apply({.cropX = stepX}, anm2::SUBTRACT);
            if (isRightPressed) frame_change_apply({.cropX = stepX}, anm2::ADD);
            if (isUpPressed) frame_change_apply({.cropY = stepY}, anm2::SUBTRACT);
            if (isDownPressed) frame_change_apply({.cropY = stepY}, anm2::ADD);

            frame_change_apply(
                {.cropX = frame->crop.x, .cropY = frame->crop.y, .sizeX = frame->size.x, .sizeY = frame->size.y});

            if (isDuring)
            {
              if (!isMouseDown)
              {
                auto minPoint = glm::min(frame->crop, frame->crop + frame->size);
                auto maxPoint = glm::max(frame->crop, frame->crop + frame->size);

                frame_change_apply({.cropX = minPoint.x,
                                    .cropY = minPoint.y,
                                    .sizeX = maxPoint.x - minPoint.x,
                                    .sizeY = maxPoint.y - minPoint.y});

                if (isGridSnap)
                {
                  auto [snapMin, snapMax] = snap_rect(frame->crop, frame->crop + frame->size);

                  frame_change_apply({.cropX = snapMin.x,
                                      .cropY = snapMin.y,
                                      .sizeX = snapMax.x - snapMin.x,
                                      .sizeY = snapMax.y - snapMin.y});
                }
              }
              if (ImGui::BeginTooltip())
              {
                ImGui::TextUnformatted(
                    std::vformat(localize.get(FORMAT_CROP), std::make_format_args(frame->crop.x, frame->crop.y))
                        .c_str());
                ImGui::TextUnformatted(
                    std::vformat(localize.get(FORMAT_SIZE), std::make_format_args(frame->size.x, frame->size.y))
                        .c_str());
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
            if (isMouseClicked)
              document.snapshot(useTool == tool::DRAW ? localize.get(EDIT_DRAW) : localize.get(EDIT_ERASE));
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

        if ((isMouseDown || isKeyDown) && useTool != tool::PAN)
        {
          if (!isAreaAllowed && areaType == tool::ANIMATION_PREVIEW)
          {
            if (ImGui::BeginTooltip())
            {
              ImGui::TextUnformatted(localize.get(TEXT_TOOL_ANIMATION_PREVIEW));
              ImGui::EndTooltip();
            }
          }
          else if (isSpritesheetRequired && !isSpritesheetAvailable)
          {
            if (ImGui::BeginTooltip())
            {
              ImGui::TextUnformatted(localize.get(TEXT_SELECT_SPRITESHEET));
              ImGui::EndTooltip();
            }
          }
          else if (isFrameRequired && !isFrameAvailable)
          {
            if (ImGui::BeginTooltip())
            {
              ImGui::TextUnformatted(localize.get(TEXT_SELECT_FRAME));
              ImGui::EndTooltip();
            }
          }
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

    if (tool == tool::PAN &&
        ImGui::BeginPopupContextWindow("##Spritesheet Editor Context Menu", ImGuiMouseButton_Right))
    {
      if (ImGui::MenuItem(localize.get(SHORTCUT_STRING_UNDO), settings.shortcutUndo.c_str(), false,
                          document.is_able_to_undo()))
        document.undo();

      if (ImGui::MenuItem(localize.get(SHORTCUT_STRING_REDO), settings.shortcutRedo.c_str(), false,
                          document.is_able_to_redo()))
        document.redo();

      ImGui::Separator();

      if (ImGui::MenuItem(localize.get(LABEL_CENTER_VIEW), settings.shortcutCenterView.c_str())) center_view();

      if (ImGui::MenuItem(localize.get(LABEL_FIT), settings.shortcutFit.c_str(), false,
                          spritesheet && spritesheet->texture.is_valid()))
        fit_view();

      ImGui::Separator();

      if (ImGui::MenuItem(localize.get(SHORTCUT_STRING_ZOOM_IN), settings.shortcutZoomIn.c_str())) zoom_in();
      if (ImGui::MenuItem(localize.get(SHORTCUT_STRING_ZOOM_OUT), settings.shortcutZoomOut.c_str())) zoom_out();

      ImGui::EndPopup();
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
