#pragma once

#include "manager.h"
#include "localization.h"

  class AutosaveRestore
  {
    PopupHelper popup{PopupHelper(LABEL_WELCOME_RESTORE_POPUP, POPUP_SMALL_NO_HEIGHT)};
    bool isInitialized{};
    bool isRestoreAvailable{};

  public:
    void update(Manager&);
  };
