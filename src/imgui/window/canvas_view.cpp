#include "canvas_view.hpp"

#include <glm/gtc/type_ptr.hpp>

#include "actions.hpp"
#include "edit/edit.hpp"
#include "util/imgui/constants.hpp"
#include "util/imgui/input.hpp"
#include "util/imgui/layout.hpp"
#include "util/imgui/shortcut.hpp"
#include "util/imgui/tooltip.hpp"

using namespace anm2ed::types;
using namespace anm2ed::util;
using namespace glm;

namespace anm2ed::imgui
{
  CanvasInput canvas_input_get(Manager& manager, bool isFocused)
  {
    auto key_axis_get = [](ImGuiKey negative, ImGuiKey positive)
    { return (float)ImGui::IsKeyPressed(positive) - (float)ImGui::IsKeyPressed(negative); };
    auto is_any = [](auto&& is_key)
    {
      return is_key(ImGuiKey_LeftArrow) || is_key(ImGuiKey_RightArrow) || is_key(ImGuiKey_UpArrow) ||
             is_key(ImGuiKey_DownArrow);
    };
    auto& io = ImGui::GetIO();
    auto isMod = ImGui::IsKeyDown(ImGuiMod_Shift);
    return {.arrow = {key_axis_get(ImGuiKey_LeftArrow, ImGuiKey_RightArrow),
                      key_axis_get(ImGuiKey_UpArrow, ImGuiKey_DownArrow)},
            .mouseDelta = vec2(ivec2(to_vec2(io.MouseDelta))),
            .wheel = io.MouseWheel,
            .step = (float)(isMod ? STEP_FAST : STEP),
            .isArrowBegin = is_any([](ImGuiKey key) { return ImGui::IsKeyPressed(key, false); }),
            .isArrowDown = is_any([](ImGuiKey key) { return ImGui::IsKeyDown(key); }),
            .isArrowEnd = is_any([](ImGuiKey key) { return ImGui::IsKeyReleased(key); }),
            .isLeftClicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left),
            .isLeftDown = ImGui::IsMouseDown(ImGuiMouseButton_Left),
            .isLeftReleased = ImGui::IsMouseReleased(ImGuiMouseButton_Left),
            .isRightClicked = ImGui::IsMouseClicked(ImGuiMouseButton_Right),
            .isRightDown = ImGui::IsMouseDown(ImGuiMouseButton_Right),
            .isRightReleased = ImGui::IsMouseReleased(ImGuiMouseButton_Right),
            .isMiddleDown = ImGui::IsMouseDown(ImGuiMouseButton_Middle),
            .isZoomIn = isFocused && shortcut(manager.chords[SHORTCUT_ZOOM_IN], shortcut::GLOBAL),
            .isZoomOut = isFocused && shortcut(manager.chords[SHORTCUT_ZOOM_OUT], shortcut::GLOBAL),
            .isMod = isMod};
  }

  void edit_begin_push(Manager& manager, StringType label)
  {
    manager.command_push({manager.selected, [label](Manager&, Document& document) { document.edit_begin(label); }});
  }

  void document_change_push(Manager& manager)
  {
    manager.command_push({manager.selected, [](Manager&, Document& document) { document.change(); }});
  }

  void frames_change_push(Manager& manager, const std::set<Reference>& frames, FrameChange change, ChangeType type)
  {
    manager.command_push({manager.selected, [=](Manager&, Document& document)
                          { edit::frames_change_apply(document.model, {.references = frames}, change, type); }});
  }

  // The rectangle spanned by two corners, grown outward to the grid when snapping, in whole pixels.
  std::pair<vec2, vec2> rect_snap(vec2 corner, vec2 otherCorner, bool isSnap, ivec2 gridSize, ivec2 gridOffset)
  {
    auto minPoint = glm::min(corner, otherCorner);
    auto maxPoint = glm::max(corner, otherCorner);
    for (int axis = 0; axis < 2 && isSnap; ++axis)
    {
      if (gridSize[axis] == 0) continue;
      auto offset = (float)gridOffset[axis];
      auto size = (float)gridSize[axis];
      minPoint[axis] = std::floor((minPoint[axis] - offset) / size) * size + offset;
      maxPoint[axis] = std::ceil((maxPoint[axis] - offset) / size) * size + offset;
    }
    return {vec2(ivec2(minPoint)), vec2(ivec2(maxPoint))};
  }

  void tooltip_lines_draw(std::initializer_list<std::string> lines)
  {
    if (!ImGui::BeginTooltip()) return;
    for (const auto& line : lines)
      ImGui::TextUnformatted(line.c_str());
    ImGui::EndTooltip();
  }

  // The tool's cursor, or a not-allowed cursor and (while in use) why: the tool belongs to the other canvas, or needs
  // a spritesheet or a frame first.
  void tool_cursor_update(tool::Type type, tool::AreaType area, bool isFrameAvailable, bool isSpritesheetAvailable,
                          bool isUsing, StringType frameHint)
  {
    auto& info = tool::INFO[type];
    auto isAreaAllowed = info.areaType == tool::ALL || info.areaType == area;
    auto isFrameMissing = info.isFrameRequired && !isFrameAvailable;
    auto isSpritesheetMissing = info.isSpritesheetRequired && !isSpritesheetAvailable;
    auto isAvailable = isAreaAllowed && !isFrameMissing && !isSpritesheetMissing;
    ImGui::SetMouseCursor(isAvailable ? info.cursor : ImGuiMouseCursor_NotAllowed);
    ImGui::SetKeyboardFocusHere();
    if (!isUsing || type == tool::PAN || isAvailable) return;
    auto hint = !isAreaAllowed
                    ? (area == tool::ANIMATION_PREVIEW ? TEXT_TOOL_SPRITESHEET_EDITOR : TEXT_TOOL_ANIMATION_PREVIEW)
                : isSpritesheetMissing ? TEXT_SELECT_SPRITESHEET
                                       : frameHint;
    tooltip_lines_draw({localize.get(hint)});
  }

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
