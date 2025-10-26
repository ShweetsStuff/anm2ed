#pragma once

#include <string>

#include <SDL3/SDL.h>

namespace anm2ed::dialog
{
  enum Type
  {
    NONE,
    ANM2_NEW,
    ANM2_OPEN,
    ANM2_SAVE,
    SPRITESHEET_OPEN,
    SPRITESHEET_REPLACE
  };

  class Dialog
  {
  public:
    SDL_Window* window{};
    std::string path{};
    Type type{NONE};
    int selectedFilter{-1};
    int replaceID{-1};

    Dialog();
    Dialog(SDL_Window*);
    void anm2_new();
    void anm2_open();
    void anm2_save();
    void spritesheet_open();
    void spritesheet_replace();
    void file_explorer_open(const std::string&);
    void reset();
    bool is_selected_file(Type);
  };
}
