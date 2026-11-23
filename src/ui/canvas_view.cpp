#include "ui/canvas_view.h"

#include "vec.h"

#include "ui/actions.h"
#include "edit/edit.h"
#include "ui/input.h"
#include "ui/layout.h"
#include "ui/shortcut.h"

CanvasInput canvas_input_get(Manager& manager, bool isFocused)
{
    auto key_axis_get = [](ImGuiKey negative, ImGuiKey positive) {
        return (float)ImGui::IsKeyPressed(positive) - (float)ImGui::IsKeyPressed(negative);
    };
    auto is_any = [](auto&& is_key) {
        return is_key(ImGuiKey_LeftArrow) || is_key(ImGuiKey_RightArrow) || is_key(ImGuiKey_UpArrow) ||
               is_key(ImGuiKey_DownArrow);
    };
    auto& io = ImGui::GetIO();
    auto isMod = ImGui::IsKeyDown(ImGuiMod_Shift);
    return {.arrow = {key_axis_get(ImGuiKey_LeftArrow, ImGuiKey_RightArrow),
                      key_axis_get(ImGuiKey_UpArrow, ImGuiKey_DownArrow)},
            .mouseDelta = Vec2(IVec2(to_vec2(io.MouseDelta))),
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
            .isZoomIn = isFocused && shortcut(manager.chords[SHORTCUT_ZOOM_IN], SCOPE_GLOBAL),
            .isZoomOut = isFocused && shortcut(manager.chords[SHORTCUT_ZOOM_OUT], SCOPE_GLOBAL),
            .isMod = isMod};
}

// Whether the canvas rectangle is hovered, known before the canvas image is drawn so input applies this frame.
bool is_canvas_hovered(ImVec2 min, ImVec2 max)
{
    return ImGui::IsWindowHovered() && ImGui::IsMouseHoveringRect(min, max);
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
    manager.command_push({manager.selected, [=](Manager&, Document& document) {
                              frames_change_apply(document.model, {.references = frames}, change, type);
                          }});
}

// The rectangle spanned by two corners, grown outward to the grid when snapping, in whole pixels.
std::pair<Vec2, Vec2> rect_snap(Vec2 corner, Vec2 otherCorner, bool isSnap, IVec2 gridSize, IVec2 gridOffset)
{
    auto minPoint = vec_min(corner, otherCorner);
    auto maxPoint = vec_max(corner, otherCorner);
    for (int axis = 0; axis < 2 && isSnap; ++axis)
    {
        if (gridSize[axis] == 0)
            continue;
        auto offset = (float)gridOffset[axis];
        auto size = (float)gridSize[axis];
        minPoint[axis] = std::floor((minPoint[axis] - offset) / size) * size + offset;
        maxPoint[axis] = std::ceil((maxPoint[axis] - offset) / size) * size + offset;
    }
    return {Vec2(IVec2(minPoint)), Vec2(IVec2(maxPoint))};
}

void tooltip_lines_draw(std::initializer_list<std::string> lines)
{
    if (!ImGui::BeginTooltip())
        return;
    for (auto& line : lines)
        ImGui::TextUnformatted(line.c_str());
    ImGui::EndTooltip();
}

// The tool's cursor, or a not-allowed cursor and (while in use) why: the tool belongs to the other canvas, or needs
// a spritesheet or a frame first.
void tool_cursor_update(Tool type, ToolArea area, bool isFrameAvailable, bool isSpritesheetAvailable, bool isUsing,
                        StringType frameHint)
{
    auto& info = TOOLS[type];
    auto isAreaAllowed = info.areaType == TOOL_AREA_ALL || info.areaType == area;
    auto isFrameMissing = info.isFrameRequired && !isFrameAvailable;
    auto isSpritesheetMissing = info.isSpritesheetRequired && !isSpritesheetAvailable;
    auto isAvailable = isAreaAllowed && !isFrameMissing && !isSpritesheetMissing;
    ImGui::SetMouseCursor(isAvailable ? info.cursor : ImGuiMouseCursor_NotAllowed);
    ImGui::SetKeyboardFocusHere();
    if (!isUsing || type == TOOL_PAN || isAvailable)
        return;
    auto hint = !isAreaAllowed
                    ? (area == TOOL_AREA_ANIMATION_PREVIEW ? TEXT_TOOL_SPRITESHEET_EDITOR : TEXT_TOOL_ANIMATION_PREVIEW)
                : isSpritesheetMissing ? TEXT_SELECT_SPRITESHEET
                                       : frameHint;
    tooltip_lines_draw({localize.get(hint)});
}

CanvasView::CanvasView() : Canvas(Vec2())
{}

bool CanvasView::is_focused_get() const
{
    return isFocused;
}

void CanvasView::checker_pan_reset(float zoom, Vec2 pan)
{
    checkerPan = pan;
    checkerSyncPan = pan;
    checkerSyncZoom = zoom;
    isCheckerPanInitialized = true;
    hasPendingZoomPanAdjust = false;
}

void CanvasView::checker_pan_sync(float zoom, Vec2 pan)
{
    if (!isCheckerPanInitialized)
        return checker_pan_reset(zoom, pan);
    if (pan == checkerSyncPan && zoom == checkerSyncZoom)
        return;

    auto isPanDeltaIgnored = hasPendingZoomPanAdjust && zoom != checkerSyncZoom;
    if (!isPanDeltaIgnored)
        checkerPan += pan - checkerSyncPan;
    checkerSyncPan = pan;
    checkerSyncZoom = zoom;
    if (isPanDeltaIgnored)
        hasPendingZoomPanAdjust = false;
}

void CanvasView::zoom_step(float& zoom, Vec2& pan, Vec2 focus, int levelDelta)
{
    auto previousZoom = zoom;
    zoom_level_adjust(zoom, pan, focus, levelDelta);
    if (zoom != previousZoom)
        hasPendingZoomPanAdjust = true;
}

void CanvasView::grid_child_draw(ImVec2 childSize, bool& isGrid, Vec4& gridColor, IVec2& gridSize, IVec2& gridOffset,
                                 bool* isGridSnap)
{
    if (ImGui::BeginChild("##Grid Child", childSize, true, ImGuiWindowFlags_HorizontalScrollbar))
    {
        checkbox_draw(BASIC_GRID, isGrid, TOOLTIP_GRID_VISIBILITY);
        if (isGridSnap)
        {
            ImGui::SameLine();
            ImGui::Checkbox(localize.get(LABEL_SNAP), isGridSnap);
            tooltip_set(TOOLTIP_GRID_SNAP);
        }
        ImGui::SameLine();
        ImGui::ColorEdit4(localize.get(BASIC_COLOR), values_get(gridColor), ImGuiColorEditFlags_NoInputs);
        tooltip_set(TOOLTIP_GRID_COLOR);

        input_int2_range(localize.get(BASIC_SIZE), gridSize, IVec2(GRID_SIZE_MIN), IVec2(GRID_SIZE_MAX));
        tooltip_set(TOOLTIP_GRID_SIZE);
        input_int2_range(localize.get(BASIC_OFFSET), gridOffset, IVec2(GRID_OFFSET_MIN), IVec2(GRID_OFFSET_MAX));
        tooltip_set(TOOLTIP_GRID_OFFSET);
    }
    ImGui::EndChild();
}

void CanvasView::view_buttons_draw(Manager& manager, Settings& settings, const std::function<void()>& center,
                                   const std::function<void()>& fit)
{
    auto widgetSize = widget_size_with_row_get(2);
    shortcut(manager.chords[SHORTCUT_CENTER_VIEW]);
    if (ImGui::Button(localize.get(LABEL_CENTER_VIEW), widgetSize))
        center();
    set_item_tooltip_shortcut(localize.get(TOOLTIP_CENTER_VIEW), settings.shortcutCenterView);
    ImGui::SameLine();
    shortcut(manager.chords[SHORTCUT_FIT]);
    if (ImGui::Button(localize.get(LABEL_FIT), widgetSize))
        fit();
    set_item_tooltip_shortcut(localize.get(TOOLTIP_FIT), settings.shortcutFit);
}

void CanvasView::view_context_menu_draw(const char* label, Manager& manager, Settings& settings, Document& document,
                                        float& zoom, Vec2& pan, const std::function<void()>& center,
                                        const std::function<void()>& fit, bool isFitEnabled)
{
    auto zoom_center_step = [&](int levelDelta) {
        zoom_step(zoom, pan, position_translate(zoom, pan, size * 0.5f), levelDelta);
    };

    Actions actions = {};
    actions_undo_redo_add(actions, manager, document);
    actions.separator();
    actions.add(ACTION_CENTER_VIEW, []() { return true; }, center);
    actions.add(ACTION_FIT_VIEW, [isFitEnabled]() { return isFitEnabled; }, fit);
    actions.separator();
    actions.add(ACTION_ZOOM_IN, []() { return true; }, [&]() { zoom_center_step(ZOOM_LEVEL_STEP); });
    actions.add(ACTION_ZOOM_OUT, []() { return true; }, [&]() { zoom_center_step(-ZOOM_LEVEL_STEP); });
    actions_context_window_draw(label, actions, settings);
}
