#include "path_.h"

#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_stdinc.h>
#include <cctype>
#include <cwctype>
#include <filesystem>
#include <type_traits>

namespace anm2ed::util::path
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

  std::filesystem::path to_lower(const std::filesystem::path& path)
  {
    auto native = path.native();
    for (auto& character : native)
      character = to_lower_char(character);
    return std::filesystem::path(native);
  }

  std::filesystem::path backslash_replace(const std::filesystem::path& path)
  {
    auto native = path.native();
    constexpr auto backslash = static_cast<std::filesystem::path::value_type>('\\');
    constexpr auto slash = static_cast<std::filesystem::path::value_type>('/');
    for (auto& character : native)
      if (character == backslash) character = slash;
    return std::filesystem::path(native);
  }

  std::string to_utf8(const std::filesystem::path& path)
  {
    auto u8 = path.u8string();
    return std::string(u8.begin(), u8.end());
  }

  std::filesystem::path from_utf8(const std::string& utf8)
  {
    return std::filesystem::path(std::u8string(utf8.begin(), utf8.end()));
  }

  bool is_exist(const std::filesystem::path& path)
  {
    if (path.empty()) return false;
    std::error_code errorCode;
    return std::filesystem::exists(path, errorCode) && ((void)std::filesystem::status(path, errorCode), !errorCode);
  }

  bool is_extension(const std::filesystem::path& path, const std::string& extension)
  {
    if (path.empty()) return false;
    auto extensionUtf8 = to_utf8(std::filesystem::path(path).extension());
    std::transform(extensionUtf8.begin(), extensionUtf8.end(), extensionUtf8.begin(), ::tolower);
    return extensionUtf8 == ("." + extension);
  }

  bool is_executable(const std::filesystem::path& path)
  {
    if (path.empty()) return false;

    std::error_code ec;
    auto status = std::filesystem::status(path, ec);
    if (ec) return false;
    if (!std::filesystem::exists(status)) return false;
    if (!std::filesystem::is_regular_file(status)) return false;

#ifdef WIN32
    auto extension = to_lower(path.extension());
    if (extension != ".exe" && extension != ".bat" && extension != ".cmd") return false;
#else
    auto permissions = status.permissions();
    auto executableMask =
        std::filesystem::perms::owner_exec | std::filesystem::perms::group_exec | std::filesystem::perms::others_exec;
    if ((permissions & executableMask) == std::filesystem::perms::none) return false;
#endif

    return true;
  }

  bool ensure_directory(const std::filesystem::path& path)
  {
    if (path.empty()) return false;

    std::error_code ec;
    if (std::filesystem::create_directories(path, ec)) return true;

    return is_exist(path.parent_path());
  }

  std::filesystem::path make_relative(const std::filesystem::path& path)
  {
    if (path.empty()) return path;
    std::error_code ec{};
    auto relative = std::filesystem::relative(path, ec);
    if (!ec) return relative;
    return path;
  }

  std::filesystem::path lower_case_backslash_handle(const std::filesystem::path& path)
  {
    auto newPath = path;
    if (is_exist(newPath)) return newPath;

    newPath = backslash_replace(newPath);
    if (is_exist(newPath)) return newPath;

    newPath = to_lower(newPath);
    return newPath;
  }
}
