#include "spritesheet_editor.h"

#include "imgui.h"
#include "math.h"
#include "tool.h"
#include "types.h"

using namespace anm2ed::document_manager;
using namespace anm2ed::settings;
using namespace anm2ed::canvas;
using namespace anm2ed::resources;
using namespace anm2ed::types;
using namespace glm;

namespace anm2ed::spritesheet_editor
{
  SpritesheetEditor::SpritesheetEditor() : Canvas(vec2())
  {
  }

  void SpritesheetEditor::update(DocumentManager& manager, Settings& settings, Resources& resources)
  {
    auto& document = *manager.get();
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
    auto& lineShader = resources.shaders[shader::LINE];

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

        if (ImGui::Button("Center View", widgetSize)) pan = vec2();
        ImGui::SameLine();
        ImGui::Button("Fit", widgetSize);

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

      if (spritesheet)
      {
        auto& texture = spritesheet->texture;
        auto transform = transform_get(zoom, pan) * math::quad_model_get(texture.size);
        texture_render(shaderTexture, texture.id, transform);
        if (isBorder) rect_render(lineShader, transform);
      }

      if (isGrid) grid_render(shaderGrid, zoom, pan, gridSize, gridOffset, gridColor);

      unbind();

      ImGui::Image(texture, to_imvec2(size));

      if (ImGui::IsItemHovered())
      {
        ImGui::SetKeyboardFocusHere(-1);

        mousePos = position_translate(zoom, pan, to_vec2(ImGui::GetMousePos()) - to_vec2(cursorScreenPos));

        auto isMouseDown = ImGui::IsMouseDown(ImGuiMouseButton_Left);
        auto isMouseMiddleDown = ImGui::IsMouseDown(ImGuiMouseButton_Middle);
        auto mouseDelta = ImGui::GetIO().MouseDelta;
        auto mouseWheel = ImGui::GetIO().MouseWheel;
        auto isZoomIn = imgui::chord_repeating(imgui::string_to_chord(settings.shortcutZoomIn));
        auto isZoomOut = imgui::chord_repeating(imgui::string_to_chord(settings.shortcutZoomOut));

        if ((tool == tool::PAN && isMouseDown) || isMouseMiddleDown) pan += vec2(mouseDelta.x, mouseDelta.y);

        switch (tool)
        {
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
