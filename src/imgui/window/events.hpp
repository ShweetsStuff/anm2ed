#pragma once

#include "clipboard.h"
#include "manager.h"
#include "resources.h"
#include "settings.h"

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
