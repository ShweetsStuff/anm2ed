#include "gl.h"

#include <SDL3/SDL.h>

Gl gl;

bool gl_load()
{
    bool isLoaded = true;
#define X(name)                                                                                                        \
    gl.name = (decltype(gl.name))SDL_GL_GetProcAddress("gl" #name);                                                    \
    isLoaded &= gl.name != nullptr;
    GL_FUNCTIONS(X)
#undef X
    return isLoaded;
}
