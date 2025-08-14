#include "dialog.h"

static void _dialog_callback(void* userdata, const char* const* filelist, s32 filter) 
{
	Dialog* self;

	self = (Dialog*)userdata;

	if (filelist && filelist[0] && strlen(filelist[0]) > 0)
	{
		self->path = filelist[0];
		self->isSelected = true;
		self->selectedFilter = filter;
	}
	else
	{
		self->isSelected = false;
		self->selectedFilter = INDEX_NONE;
	}
}

void dialog_init(Dialog* self, SDL_Window* window)
{
	self->window = window;
}

void dialog_anm2_open(Dialog* self)
{
	SDL_ShowOpenFileDialog(_dialog_callback, self, self->window, DIALOG_FILE_FILTER_ANM2, 1, nullptr, false);
	self->type = DIALOG_ANM2_OPEN;
}

void dialog_anm2_save(Dialog* self)
{
	SDL_ShowSaveFileDialog(_dialog_callback, self, self->window, DIALOG_FILE_FILTER_ANM2, 1, nullptr);
	self->type = DIALOG_ANM2_SAVE;
}

void dialog_spritesheet_add(Dialog* self)
{
	SDL_ShowOpenFileDialog(_dialog_callback, self, self->window, DIALOG_FILE_FILTER_PNG, 1, nullptr, false);
	self->type = DIALOG_SPRITESHEET_ADD;
}

void dialog_spritesheet_replace(Dialog* self, s32 id)
{
	SDL_ShowOpenFileDialog(_dialog_callback, self, self->window, DIALOG_FILE_FILTER_PNG, 1, nullptr, false);
	self->replaceID = id;
	self->type = DIALOG_SPRITESHEET_REPLACE;
}

void dialog_render_path_set(Dialog* self)
{
	SDL_ShowSaveFileDialog(_dialog_callback, self, self->window, DIALOG_FILE_FILTER_RENDER, 2, nullptr);
	self->type = DIALOG_RENDER_PATH_SET;
}

void dialog_render_directory_set(Dialog* self)
{
	SDL_ShowOpenFolderDialog(_dialog_callback, self, self->window, nullptr, false);
	self->type = DIALOG_RENDER_DIRECTORY_SET;
}

void dialog_ffmpeg_path_set(Dialog* self)
{
	SDL_ShowOpenFileDialog(_dialog_callback, self, self->window, DIALOG_FILE_FILTER_FFMPEG, 1, nullptr, false);
	self->type = DIALOG_FFMPEG_PATH_SET;
}

void
dialog_reset(Dialog* self)
{
	self->replaceID = ID_NONE;
	self->type = DIALOG_NONE;
	self->path.clear();
	self->isSelected = false;
}