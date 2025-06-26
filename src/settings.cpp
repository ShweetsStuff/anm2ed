#include "settings.h"

static void _settings_setting_load(Settings* self, char* line);
static void _settings_setting_write(Settings* self, SDL_IOStream* io, SettingsItem type);

/* Load a particular settings from a line */
static void
_settings_setting_load(Settings* self, char* line)
{
    for (int i = 0; i < SETTINGS_COUNT; i++) 
    {
        if (strncmp(line, SETTINGS_ENTRIES[i].value, strlen(SETTINGS_ENTRIES[i].value)) == 0) 
        {
            char* value = line + strlen(SETTINGS_ENTRIES[i].value);
            void* target = (u8*)self + SETTINGS_ENTRIES[i].offset;


            switch (SETTINGS_ENTRIES[i].type) 
            {
                case SETTINGS_TYPE_INT:
                case SETTINGS_TYPE_BOOL:
                    *(s32*)target = atoi(value);
                    break;
                case SETTINGS_TYPE_FLOAT:
                    *(f32*)target = atof(value);
                    break;
                case SETTINGS_TYPE_STRING:
                    strncpy((char*)target, value, SETTINGS_BUFFER_ITEM - 1);
                    ((char*)target)[SETTINGS_BUFFER_ITEM - 1] = '\0'; 
                    break;
                default:
                    break;
            }
            return; 
        }
    }
}

/* Writes a particular setting to the current IO */
static void
_settings_setting_write(Settings* self, SDL_IOStream* io, SettingsItem type)
{
    char valueBuffer[SETTINGS_BUFFER_ITEM];
    SettingsEntry entry = SETTINGS_ENTRIES[type];

    u8* selfPointer = (u8*)self;
    
    memset(valueBuffer, '\0', sizeof(valueBuffer));

    switch (entry.type)
    {
        case SETTINGS_TYPE_INT:
            snprintf(valueBuffer, SETTINGS_BUFFER_ITEM, entry.format, *(s32*)(selfPointer + entry.offset));
            break;
        case SETTINGS_TYPE_BOOL:
            snprintf(valueBuffer, SETTINGS_BUFFER_ITEM, entry.format, *(bool*)(selfPointer + entry.offset));
            break;
       case SETTINGS_TYPE_FLOAT:
            snprintf(valueBuffer, SETTINGS_BUFFER_ITEM, entry.format, *(f32*)(selfPointer + entry.offset));
            break;
        case SETTINGS_TYPE_STRING:
            snprintf(valueBuffer, SETTINGS_BUFFER_ITEM, entry.value, entry.format, *(char*)(selfPointer + entry.offset));
            break;
        default:
            break;
    }

    SDL_WriteIO(io, valueBuffer, strlen(valueBuffer));
    SDL_WriteIO(io, "\n", strlen("\n"));
}

/* Saves the program's settings to the PATH_SETTINGS file */
void
settings_save(Settings* self)
{
    char buffer[SETTINGS_BUFFER];

    /* Get the original settings.ini buffer (as previously saved by imgui before this is called) */
	memset(buffer, '\0', SETTINGS_BUFFER);

	SDL_IOStream* io = SDL_IOFromFile(PATH_SETTINGS, "r");

	if (!io)
	{
		printf(STRING_ERROR_SETTINGS_INIT, PATH_SETTINGS);
		return;
	}

	SDL_ReadIO(io, buffer, SETTINGS_BUFFER);
	SDL_CloseIO(io);

	io = SDL_IOFromFile(PATH_SETTINGS, "w");
    
    if (!io)
    {
        printf(STRING_ERROR_SETTINGS_INIT, PATH_SETTINGS);
        return;
    }

    /* [Settings] */
    SDL_WriteIO(io, SETTINGS_SECTION, strlen(SETTINGS_SECTION));
    SDL_WriteIO(io, "\n", strlen("\n"));

    /* Write down all elements */
    for (s32 i = 0; i < SETTINGS_COUNT; i++)
        _settings_setting_write(self, io, (SettingsItem)i);

    SDL_WriteIO(io, "\n", strlen("\n"));

    /* specify that the other settings are imgui */
    SDL_WriteIO(io, SETTINGS_SECTION_IMGUI, strlen(SETTINGS_SECTION_IMGUI));
    SDL_WriteIO(io, "\n", strlen("\n"));

    /* Then write original contents */
    SDL_WriteIO(io, buffer, strlen(buffer));
    SDL_CloseIO(io);
}

/* Loads the settings from the PATH_SETTINGS file */
void
settings_load(Settings* self)
{
    char buffer[SETTINGS_BUFFER];
    char* line = NULL;

	memset(buffer, '\0', SETTINGS_BUFFER);

    SDL_IOStream* io = SDL_IOFromFile(PATH_SETTINGS, "r");

	if (!io)
	{
		printf(STRING_ERROR_SETTINGS_INIT, PATH_SETTINGS);
		return;
	}

	size_t bytesRead = SDL_ReadIO(io, buffer, SETTINGS_BUFFER);
    SDL_CloseIO(io);

    buffer[bytesRead] = '\0';

    line = strtok(buffer, "\n");

    /* The settings will be the first section in the file */
    /* Go through its elements, load them to settings, and go on with your day */
    while (line != NULL)
    {
        if (strcmp(line, SETTINGS_SECTION) == 0)
        {
            line = strtok(NULL, "\n");
            continue;
        }
            
        _settings_setting_load(self, line);

        /* get out here */
        if (strcmp(line, SETTINGS_SECTION_IMGUI) == 0)
            break;

        line = strtok(NULL, "\n");
    }
}