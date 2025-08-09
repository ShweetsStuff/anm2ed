#pragma once

#include "anm2.h"
#include "resources.h"
#include "window.h"

const SDL_DialogFileFilter DIALOG_FILE_FILTER_ANM2[] =
{
    {"Anm2", "anm2;xml"}
};

const SDL_DialogFileFilter DIALOG_FILE_FILTER_PNG[] =
{
    {"png", "png"}
};

enum DialogType 
{
    DIALOG_NONE,
    DIALOG_ANM2_OPEN,
    DIALOG_ANM2_SAVE,
    DIALOG_PNG_OPEN,
    DIALOG_PNG_REPLACE,
    DIALOG_FRAME_DIRECTORY_OPEN,
};

struct Dialog
{
    Anm2* anm2 = nullptr;
    Anm2Reference* reference = nullptr;
    Resources* resources = nullptr;
    SDL_Window* window = nullptr;
    std::string path{};
    std::string lastPath{};
    s32 replaceID = ID_NONE;
    s32 selectedFilter{};
    DialogType type = DIALOG_NONE;
    DialogType lastType = DIALOG_NONE;
    bool isSelected = false;
    bool isJustSelected = false;
};

void dialog_init(Dialog* self, Anm2* anm2, Anm2Reference* reference, Resources* resources, SDL_Window* window);
void dialog_anm2_open(Dialog* self);
void dialog_png_open(Dialog* self);
void dialog_png_replace(Dialog* self);
void dialog_anm2_save(Dialog* self);
void dialog_frame_directory_open(Dialog* self);
void dialog_update(Dialog* self);
void dialog_reset(Dialog* self);