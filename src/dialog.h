#pragma once

#include "anm2.h"
#include "resources.h"
#include "window.h"

static const SDL_DialogFileFilter DIALOG_FILE_FILTER_ANM2[] =
{
    {"Anm2", "anm2;xml"}
};

static const SDL_DialogFileFilter DIALOG_FILE_FILTER_PNG[] =
{
    {"png", "png"}
};

enum DialogType 
{
    DIALOG_NONE,
    DIALOG_ANM2_OPEN,
    DIALOG_ANM2_SAVE,
    DIALOG_PNG_OPEN,
    DIALOG_PNG_REPLACE
};

struct Dialog
{
    Anm2* anm2 = NULL;
    Anm2Reference* reference = NULL;
    Resources* resources = NULL;
    SDL_Window* window = NULL;
    s32 replaceID = -1;
    enum DialogType type = DIALOG_NONE;
    char path[PATH_MAX] = "";
    bool isSelected = false;
};

void dialog_init(Dialog* self, Anm2* anm2, Anm2Reference* reference, Resources* resources, SDL_Window* window);
void dialog_anm2_open(Dialog* self);
void dialog_png_open(Dialog* self);
void dialog_png_replace(Dialog* self);
void dialog_anm2_save(Dialog* self);
void dialog_tick(Dialog* self);