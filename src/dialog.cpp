#include "dialog.h"

static void _dialog_callback(void* userdata, const char* const* filelist, s32 filter); 

static void
_dialog_callback(void* userdata, const char* const* filelist, s32 filter) 
{
	Dialog* self;

	self = (Dialog*)userdata;

	if (filelist && filelist[0] && strlen(filelist[0]) > 0)
	{
		strncpy(self->path, filelist[0], PATH_MAX - 1);
		self->isSelected = true;
	}
	else
		self->isSelected = false;
	
}

void
dialog_init(Dialog* self, Anm2* anm2, Resources* resources)
{
	self->anm2 = anm2;
	self->resources = resources;
}

/* Opens file dialog for user to pick anm2 files */
void
dialog_anm2_open(Dialog* self)
{
	SDL_ShowOpenFileDialog(_dialog_callback, self, NULL, DIALOG_FILE_FILTER_ANM2, 1, NULL, false);
	self->type = DIALOG_ANM2_OPEN;
}

/* Opens file dialog for user to save new anm2 files */
void
dialog_anm2_save(Dialog* self)
{
	SDL_ShowSaveFileDialog(_dialog_callback, self, NULL, DIALOG_FILE_FILTER_ANM2, 1, NULL);
	self->type = DIALOG_ANM2_SAVE;
}

/* Opens file dialog for user to pick png files for spritesheets */
void
dialog_png_open(Dialog* self)
{
	SDL_ShowOpenFileDialog(_dialog_callback, self, NULL, DIALOG_FILE_FILTER_PNG, 1, NULL, false);
	self->type = DIALOG_PNG_OPEN;
}

/* Opens file dialog for user to pick png file to replace selected one */
void
dialog_png_replace(Dialog* self)
{
	SDL_ShowOpenFileDialog(_dialog_callback, self, NULL, DIALOG_FILE_FILTER_PNG, 1, NULL, false);
	self->type = DIALOG_PNG_REPLACE;
}

void
dialog_tick(Dialog* self)
{
	if (self->isSelected)
	{
		Texture texture;
		s32 id;
		char relativePath[PATH_MAX];

		/* Get the relative path */
		std::filesystem::path baseDirectory = std::filesystem::current_path();
		std::filesystem::path relativePathString = std::filesystem::relative(self->path, baseDirectory);
		
		strncpy(relativePath, relativePathString.c_str(), PATH_MAX - 1);
		
		switch (self->type)
		{
			case DIALOG_ANM2_OPEN:
				anm2_deserialize(self->anm2, self->resources, relativePath);
				resources_loaded_textures_free(self->resources);
				break;
			case DIALOG_ANM2_SAVE:
				anm2_serialize(self->anm2, relativePath);
				break;
			case DIALOG_PNG_OPEN:
				id = map_next_id_get(self->resources->loadedTextures);
				self->anm2->spritesheets[id] = Anm2Spritesheet{};
				strncpy(self->anm2->spritesheets[id].path, relativePath, PATH_MAX);
				anm2_spritesheet_texture_load(self->anm2, self->resources, relativePath, id);
				break;
			case DIALOG_PNG_REPLACE:
				strncpy(self->anm2->spritesheets[self->replaceID].path, relativePath, PATH_MAX);
				anm2_spritesheet_texture_load(self->anm2, self->resources, relativePath, self->replaceID);
				self->replaceID = -1;
				break;
			default:
				break;
		}

		memset(self->path, '\0', PATH_MAX);
		self->isSelected = false;
	}
}

