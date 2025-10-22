#pragma once

#include "document_manager.h"
#include "resources.h"
#include "taskbar.h"

namespace anm2ed::documents
{
  class Documents
  {
    bool isCloseDocument{};
    bool isOpenCloseDocumentPopup{};
    int closeDocumentIndex{};

  public:
    float height{};

    void update(taskbar::Taskbar& taskbar, document_manager::DocumentManager& manager, resources::Resources& resources);
  };
}
