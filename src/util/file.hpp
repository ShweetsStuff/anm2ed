#pragma once

#include <filesystem>
#include <string>
#include <string_view>

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

  namespace file
  {
    bool read_to_string(const std::filesystem::path&, std::string*, const char* mode = "rb");
    bool write_string(const std::filesystem::path&, std::string_view, const char* mode = "wb");
  }
}
