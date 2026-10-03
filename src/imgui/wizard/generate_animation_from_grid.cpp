#include "generate_animation_from_grid.hpp"

#include "math.hpp"
#include "model/frames.hpp"
#include "types.hpp"
#include "util/imgui/draw.hpp"
#include "util/imgui/input.hpp"
#include "util/imgui/layout.hpp"
#include <algorithm>
#include <format>

using namespace anm2ed::types;
using namespace anm2ed::util;
using namespace glm;

namespace anm2ed::imgui::wizard
{
  GenerateAnimationFromGrid::GenerateAnimationFromGrid() : Canvas(vec2()) {}

  void GenerateAnimationFromGrid::update(Manager& manager, Document& document, Resources& resources, Settings& settings)
  {
    isEnd = false;

    auto& model = document.model;
    auto reference = document.reference_get();
    auto& startPosition = settings.generateStartPosition;
    auto& size = settings.generateSize;
    auto& pivot = settings.generatePivot;
    auto& rows = settings.generateRows;
    auto& columns = settings.generateColumns;
    auto& count = settings.generateCount;
    auto& delay = settings.generateDuration;
    auto& isMakeRegions = settings.generateIsMakeRegions;
    auto& regionNameFormat = settings.generateRegionNameFormat;
    auto& zoom = settings.generateZoom;
    auto layerReferences = document.layer_references_get();
    auto previewReference = layerReferences.empty() ? Reference{} : layerReferences.front();
    for (auto itemReference : layerReferences)
      if (itemReference.animationIndex == reference.animationIndex && itemReference.itemType == reference.itemType &&
          itemReference.itemID == reference.itemID)
        previewReference = itemReference;

    auto childSize = ImVec2(row_widget_width_get(2), size_without_footer_get().y);

    if (ImGui::BeginChild("##Options Child", childSize, ImGuiChildFlags_Borders))
    {
      ImGui::InputInt2(localize.get(LABEL_GENERATE_START_POSITION), value_ptr(startPosition));
      ImGui::InputInt2(localize.get(LABEL_GENERATE_FRAME_SIZE), value_ptr(size));
      ImGui::InputFloat2(localize.get(BASIC_PIVOT), value_ptr(pivot), math::vec2_format_get(pivot));
      ImGui::InputInt(localize.get(LABEL_GENERATE_ROWS), &rows, STEP, STEP_FAST);
      ImGui::InputInt(localize.get(LABEL_GENERATE_COLUMNS), &columns, STEP, STEP_FAST);

      input_int_range(localize.get(LABEL_GENERATE_COUNT), count, FRAME_NUM_MIN, rows * columns);

      ImGui::InputInt(localize.get(BASIC_DURATION), &delay, STEP, STEP_FAST);
      ImGui::Separator();
      ImGui::Checkbox(localize.get(LABEL_GENERATE_MAKE_REGIONS), &isMakeRegions);
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_GENERATE_MAKE_REGIONS));
      ImGui::BeginDisabled(!isMakeRegions);
      input_text_string(localize.get(LABEL_FORMAT), &regionNameFormat);
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_GENERATE_REGION_NAME_FORMAT));
      ImGui::EndDisabled();
    }
    ImGui::EndChild();

    rows = std::max(rows, 1);
    columns = std::max(columns, 1);
    count = std::clamp(count, FRAME_NUM_MIN, rows * columns);
    delay = std::max(delay, FRAME_DURATION_MIN);

    ImGui::SameLine();

    if (ImGui::BeginChild("##Preview Child", childSize, ImGuiChildFlags_Borders))
    {
      auto& backgroundColor = settings.previewBackgroundColor;
      auto& isTransparent = settings.animationPreviewTransparent;
      auto& shaderTexture = resources.shaders[resource::shader::TEXTURE];

      auto previewSize = ImVec2(ImGui::GetContentRegionAvail().x, size_without_footer_get(2).y);
      auto previewMin = ImGui::GetCursorScreenPos();
      auto previewMax = to_imvec2(to_vec2(previewMin) + to_vec2(previewSize));

      size_set(to_vec2(previewSize));
      bind();
      viewport_set();
      clear(isTransparent ? vec4(0) : vec4(backgroundColor, 1.0f));

      if (!layerReferences.empty())
      {
        auto layer = model::item_get(model.content.layers, previewReference.itemID);
        auto sourceTexture = layer ? document.texture_get(layer->spritesheetId) : nullptr;
        if (sourceTexture && sourceTexture->is_valid())
        {
          model::Track previewTrack{};
          model::frames_generate_from_grid(previewTrack, startPosition, size, pivot, columns, count, delay);

          auto length = std::max(model::track_length_get(previewTrack), FRAME_DURATION_MIN);
          auto sampleTime = std::min(time * (float)length, (float)length - 0.001f);
          auto frame = model::frame_generate(previewTrack, sampleTime);
          auto textureSize = vec2(sourceTexture->size);

          if (textureSize.x > 0.0f && textureSize.y > 0.0f && frame.size.x > 0.0f && frame.size.y > 0.0f)
          {
            auto uvMin = frame.crop / textureSize;
            auto uvMax = (frame.crop + frame.size) / textureSize;
            mat4 transform =
                transform_get(zoom) * math::quad_model_get(frame.size, frame.position, frame.pivot,
                                                           math::percent_to_unit(frame.scale), frame.rotation,
                                                           math::percent_to_unit(frame.shear));
            auto vertices = math::uv_vertices_get(uvMin, uvMax);

            texture_render(shaderTexture, resource::texture::id_get(*sourceTexture), transform, frame.tint,
                           frame.colorOffset, vertices.data());
          }
        }
      }

      unbind();

      if (isTransparent)
        render_checker_background(ImGui::GetWindowDrawList(), previewMin, previewMax, -to_vec2(previewSize) * 0.5f,
                                  CHECKER_SIZE);
      image_premultiplied_draw(texture, previewSize);

      ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
      ImGui::SliderFloat("##Time", &time, 0.0f, 1.0f, "");

      ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
      ImGui::InputFloat("##Zoom", &zoom, 0.0f, 0.0f, "%.0f%%");
    }

    ImGui::EndChild();

    auto widgetSize = widget_size_with_row_get(2);

    ImGui::BeginDisabled(layerReferences.empty());
    if (ImGui::Button(localize.get(LABEL_GENERATE), widgetSize))
    {
      auto queuedLayerReferences = layerReferences;
      auto queuedStartPosition = startPosition;
      auto queuedSize = size;
      auto queuedPivot = pivot;
      auto queuedColumns = columns;
      auto queuedCount = count;
      auto queuedDelay = delay;
      auto queuedIsMakeRegions = isMakeRegions;
      auto queuedRegionNameFormat = regionNameFormat;

      manager.command_push(
          {manager.selected, [=](Manager&, Document& document)
           {
             edit::GridOptions options{queuedStartPosition, queuedSize,  queuedPivot,         queuedColumns,
                                       queuedCount,         queuedDelay, queuedIsMakeRegions, queuedRegionNameFormat};
             document.edit_apply(EDIT_GENERATE_ANIMATION_FROM_GRID, [&](model::Model& model)
                                 { return edit::animation_grid_generate(model, queuedLayerReferences, options); });
           }});
      isEnd = true;
    }
    ImGui::EndDisabled();

    ImGui::SameLine();

    if (ImGui::Button(localize.get(BASIC_CANCEL), widgetSize)) isEnd = true;
  }
}
