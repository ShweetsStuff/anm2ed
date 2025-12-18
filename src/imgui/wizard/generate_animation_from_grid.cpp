#include "generate_animation_from_grid.h"

#include "math_.h"
#include "types.h"

using namespace anm2ed::types;
using namespace anm2ed::util;
using namespace glm;

namespace anm2ed::imgui::wizard
{
  GenerateAnimationFromGrid::GenerateAnimationFromGrid() : Canvas(vec2()) {}

  void GenerateAnimationFromGrid::update(Document& document, Resources& resources, Settings& settings)
  {
    isEnd = false;

    auto& startPosition = settings.generateStartPosition;
    auto& size = settings.generateSize;
    auto& pivot = settings.generatePivot;
    auto& rows = settings.generateRows;
    auto& columns = settings.generateColumns;
    auto& count = settings.generateCount;
    auto& delay = settings.generateDuration;
    auto& zoom = settings.generateZoom;
    auto& zoomStep = settings.inputZoomStep;

    auto childSize = ImVec2(row_widget_width_get(2), size_without_footer_get().y);

    if (ImGui::BeginChild("##Options Child", childSize, ImGuiChildFlags_Borders))
    {
      ImGui::InputInt2(localize.get(LABEL_GENERATE_START_POSITION), value_ptr(startPosition));
      ImGui::InputInt2(localize.get(LABEL_GENERATE_FRAME_SIZE), value_ptr(size));
      ImGui::InputInt2(localize.get(BASIC_PIVOT), value_ptr(pivot));
      ImGui::InputInt(localize.get(LABEL_GENERATE_ROWS), &rows, STEP, STEP_FAST);
      ImGui::InputInt(localize.get(LABEL_GENERATE_COLUMNS), &columns, STEP, STEP_FAST);

      input_int_range(localize.get(LABEL_GENERATE_COUNT), count, anm2::FRAME_NUM_MIN, rows * columns);

      ImGui::InputInt(localize.get(BASIC_DURATION), &delay, STEP, STEP_FAST);
    }
    ImGui::EndChild();

    ImGui::SameLine();

    if (ImGui::BeginChild("##Preview Child", childSize, ImGuiChildFlags_Borders))
    {
      auto& backgroundColor = settings.previewBackgroundColor;
      auto& shaderTexture = resources.shaders[resource::shader::TEXTURE];

      auto previewSize = ImVec2(ImGui::GetContentRegionAvail().x, size_without_footer_get(2).y);

      bind();
      size_set(to_vec2(previewSize));
      viewport_set();
      clear(vec4(backgroundColor, 1.0f));

      if (document.reference.itemType == anm2::LAYER)
      {
        auto& texture =
            document.anm2.content.spritesheets[document.anm2.content.layers[document.reference.itemID].spritesheetID]
                .texture;

        auto index = std::clamp((int)(time * (count - 1)), 0, (count - 1));
        auto row = index / columns;
        auto column = index % columns;
        auto crop = startPosition + ivec2(size.x * column, size.y * row);
        auto uvMin = (vec2(crop) + vec2(0.5f)) / vec2(texture.size);
        auto uvMax = (vec2(crop) + vec2(size) - vec2(0.5f)) / vec2(texture.size);

        mat4 transform = transform_get(zoom) * math::quad_model_get(size, {}, pivot);

        texture_render(shaderTexture, texture.id, transform, vec4(1.0f), {},
                       math::uv_vertices_get(uvMin, uvMax).data());
      }

      unbind();

      ImGui::Image(texture, previewSize);

      ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
      ImGui::SliderFloat("##Time", &time, 0.0f, 1.0f, "");

      ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
      ImGui::InputFloat("##Zoom", &zoom, zoomStep, zoomStep, "%.0f%%");
      zoom = glm::clamp(zoom, ZOOM_MIN, ZOOM_MAX);
    }

    ImGui::EndChild();

    auto widgetSize = widget_size_with_row_get(2);

    if (ImGui::Button(localize.get(LABEL_GENERATE), widgetSize))
    {
      auto generate_from_grid = [&]()
      {
        auto item = document.item_get();
        auto animation = document.animation_get();

        if (item && animation)
        {
          item->frames_generate_from_grid(startPosition, size, pivot, columns, count, delay);
          animation->frameNum = animation->length();
        }
      };

      DOCUMENT_EDIT(document, localize.get(EDIT_GENERATE_ANIMATION_FROM_GRID), Document::FRAMES, generate_from_grid());
      isEnd = true;
    }

    ImGui::SameLine();

    if (ImGui::Button(localize.get(BASIC_CANCEL), widgetSize)) isEnd = true;
  }
}