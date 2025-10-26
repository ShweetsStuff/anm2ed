#pragma once

#include "clipboard.h"
#include "dialog.h"
#include "manager.h"
#include "resources.h"
#include "settings.h"

namespace anm2ed::spritesheets
{
  class Spritesheets
  {
  public:
    void update(manager::Manager&, settings::Settings&, resources::Resources&, dialog::Dialog&,
                clipboard::Clipboard& clipboard);
  };
}
