#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui/imgui.h>

namespace anm2ed::types::theme
{
#define THEMES                                                                                                         \
  X(LIGHT, "Light")                                                                                                    \
  X(DARK, "Dark")                                                                                                      \
  X(CLASSIC, "ImGui Classic")

  enum Type
  {
#define X(symbol, string) symbol,
    THEMES
#undef X
        COUNT
  };

  constexpr const char* STRINGS[] = {
#define X(symbol, string) string,
      THEMES
#undef X
  };

#undef THEMES
}

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

namespace anm2ed::types::locale
{
  enum Type
  {
    LOCAL,
    GLOBAL
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

  constexpr ImVec2 to_imvec2(const glm::vec2& v) noexcept { return {v.x, v.y}; }
  constexpr glm::vec2 to_vec2(const ImVec2& v) noexcept { return {v.x, v.y}; }

  constexpr glm::ivec2 to_ivec2(const ImVec2& v) noexcept { return {v.x, v.y}; }

  constexpr ImVec4 to_imvec4(const glm::vec4& v) noexcept { return {v.x, v.y, v.z, v.w}; }

  constexpr glm::vec4 to_vec4(const ImVec4& v) noexcept { return {v.x, v.y, v.z, v.w}; }

  template <typename T> constexpr T& dummy_value()
  {
    static T value{};
    return value;
  }

  template <typename T> constexpr T& dummy_value_negative()
  {
    static T value{-1};
    return value;
  }
}
