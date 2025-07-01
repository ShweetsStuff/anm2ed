#include "dialog.h"

static void _dialog_callback(void* userdata, const char* const* filelist, s32 filter); 

// Callback that runs during the file dialog; will get the path and determine if one has been selected
static void
_dialog_callback(void* userdata, const char* const* filelist, s32 filter) 
{
	Dialog* self;

	self = (Dialog*)userdata;

	if (filelist && filelist[0] && strlen(filelist[0]) > 0)
	{
		self->path = filelist[0];
		self->isSelected = true;
	}
	else
		self->isSelected = false;
}

// Initializes dialog
void
dialog_init(Dialog* self, Anm2* anm2, Anm2Reference* reference, Resources* resources, SDL_Window* window)
{
	self->anm2 = anm2;
	self->reference = reference;
	self->resources = resources;
	self->window = window;
}

// Opens file dialog to open a new anm2
void
dialog_anm2_open(Dialog* self)
{
	SDL_ShowOpenFileDialog(_dialog_callback, self, NULL, DIALOG_FILE_FILTER_ANM2, 1, NULL, false);
	self->type = DIALOG_ANM2_OPEN;
}

// Opens file dialog to save anm2
void
dialog_anm2_save(Dialog* self)
{
	SDL_ShowSaveFileDialog(_dialog_callback, self, NULL, DIALOG_FILE_FILTER_ANM2, 1, NULL);
	self->type = DIALOG_ANM2_SAVE;
}

// Opens file dialog to open png
void
dialog_png_open(Dialog* self)
{
	SDL_ShowOpenFileDialog(_dialog_callback, self, NULL, DIALOG_FILE_FILTER_PNG, 1, NULL, false);
	self->type = DIALOG_PNG_OPEN;
}

// Opens file dialog to replace a given png
void
dialog_png_replace(Dialog* self)
{
	SDL_ShowOpenFileDialog(_dialog_callback, self, NULL, DIALOG_FILE_FILTER_PNG, 1, NULL, false);
	self->type = DIALOG_PNG_REPLACE;
}

// Ticks dialog
void
dialog_tick(Dialog* self)
{
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
				self->path = self->anm2->spritesheets[id].path;
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

		self->path.clear();
		self->isSelected = false;
	}
}

