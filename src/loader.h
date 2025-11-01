#pragma once

#include <string>
#include <vector>

#include <SDL3/SDL.h>

#include "settings.h"

namespace anm2ed
{
  class Loader
  {
    std::string settings_path();

  public:
    SDL_Window* window{};
    SDL_GLContext glContext{};
    Settings settings;
    std::vector<std::string> arguments;
    bool isError{};

    Loader(int, const char**);
    ~Loader();
  };
}
