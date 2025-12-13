#pragma once

#include "clipboard.h"
#include "manager.h"
#include "resources.h"
#include "settings.h"
#include "strings.h"

namespace anm2ed::imgui
{
  class Animations
  {
    PopupHelper mergePopup{PopupHelper(LABEL_ANIMATIONS_MERGE_POPUP)};
    int newAnimationSelectedIndex{-1};
    int renameQueued{-1};
    bool isInContextMenu{};
    RenameState renameState{RENAME_SELECTABLE};

  public:
    void update(Manager&, Settings&, Resources&, Clipboard&);
  };
}
