#include "filesystem_.h"

#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_stdinc.h>
#include <filesystem>

namespace anm2ed::util::filesystem
{
  std::string path_preferences_get()
  {
    char* preferencesPath = SDL_GetPrefPath("", "anm2ed");
    std::string preferencesPathString = preferencesPath;
    SDL_free(preferencesPath);
    return preferencesPathString;
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

  WorkingDirectory::~WorkingDirectory()
  {
    std::filesystem::current_path(previous);
  }
}