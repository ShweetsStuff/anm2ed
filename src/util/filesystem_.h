#pragma once

#include <filesystem>
#include <string>

namespace anm2ed::util::filesystem
{
#ifdef __unix__
  std::string path_application_get();
  std::string path_mime_get();
  std::string path_icon_get();
  std::string path_icon_file_get();
#endif

  std::string path_pref_get();
  std::string path_preferences_get();
  std::string path_base_get();
  std::string path_executable_get();

  bool path_is_exist(const std::string&);
  bool path_is_extension(const std::string&, const std::string&);

  std::filesystem::path path_lower_case_backslash_handle(std::filesystem::path&);

  class WorkingDirectory
  {
  public:
    std::filesystem::path previous;

    WorkingDirectory(const std::string&, bool = false);
    ~WorkingDirectory();
  };
}
