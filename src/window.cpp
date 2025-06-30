#include "window.h"

/* Sets the window title from the given anm2 */
void
window_title_from_path_set(SDL_Window* self, const char* path)
{
    if (!strcmp(path, STRING_EMPTY) == 0)
    {
        char windowTitle[WINDOW_TITLE_MAX];
        snprintf(windowTitle, WINDOW_TITLE_MAX, STRING_WINDOW_TITLE_EDITING, path);
        SDL_SetWindowTitle(self, windowTitle);
    }
    else
        SDL_SetWindowTitle(self, STRING_WINDOW_TITLE);
}