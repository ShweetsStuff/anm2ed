#pragma once

#include <functional>
#include <initializer_list>
#include <set>
#include <string>
#include <utility>

#include <imgui/imgui.h>

#include "canvas.hpp"
#include "manager.hpp"
#include "settings.hpp"
#include "tool.hpp"

namespace anm2ed::imgui
{
  // One frame of canvas input. `arrow` is the arrow keys pressed this frame (with key repeat), x right and y down.
  struct CanvasInput
  {
    glm::vec2 arrow{};
    glm::vec2 mouseDelta{};
    float wheel{};
    float step{};
    bool isArrowBegin{};
    bool isArrowDown{};
    bool isArrowEnd{};
    bool isLeftClicked{};
    bool isLeftDown{};
    bool isLeftReleased{};
    bool isRightClicked{};
    bool isRightDown{};
    bool isRightReleased{};
    bool isMiddleDown{};
    bool isZoomIn{};
    bool isZoomOut{};
    bool isMod{};
  };

  CanvasInput canvas_input_get(Manager&, bool);
  bool is_canvas_hovered(ImVec2, ImVec2);
  void edit_begin_push(Manager&, StringType);
  void document_change_push(Manager&);
  void frames_change_push(Manager&, const std::set<Reference>&, FrameChange, ChangeType = ChangeType::ADJUST);
  std::pair<glm::vec2, glm::vec2> rect_snap(glm::vec2, glm::vec2, bool, glm::ivec2, glm::ivec2);
  void tooltip_lines_draw(std::initializer_list<std::string>);
  void tool_cursor_update(tool::Type, tool::AreaType, bool, bool, bool, StringType);

  class CanvasView : public Canvas
  {
  protected:
    glm::vec2 mousePos{};
    glm::vec2 checkerPan{};
    glm::vec2 checkerSyncPan{};
    float checkerSyncZoom{};
    bool isCheckerPanInitialized{};
    bool hasPendingZoomPanAdjust{};
    bool isFocused{};

    void checker_pan_reset(float, glm::vec2);
    void checker_pan_sync(float, glm::vec2);
    void zoom_step(float&, glm::vec2&, glm::vec2, int);
    void grid_child_draw(ImVec2, bool&, glm::vec4&, glm::ivec2&, glm::ivec2&, bool* = nullptr);
    void view_buttons_draw(Manager&, Settings&, const std::function<void()>&, const std::function<void()>&);
    void view_context_menu_draw(const char*, Manager&, Settings&, Document&, float&, glm::vec2&,
                                const std::function<void()>&, const std::function<void()>&, bool);

  public:
    CanvasView();
    bool is_focused_get() const;
  };
}
