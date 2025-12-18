#pragma once

#include <string>

namespace anm2ed::util
{
  class Process
  {
  public:
    Process() = default;
    Process(const char*, const char*);
    ~Process();

    bool open(const char*, const char*);
    int close();
    FILE* get() const;
    explicit operator bool() const;

    std::string output_get_and_close();

  private:
    FILE* pipe{};
  };
}