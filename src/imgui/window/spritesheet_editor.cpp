#include "spritesheet_editor.hpp"

#include <cmath>
#include <format>
#include <set>
#include <utility>

#include <glm/gtc/type_ptr.hpp>

#include "actions.hpp"
#include "imgui_internal.h"
#include "math.hpp"
#include "model/frames.hpp"
#include "strings.hpp"
#include "tool.hpp"
#include "types.hpp"
#include "util/imgui/draw.hpp"
#include "util/imgui/input.hpp"
#include "util/imgui/layout.hpp"
#include "util/imgui/shortcut.hpp"

using namespace anm2ed::types;
using namespace anm2ed::resource;
using namespace anm2ed::util;
using namespace glm;

namespace anm2ed::imgui
{
  void SpritesheetEditor::update(Manager& manager, Settings& settings, Resources& resources)
  {
    isFocused = false;

    auto& document = *manager.get();
    auto& model = document.model;
    auto reference = document.reference_get();
    auto referenceSpritesheet = document.focused_id_get(SelectionKind::SPRITESHEETS);
    auto& pan = document.editorPan;
    auto& zoom = document.editorZoom;
    auto& backgroundColor = settings.editorBackgroundColor;
    auto& gridColor = settings.editorGridColor;
    auto& gridSize = settings.editorGridSize;
    auto& gridOffset = settings.editorGridOffset;
    auto& toolColor = settings.toolColor;
    auto& isGrid = settings.editorIsGrid;
    auto& isGridSnap = settings.editorIsGridSnap;
    auto& isBorder = settings.editorIsBorder;
    auto& isTransparent = settings.editorIsTransparent;
    auto spritesheet = model::item_get(model.content.spritesheets, referenceSpritesheet);
    auto baseTexture = document.texture_get(referenceSpritesheet);
    auto texture = baseTexture;
    auto& tool = settings.tool;
    auto& shaderGrid = resources.shaders[shader::GRID];
    auto& shaderTexture = resources.shaders[shader::TEXTURE];
    auto& shaderLine = resources.shaders[shader::LINE];
    auto& dashedShader = resources.shaders[shader::DASHED];
    auto regionReference = document.focused_id_get(SelectionKind::REGIONS);
    auto regionSelection = document.selected_ids_get(SelectionKind::REGIONS);

    auto center_view = [&]() { pan = -size * 0.5f; };

    auto fit_view = [&]()
    {
      auto fitTexture = baseTexture && baseTexture->is_valid() ? baseTexture : texture;
      if (fitTexture && fitTexture->is_valid())
        set_to_rect(zoom, pan, {0, 0, (float)fitTexture->size.x, (float)fitTexture->size.y});
    };

    auto region_get = [&](int id) { return spritesheet ? model::item_get(spritesheet->regions, id) : nullptr; };

    if (ImGui::Begin(localize.get(LABEL_SPRITESHEET_EDITOR_WINDOW), &settings.windowIsSpritesheetEditor))
    {
      isFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

      auto childSize = ImVec2(imgui::row_widget_width_get(3),
                              (ImGui::GetTextLineHeightWithSpacing() * 4) + (ImGui::GetStyle().WindowPadding.y * 2));

      grid_child_draw(childSize, isGrid, gridColor, gridSize, gridOffset, &isGridSnap);

      ImGui::SameLine();

      if (ImGui::BeginChild("##View Child", childSize, true, ImGuiWindowFlags_HorizontalScrollbar))
      {
        ImGui::InputFloat(localize.get(BASIC_ZOOM), &zoom, 0.0f, 0.0f, "%.0f%%");
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_EDITOR_ZOOM));

        view_buttons_draw(manager, settings, center_view, fit_view);

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

      auto frame = model.frame_get(reference);

      auto viewTexture = baseTexture && baseTexture->is_valid() ? baseTexture : texture;

      if (spritesheet && viewTexture && viewTexture->is_valid())
      {
        auto transform = transform_get(zoom, pan);

        auto spritesheetModel = math::quad_model_get(viewTexture->size);
        auto spritesheetTransform = transform * spritesheetModel;

        if (baseTexture && baseTexture->is_valid())
          texture_render(shaderTexture, resource::texture::id_get(*baseTexture), spritesheetTransform);

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

        auto layer = model::item_get(model.content.layers, reference.itemID);
        bool isReferenceLayerOnSpritesheet =
            frame && reference.itemID > -1 && layer && layer->spritesheetId == referenceSpritesheet;

        int highlightedRegionId = -1;
        if (document.editTarget == Document::EditTarget::REGION && regionReference != -1 && region_get(regionReference))
        {
          highlightedRegionId = regionReference;
        }
        else if (isReferenceLayerOnSpritesheet && frame->regionId != -1 && region_get(frame->regionId))
        {
          highlightedRegionId = frame->regionId;
        }
        else if (regionReference != -1 && region_get(regionReference))
        {
          highlightedRegionId = regionReference;
        }

        auto draw_region_rect = [&](const model::Region& region, vec4 regionColor)
        {
          auto cropModel = math::quad_model_get(region.size, region.crop);
          auto cropTransform = transform * cropModel;
          rect_render(dashedShader, cropTransform, cropModel, regionColor, BORDER_DASH_LENGTH, BORDER_DASH_GAP,
                      BORDER_DASH_OFFSET);
        };

        for (auto& region : spritesheet->regions)
        {
          auto id = region.id;
          if (id == highlightedRegionId) continue;
          draw_region_rect(region, color::WHITE);

          auto pivotTransform =
              transform * math::quad_model_get(PIVOT_SIZE, region.crop + region.pivot, PIVOT_SIZE * 0.5f);
          texture_render(shaderTexture, resources.icon_id_get(icon::PIVOT), pivotTransform, color::WHITE);
        }

        if (highlightedRegionId != -1)
        {
          if (auto region = region_get(highlightedRegionId))
          {
            draw_region_rect(*region, color::RED);

            auto pivotTransform =
                transform * math::quad_model_get(PIVOT_SIZE, region->crop + region->pivot, PIVOT_SIZE * 0.5f);
            texture_render(shaderTexture, resources.icon_id_get(icon::PIVOT), pivotTransform, PIVOT_COLOR);
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
          texture_render(shaderTexture, resources.icon_id_get(icon::PIVOT), pivotTransform, PIVOT_COLOR);
        }
      }

      unbind();

      checker_pan_sync(zoom, pan);
      if (isTransparent)
        render_checker_background(drawList, min, max, -size * 0.5f - checkerPan, CHECKER_SIZE);
      else
        drawList->AddRectFilled(min, max, ImGui::GetColorU32(to_imvec4(vec4(backgroundColor, 1.0f))));
      image_premultiplied_draw(this->texture, to_imvec2(size));

      if (ImGui::IsItemHovered())
      {
        auto input = canvas_input_get(manager, isFocused);
        auto isMouseClicked = input.isLeftClicked || input.isRightClicked;
        auto isMouseDown = input.isLeftDown || input.isRightDown;
        auto isBegin = isMouseClicked || input.isArrowBegin;
        auto isDuring = isMouseDown || input.isArrowDown;
        auto isEnd = input.isLeftReleased || input.isRightReleased || input.isArrowEnd;
        auto arrow = input.arrow * input.step;
        auto gridArrow = isGridSnap ? arrow * vec2(gridSize) : arrow;
        auto isArrow = arrow != vec2();

        auto selectedFrameReferences = document.frame_references_get();
        std::erase_if(selectedFrameReferences,
                      [](const Reference& frameReference) { return frameReference.itemType != LAYER; });
        auto editReference = reference;
        if (!selectedFrameReferences.contains(reference) && !selectedFrameReferences.empty())
          editReference = *selectedFrameReferences.begin();
        auto frame = model.frame_get(editReference);
        auto layer = model::item_get(model.content.layers, editReference.itemID);
        bool isReferenceLayerOnSpritesheet =
            frame && editReference.itemID > -1 && layer && layer->spritesheetId == referenceSpritesheet;
        if (selectedFrameReferences.empty() && isReferenceLayerOnSpritesheet)
          selectedFrameReferences.insert(editReference);
        previousMousePos = mousePos;
        mousePos = position_translate(zoom, pan, to_ivec2(ImGui::GetMousePos()) - to_ivec2(cursorScreenPos));

        auto pivot_snap = [&](vec2 value, vec2 current)
        { return isGridSnap ? vec2(ivec2(value)) : glm::floor(value) + (current - glm::floor(current)); };
        // Region edits apply to the given regions of the edited spritesheet when the command runs.
        auto regions_update = [&](std::set<int> ids, std::function<void(model::Region&)> update)
        {
          manager.command_push(
              {manager.selected, [=, spritesheetId = referenceSpritesheet](Manager&, Document& document)
               {
                 auto spritesheet = model::item_get(document.model.content.spritesheets, spritesheetId);
                 for (auto id : ids)
                   if (auto region = spritesheet ? model::item_get(spritesheet->regions, id) : nullptr) update(*region);
               }});
        };
        auto region_pivot_update = [&](std::function<vec2(vec2)> pivot_get)
        {
          regions_update({regionReference},
                         [=](model::Region& region)
                         {
                           region.origin = Origin::CUSTOM;
                           region.pivot = pivot_get(region.pivot);
                         });
        };

        auto useTool = (tool::Type)tool;
        if (input.isMiddleDown) useTool = tool::PAN;
        if (tool == tool::MOVE && input.isRightDown) useTool = tool::CROP;
        if (tool == tool::CROP && input.isRightDown) useTool = tool::MOVE;
        if (tool == tool::DRAW && input.isRightDown) useTool = tool::ERASE;
        if (tool == tool::ERASE && input.isRightDown) useTool = tool::DRAW;

        hoveredRegionId = -1;
        if (useTool == tool::PAN && spritesheet && texture && texture->is_valid() && isMouseOverCanvas)
          for (auto& region : spritesheet->regions)
          {
            auto minPoint = glm::min(region.crop, region.crop + region.size);
            auto maxPoint = glm::max(region.crop, region.crop + region.size);
            if (glm::all(glm::greaterThanEqual(hoverMousePos, minPoint)) &&
                glm::all(glm::lessThanEqual(hoverMousePos, maxPoint)))
            {
              hoveredRegionId = region.id;
              break;
            }
          }

        // Crop and pivot edit regions while a region is the edit target, or a frame's region is in use.
        auto isRegionEditTarget = document.editTarget == Document::EditTarget::REGION;
        auto toolFrames = isRegionEditTarget ? std::set<Reference>{} : selectedFrameReferences;
        bool isRegionInUse = !isRegionEditTarget && toolFrames.size() <= 1 && frame && frame->regionId != -1 &&
                             (useTool == tool::CROP || useTool == tool::MOVE);
        auto isRegionTool = isRegionEditTarget || isRegionInUse || toolFrames.empty();
        auto isFrameTool = frame && !toolFrames.empty() && !isRegionInUse;
        bool isFrameAvailable = isFrameTool || (useTool == tool::CROP && !regionSelection.empty()) ||
                                (useTool == tool::MOVE && regionReference != -1);
        auto frames_change = [&](FrameChange change, ChangeType type = ChangeType::ADJUST)
        { frames_change_push(manager, toolFrames, change, type); };

        tool_cursor_update(useTool, tool::SPRITESHEET_EDITOR, isFrameAvailable, texture && texture->is_valid(),
                           isMouseDown || input.isArrowDown,
                           isRegionInUse           ? TEXT_REGION_IN_USE
                           : useTool == tool::CROP ? TEXT_SELECT_FRAME_OR_REGION
                                                   : TEXT_SELECT_FRAME);

        if (useTool == tool::PAN)
        {
          if (input.isLeftClicked && hoveredRegionId != -1)
          {
            document.editTarget = Document::EditTarget::REGION;
            regionReference = hoveredRegionId;
            regionSelection = {hoveredRegionId};
            document.selected_ids_set(SelectionKind::REGIONS, regionSelection);
            document.focused_id_set(SelectionKind::REGIONS, hoveredRegionId);
            if (auto region = region_get(hoveredRegionId); region && isReferenceLayerOnSpritesheet)
            {
              edit_begin_push(manager, EDIT_FRAME_REGION);
              frames_change_push(manager, selectedFrameReferences,
                                 {.regionId = hoveredRegionId,
                                  .cropX = region->crop.x,
                                  .cropY = region->crop.y,
                                  .sizeX = region->size.x,
                                  .sizeY = region->size.y,
                                  .pivotX = region->pivot.x,
                                  .pivotY = region->pivot.y});
              document_change_push(manager);
            }
          }
          if (isMouseDown || input.isMiddleDown) pan += input.mouseDelta;
        }

        auto region = isRegionTool ? region_get(regionReference) : nullptr;
        if (useTool == tool::MOVE && region)
        {
          if (isBegin) edit_begin_push(manager, EDIT_REGION_MOVE);
          if (isMouseDown)
            region_pivot_update([pivot = pivot_snap(mousePos - region->crop, region->pivot)](vec2) { return pivot; });
          if (isArrow)
            region_pivot_update([=, this](vec2 pivot)
                                { return isGridSnap ? vec2(ivec2(pivot + arrow)) : pivot + arrow; });
          if (isDuring) tooltip_lines_draw({localize_format(FORMAT_PIVOT, region->pivot.x, region->pivot.y)});
          if (isEnd) document_change_push(manager);
        }
        else if (useTool == tool::MOVE && isFrameTool)
        {
          if (isBegin) edit_begin_push(manager, EDIT_FRAME_PIVOT);
          if (isMouseDown)
          {
            auto pivot = pivot_snap(mousePos - frame->crop, frame->pivot);
            frames_change({.regionId = -1, .pivotX = pivot.x, .pivotY = pivot.y});
          }
          if (isArrow) frames_change({.regionId = -1, .pivotX = arrow.x, .pivotY = arrow.y}, ChangeType::ADD);
          if (isDuring) tooltip_lines_draw({localize_format(FORMAT_PIVOT, frame->pivot.x, frame->pivot.y)});
          if (isEnd) document_change_push(manager);
        }

        auto isRegionCrop = isRegionTool && !regionSelection.empty() && spritesheet;
        if (useTool == tool::CROP && (isRegionCrop || isFrameTool))
        {
          if (isBegin) edit_begin_push(manager, isRegionCrop ? EDIT_REGION_CROP : EDIT_FRAME_CROP);
          auto rect_set = [&](vec2 crop, vec2 cropSize)
          {
            if (isRegionCrop)
              regions_update(regionSelection,
                             [=](model::Region& region)
                             {
                               region.crop = crop;
                               region.size = cropSize;
                             });
            else
              frames_change(
                  {.regionId = -1, .cropX = crop.x, .cropY = crop.y, .sizeX = cropSize.x, .sizeY = cropSize.y});
          };
          if (isMouseClicked)
          {
            cropAnchor = mousePos;
            auto crop = vec2(ivec2(cropAnchor));
            if (isRegionCrop)
              rect_set(crop, vec2());
            else
              frames_change({.regionId = -1, .cropX = crop.x, .cropY = crop.y});
          }
          if (isMouseDown)
          {
            auto [minPoint, maxPoint] = rect_snap(cropAnchor, mousePos, isGridSnap, gridSize, gridOffset);
            rect_set(minPoint, maxPoint - minPoint);
          }
          if (isArrow && isRegionCrop)
            regions_update(regionSelection,
                           [=](model::Region& region)
                           {
                             region.crop = vec2(ivec2(region.crop + gridArrow));
                             region.size = vec2(ivec2(region.size));
                           });
          else if (isArrow)
            frames_change({.regionId = -1, .cropX = gridArrow.x, .cropY = gridArrow.y}, ChangeType::ADD);

          // Arrow nudges are snapped (and the rectangle normalized) when the command runs, after the nudge.
          if (isDuring && !isMouseDown && isRegionCrop)
            regions_update(regionSelection,
                           [=, this](model::Region& region)
                           {
                             auto [minPoint, maxPoint] =
                                 rect_snap(region.crop, region.crop + region.size, isGridSnap, gridSize, gridOffset);
                             region.crop = minPoint;
                             region.size = maxPoint - minPoint;
                           });
          else if (isDuring && !isMouseDown)
            manager.command_push({manager.selected, [=, this](Manager&, Document& document)
                                  {
                                    auto source = document.model.frame_get(editReference);
                                    if (!source) return;
                                    auto [minPoint, maxPoint] = rect_snap(source->crop, source->crop + source->size,
                                                                          isGridSnap, gridSize, gridOffset);
                                    edit::frames_change_apply(document.model, {.references = toolFrames},
                                                              {.regionId = -1,
                                                               .cropX = minPoint.x,
                                                               .cropY = minPoint.y,
                                                               .sizeX = maxPoint.x - minPoint.x,
                                                               .sizeY = maxPoint.y - minPoint.y},
                                                              ChangeType::ADJUST);
                                  }});

          auto shown = isRegionCrop ? region_get(*regionSelection.begin()) : nullptr;
          auto crop = shown ? shown->crop : frame ? frame->crop : vec2();
          auto cropSize = shown ? shown->size : frame ? frame->size : vec2();
          if (isDuring && (shown || !isRegionCrop))
            tooltip_lines_draw(
                {localize_format(FORMAT_CROP, crop.x, crop.y), localize_format(FORMAT_SIZE, cropSize.x, cropSize.y)});
          if (isEnd) document_change_push(manager);
        }

        if ((useTool == tool::DRAW || useTool == tool::ERASE) && texture)
        {
          if (isMouseClicked) edit_begin_push(manager, useTool == tool::DRAW ? EDIT_DRAW : EDIT_ERASE);
          if (isMouseDown)
            manager.command_push({manager.selected, [start = ivec2(previousMousePos), end = ivec2(mousePos),
                                                     color = useTool == tool::DRAW ? toolColor : vec4(),
                                                     spritesheetId = referenceSpritesheet](Manager&, Document& document)
                                  {
                                    if (auto texture = document.texture_edit(spritesheetId))
                                      texture->pixel_line(start, end, color);
                                  }});
          if (input.isLeftReleased || input.isRightReleased)
            manager.command_push({manager.selected, [spritesheetId = referenceSpritesheet](Manager&, Document& document)
                                  { document.texture_change(spritesheetId); }});
        }

        if (useTool == tool::COLOR_PICKER && texture && isDuring)
        {
          toolColor = texture->pixel_read(mousePos);
          if (ImGui::BeginTooltip())
          {
            ImGui::ColorButton("##Color Picker Button", to_imvec4(toolColor));
            ImGui::SameLine();
            auto rgba8 = glm::clamp(ivec4(toolColor * 255.0f + 0.5f), ivec4(0), ivec4(255));
            ImGui::TextUnformatted(
                std::format("#{:02X}{:02X}{:02X}{:02X}", rgba8.r, rgba8.g, rgba8.b, rgba8.a).c_str());
            ImGui::SameLine();
            ImGui::Text("(%d, %d, %d, %d)", rgba8.r, rgba8.g, rgba8.b, rgba8.a);
            ImGui::EndTooltip();
          }
        }

        if (tool == tool::PAN && hoveredRegionId != -1 && spritesheet)
          if (auto region = region_get(hoveredRegionId); region && ImGui::BeginTooltip())
          {
            ImGui::PushFont(resources.fonts[font::BOLD].get(), font::SIZE);
            ImGui::TextUnformatted(region->name.c_str());
            ImGui::PopFont();
            auto originLabel = localize.get(region->origin == Origin::TOP_LEFT ? LABEL_REGION_ORIGIN_TOP_LEFT
                                                                               : LABEL_REGION_ORIGIN_CENTER);
            for (const auto& line :
                 {localize_format(FORMAT_ID, hoveredRegionId),
                  localize_format(FORMAT_CROP, region->crop.x, region->crop.y),
                  localize_format(FORMAT_SIZE, region->size.x, region->size.y),
                  region->origin == Origin::CUSTOM ? localize_format(FORMAT_PIVOT, region->pivot.x, region->pivot.y)
                                                   : localize_format(FORMAT_ORIGIN, originLabel)})
              ImGui::TextUnformatted(line.c_str());
            ImGui::EndTooltip();
          }

        if (input.wheel != 0 || input.isZoomIn || input.isZoomOut)
        {
          auto focus = input.wheel != 0 ? vec2(mousePos) : texture ? vec2(texture->size) / 2.0f : vec2();
          zoom_step(zoom, pan, focus, (input.wheel > 0 || input.isZoomIn) ? ZOOM_LEVEL_STEP : -ZOOM_LEVEL_STEP);
        }
      }
    }

    if (tool == tool::PAN)
      view_context_menu_draw("##Spritesheet Editor Context Menu", manager, settings, document, zoom, pan, center_view,
                             fit_view, (baseTexture && baseTexture->is_valid()) || (texture && texture->is_valid()));

    if (!document.isSpritesheetEditorSet)
    {
      size = settings.editorSize;
      zoom = settings.editorStartZoom;
      set();
      center_view();
      checker_pan_reset(zoom, pan);
      document.isSpritesheetEditorSet = true;
    }

    settings.editorSize = size;
    settings.editorStartZoom = zoom;
    ImGui::End();
  }

}
