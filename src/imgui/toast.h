#pragma once

#include <string>
#include <vector>

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
    void info(const std::string&);
    void error(const std::string&);
    void warning(const std::string&);
  };

}

extern anm2ed::imgui::Toasts toasts;
