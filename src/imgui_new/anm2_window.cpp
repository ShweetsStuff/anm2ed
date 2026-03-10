#include "anm2_window.hpp"

#include "imgui.h"

using namespace anm2ed::resource::anm2_new;

namespace anm2ed::imgui_new
{
  void Window::update(Anm2& anm2, Anm2::Type type, Settings& settings)
  {
    if (ImGui::Begin("TestWindow", &settings.windowIsEvents))
    {
    }
    ImGui::End();
  }
}