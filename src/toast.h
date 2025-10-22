#pragma once

#include <string>
#include <vector>

namespace anm2ed::toast
{

  class Toast
  {
  public:
    std::string message{};
    float lifetime{};

    Toast(const std::string& message);
  };

  class Toasts
  {
  public:
    std::vector<Toast> toasts{};

    void update();
    void add(const std::string& message);
    void add_error(const std::string& message);
  };

  extern Toasts toasts;
}
