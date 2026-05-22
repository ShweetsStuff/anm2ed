#pragma once

#include "strings.hpp"

namespace anm2ed::imgui
{
#define POPUP_LIST                                                                                                     \
  X(POPUP_SMALL, 0.25f, true)                                                                                          \
  X(POPUP_NORMAL, 0.5f, true)                                                                                          \
  X(POPUP_TO_CONTENT, 0.0f, true)                                                                                      \
  X(POPUP_SMALL_NO_HEIGHT, 0.25f, false)                                                                               \
  X(POPUP_NORMAL_NO_HEIGHT, 0.5f, false)

  enum PopupType
  {
#define X(name, multiplier, isHeightSet) name,
    POPUP_LIST
#undef X
  };

  enum PopupPosition
  {
    POPUP_CENTER,
    POPUP_BY_ITEM,
    POPUP_BY_CURSOR
  };

  class PopupHelper
  {
  public:
    StringType labelId{};
    PopupType type{};
    PopupPosition position{};
    bool isOpen{};
    bool isTriggered{};
    bool isJustOpened{};

    PopupHelper(StringType, PopupType = POPUP_NORMAL, PopupPosition = POPUP_CENTER);
    const char* label() const { return localize.get(labelId); }
    bool is_open();
    void open();
    void trigger();
    void end();
    void close();
  };

#undef POPUP_LIST
}
