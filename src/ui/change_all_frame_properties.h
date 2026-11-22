#pragma once

#include "document.h"
#include "manager.h"
#include "settings.h"

inline constexpr StringType INTERPOLATION_LABELS[] = {BASIC_NONE, BASIC_LINEAR, BASIC_EASE_IN, BASIC_EASE_OUT,
                                                      BASIC_EASE_IN_OUT};
inline constexpr StringType CHILD_MODE_LABELS[] = {LABEL_LOOP, LABEL_ONCE, LABEL_HOLD};

class ChangeAllFrameProperties
{
public:
    bool isChanged = false;

    void update(Manager&, Document&, Settings&, bool = false);
};
