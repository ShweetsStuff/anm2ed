#pragma once

#include "clipboard.h"
#include "manager.h"
#include "resources.h"
#include "settings.h"

namespace anm2ed::events
{
  class Events
  {

  public:
    void update(manager::Manager&, settings::Settings&, resources::Resources&, clipboard::Clipboard&);
  };
}
