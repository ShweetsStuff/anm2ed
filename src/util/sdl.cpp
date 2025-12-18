#include "sdl.h"

#include <SDL3/SDL.h>

#include "path_.h"

namespace anm2ed::util::sdl
{
  std::filesystem::path preferences_directory_get()
  {
    auto sdlPath = SDL_GetPrefPath(nullptr, "anm2ed");
    if (!sdlPath) return {};
    auto filePath = path::from_utf8(sdlPath);
    SDL_free(sdlPath);
    return filePath;
  }
}