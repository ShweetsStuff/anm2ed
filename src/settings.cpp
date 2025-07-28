#include "settings.h"

static void _settings_setting_load(Settings* self, const std::string& line)
{
    for (s32 i = 0; i < SETTINGS_COUNT; i++) 
    {
        const std::string& key = SETTINGS_ENTRIES[i].key;
        size_t keyLength = key.length();

        if (line.compare(0, keyLength, key) == 0) 
        {
            const char* value = line.c_str() + keyLength;
            void* target = (u8*)self + SETTINGS_ENTRIES[i].offset;

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

static void _settings_setting_write(Settings* self, std::ostream& out, SettingsEntry entry)
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

void settings_save(Settings* self)
{
    std::ifstream input(SETTINGS_PATH);
    std::string oldContents;

    if (!input)
    {
        log_error(std::format(SETTINGS_INIT_ERROR, SETTINGS_PATH));
        return;
    }

    oldContents.assign((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    input.close();

    std::ofstream output(SETTINGS_PATH);
    if (!output)
    {
        log_error(std::format(SETTINGS_INIT_ERROR, SETTINGS_PATH));
        return;
    }

    output << SETTINGS_SECTION << "\n";

    for (s32 i = 0; i < SETTINGS_COUNT; i++)
        _settings_setting_write(self, output, SETTINGS_ENTRIES[i]);

    output << "\n" << SETTINGS_SECTION_IMGUI << "\n";
    output << oldContents;

    output.close();
}

void settings_init(Settings* self)
{
    std::ifstream file(SETTINGS_PATH);
    
    if (!file)
    {
        log_error(std::format(SETTINGS_INIT_ERROR, SETTINGS_PATH));
        return;
    }

    std::string line;
    bool inSettingsSection = false;

    while (std::getline(file, line))
    {
        if (line == SETTINGS_SECTION)
        {
            inSettingsSection = true;
            continue;
        }

        if (line == SETTINGS_SECTION_IMGUI) break; 
        if (inSettingsSection) _settings_setting_load(self, line);
    }
}