#pragma once

#include "clipboard.hpp"
#include "manager.hpp"
#include "resources.hpp"
#include "settings.hpp"
#include "strings.hpp"

namespace anm2ed::imgui
{
  class Animations
  {
    PopupHelper mergePopup{PopupHelper(LABEL_ANIMATIONS_MERGE_POPUP)};
    int newAnimationSelectedIndex{-1};
    int renameQueued{-1};
    bool isInContextMenu{};
    bool isUpdateScroll{};
    RenameState renameState{RENAME_SELECTABLE};

  public:
    void update(Manager&, Settings&, Resources&, Clipboard&);
  };
}
