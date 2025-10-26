#pragma once

#include <string>
#include <vector>

#include <SDL3/SDL.h>

#include "settings.h"

namespace anm2ed::loader
{
  class Loader
  {
  public:
    SDL_Window* window{};
    SDL_GLContext glContext{};
    settings::Settings settings;
    std::vector<std::string> arguments;
    bool isError{};

    Loader(int, const char**);
    ~Loader();
  };
}
