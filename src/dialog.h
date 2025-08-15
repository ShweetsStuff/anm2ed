#pragma once

#include "render.h"
#include "window.h"

const SDL_DialogFileFilter DIALOG_FILE_FILTER_ANM2[] =
{
    {"Anm2 file", "anm2;xml"}
};

const SDL_DialogFileFilter DIALOG_FILE_FILTER_PNG[] =
{
    {"PNG image", "png"}
};

const SDL_DialogFileFilter DIALOG_RENDER_FILE_FILTERS[] =
{
    {"PNG image", "png"},
    {"GIF image", "gif"}, 
    {"WebM video", "webm"},
    {"MP4 video", "mp4"}
};

const SDL_DialogFileFilter DIALOG_FILE_FILTER_FFMPEG[] =
{
    {"Executable", "exe"}
};

enum DialogType 
{
    DIALOG_NONE,
    DIALOG_ANM2_OPEN,
    DIALOG_ANM2_SAVE,
    DIALOG_SPRITESHEET_ADD,
    DIALOG_SPRITESHEET_REPLACE,
    DIALOG_RENDER_PATH_SET,
    DIALOG_FFMPEG_PATH_SET
};

struct Dialog
{
    SDL_Window* window = nullptr;
    s32 selectedFilter = ID_NONE;
    std::string path{};
    s32 replaceID = ID_NONE;
    DialogType type = DIALOG_NONE;
    bool isSelected = false;
};

void dialog_init(Dialog* self, SDL_Window* window);
void dialog_anm2_open(Dialog* self);
void dialog_spritesheet_add(Dialog* self);
void dialog_spritesheet_replace(Dialog* self, s32 id);
void dialog_anm2_save(Dialog* self);
void dialog_render_path_set(Dialog* self, RenderType type);
void dialog_render_directory_set(Dialog* self);
void dialog_ffmpeg_path_set(Dialog* self);
void dialog_reset(Dialog* self);
void dialog_explorer_open(const std::string& path);