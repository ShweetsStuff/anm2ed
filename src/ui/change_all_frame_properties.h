#pragma once

#include "document.h"
#include "manager.h"
#include "settings.h"

inline constexpr StringType INTERPOLATION_LABELS[] = {BASIC_NONE, BASIC_LINEAR, BASIC_EASE_IN, BASIC_EASE_OUT,
                                                      BASIC_EASE_IN_OUT};
inline constexpr StringType CHILD_MODE_LABELS[] = {LABEL_LOOP, LABEL_ONCE, LABEL_HOLD};

// Where the frame property changes are drawn: in the Frame Properties window (applied to the selected frames), or in
// the Change All Frame Properties wizard (applied to chosen frames or items).
enum ChangeHost
{
    CHANGE_IN_WINDOW,
    CHANGE_IN_WIZARD
};

// Draws the property changes. True on the frame they were applied.
bool frame_properties_change_draw(Manager&, Document&, Settings&, ChangeHost);
