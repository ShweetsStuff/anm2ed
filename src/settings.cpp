#include "settings.h"

static void _settings_setting_load(Settings* self, const std::string& line);
static void _settings_setting_write(Settings* self, std::ostream& out, SettingsEntry entry);

static void 
_settings_setting_load(Settings* self, const std::string& line)
{
    for (s32 i = 0; i < SETTINGS_COUNT; i++) 
    {
        const std::string& key = SETTINGS_ENTRIES[i].key;
        size_t keyLength = key.length();

        // Compare keys
        if (line.compare(0, keyLength, key) == 0) 
        {
            const char* value = line.c_str() + keyLength;
            void* target = (u8*)self + SETTINGS_ENTRIES[i].offset;

            // Based on type, assign value to offset of settings
            switch (SETTINGS_ENTRIES[i].type) 
            {
                case SETTINGS_TYPE_INT:
                    *(s32*)target = std::atoi(value);
                    break;
                case SETTINGS_TYPE_BOOL:
                    *(s32*)target = string_to_bool(std::string(value));
                    break;
                case SETTINGS_TYPE_FLOAT:
                    *(f32*)target = std::atof(value);
                    break;
                case SETTINGS_TYPE_STRING:
                    *(std::string*)target = std::string(value);
                    break;
                default:
                    break;
            }
            return; 
        }
    }
}

// Writes a given setting to the stream
static void 
_settings_setting_write(Settings* self, std::ostream& out, SettingsEntry entry)
{
    u8* selfPointer = (u8*)self;
    std::string value;

    switch (entry.type)
    {
        case SETTINGS_TYPE_INT:
            value = std::format("{}", *(s32*)(selfPointer + entry.offset));
            break;
        case SETTINGS_TYPE_BOOL:
            value = std::format("{}", *(bool*)(selfPointer + entry.offset));
            break;
        case SETTINGS_TYPE_FLOAT:
            value = std::format("{:.3f}", *(f32*)(selfPointer + entry.offset));
            break;
        case SETTINGS_TYPE_STRING:
            value = *(std::string*)(selfPointer + entry.offset);
            break;
        default:
            break;
    }

    out << entry.key << value << "\n";
}

// Saves the current settings
// Note: this is just for this program's settings, additional imgui settings handled elsewhere
void 
settings_save(Settings* self)
{
    std::ifstream input(PATH_SETTINGS);
    std::string oldContents;

    if (!input)
    {
        std::cout << STRING_ERROR_SETTINGS_INIT << PATH_SETTINGS << std::endl;
        return;
    }

    // We're writing after the imgui stuff 
    oldContents.assign((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    input.close();

    std::ofstream output(PATH_SETTINGS);
    if (!output)
    {
        std::cout << STRING_ERROR_SETTINGS_INIT << PATH_SETTINGS << std::endl;
        return;
    }

    // [Settings] 
    output << SETTINGS_SECTION << "\n";

    // Write each setting
    for (s32 i = 0; i < SETTINGS_COUNT; i++)
        _settings_setting_write(self, output, SETTINGS_ENTRIES[i]);

    // Write the the imgui section
    output << "\n" << SETTINGS_SECTION_IMGUI << "\n";
    output << oldContents;

    output.close();
}

// Load settings
void 
settings_load(Settings* self)
{
    std::ifstream file(PATH_SETTINGS);
    if (!file)
    {
        std::cerr << STRING_ERROR_SETTINGS_INIT << PATH_SETTINGS << std::endl;
        return;
    }

    std::string line;
    bool inSettingsSection = false;

    // Iterare through settings lines until the imgui section is reached, then end
    while (std::getline(file, line))
    {
        if (line == SETTINGS_SECTION)
        {
            inSettingsSection = true;
            continue;
        }

        if (line == SETTINGS_SECTION_IMGUI)
            break; 

        if (inSettingsSection)
            _settings_setting_load(self, line);
    }
}