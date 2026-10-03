#pragma once

#include "document.hpp"
#include "manager.hpp"
#include "settings.hpp"

namespace anm2ed::imgui::wizard
{
  inline constexpr StringType INTERPOLATION_LABELS[] = {BASIC_NONE, BASIC_LINEAR, BASIC_EASE_IN, BASIC_EASE_OUT,
                                                        BASIC_EASE_IN_OUT};

  class ChangeAllFrameProperties
  {
  public:
    bool isChanged{};

    void update(Manager&, Document&, Settings&, bool = false);
  };
}
