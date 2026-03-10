#pragma once

#include <filesystem>

namespace anm2ed::util
{
  class File
  {
  public:
    File() = default;
    File(const std::filesystem::path&, const char* mode);
    ~File();

    bool open(const std::filesystem::path&, const char* mode);
    int close();
    FILE* get() const;
    explicit operator bool() const;

  private:
    FILE* handle{};
  };
}