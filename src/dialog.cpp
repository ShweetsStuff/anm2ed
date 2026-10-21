#include "dialog.hpp"

#ifdef _WIN32
  #include <windows.h>
#elif __unix__
#else
  #include "log.hpp"
  #include "strings.hpp"
  #include "toast.hpp"
#endif

#include <cctype>
#include <cstdlib>
#include <cstring>
#include <format>

#include "path.hpp"

using namespace anm2ed::util;

namespace anm2ed
{
  static void callback(void* userData, const char* const* filelist, int filter)
  {
    auto self = (Dialog*)(userData);

    if (filelist && filelist[0] && strlen(filelist[0]) > 0)
    {
      self->paths.clear();
      for (int i = 0; filelist[i] && strlen(filelist[i]) > 0; ++i)
        self->paths.push_back(path::from_utf8(filelist[i]));
      self->path = self->paths.empty() ? std::filesystem::path{} : self->paths.front();
      self->selectedFilter = filter;
    }
    else
    {
      self->selectedFilter = -1;
      self->path.clear();
      self->paths.clear();
    }
  }

  Dialog::Dialog(SDL_Window* window)
  {
    *this = Dialog();
    this->window = window;
  }

  void Dialog::file_open(Type type, bool isMany)
  {
    if (type == Dialog::NONE) return;
    path.clear();
    paths.clear();
    SDL_ShowOpenFileDialog(callback, this, window, FILTERS[TYPE_FILTERS[type]], std::size(FILTERS[TYPE_FILTERS[type]]),
                           nullptr, isMany);
    this->type = type;
  }

  void Dialog::file_save(Type type, const std::filesystem::path& defaultPath)
  {
    if (type == Dialog::NONE) return;
    path.clear();
    paths.clear();
    auto defaultPathString = path::to_utf8(defaultPath);
    auto defaultPathCStr = defaultPathString.empty() ? nullptr : defaultPathString.c_str();
    SDL_ShowSaveFileDialog(callback, this, window, FILTERS[TYPE_FILTERS[type]], std::size(FILTERS[TYPE_FILTERS[type]]),
                           defaultPathCStr);
    this->type = type;
  }

  void Dialog::folder_open(Type type)
  {
    if (type == Dialog::NONE) return;
    path.clear();
    paths.clear();
    SDL_ShowOpenFolderDialog(callback, this, window, nullptr, false);
    this->type = type;
  }

#ifdef __unix__
  // A string as one single-quoted shell word.
  std::string shell_quote(std::string_view text)
  {
    std::string quoted = "'";
    for (auto character : text)
      quoted += character == '\'' ? std::string("'\\''") : std::string(1, character);
    return quoted + "'";
  }

  // A file:// URI; everything but unreserved characters and slashes is percent-encoded (commas too, which dbus-send
  // would otherwise take as array separators).
  std::string file_uri_get(const std::filesystem::path& path)
  {
    std::string uri = "file://";
    for (unsigned char character : path::to_utf8(path))
      if (std::isalnum(character) || std::string_view("-._~/").contains((char)character))
        uri += (char)character;
      else
        uri += std::format("%{:02X}", character);
    return uri;
  }
#endif

  // Shows a folder in the system's file manager; a file is shown selected in its folder.
  void Dialog::file_explorer_open(const std::filesystem::path& path)
  {
    if (path.empty()) return;
    std::error_code ec{};
    auto absolutePath = std::filesystem::absolute(path, ec);
    auto isFile = std::filesystem::is_regular_file(absolutePath, ec);
#ifdef _WIN32
    if (isFile)
      ShellExecuteW(nullptr, L"open", L"explorer.exe", (L"/select,\"" + absolutePath.native() + L"\"").c_str(), nullptr,
                    SW_SHOWNORMAL);
    else
      ShellExecuteW(nullptr, L"open", absolutePath.native().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
#elif __unix__
    // File managers that implement org.freedesktop.FileManager1 (Nautilus, Dolphin, Nemo, Caja, Thunar...) select the
    // file; anything else opens its folder.
    auto folder = shell_quote(path::to_utf8(isFile ? absolutePath.parent_path() : absolutePath));
    auto command = isFile ? std::format("(dbus-send --session --print-reply --dest=org.freedesktop.FileManager1 "
                                        "--type=method_call /org/freedesktop/FileManager1 "
                                        "org.freedesktop.FileManager1.ShowItems array:string:{} string:'' "
                                        ">/dev/null 2>&1 || xdg-open {}) &",
                                        shell_quote(file_uri_get(absolutePath)), folder)
                          : std::format("xdg-open {} &", folder);
    system(command.c_str());
#else
    toast_log(Level::WARNING, TOAST_NOT_SUPPORTED);
#endif
  }

  void Dialog::reset() { *this = Dialog(this->window); }

  bool Dialog::is_selected(Type type) const { return this->type == type && !paths.empty(); }

};
