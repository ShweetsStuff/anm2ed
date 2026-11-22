#include "sdl.h"

#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_stdinc.h>

#include "path.h"

std::filesystem::path preferences_directory_get()
{
    auto sdlPath = SDL_GetPrefPath(nullptr, "anm2ed");
    if (!sdlPath)
        return {};
    auto filePath = path_from_utf8(sdlPath);
    SDL_free(sdlPath);
    return filePath;
}
