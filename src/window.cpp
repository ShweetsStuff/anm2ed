#include "window.h"

/* Sets the window title from the given anm2 */
void
window_title_from_path_set(SDL_Window* self, const std::string& path)
{
    if (path.empty())
    {
        std::string windowTitle = path;
        windowTitle = windowTitle + " (" + path + ")";
        SDL_SetWindowTitle(self, windowTitle.c_str());
    }
    else
        SDL_SetWindowTitle(self, STRING_WINDOW_TITLE);
}