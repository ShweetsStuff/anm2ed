#pragma once

#include <glm/glm.hpp>

namespace anm2ed::types::shortcut
{
  enum Type
  {
    FOCUSED,
    GLOBAL,
    FOCUSED_SET,
    GLOBAL_SET
  };
}

namespace anm2ed::types::destination
{
  enum Type
  {
    ALL,
    THIS
  };
}

namespace anm2ed::types::source
{
  enum Type
  {
    NEW,
    EXISTING
  };
}

namespace anm2ed::types::overlay_draw_order
{
  enum Type
  {
    OVER,
    UNDER
  };
}

namespace anm2ed::types::merge
{
  enum Type
  {
    PREPEND,
    APPEND,
    REPLACE,
    IGNORE
  };
}

namespace anm2ed::types::edit_state
{
  enum Type
  {
    NONE,
    START,
    DURING,
    END,
    COMPLETE
  };
}

namespace anm2ed::types::color
{
  using namespace glm;

  constexpr auto WHITE = vec4(1.0);
  constexpr auto BLACK = vec4(0.0, 0.0, 0.0, 1.0);
  constexpr auto RED = vec4(1.0, 0.0, 0.0, 1.0);
  constexpr auto GREEN = vec4(0.0, 1.0, 0.0, 1.0);
  constexpr auto BLUE = vec4(0.0, 0.0, 1.0, 1.0);
  constexpr auto PINK = vec4(1.0, 0.0, 1.0, 1.0);
}

namespace anm2ed::types
{
  enum class OnionskinMode
  {
    TIME,
    INDEX
  };

  constexpr auto ID_NONE = -1;
}
