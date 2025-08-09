// File dialog; saving/writing to files

#include "dialog.h"

static void _dialog_callback(void* userdata, const char* const* filelist, s32 filter); 

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

void dialog_init(Dialog* self, Anm2* anm2, Anm2Reference* reference, Resources* resources, SDL_Window* window)
{
	self->anm2 = anm2;
	self->reference = reference;
	self->resources = resources;
	self->window = window;
}

void dialog_anm2_open(Dialog* self)
{
	SDL_ShowOpenFileDialog(_dialog_callback, self, nullptr, DIALOG_FILE_FILTER_ANM2, 1, nullptr, false);
	self->type = DIALOG_ANM2_OPEN;
}

void dialog_anm2_save(Dialog* self)
{
	SDL_ShowSaveFileDialog(_dialog_callback, self, nullptr, DIALOG_FILE_FILTER_ANM2, 1, nullptr);
	self->type = DIALOG_ANM2_SAVE;
}

void dialog_png_open(Dialog* self)
{
	SDL_ShowOpenFileDialog(_dialog_callback, self, nullptr, DIALOG_FILE_FILTER_PNG, 1, nullptr, false);
	self->type = DIALOG_PNG_OPEN;
}

void dialog_png_replace(Dialog* self)
{
	SDL_ShowOpenFileDialog(_dialog_callback, self, nullptr, DIALOG_FILE_FILTER_PNG, 1, nullptr, false);
	self->type = DIALOG_PNG_REPLACE;
}

void dialog_update(Dialog* self)
{
	self->isJustSelected = false;

	if (self->isSelected)
	{
		Texture texture;
		s32 id;
		
		switch (self->type)
		{
			case DIALOG_ANM2_OPEN:
				*self->reference = Anm2Reference{};
				resources_textures_free(self->resources);
				anm2_deserialize(self->anm2, self->resources, self->path);
				window_title_from_path_set(self->window, self->path);
				break;
			case DIALOG_ANM2_SAVE:
				anm2_serialize(self->anm2, self->path);
				window_title_from_path_set(self->window, self->path);
				break;
			case DIALOG_PNG_OPEN:
				id = map_next_id_get(self->resources->textures);
				self->anm2->spritesheets[id] = Anm2Spritesheet{};
				self->anm2->spritesheets[id].path = self->path;
				resources_texture_init(self->resources, self->path, id);
				break;
			case DIALOG_PNG_REPLACE:
				self->anm2->spritesheets[self->replaceID].path = self->path;
				resources_texture_init(self->resources, self->path, self->replaceID);
				self->replaceID = -1;
				break;
			default:
				break;
		}

		self->lastType = self->type;
		self->lastPath = self->path;
		self->type = DIALOG_NONE;
		self->path.clear();

		self->isJustSelected = true;
		self->isSelected = false;
	}
}

void
dialog_reset(Dialog* self)
{
	self->lastType = DIALOG_NONE;
	self->type = DIALOG_NONE;
	self->lastPath.clear();
	self->path.clear();
	self->isJustSelected = false;
	self->isSelected = false;
}