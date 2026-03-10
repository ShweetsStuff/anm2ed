#pragma once

#include "clipboard.hpp"
#include "manager.hpp"
#include "resources.hpp"
#include "settings.hpp"

namespace anm2ed::imgui
{
  class Events
  {
    int newEventId{-1};
    int renameQueued{-1};
    RenameState renameState{RENAME_SELECTABLE};

  public:
    void update(Manager&, Settings&, Resources&, Clipboard&);
  };
}
