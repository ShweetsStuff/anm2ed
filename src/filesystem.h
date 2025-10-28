#pragma once

#include <filesystem>
#include <string>

namespace anm2ed::filesystem
{
  std::string path_preferences_get();
  bool path_is_exist(const std::string&);
  bool path_is_extension(const std::string&, const std::string&);
  bool directories_create(const std::string&);

  class WorkingDirectory
  {
  public:
    std::filesystem::path previous;

    WorkingDirectory(const std::string&, bool = false);
    ~WorkingDirectory();
  };
}
