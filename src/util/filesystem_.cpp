#include "filesystem_.h"

#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_stdinc.h>
#include <cctype>
#include <cwctype>
#include <filesystem>
#include <type_traits>

namespace anm2ed::util::filesystem
{
  namespace
  {
    template <typename CharT> CharT to_lower_char(CharT character)
    {
      if constexpr (std::is_same_v<CharT, wchar_t>)
        return static_cast<CharT>(std::towlower(static_cast<wint_t>(character)));
      else
        return static_cast<CharT>(std::tolower(static_cast<unsigned char>(character)));
    }
  }

  File::File(const std::filesystem::path& path, const char* mode)
  {
#ifdef _WIN32
    _wfopen_s(&this->internal, path.c_str(), mode);
#else
    this->internal = fopen(path.c_str(), mode);
#endif
  }

  File::~File() { fclose(this->internal); }

  FILE* File::get() { return internal; }

  std::filesystem::path path_preferences_get()
  {
    auto path = SDL_GetPrefPath(nullptr, "anm2ed");
    auto filePath = std::filesystem::path(path);
    SDL_free(path);
    return filePath;
  }

  std::filesystem::path path_to_lower(const std::filesystem::path& path)
  {
    auto native = path.native();
    for (auto& character : native)
      character = to_lower_char(character);
    return std::filesystem::path(native);
  }

  std::filesystem::path path_backslash_replace(const std::filesystem::path& path)
  {
    auto native = path.native();
    constexpr auto backslash = static_cast<std::filesystem::path::value_type>('\\');
    constexpr auto slash = static_cast<std::filesystem::path::value_type>('/');
    for (auto& character : native)
      if (character == backslash) character = slash;
    return std::filesystem::path(native);
  }

  bool path_is_exist(const std::filesystem::path& path)
  {
    std::error_code errorCode;
    return std::filesystem::exists(path, errorCode) && ((void)std::filesystem::status(path, errorCode), !errorCode);
  }

  bool path_is_extension(const std::filesystem::path& path, const std::string& extension)
  {
    auto e = std::filesystem::path(path).extension().string();
    std::transform(e.begin(), e.end(), e.begin(), ::tolower);
    return e == ("." + extension);
  }

  std::filesystem::path path_lower_case_backslash_handle(const std::filesystem::path& path)
  {
    auto newPath = path;
    if (path_is_exist(newPath)) return newPath;

    newPath = path_backslash_replace(newPath);
    if (path_is_exist(newPath)) return newPath;

    newPath = path_to_lower(newPath);
    return newPath;
  }

  WorkingDirectory::WorkingDirectory(const std::filesystem::path& path, bool isFile)
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
