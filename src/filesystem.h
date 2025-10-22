#pragma once

#include <filesystem>
#include <string>

namespace anm2ed::filesystem
{
  std::string path_preferences_get();
  bool path_is_exist(const std::string& path);
  bool path_is_extension(const std::string& path, const std::string& extension);

  class WorkingDirectory
  {
  public:
    std::filesystem::path previous;

    WorkingDirectory(const std::string& path, bool isFile = false);
    ~WorkingDirectory();
  };
}