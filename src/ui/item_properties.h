#pragma once

#include <string>

#include "document.h"
#include "manager.h"
#include "settings.h"
#include "localization.h"

class ItemProperties
{
    PopupHelper popup = PopupHelper(LABEL_TIMELINE_PROPERTIES_POPUP, POPUP_SMALL_NO_HEIGHT);
    Manager::ItemEdit edit = {};

    void reset();

public:
    void open();
    bool update(Manager&, Settings&, Document&, Reference&);
};
