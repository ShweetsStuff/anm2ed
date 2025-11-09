#include "filesystem_.h"

#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_stdinc.h>
#include <filesystem>

#include "string_.h"

namespace anm2ed::util::filesystem
{
  std::string path_pref_get(const char* org, const char* app)
  {
    auto path = SDL_GetPrefPath(org, app);
    std::string string = path;
    SDL_free(path);
    return string;
  }

  std::string path_preferences_get() { return path_pref_get(nullptr, "anm2ed"); }
  std::string path_base_get() { return std::string(SDL_GetBasePath()); }
  std::string path_executable_get() { return std::filesystem::path(path_base_get()) / "anm2ed"; }

#ifdef __unix__
  std::string path_application_get()
  {
    return std::filesystem::path(path_pref_get(nullptr, "applications")) / "anm2ed.desktop";
  }

  std::string path_mime_get()
  {
    return std::filesystem::path(path_pref_get(nullptr, "mime/application")) / "x-anm2+xml.xml";
  }

  std::string path_icon_get() { return std::filesystem::path(path_preferences_get()) / "anm2ed.png"; }
  std::string path_icon_file_get()
  {
    return std::filesystem::path(path_preferences_get()) / "application-x-anm2+xml.png";
  }

#endif

  bool path_is_exist(const std::string& path)
  {
    std::error_code errorCode;
    return std::filesystem::exists(path, errorCode) && ((void)std::filesystem::status(path, errorCode), !errorCode);
  }

  bool path_is_extension(const std::string& path, const std::string& extension)
  {
    auto e = std::filesystem::path(path).extension().string();
    std::transform(e.begin(), e.end(), e.begin(), ::tolower);
    return e == ("." + extension);
  }

  std::filesystem::path path_lower_case_backslash_handle(std::filesystem::path& path)
  {
    if (path_is_exist(path)) return path;
    if (path_is_exist(string::backslash_replace(path))) return path;
    return string::to_lower(path);
  }

  WorkingDirectory::WorkingDirectory(const std::string& path, bool isFile)
  {
    previous = std::filesystem::current_path();
    if (isFile)
    {
      std::filesystem::path filePath = path;
      std::filesystem::path parentPath = filePath.parent_path();
      std::filesystem::current_path(parentPath);
    }
    else
      std::filesystem::current_path(path);
  }

  WorkingDirectory::~WorkingDirectory() { std::filesystem::current_path(previous); }
}