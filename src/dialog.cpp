#include "dialog.h"

#ifdef _WIN32
  #include <windows.h>
#elif __unix__
#else
  #include "log.h"
  #include "strings.h"
  #include "toast.h"
#endif

#include <cstdlib>
#include <cstring>
#include <format>

#include "filesystem_.h"

namespace anm2ed::dialog
{
  static void callback(void* userData, const char* const* filelist, int filter)
  {
    auto self = (Dialog*)(userData);

    if (filelist && filelist[0] && strlen(filelist[0]) > 0)
    {
      self->path = anm2ed::util::filesystem::path_from_utf8(filelist[0]);
      self->selectedFilter = filter;
    }
    else
    {
      self->selectedFilter = -1;
      self->path.clear();
    }
  }
}

using namespace anm2ed::dialog;
namespace filesystem = anm2ed::util::filesystem;

namespace anm2ed
{

  Dialog::Dialog(SDL_Window* window)
  {
    *this = Dialog();
    this->window = window;
  }

  void Dialog::file_open(dialog::Type type)
  {
    SDL_ShowOpenFileDialog(callback, this, window, FILTERS[TYPE_FILTERS[type]], std::size(FILTERS[TYPE_FILTERS[type]]),
                           nullptr, false);
    this->type = type;
  }

  void Dialog::file_save(dialog::Type type)
  {
    SDL_ShowSaveFileDialog(callback, this, window, FILTERS[TYPE_FILTERS[type]], std::size(FILTERS[TYPE_FILTERS[type]]),
                           nullptr);
    this->type = type;
  }

  void Dialog::folder_open(dialog::Type type)
  {
    SDL_ShowOpenFolderDialog(callback, this, window, nullptr, false);
    this->type = type;
  }

  void Dialog::file_explorer_open(const std::filesystem::path& path)
  {
    if (path.empty()) return;
#ifdef _WIN32
    ShellExecuteW(nullptr, L"open", path.native().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
#elif __unix__
    auto pathUtf8 = filesystem::path_to_utf8(path);
    system(std::format("xdg-open \"{}\" &", pathUtf8).c_str());
#else
    toasts.push(localize.get(TOAST_NOT_SUPPORTED));
    logger.warning(localize.get(TOAST_NOT_SUPPORTED, anm2ed::ENGLISH));
#endif
  }

  void Dialog::reset() { *this = Dialog(this->window); }

  bool Dialog::is_selected(dialog::Type type) const { return this->type == type && !path.empty(); }

};
