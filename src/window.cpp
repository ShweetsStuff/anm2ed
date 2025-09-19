#include "window.h"
#include <SDL3/SDL_video.h>

void window_title_from_path_set(SDL_Window* self, const std::string& path) {
  if (!path.empty())
    SDL_SetWindowTitle(self, std::format(WINDOW_TITLE_FORMAT, path).c_str());
  else
    SDL_SetWindowTitle(self, WINDOW_TITLE);
}

void window_vsync_set(bool isVsync) { SDL_GL_SetSwapInterval(isVsync); }