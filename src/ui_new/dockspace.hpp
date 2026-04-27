#pragma once

#include <vector>

#include "manager_new.hpp"
#include "documents.hpp"
#include "taskbar.hpp"
#include "window.hpp"

namespace anm2ed::ui_new
{
  class Dockspace
  {
    std::vector<Window> windows{};

  public:
    Dockspace();

    void window_register(const char*, Window::Type, ButtonOptions = BUTTON_NONE,
                         ContextMenuOptions = CONTEXT_MENU_NONE,
                         anm2_new::Anm2::Element::Type = anm2_new::Anm2::Element::UNKNOWN);
    void update(Taskbar&, Documents&, ManagerNew&);
  };
}
