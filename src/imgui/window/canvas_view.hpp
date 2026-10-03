#pragma once

#include <functional>

#include "canvas.hpp"
#include "manager.hpp"
#include "settings.hpp"

namespace anm2ed::imgui
{
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
