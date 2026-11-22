#pragma once

#include <cstdint>
#include <vector>

#include "manager.h"
#include "resources.h"
#include "settings.h"
#include "localization.h"
#include "ui/taskbar.h"

  class Documents
  {
    int closeDocumentIndex = -1;
    std::vector<uint64_t> pendingDocumentTabIds;
    PopupHelper closePopup = PopupHelper(LABEL_DOCUMENT_CLOSE, POPUP_TO_CONTENT);

  public:
    float height = 0;

    void pending_document_order_apply(Manager&);
    void update(Taskbar&, Manager&, Settings&, Resources&, bool&);
  };
