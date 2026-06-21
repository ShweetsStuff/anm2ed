#include "spritesheet_editor.hpp"

#include <cmath>
#include <format>
#include <set>
#include <utility>

#include "actions.hpp"
#include "imgui_internal.h"
#include "math.hpp"
#include "strings.hpp"
#include "tool.hpp"
#include "types.hpp"
#include "util/imgui/draw.hpp"
#include "util/imgui/input.hpp"
#include "util/imgui/layout.hpp"
#include "util/imgui/shortcut.hpp"
#include "util/imgui/tooltip.hpp"

using namespace anm2ed::types;
using namespace anm2ed::resource;
using namespace anm2ed::util;
using namespace glm;

namespace anm2ed::imgui
{
  SpritesheetEditor::SpritesheetEditor() : Canvas(vec2()) {}

  bool SpritesheetEditor::is_focused_get() const { return isFocused; }

  void SpritesheetEditor::update(Manager& manager, Settings& settings, Resources& resources)
  {
    isFocused = false;

    auto& document = *manager.get();
    auto& anm2 = document.anm2;
    auto& reference = document.reference;
    auto& referenceSpritesheet = document.spritesheet.reference;
    auto referenceItemType = static_cast<ItemType>(reference.itemType);
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
    auto selected_layer_spritesheet_get = [&]()
    {
      auto is_valid_layer_frame = [&](const Reference& frameReference)
      {
        if (frameReference.frameIndex < 0 || frameReference.itemType != LAYER) return false;
        auto frameItem = anm2.element_get(frameReference.animationIndex, ItemType::LAYER, frameReference.itemID);
        return frameItem && track_frame_get(*frameItem, frameReference.frameIndex);
      };
      if (is_valid_layer_frame(reference))
        if (auto layer = anm2.element_get(ElementType::LAYER_ELEMENT, reference.itemID))
          return layer->spritesheetId;

      auto frameReferences = document.frames.references;
      for (auto frameIndex : document.frames.selection)
        frameReferences.insert({reference.animationIndex, reference.itemType, reference.itemID, frameIndex});
      for (auto frameReference : frameReferences)
        if (is_valid_layer_frame(frameReference))
          if (auto layer = anm2.element_get(ElementType::LAYER_ELEMENT, frameReference.itemID))
            return layer->spritesheetId;
      return -1;
    };
    if (auto selectedLayerSpritesheet = selected_layer_spritesheet_get(); selectedLayerSpritesheet != -1)
      referenceSpritesheet = selectedLayerSpritesheet;
    auto spritesheet = anm2.element_get(ElementType::SPRITESHEET, referenceSpritesheet);
    auto texture = document.texture_get(referenceSpritesheet);
    auto& tool = settings.tool;
    auto& shaderGrid = resources.shaders[shader::GRID];
    auto& shaderTexture = resources.shaders[shader::TEXTURE];
    auto& shaderLine = resources.shaders[shader::LINE];
    auto& dashedShader = resources.shaders[shader::DASHED];
    auto& regionReference = document.region.reference;
    auto& regionSelection = document.region.selection;

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
      if (texture && texture->is_valid())
        set_to_rect(zoom, pan, {0, 0, (float)texture->size.x, (float)texture->size.y});
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

    auto region_get = [&](int id)
    { return spritesheet ? element_child_id_get(*spritesheet, ElementType::REGION, id) : nullptr; };

    if (ImGui::Begin(localize.get(LABEL_SPRITESHEET_EDITOR_WINDOW), &settings.windowIsSpritesheetEditor))
    {
      isFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

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
      size_set(to_vec2(ImGui::GetContentRegionAvail()));

      auto cursorScreenPos = ImGui::GetCursorScreenPos();
      auto min = ImGui::GetCursorScreenPos();
      auto max = to_imvec2(to_vec2(min) + size);

      auto mouseScreenPos = ImGui::GetIO().MousePos;
      bool isMouseOverCanvas = mouseScreenPos.x >= min.x && mouseScreenPos.x <= max.x && mouseScreenPos.y >= min.y &&
                               mouseScreenPos.y <= max.y;
      auto hoverMousePos = vec2();
      if (isMouseOverCanvas)
        hoverMousePos = position_translate(zoom, pan, to_ivec2(mouseScreenPos) - to_ivec2(cursorScreenPos));

      bind();
      viewport_set();
      clear(isTransparent ? vec4(0) : vec4(backgroundColor, 1.0f));

      auto frame =
          anm2.element_get(reference.animationIndex, referenceItemType, reference.frameIndex, reference.itemID);

      if (spritesheet && texture && texture->is_valid())
      {
        auto transform = transform_get(zoom, pan);

        auto spritesheetModel = math::quad_model_get(texture->size);
        auto spritesheetTransform = transform * spritesheetModel;

        texture_render(shaderTexture, texture->id, spritesheetTransform);

        if (isGrid) grid_render(shaderGrid, zoom, pan, gridSize, gridOffset, gridColor);

        if (isBorder)
          rect_render(dashedShader, spritesheetTransform, spritesheetModel, color::WHITE, BORDER_DASH_LENGTH,
                      BORDER_DASH_GAP, BORDER_DASH_OFFSET);

        if (hoveredRegionId != -1)
        {
          if (auto region = region_get(hoveredRegionId))
          {
            auto cropModel = math::quad_model_get(region->size, region->crop);
            auto cropTransform = transform * cropModel;
            rect_fill_render(shaderLine, cropTransform, cropModel, vec4(1.0f, 1.0f, 1.0f, 0.5f));
          }
        }

        auto layer = anm2.element_get(ElementType::LAYER_ELEMENT, reference.itemID);
        bool isReferenceLayerOnSpritesheet =
            frame && reference.itemID > -1 && layer && layer->spritesheetId == referenceSpritesheet;

        int highlightedRegionId = -1;
        if (isReferenceLayerOnSpritesheet && frame->regionId != -1 && region_get(frame->regionId))
        {
          highlightedRegionId = frame->regionId;
        }
        else if (regionReference != -1 && region_get(regionReference))
        {
          highlightedRegionId = regionReference;
        }

        auto draw_region_rect = [&](Element& region, vec4 regionColor)
        {
          auto cropModel = math::quad_model_get(region.size, region.crop);
          auto cropTransform = transform * cropModel;
          rect_render(dashedShader, cropTransform, cropModel, regionColor, BORDER_DASH_LENGTH, BORDER_DASH_GAP,
                      BORDER_DASH_OFFSET);
        };

        for (auto& region : spritesheet->children)
        {
          if (region.type != ElementType::REGION) continue;
          auto id = region.id;
          if (id == highlightedRegionId) continue;
          draw_region_rect(region, color::WHITE);

          auto pivotTransform =
              transform * math::quad_model_get(PIVOT_SIZE, region.crop + region.pivot, PIVOT_SIZE * 0.5f);
          texture_render(shaderTexture, resources.icons[icon::PIVOT].id, pivotTransform, color::WHITE);
        }

        if (highlightedRegionId != -1)
        {
          if (auto region = region_get(highlightedRegionId))
          {
            draw_region_rect(*region, color::RED);

            auto pivotTransform =
                transform * math::quad_model_get(PIVOT_SIZE, region->crop + region->pivot, PIVOT_SIZE * 0.5f);
            texture_render(shaderTexture, resources.icons[icon::PIVOT].id, pivotTransform, PIVOT_COLOR);
          }
        }

        bool isFrameOnSpritesheet = isReferenceLayerOnSpritesheet;
        if (isFrameOnSpritesheet && frame->regionId == -1)
        {
          auto frameModel = math::quad_model_get(frame->size, frame->crop);
          auto frameTransform = transform * frameModel;
          rect_render(shaderLine, frameTransform, frameModel, color::RED);

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
      ImGui::Image(this->texture, to_imvec2(size));

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

        auto isZoomIn = isFocused && shortcut(manager.chords[SHORTCUT_ZOOM_IN], shortcut::GLOBAL);
        auto isZoomOut = isFocused && shortcut(manager.chords[SHORTCUT_ZOOM_OUT], shortcut::GLOBAL);

        auto isBegin = isMouseClicked || isKeyJustPressed;
        auto isDuring = isMouseDown || isKeyDown;
        auto isEnd = isMouseReleased || isKeyReleased;

        auto isMod = ImGui::IsKeyDown(ImGuiMod_Shift);

        auto is_frame_reference_valid = [&](const Reference& frameReference)
        {
          if (frameReference.frameIndex < 0 || frameReference.itemType != LAYER) return false;
          auto frameLayer = anm2.element_get(ElementType::LAYER_ELEMENT, frameReference.itemID);
          if (!frameLayer || frameLayer->spritesheetId != referenceSpritesheet) return false;
          auto frameItem = anm2.element_get(frameReference.animationIndex, ItemType::LAYER, frameReference.itemID);
          return frameItem && track_frame_get(*frameItem, frameReference.frameIndex);
        };
        auto selectedFrameReferences = document.frames.references;
        if (selectedFrameReferences.empty())
          for (auto frameIndex : document.frames.selection)
            selectedFrameReferences.insert({reference.animationIndex, reference.itemType, reference.itemID, frameIndex});
        std::erase_if(selectedFrameReferences, [&](const Reference& frameReference)
                      { return !is_frame_reference_valid(frameReference); });
        if (selectedFrameReferences.empty() && is_frame_reference_valid(reference))
          selectedFrameReferences.insert(reference);
        auto editReference = reference;
        if (!selectedFrameReferences.contains(reference) && !selectedFrameReferences.empty())
          editReference = *selectedFrameReferences.begin();
        auto editReferenceItemType = static_cast<ItemType>(editReference.itemType);
        auto frame =
            anm2.element_get(editReference.animationIndex, editReferenceItemType, editReference.frameIndex,
                             editReference.itemID);
        auto item = anm2.element_get(editReference.animationIndex, editReferenceItemType, editReference.itemID);
        auto layer = anm2.element_get(ElementType::LAYER_ELEMENT, editReference.itemID);
        bool isReferenceLayerOnSpritesheet =
            frame && editReference.itemID > -1 && layer && layer->spritesheetId == referenceSpritesheet;
        if (selectedFrameReferences.empty() && isReferenceLayerOnSpritesheet)
          selectedFrameReferences.insert(editReference);
        auto useTool = tool;
        auto step = (float)(isMod ? STEP_FAST : STEP);
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
          minPoint = vec2(ivec2(minPoint));
          maxPoint = vec2(ivec2(maxPoint));
          return std::pair{minPoint, maxPoint};
        };

        auto clamp_vec2_to_int = [](const vec2& value) { return vec2(ivec2(value)); };
        auto pivot_snap = [&](vec2 value, vec2 current)
        {
          if (isGridSnap) return clamp_vec2_to_int(value);
          auto fraction = current - glm::floor(current);
          return glm::floor(value) + fraction;
        };
        auto snapshot_push = [&](StringType messageType)
        {
          auto message = std::string(localize.get(messageType));
          manager.command_push(
              {manager.selected, [message](Manager&, Document& document) { document.snapshot(message); }});
        };
        auto document_change_push = [&](Document::ChangeType changeType)
        {
          manager.command_push({manager.selected,
                                [changeType](Manager&, Document& document) { document.anm2_change(changeType); }});
        };
        auto frame_change_apply = [&](FrameChange frameChange, ChangeType changeType = ChangeType::ADJUST)
        {
          auto queuedFrameReferences = selectedFrameReferences;
          manager.command_push({manager.selected,
                                [=](Manager&, Document& document)
                                {
                                  std::map<Reference, std::set<int>> groupedFrames{};
                                  for (auto frameReference : queuedFrameReferences)
                                    groupedFrames[{frameReference.animationIndex, frameReference.itemType,
                                                   frameReference.itemID, -1}]
                                        .insert(frameReference.frameIndex);

                                  for (auto& [itemReference, itemFrames] : groupedFrames)
                                  {
                                    auto itemType = static_cast<ItemType>(itemReference.itemType);
                                    auto item = document.anm2.element_get(itemReference.animationIndex, itemType,
                                                                          itemReference.itemID);
                                    if (!item) continue;
                                    frames_change(*item, frameChange, itemType, changeType, itemFrames);
                                  }
                                }});
        };
        auto frame_change_from_current_apply = [&](auto frameChangeGet, ChangeType changeType = ChangeType::ADJUST)
        {
          auto queuedReference = editReference;
          auto queuedReferenceItemType = editReferenceItemType;
          auto queuedFrameReferences = selectedFrameReferences;
          manager.command_push({manager.selected,
                                [=](Manager&, Document& document)
                                {
                                  auto frame = document.anm2.element_get(queuedReference.animationIndex,
                                                                         queuedReferenceItemType,
                                                                         queuedReference.frameIndex,
                                                                         queuedReference.itemID);
                                  if (!frame) return;

                                  std::map<Reference, std::set<int>> groupedFrames{};
                                  for (auto frameReference : queuedFrameReferences)
                                    groupedFrames[{frameReference.animationIndex, frameReference.itemType,
                                                   frameReference.itemID, -1}]
                                        .insert(frameReference.frameIndex);

                                  for (auto& [itemReference, itemFrames] : groupedFrames)
                                  {
                                    auto itemType = static_cast<ItemType>(itemReference.itemType);
                                    auto item = document.anm2.element_get(itemReference.animationIndex, itemType,
                                                                          itemReference.itemID);
                                    if (!item) continue;
                                    frames_change(*item, frameChangeGet(*frame), itemType, changeType, itemFrames);
                                  }
                                }});
        };
        auto frame_crop_normalize_apply = [&](bool isSnap, ivec2 snapGridSize, ivec2 snapGridOffset)
        {
          frame_change_from_current_apply(
              [=](const Element& frame)
              {
                auto minPoint = glm::min(frame.crop, frame.crop + frame.size);
                auto maxPoint = glm::max(frame.crop, frame.crop + frame.size);

                if (isSnap)
                {
                  if (snapGridSize.x != 0)
                  {
                    auto offsetX = (float)snapGridOffset.x;
                    auto sizeX = (float)snapGridSize.x;
                    minPoint.x = std::floor((minPoint.x - offsetX) / sizeX) * sizeX + offsetX;
                    maxPoint.x = std::ceil((maxPoint.x - offsetX) / sizeX) * sizeX + offsetX;
                  }
                  if (snapGridSize.y != 0)
                  {
                    auto offsetY = (float)snapGridOffset.y;
                    auto sizeY = (float)snapGridSize.y;
                    minPoint.y = std::floor((minPoint.y - offsetY) / sizeY) * sizeY + offsetY;
                    maxPoint.y = std::ceil((maxPoint.y - offsetY) / sizeY) * sizeY + offsetY;
                  }
                }

                return FrameChange{.cropX = minPoint.x,
                                   .cropY = minPoint.y,
                                   .sizeX = maxPoint.x - minPoint.x,
                                   .sizeY = maxPoint.y - minPoint.y};
              });
        };
        auto region_update = [&](int id, auto update)
        {
          auto queuedSpritesheet = referenceSpritesheet;
          manager.command_push({manager.selected,
                                [=](Manager&, Document& document)
                                {
                                  auto spritesheet = document.anm2.element_get(ElementType::SPRITESHEET,
                                                                               queuedSpritesheet);
                                  if (!spritesheet) return;
                                  auto region = element_child_id_get(*spritesheet, ElementType::REGION, id);
                                  if (!region) return;
                                  update(*region);
                                }});
        };
        auto region_update_all = [&](auto update)
        {
          auto queuedSpritesheet = referenceSpritesheet;
          std::set<int> queuedSelection(regionSelection.begin(), regionSelection.end());
          manager.command_push({manager.selected,
                                [=](Manager&, Document& document)
                                {
                                  auto spritesheet = document.anm2.element_get(ElementType::SPRITESHEET,
                                                                               queuedSpritesheet);
                                  if (!spritesheet) return;
                                  for (auto id : queuedSelection)
                                  {
                                    auto region = element_child_id_get(*spritesheet, ElementType::REGION, id);
                                    if (!region) continue;
                                    update(*region);
                                  }
                                }});
        };
        auto region_set_all = [&](const vec2& crop, const vec2& size)
        {
          auto queuedCrop = clamp_vec2_to_int(crop);
          auto queuedSize = clamp_vec2_to_int(size);
          region_update_all(
              [=](Element& region)
              {
                region.crop = queuedCrop;
                region.size = queuedSize;
              });
        };
        auto region_offset_all = [&](const vec2& delta)
        {
          region_update_all(
              [=](Element& region)
              {
                region.crop = clamp_vec2_to_int(region.crop + delta);
                region.size = clamp_vec2_to_int(region.size);
              });
        };
        auto region_crop_normalize_all = [&](bool isSnap, ivec2 snapGridSize, ivec2 snapGridOffset)
        {
          region_update_all(
              [=](Element& region)
              {
                auto minPoint = glm::min(region.crop, region.crop + region.size);
                auto maxPoint = glm::max(region.crop, region.crop + region.size);

                if (isSnap)
                {
                  if (snapGridSize.x != 0)
                  {
                    auto offsetX = (float)snapGridOffset.x;
                    auto sizeX = (float)snapGridSize.x;
                    minPoint.x = std::floor((minPoint.x - offsetX) / sizeX) * sizeX + offsetX;
                    maxPoint.x = std::ceil((maxPoint.x - offsetX) / sizeX) * sizeX + offsetX;
                  }
                  if (snapGridSize.y != 0)
                  {
                    auto offsetY = (float)snapGridOffset.y;
                    auto sizeY = (float)snapGridSize.y;
                    minPoint.y = std::floor((minPoint.y - offsetY) / sizeY) * sizeY + offsetY;
                    maxPoint.y = std::ceil((maxPoint.y - offsetY) / sizeY) * sizeY + offsetY;
                  }
                }

                region.crop = clamp_vec2_to_int(minPoint);
                region.size = clamp_vec2_to_int(maxPoint - minPoint);
              });
        };
        auto texture_line_apply = [&](ivec2 start, ivec2 end, vec4 color)
        {
          auto queuedSpritesheet = referenceSpritesheet;
          manager.command_push({manager.selected,
                                [=](Manager&, Document& document)
                                {
                                  auto texture = document.texture_get(queuedSpritesheet);
                                  if (!texture) return;
                                  texture->pixel_line(start, end, color);
                                }});
        };
        auto texture_change_push = [&]()
        {
          auto queuedSpritesheet = referenceSpritesheet;
          manager.command_push({manager.selected,
                                [=](Manager&, Document& document) { document.texture_change(queuedSpritesheet); }});
        };

        auto region_selection_set = [&](int id)
        {
          manager.command_push({manager.selected,
                                [=](Manager&, Document& document)
                                {
                                  document.region.reference = id;
                                  document.region.selection = {id};
                                }});
        };
        auto region_pivot_set = [&](int id, vec2 pivot)
        {
          auto queuedPivot = pivot;
          region_update(id,
                        [=](Element& region)
                        {
                          region.origin = Origin::CUSTOM;
                          region.pivot = queuedPivot;
                        });
        };
        auto region_pivot_offset = [&](int id, vec2 delta)
        {
          region_update(id,
                        [=](Element& region)
                        {
                          region.origin = Origin::CUSTOM;
                          region.pivot += delta;
                          if (isGridSnap) region.pivot = clamp_vec2_to_int(region.pivot);
                        });
        };

        if (isMouseMiddleDown) useTool = tool::PAN;
        if (tool == tool::MOVE && isMouseRightDown) useTool = tool::CROP;
        if (tool == tool::CROP && isMouseRightDown) useTool = tool::MOVE;
        if (tool == tool::DRAW && isMouseRightDown) useTool = tool::ERASE;
        if (tool == tool::ERASE && isMouseRightDown) useTool = tool::DRAW;

        hoveredRegionId = -1;

        if (useTool == tool::PAN && spritesheet && texture && texture->is_valid() && isMouseOverCanvas)
        {
          for (auto& region : spritesheet->children)
          {
            if (region.type != ElementType::REGION) continue;
            auto minPoint = glm::min(region.crop, region.crop + region.size);
            auto maxPoint = glm::max(region.crop, region.crop + region.size);
            if (hoverMousePos.x >= minPoint.x && hoverMousePos.x <= maxPoint.x && hoverMousePos.y >= minPoint.y &&
                hoverMousePos.y <= maxPoint.y)
            {
              hoveredRegionId = region.id;
              break;
            }
          }
        }

        auto& toolInfo = tool::INFO[useTool];
        auto& areaType = toolInfo.areaType;
        bool isAreaAllowed = areaType == tool::ALL || areaType == tool::SPRITESHEET_EDITOR;
        bool isFrameRequired =
            !(useTool == tool::PAN || useTool == tool::DRAW || useTool == tool::ERASE || useTool == tool::COLOR_PICKER);
        bool isRegionInUse = frame && frame->regionId != -1 && (useTool == tool::CROP || useTool == tool::MOVE);
        bool isFrameAvailable = !isFrameRequired || (frame && !isRegionInUse && !selectedFrameReferences.empty()) ||
                                (useTool == tool::CROP && !regionSelection.empty()) ||
                                (useTool == tool::MOVE && regionReference != -1);
        bool isSpritesheetRequired = useTool == tool::DRAW || useTool == tool::ERASE || useTool == tool::COLOR_PICKER;
        bool isSpritesheetAvailable = !isSpritesheetRequired || (texture && texture->is_valid());
        auto cursor = (isAreaAllowed && isFrameAvailable && isSpritesheetAvailable) ? toolInfo.cursor
                                                                                    : ImGuiMouseCursor_NotAllowed;
        ImGui::SetMouseCursor(cursor);
        ImGui::SetKeyboardFocusHere();

        switch (useTool)
        {
          case tool::PAN:
            if (isMouseLeftClicked && hoveredRegionId != -1)
            {
              region_selection_set(hoveredRegionId);
              if (isReferenceLayerOnSpritesheet)
              {
                snapshot_push(EDIT_FRAME_REGION);
                FrameChange change{};
                change.regionId = hoveredRegionId;
                if (auto region = region_get(hoveredRegionId))
                {
                  change.cropX = region->crop.x;
                  change.cropY = region->crop.y;
                  change.sizeX = region->size.x;
                  change.sizeY = region->size.y;
                  change.pivotX = region->pivot.x;
                  change.pivotY = region->pivot.y;
                }
                frame_change_apply(change);
                document_change_push(Document::FRAMES);
              }
            }
            if (isMouseDown || isMouseMiddleDown) pan += mouseDelta;
            break;
          case tool::MOVE:
            if ((isRegionInUse || selectedFrameReferences.empty()) && regionReference != -1)
            {
              if (!spritesheet) break;
              auto region = region_get(regionReference);
              if (!region) break;

              if (isBegin) snapshot_push(EDIT_REGION_MOVE);
              if (isMouseDown) region_pivot_set(regionReference, pivot_snap(mousePos - region->crop, region->pivot));
              if (isLeftPressed) region_pivot_offset(regionReference, vec2(-step, 0));
              if (isRightPressed) region_pivot_offset(regionReference, vec2(step, 0));
              if (isUpPressed) region_pivot_offset(regionReference, vec2(0, -step));
              if (isDownPressed) region_pivot_offset(regionReference, vec2(0, step));

              if (isDuring)
              {
                if (ImGui::BeginTooltip())
                {
                  ImGui::TextUnformatted(
                      std::vformat(localize.get(FORMAT_PIVOT), std::make_format_args(region->pivot.x, region->pivot.y))
                          .c_str());
                  ImGui::EndTooltip();
                }
              }

              if (isEnd) document_change_push(Document::SPRITESHEETS);
              break;
            }

            if (!item || !frame || selectedFrameReferences.empty() || isRegionInUse) break;
            if (isBegin) snapshot_push(EDIT_FRAME_PIVOT);
            if (isMouseDown)
            {
              auto pivot = pivot_snap(mousePos - frame->crop, frame->pivot);
              frame_change_apply({.pivotX = pivot.x, .pivotY = pivot.y});
            }
            if (isLeftPressed) frame_change_apply({.pivotX = step}, ChangeType::SUBTRACT);
            if (isRightPressed) frame_change_apply({.pivotX = step}, ChangeType::ADD);
            if (isUpPressed) frame_change_apply({.pivotY = step}, ChangeType::SUBTRACT);
            if (isDownPressed) frame_change_apply({.pivotY = step}, ChangeType::ADD);

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

            if (isEnd) document_change_push(Document::FRAMES);
            break;
          case tool::CROP:
            if ((isRegionInUse || selectedFrameReferences.empty()) && !regionSelection.empty())
            {
              if (!spritesheet || regionSelection.empty()) break;
              if (isBegin) snapshot_push(EDIT_REGION_CROP);

              if (isMouseClicked)
              {
                cropAnchor = mousePos;
                region_set_all(vec2((int)cropAnchor.x, (int)cropAnchor.y), vec2());
              }
              if (isMouseDown)
              {
                auto [minPoint, maxPoint] = snap_rect(glm::min(cropAnchor, mousePos), glm::max(cropAnchor, mousePos));
                region_set_all(vec2(minPoint), vec2(maxPoint - minPoint));
              }
              if (isLeftPressed) region_offset_all(vec2(stepX * -1, 0));
              if (isRightPressed) region_offset_all(vec2(stepX, 0));
              if (isUpPressed) region_offset_all(vec2(0, stepY * -1));
              if (isDownPressed) region_offset_all(vec2(0, stepY));

              if (isDuring)
              {
                if (!isMouseDown)
                  region_crop_normalize_all(isGridSnap, gridSize, gridOffset);

                if (ImGui::BeginTooltip())
                {
                  if (auto region = region_get(*regionSelection.begin()))
                  {
                    ImGui::TextUnformatted(
                        std::vformat(localize.get(FORMAT_CROP), std::make_format_args(region->crop.x, region->crop.y))
                            .c_str());
                    ImGui::TextUnformatted(
                        std::vformat(localize.get(FORMAT_SIZE), std::make_format_args(region->size.x, region->size.y))
                            .c_str());
                  }
                  ImGui::EndTooltip();
                }
              }

              if (isEnd) document_change_push(Document::SPRITESHEETS);
              break;
            }

            if (!item || !frame || selectedFrameReferences.empty() || isRegionInUse) break;
            if (isBegin) snapshot_push(EDIT_FRAME_CROP);

            if (isMouseClicked)
            {
              cropAnchor = mousePos;
              frame_change_apply(
                  {.cropX = (float)(int)cropAnchor.x, .cropY = (float)(int)cropAnchor.y, .sizeX = {}, .sizeY = {}});
            }
            if (isMouseDown)
            {
              auto [minPoint, maxPoint] = snap_rect(glm::min(cropAnchor, mousePos), glm::max(cropAnchor, mousePos));
              frame_change_apply({.cropX = minPoint.x,
                                  .cropY = minPoint.y,
                                  .sizeX = maxPoint.x - minPoint.x,
                                  .sizeY = maxPoint.y - minPoint.y});
            }
            if (isLeftPressed) frame_change_apply({.cropX = stepX}, ChangeType::SUBTRACT);
            if (isRightPressed) frame_change_apply({.cropX = stepX}, ChangeType::ADD);
            if (isUpPressed) frame_change_apply({.cropY = stepY}, ChangeType::SUBTRACT);
            if (isDownPressed) frame_change_apply({.cropY = stepY}, ChangeType::ADD);

            if (isDuring)
            {
              if (!isMouseDown)
              {
                frame_crop_normalize_apply(isGridSnap, gridSize, gridOffset);
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
            if (isEnd) document_change_push(Document::FRAMES);
            break;
          case tool::DRAW:
          case tool::ERASE:
          {
            if (!texture) break;
            auto color = useTool == tool::DRAW ? toolColor : vec4();
            if (isMouseClicked)
              snapshot_push(useTool == tool::DRAW ? EDIT_DRAW : EDIT_ERASE);
            if (isMouseDown) texture_line_apply(ivec2(previousMousePos), ivec2(mousePos), color);
            if (isMouseReleased) texture_change_push();
            break;
          }
          case tool::COLOR_PICKER:
          {
            if (texture && isDuring)
            {
              toolColor = texture->pixel_read(mousePos);
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

        if (tool == tool::PAN && hoveredRegionId != -1 && spritesheet)
        {
          if (auto region = region_get(hoveredRegionId))
          {
            if (ImGui::BeginTooltip())
            {
              ImGui::PushFont(resources.fonts[font::BOLD].get(), font::SIZE);
              ImGui::TextUnformatted(region->name.c_str());
              ImGui::PopFont();
              ImGui::TextUnformatted(
                  std::vformat(localize.get(FORMAT_ID), std::make_format_args(hoveredRegionId)).c_str());
              ImGui::TextUnformatted(
                  std::vformat(localize.get(FORMAT_CROP), std::make_format_args(region->crop.x, region->crop.y))
                      .c_str());
              ImGui::TextUnformatted(
                  std::vformat(localize.get(FORMAT_SIZE), std::make_format_args(region->size.x, region->size.y))
                      .c_str());
              if (region->origin == Origin::CUSTOM)
              {
                ImGui::TextUnformatted(
                    std::vformat(localize.get(FORMAT_PIVOT), std::make_format_args(region->pivot.x, region->pivot.y))
                        .c_str());
              }
              else
              {
                StringType originString = LABEL_REGION_ORIGIN_CENTER;
                if (region->origin == Origin::TOP_LEFT) originString = LABEL_REGION_ORIGIN_TOP_LEFT;
                auto originLabel = localize.get(originString);
                ImGui::TextUnformatted(
                    std::vformat(localize.get(FORMAT_ORIGIN), std::make_format_args(originLabel)).c_str());
              }
              ImGui::EndTooltip();
            }
          }
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
              if (isRegionInUse)
                ImGui::TextUnformatted(localize.get(TEXT_REGION_IN_USE));
              else if (useTool == tool::CROP)
                ImGui::TextUnformatted(localize.get(TEXT_SELECT_FRAME_OR_REGION));
              else
                ImGui::TextUnformatted(localize.get(TEXT_SELECT_FRAME));
              ImGui::EndTooltip();
            }
          }
        }

        if (mouseWheel != 0 || isZoomIn || isZoomOut)
        {
          auto focus = mouseWheel != 0 ? vec2(mousePos) : vec2();
          if (texture && mouseWheel == 0) focus = texture->size / 2;

          auto wheelZoomStep = mouseWheel != 0 && ImGui::IsKeyDown(ImGuiMod_Ctrl)
                                   ? zoomStep * ZOOM_STEP_FAST_MULTIPLIER
                                   : zoomStep;
          auto previousZoom = zoom;
          zoom_set(zoom, pan, focus, (mouseWheel > 0 || isZoomIn) ? wheelZoomStep : -wheelZoomStep);
          if (zoom != previousZoom) hasPendingZoomPanAdjust = true;
        }
      }
    }

    if (tool == tool::PAN)
    {
      Actions actions{};
      actions_undo_redo_add(actions, manager, document);
      actions.separator();
      actions.add(ACTION_CENTER_VIEW, []() { return true; }, center_view);
      actions.add(ACTION_FIT_VIEW, [&]() { return texture && texture->is_valid(); }, fit_view);
      actions.separator();
      actions.add(ACTION_ZOOM_IN, []() { return true; }, zoom_in);
      actions.add(ACTION_ZOOM_OUT, []() { return true; }, zoom_out);
      actions_context_window_draw("##Spritesheet Editor Context Menu", actions, settings);
    }

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
    ImGui::End();
  }

}
