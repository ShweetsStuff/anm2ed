#pragma once

#include "anm2.h"

#define WINDOW_TITLE_MAX 0xFF + PATH_MAX 

void window_title_from_anm2_set(SDL_Window* self, Anm2* anm2);