#include "dialog.h"

#ifdef _WIN32
    #include <windows.h>
#endif

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
	SDL_ShowOpenFileDialog(_dialog_callback, self, self->window, DIALOG_FILE_FILTER_ANM2, std::size(DIALOG_FILE_FILTER_ANM2), nullptr, false);
	self->type = DIALOG_ANM2_OPEN;
}

void dialog_anm2_save(Dialog* self)
{
	SDL_ShowSaveFileDialog(_dialog_callback, self, self->window, DIALOG_FILE_FILTER_ANM2, std::size(DIALOG_FILE_FILTER_ANM2), nullptr);
	self->type = DIALOG_ANM2_SAVE;
}

void dialog_spritesheet_add(Dialog* self)
{
	SDL_ShowOpenFileDialog(_dialog_callback, self, self->window, DIALOG_FILE_FILTER_PNG, std::size(DIALOG_FILE_FILTER_PNG), nullptr, false);
	self->type = DIALOG_SPRITESHEET_ADD;
}

void dialog_spritesheet_replace(Dialog* self, s32 id)
{
	SDL_ShowOpenFileDialog(_dialog_callback, self, self->window, DIALOG_FILE_FILTER_PNG, std::size(DIALOG_FILE_FILTER_PNG), nullptr, false);
	self->replaceID = id;
	self->type = DIALOG_SPRITESHEET_REPLACE;
}

void dialog_render_path_set(Dialog* self, RenderType type)
{
	SDL_DialogFileFilter filter = DIALOG_RENDER_FILE_FILTERS[type];

	if (type == RENDER_PNG)
		SDL_ShowOpenFolderDialog(_dialog_callback, self, self->window, nullptr, false);
	else
		SDL_ShowSaveFileDialog(_dialog_callback, self, self->window, &filter, 1, nullptr);
	self->type = DIALOG_RENDER_PATH_SET;
}

void dialog_ffmpeg_path_set(Dialog* self)
{
	SDL_ShowOpenFileDialog(_dialog_callback, self, self->window, DIALOG_FILE_FILTER_FFMPEG, std::size(DIALOG_FILE_FILTER_FFMPEG), nullptr, false);
	self->type = DIALOG_FFMPEG_PATH_SET;
}

void dialog_explorer_open(const std::string& path)
{
#ifdef _WIN32
		ShellExecuteA(NULL, "open", path.c_str(), NULL, NULL, SW_SHOWNORMAL);
#else 
		char cmd[512];
		snprintf(cmd, sizeof(cmd), "xdg-open \"%s\" &", path.c_str());
		system(cmd);
#endif
}

void
dialog_reset(Dialog* self)
{
	self->replaceID = ID_NONE;
	self->type = DIALOG_NONE;
	self->path.clear();
	self->isSelected = false;
}