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

/* Opens file dialog for user to pick anm2 files */
void
dialog_anm2_open(Dialog* self)
{
	SDL_ShowOpenFileDialog(_dialog_callback, self, NULL, ANM2_DIALOG_FILE_FILTER, 1, NULL, false);
	self->type = DIALOG_ANM2_OPEN;
}

/* Opens file dialog for user to save new anm2 files */
void
dialog_anm2_save(Dialog* self)
{
	SDL_ShowSaveFileDialog(_dialog_callback, self, NULL, ANM2_DIALOG_FILE_FILTER, 1, NULL);
	self->type = DIALOG_ANM2_SAVE;
}

void
dialog_tick(Dialog* self)
{
	if (self->isSelected)
	{
		switch (self->type)
		{
			case DIALOG_ANM2_OPEN:
				anm2_deserialize(self->anm2, self->path);
				break;
			case DIALOG_ANM2_SAVE:
				anm2_serialize(self->anm2, self->path);
				break;
			default:
				break;
		}

		memset(self->path, '\0', PATH_MAX);
		self->isSelected = false;
	}
}

