#include "generate_animation_from_grid.hpp"

#include <algorithm>
#include "math.hpp"
#include "types.hpp"
#include "util/imgui/imgui.hpp"

using namespace anm2ed::types;
using namespace anm2ed::util;
using namespace glm;

namespace anm2ed::imgui::wizard
{
  GenerateAnimationFromGrid::GenerateAnimationFromGrid() : Canvas(vec2()) {}

  void GenerateAnimationFromGrid::update(Manager& manager, Document& document, Resources& resources, Settings& settings)
  {
    isEnd = false;

    auto& anm2 = document.anm2;
    auto& reference = document.reference;
    auto& startPosition = settings.generateStartPosition;
    auto& size = settings.generateSize;
    auto& pivot = settings.generatePivot;
    auto& rows = settings.generateRows;
    auto& columns = settings.generateColumns;
    auto& count = settings.generateCount;
    auto& delay = settings.generateDuration;
    auto& zoom = settings.generateZoom;
    auto& zoomStep = settings.inputZoomStep;
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
      ImGui::InputInt2(localize.get(BASIC_PIVOT), value_ptr(pivot));
      ImGui::InputInt(localize.get(LABEL_GENERATE_ROWS), &rows, STEP, STEP_FAST);
      ImGui::InputInt(localize.get(LABEL_GENERATE_COLUMNS), &columns, STEP, STEP_FAST);

      input_int_range(localize.get(LABEL_GENERATE_COUNT), count, FRAME_NUM_MIN, rows * columns);

      ImGui::InputInt(localize.get(BASIC_DURATION), &delay, STEP, STEP_FAST);
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
        auto layer = anm2.element_get(ElementType::LAYER_ELEMENT, previewReference.itemID);
        auto sourceTexture = layer ? document.texture_get(layer->spritesheetId) : nullptr;
        if (sourceTexture && sourceTexture->is_valid())
        {
          auto previewTrack = element_make(ElementType::LAYER_ANIMATION);
          frames_generate_from_grid(previewTrack, startPosition, size, pivot, columns, count, delay);

          auto length = std::max(track_length_get(previewTrack), FRAME_DURATION_MIN);
          auto sampleTime = std::min(time * (float)length, (float)length - 0.001f);
          auto frame = frame_generate(previewTrack, sampleTime);
          auto textureSize = vec2(sourceTexture->size);

          if (textureSize.x > 0.0f && textureSize.y > 0.0f && frame.size.x > 0.0f && frame.size.y > 0.0f)
          {
            auto uvMin = frame.crop / textureSize;
            auto uvMax = (frame.crop + frame.size) / textureSize;
            mat4 transform = transform_get(zoom) *
                             math::quad_model_get(frame.size, frame.position, frame.pivot,
                                                  math::percent_to_unit(frame.scale), frame.rotation);
            auto vertices = math::uv_vertices_get(uvMin, uvMax);

            texture_render(shaderTexture, sourceTexture->id, transform, frame.tint, frame.colorOffset, vertices.data());
          }
        }
      }

      unbind();

      if (isTransparent)
        render_checker_background(ImGui::GetWindowDrawList(), previewMin, previewMax, -to_vec2(previewSize) * 0.5f,
                                  CHECKER_SIZE);
      ImGui::Image(texture, previewSize);

      ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
      ImGui::SliderFloat("##Time", &time, 0.0f, 1.0f, "");

      ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
      ImGui::InputFloat("##Zoom", &zoom, zoomStep, zoomStep, "%.0f%%");
      zoom = glm::clamp(zoom, ZOOM_MIN, ZOOM_MAX);
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

      manager.command_push({manager.selected,
                            [=](Manager&, Document& document)
                            {
                              bool isChanged{};
                              for (auto queuedReference : queuedLayerReferences)
                              {
                                auto item = document.anm2.element_get(queuedReference.animationIndex, ItemType::LAYER,
                                                                      queuedReference.itemID);
                                auto animation =
                                    document.anm2.element_get(ElementType::ANIMATION, queuedReference.animationIndex);
                                if (!item || !animation) continue;

                                if (!isChanged)
                                {
                                  document.snapshot(localize.get(EDIT_GENERATE_ANIMATION_FROM_GRID));
                                  isChanged = true;
                                }
                                frames_generate_from_grid(*item, queuedStartPosition, queuedSize, queuedPivot,
                                                          queuedColumns, queuedCount, queuedDelay);
                                animation->frameNum = animation_length_get(*animation);
                              }
                              if (isChanged) document.anm2_change(Document::FRAMES);
                            }});
      isEnd = true;
    }
    ImGui::EndDisabled();

    ImGui::SameLine();

    if (ImGui::Button(localize.get(BASIC_CANCEL), widgetSize)) isEnd = true;
  }
}
