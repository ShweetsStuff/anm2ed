#include "canvas_view.hpp"

#include <glm/gtc/type_ptr.hpp>

#include "actions.hpp"
#include "util/imgui/input.hpp"
#include "util/imgui/layout.hpp"
#include "util/imgui/shortcut.hpp"
#include "util/imgui/tooltip.hpp"

using namespace glm;

namespace anm2ed::imgui
{
  CanvasView::CanvasView() : Canvas(vec2()) {}

  bool CanvasView::is_focused_get() const { return isFocused; }

  void CanvasView::checker_pan_reset(float zoom, vec2 pan)
  {
    checkerPan = pan;
    checkerSyncPan = pan;
    checkerSyncZoom = zoom;
    isCheckerPanInitialized = true;
    hasPendingZoomPanAdjust = false;
  }

  void CanvasView::checker_pan_sync(float zoom, vec2 pan)
  {
    if (!isCheckerPanInitialized) return checker_pan_reset(zoom, pan);
    if (pan == checkerSyncPan && zoom == checkerSyncZoom) return;

    auto isPanDeltaIgnored = hasPendingZoomPanAdjust && zoom != checkerSyncZoom;
    if (!isPanDeltaIgnored) checkerPan += pan - checkerSyncPan;
    checkerSyncPan = pan;
    checkerSyncZoom = zoom;
    if (isPanDeltaIgnored) hasPendingZoomPanAdjust = false;
  }

  void CanvasView::zoom_step(float& zoom, vec2& pan, vec2 focus, int levelDelta)
  {
    auto previousZoom = zoom;
    zoom_level_adjust(zoom, pan, focus, levelDelta);
    if (zoom != previousZoom) hasPendingZoomPanAdjust = true;
  }

  void CanvasView::grid_child_draw(ImVec2 childSize, bool& isGrid, vec4& gridColor, ivec2& gridSize, ivec2& gridOffset,
                                   bool* isGridSnap)
  {
    if (ImGui::BeginChild("##Grid Child", childSize, true, ImGuiWindowFlags_HorizontalScrollbar))
    {
      ImGui::Checkbox(localize.get(BASIC_GRID), &isGrid);
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_GRID_VISIBILITY));
      if (isGridSnap)
      {
        ImGui::SameLine();
        ImGui::Checkbox(localize.get(LABEL_SNAP), isGridSnap);
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_GRID_SNAP));
      }
      ImGui::SameLine();
      ImGui::ColorEdit4(localize.get(BASIC_COLOR), value_ptr(gridColor), ImGuiColorEditFlags_NoInputs);
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_GRID_COLOR));

      input_int2_range(localize.get(BASIC_SIZE), gridSize, ivec2(GRID_SIZE_MIN), ivec2(GRID_SIZE_MAX));
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_GRID_SIZE));
      input_int2_range(localize.get(BASIC_OFFSET), gridOffset, ivec2(GRID_OFFSET_MIN), ivec2(GRID_OFFSET_MAX));
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_GRID_OFFSET));
    }
    ImGui::EndChild();
  }

  void CanvasView::view_buttons_draw(Manager& manager, Settings& settings, const std::function<void()>& center,
                                     const std::function<void()>& fit)
  {
    auto widgetSize = widget_size_with_row_get(2);
    shortcut(manager.chords[SHORTCUT_CENTER_VIEW]);
    if (ImGui::Button(localize.get(LABEL_CENTER_VIEW), widgetSize)) center();
    set_item_tooltip_shortcut(localize.get(TOOLTIP_CENTER_VIEW), settings.shortcutCenterView);
    ImGui::SameLine();
    shortcut(manager.chords[SHORTCUT_FIT]);
    if (ImGui::Button(localize.get(LABEL_FIT), widgetSize)) fit();
    set_item_tooltip_shortcut(localize.get(TOOLTIP_FIT), settings.shortcutFit);
  }

  void CanvasView::view_context_menu_draw(const char* label, Manager& manager, Settings& settings, Document& document,
                                          float& zoom, vec2& pan, const std::function<void()>& center,
                                          const std::function<void()>& fit, bool isFitEnabled)
  {
    auto zoom_center_step = [&](int levelDelta)
    { zoom_step(zoom, pan, position_translate(zoom, pan, size * 0.5f), levelDelta); };

    Actions actions{};
    actions_undo_redo_add(actions, manager, document);
    actions.separator();
    actions.add(ACTION_CENTER_VIEW, []() { return true; }, center);
    actions.add(ACTION_FIT_VIEW, [isFitEnabled]() { return isFitEnabled; }, fit);
    actions.separator();
    actions.add(ACTION_ZOOM_IN, []() { return true; }, [&]() { zoom_center_step(ZOOM_LEVEL_STEP); });
    actions.add(ACTION_ZOOM_OUT, []() { return true; }, [&]() { zoom_center_step(-ZOOM_LEVEL_STEP); });
    actions_context_window_draw(label, actions, settings);
  }
}
