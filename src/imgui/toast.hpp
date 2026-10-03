#pragma once

#include <format>
#include <string>
#include <vector>

#include "log.hpp"
#include "strings.hpp"

namespace anm2ed::imgui
{
  class Toast
  {
  public:
    std::string message{};
    float lifetime{};

    Toast(const std::string&);
  };

  class Toasts
  {
  public:
    std::vector<Toast> toasts{};

    void update();
    void push(const std::string&);
  };

}

extern anm2ed::imgui::Toasts toasts;

namespace anm2ed
{
  template <class... Args> void toast_log(Level level, StringType type, Args&&... args)
  {
    auto format = [&](Language language) -> std::string
    {
      if constexpr (sizeof...(Args) == 0)
        return localize.get(type, language);
      else
        return std::vformat(localize.get(type, language), std::make_format_args(args...));
    };
    toasts.push(format(localize.language));
    logger.write(level, format(ENGLISH));
  }
}
