#pragma once

#include "manager.h"

class Configure
{
    Settings temporary = {};
    int selectedShortcut = -1;

public:
    bool isSet = false;

    void reset(Settings&);
    void update(Manager&, Settings&);
};
