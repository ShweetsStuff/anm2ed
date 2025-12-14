#include "filesystem_.h"

#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_stdinc.h>
#include <cctype>
#include <cstdio>
#include <cstring>
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

  std::filesystem::path path_preferences_get()
  {
    auto sdlPath = SDL_GetPrefPath(nullptr, "anm2ed");
    if (!sdlPath) return {};
    auto filePath = path_from_utf8(sdlPath);
    SDL_free(sdlPath);
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

  std::string path_to_utf8(const std::filesystem::path& path)
  {
    auto u8 = path.u8string();
    return std::string(u8.begin(), u8.end());
  }

  std::filesystem::path path_from_utf8(const std::string& utf8)
  {
    return std::filesystem::path(std::u8string(utf8.begin(), utf8.end()));
  }

  FILE* open(const std::filesystem::path& path, const char* mode)
  {
#ifdef _WIN32
    std::wstring wideMode{};
    if (mode)
      wideMode.assign(mode, mode + std::strlen(mode));
    else
      wideMode = L"rb";
    return _wfopen(path.native().c_str(), wideMode.c_str());
#else
    return std::fopen(path.string().c_str(), mode);
#endif
  }

  bool path_is_exist(const std::filesystem::path& path)
  {
    std::error_code errorCode;
    return std::filesystem::exists(path, errorCode) && ((void)std::filesystem::status(path, errorCode), !errorCode);
  }

  bool path_is_extension(const std::filesystem::path& path, const std::string& extension)
  {
    auto extensionUtf8 = path_to_utf8(std::filesystem::path(path).extension());
    std::transform(extensionUtf8.begin(), extensionUtf8.end(), extensionUtf8.begin(), ::tolower);
    return extensionUtf8 == ("." + extension);
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

  File::File(const std::filesystem::path& path, const char* mode) { open(path, mode); }

  File::~File() { close(); }

  bool File::open(const std::filesystem::path& path, const char* mode)
  {
    close();
    handle = filesystem::open(path, mode);
    return handle != nullptr;
  }

  void File::close()
  {
    if (handle)
    {
      std::fclose(handle);
      handle = nullptr;
    }
  }

  FILE* File::get() const { return handle; }

  File::operator bool() const { return handle != nullptr; }
}
