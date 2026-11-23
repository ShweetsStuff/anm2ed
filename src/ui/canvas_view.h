#pragma once

#include <functional>
#include <initializer_list>
#include <set>
#include <string>
#include <utility>

#include <imgui/imgui.h>

#include "canvas.h"
#include "manager.h"
#include "settings.h"
#include "tool.h"

// One frame of canvas input. `arrow` is the arrow keys pressed this frame (with key repeat), x right and y down.
struct CanvasInput
{
    Vec2 arrow{};
    Vec2 mouseDelta{};
    float wheel = 0;
    float step = 0;
    bool isArrowBegin = false;
    bool isArrowDown = false;
    bool isArrowEnd = false;
    bool isLeftClicked = false;
    bool isLeftDown = false;
    bool isLeftReleased = false;
    bool isRightClicked = false;
    bool isRightDown = false;
    bool isRightReleased = false;
    bool isMiddleDown = false;
    bool isZoomIn = false;
    bool isZoomOut = false;
    bool isMod = false;
};

CanvasInput canvas_input_get(Manager&, bool);
bool is_canvas_hovered(ImVec2, ImVec2);
void edit_begin_push(Manager&, StringType);
void document_change_push(Manager&);
void frames_change_push(Manager&, const std::set<Reference>&, FrameChange, ChangeType = ChangeType::ADJUST);
std::pair<Vec2, Vec2> rect_snap(Vec2, Vec2, bool, IVec2, IVec2);
void tooltip_lines_draw(std::initializer_list<std::string>);
void tool_cursor_update(Tool, ToolArea, bool, bool, bool, StringType);

class CanvasView : public Canvas
{
protected:
    Vec2 mousePos{};
    Vec2 checkerPan{};
    Vec2 checkerSyncPan{};
    float checkerSyncZoom = 0;
    bool isCheckerPanInitialized = false;
    bool isZoomPanAdjustPending = false;
    bool isFocused = false;

    void checker_pan_reset(float, Vec2);
    void checker_pan_sync(float, Vec2);
    void zoom_step(float&, Vec2&, Vec2, int);
    void grid_child_draw(ImVec2, bool&, Vec4&, IVec2&, IVec2&, bool* = nullptr);
    void view_buttons_draw(Manager&, Settings&, const std::function<void()>&, const std::function<void()>&);
    void view_context_menu_draw(const char*, Manager&, Settings&, Document&, float&, Vec2&,
                                const std::function<void()>&, const std::function<void()>&, bool);

public:
    CanvasView();
    bool is_focused_get() const;
};
