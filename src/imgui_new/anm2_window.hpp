#pragma once

#include "../anm2_new/anm2.hpp"
#include "../settings.hpp"

namespace anm2ed::imgui_new
{
  class Window
  {
    void update(anm2ed::resource::anm2_new::Anm2&, anm2ed::resource::anm2_new::Anm2::Type type, Settings&);
  };
}