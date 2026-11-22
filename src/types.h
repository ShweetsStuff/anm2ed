#pragma once

#include <glm/glm.hpp>

  enum ShortcutScope
  {
    SCOPE_FOCUSED,
    SCOPE_GLOBAL,
    SCOPE_FOCUSED_SET,
    SCOPE_GLOBAL_SET
  };

  enum Destination
  {
    DESTINATION_ALL,
    DESTINATION_THIS
  };

  enum Source
  {
    SOURCE_NEW,
    SOURCE_EXISTING
  };

  enum OverlayDrawOrder
  {
    OVERLAY_OVER,
    OVERLAY_UNDER
  };

  enum Merge
  {
    MERGE_PREPEND,
    MERGE_APPEND,
    MERGE_REPLACE,
    MERGE_IGNORE
  };

  enum EditState
  {
    EDIT_STATE_NONE,
    EDIT_STATE_START,
    EDIT_STATE_DURING,
    EDIT_STATE_END,
    EDIT_STATE_COMPLETE
  };

  using namespace glm;

  constexpr auto COLOR_WHITE = vec4(1.0);
  constexpr auto COLOR_BLACK = vec4(0.0, 0.0, 0.0, 1.0);
  constexpr auto COLOR_RED = vec4(1.0, 0.0, 0.0, 1.0);
  constexpr auto COLOR_GREEN = vec4(0.0, 1.0, 0.0, 1.0);
  constexpr auto COLOR_BLUE = vec4(0.0, 0.0, 1.0, 1.0);
  constexpr auto COLOR_PINK = vec4(1.0, 0.0, 1.0, 1.0);

  enum class OnionskinMode
  {
    TIME,
    INDEX
  };

  constexpr auto ID_NONE = -1;
