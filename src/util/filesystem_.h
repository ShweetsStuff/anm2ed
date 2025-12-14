#pragma once

#include <cstdio>
#include <filesystem>
#include <string>

namespace anm2ed::util::filesystem
{
  std::filesystem::path path_preferences_get();
  std::filesystem::path path_to_lower(const std::filesystem::path&);
  std::filesystem::path path_backslash_replace(const std::filesystem::path&);
  std::string path_to_utf8(const std::filesystem::path&);
  std::filesystem::path path_from_utf8(const std::string&);

  bool path_is_exist(const std::filesystem::path&);
  bool path_is_extension(const std::filesystem::path&, const std::string&);

  std::filesystem::path path_lower_case_backslash_handle(const std::filesystem::path&);

  FILE* open(const std::filesystem::path&, const char*);

  class File
  {
  public:
    File() = default;
    File(const std::filesystem::path&, const char* mode);
    ~File();

    bool open(const std::filesystem::path&, const char* mode);
    void close();
    FILE* get() const;
    explicit operator bool() const;

  private:
    FILE* handle{};
  };

  class WorkingDirectory
  {
  public:
    std::filesystem::path previous;

    WorkingDirectory(const std::filesystem::path&, bool = false);
    ~WorkingDirectory();
  };

}
