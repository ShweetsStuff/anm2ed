#pragma once

#include <string>

namespace anm2ed
{
  class Clipboard
  {
  public:
    Clipboard();
    bool is_empty();
    std::string get();
    void set(const std::string&);
  };
}
