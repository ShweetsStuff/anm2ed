#pragma once

#include "settings.h"

namespace anm2ed::onionskin
{
  enum Type
  {
    BELOW,
    ABOVE
  };

  class Onionskin
  {
  public:
    void update(settings::Settings& settings);
  };
}
