#pragma once

#include "anm2.h"

static const SDL_DialogFileFilter ANM2_DIALOG_FILE_FILTER[] =
{
    {"Anm2", "anm2;xml"}
};

enum DialogType 
{
    DIALOG_ANM2_OPEN,
    DIALOG_ANM2_SAVE
};

struct Dialog
{
    Anm2* anm2 = NULL;
    enum DialogType type = DIALOG_ANM2_OPEN;
    char path[PATH_MAX] = "";
    bool isSelected = false;
};

void dialog_anm2_open(Dialog* self);
void dialog_anm2_save(Dialog* self);
void dialog_tick(Dialog* self);