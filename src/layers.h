#pragma once

#include "clipboard.h"
#include "manager.h"
#include "resources.h"
#include "settings.h"

namespace anm2ed::layers
{
  class Layers
  {
  public:
    void update(manager::Manager&, settings::Settings&, resources::Resources&, clipboard::Clipboard&);
  };
}
