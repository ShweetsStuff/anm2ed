#include "dialog.h"

#ifdef _WIN32
  #include <window.h>
#endif

#include <format>

namespace anm2ed::dialog
{

  constexpr SDL_DialogFileFilter FILE_FILTER_ANM2[] = {{"Anm2 file", "anm2;xml"}};
  constexpr SDL_DialogFileFilter FILE_FILTER_SPRITESHEET[] = {{"PNG image", "png"}};

  void callback(void* userData, const char* const* filelist, int filter)
  {
    auto self = (Dialog*)(userData);

    if (filelist && filelist[0] && strlen(filelist[0]) > 0)
    {
      self->path = filelist[0];
      self->selectedFilter = filter;
    }
    else
      self->selectedFilter = -1;
  }

  Dialog::Dialog() = default;

  Dialog::Dialog(SDL_Window* window)
  {
    *this = Dialog();
    this->window = window;
  }

  void Dialog::anm2_new()
  {
    SDL_ShowSaveFileDialog(callback, this, window, FILE_FILTER_ANM2, std::size(FILE_FILTER_ANM2), nullptr);
    type = ANM2_NEW;
  }

  void Dialog::anm2_open()
  {
    SDL_ShowOpenFileDialog(callback, this, window, FILE_FILTER_ANM2, std::size(FILE_FILTER_ANM2), nullptr, false);
    type = ANM2_OPEN;
  }

  void Dialog::anm2_save()
  {
    SDL_ShowSaveFileDialog(callback, this, window, FILE_FILTER_ANM2, std::size(FILE_FILTER_ANM2), nullptr);
    type = ANM2_SAVE;
  }

  void Dialog::spritesheet_open()
  {
    SDL_ShowOpenFileDialog(callback, this, window, FILE_FILTER_SPRITESHEET, std::size(FILE_FILTER_SPRITESHEET), nullptr,
                           false);
    type = SPRITESHEET_OPEN;
  }

  void Dialog::spritesheet_replace()
  {
    SDL_ShowOpenFileDialog(callback, this, window, FILE_FILTER_SPRITESHEET, std::size(FILE_FILTER_SPRITESHEET), nullptr,
                           false);
    type = SPRITESHEET_REPLACE;
  }

  void Dialog::file_explorer_open(const std::string& path)
  {
#ifdef _WIN32
    ShellExecuteA(NULL, "open", path.c_str(), NULL, NULL, SW_SHOWNORMAL);
#else
    system(std::format("xdg-open \"{}\" &", path).c_str());
#endif
  }

  void Dialog::reset()
  {
    *this = Dialog(this->window);
  }

  bool Dialog::is_selected_file(Type type)
  {
    return this->type == type && !path.empty();
  }
};
