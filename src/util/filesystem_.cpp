#include "filesystem_.h"

#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_stdinc.h>
#include <filesystem>

#include "string_.h"

namespace anm2ed::util::filesystem
{
  std::string path_preferences_get()
  {
    auto path = SDL_GetPrefPath(nullptr, "anm2ed");
    std::string string = path;
    SDL_free(path);
    return string;
  }

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
    auto asString = path.generic_string();
    if (path_is_exist(asString)) return path;

    asString = string::backslash_replace(asString);
    path = asString;
    if (path_is_exist(asString)) return path;

    asString = string::to_lower(asString);
    path = asString;
    return path;
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

  WorkingDirectory::WorkingDirectory(const std::filesystem::path& path, bool isFile)
    : WorkingDirectory(path.string(), isFile)
  {
  }

  WorkingDirectory::~WorkingDirectory() { std::filesystem::current_path(previous); }
}
