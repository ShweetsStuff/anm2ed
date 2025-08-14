#include "settings.h"

static void _settings_setting_load(Settings* self, const std::string& line)
{
    for (s32 i = 0; i < SETTINGS_COUNT; i++)
    {
        const auto& entry = SETTINGS_ENTRIES[i];
        const std::string& key = entry.key;

        void* target = (u8*)self + entry.offset;

        auto match_key = [&](const std::string& full) -> const char*
        {
            if (!line.starts_with(full)) 
                return nullptr;
            size_t p = full.size();
            while (p < line.size() && std::isspace((u8)line[p])) ++p;
            if (p < line.size() && line[p] == '=') return line.c_str() + p + 1;
            return nullptr;
        };

        auto* value = match_key(key);

        if (value)
        {
            switch (entry.type)
            {
                case TYPE_INT:    
                    *(s32*)target = std::atoi(value); 
                    return;
                case TYPE_BOOL:   
                    *(bool*)target = string_to_bool(value); 
                    return;
                case TYPE_FLOAT:  
                    *(f32*)target = std::atof(value); 
                    return;
                case TYPE_STRING: 
                    *(std::string*)target = value; 
                    return;
                default: 
                    break;
            }
        }

        if (entry.type == TYPE_VEC2)
        {
            vec2* v = (vec2*)target;
            if ((value = match_key(key + "X"))) { v->x = std::atof(value); return; }
            if ((value = match_key(key + "Y"))) { v->y = std::atof(value); return; }
        }
        else if (entry.type == TYPE_IVEC2)
        {
            ivec2* v = (ivec2*)target;
            if ((value = match_key(key + "X"))) { v->x = std::atoi(value); return; }
            if ((value = match_key(key + "Y"))) { v->y = std::atoi(value); return; }
        }
        else if (entry.type == TYPE_VEC3)
        {
            vec3* v = (vec3*)target;
            if ((value = match_key(key + "R"))) { v->x = std::atof(value); return; }
            if ((value = match_key(key + "G"))) { v->y = std::atof(value); return; }
            if ((value = match_key(key + "B"))) { v->z = std::atof(value); return; }
        }
        else if (entry.type == TYPE_VEC4)
        {
            vec4* v = (vec4*)target;
            if ((value = match_key(key + "R"))) { v->x = std::atof(value); return; }
            if ((value = match_key(key + "G"))) { v->y = std::atof(value); return; }
            if ((value = match_key(key + "B"))) { v->z = std::atof(value); return; }
            if ((value = match_key(key + "A"))) { v->w = std::atof(value); return; }
        }
    }
}

static void _settings_setting_write(Settings* self, std::ostream& out, SettingsEntry entry)
{
    u8* selfPointer = (u8*)self;
    std::string value;

    switch (entry.type)
    {
        case TYPE_INT:
            value = std::format("{}", *(s32*)(selfPointer + entry.offset));
            out << entry.key << "=" << value << "\n";
            break;
        case TYPE_BOOL:
            value = std::format("{}", *(bool*)(selfPointer + entry.offset));
            out << entry.key << "=" << value << "\n";
            break;
        case TYPE_FLOAT:
            value = std::format("{:.3f}", *(f32*)(selfPointer + entry.offset));
            out << entry.key << "=" << value << "\n";
            break;
        case TYPE_STRING:
        {
            const std::string data = *reinterpret_cast<const std::string*>(selfPointer + entry.offset);
            if (!data.empty())
                out << entry.key << "=" << data.c_str() << "\n";
            break;
        }
        case TYPE_IVEC2:
        {
            ivec2* data = (ivec2*)(selfPointer + entry.offset);
            out << entry.key << "X=" << data->x << "\n";
            out << entry.key << "Y=" << data->y << "\n";
            break;
        }
        case TYPE_VEC2:
        {
            vec2* data = (vec2*)(selfPointer + entry.offset);
            out << entry.key << "X=" << std::format(SETTINGS_FLOAT_FORMAT, data->x) << "\n";
            out << entry.key << "Y=" << std::format(SETTINGS_FLOAT_FORMAT, data->y) << "\n";
            break;
        }
        case TYPE_VEC3:
        {
            vec3* data = (vec3*)(selfPointer + entry.offset);
            out << entry.key << "R=" << std::format(SETTINGS_FLOAT_FORMAT, data->r) << "\n";
            out << entry.key << "G=" << std::format(SETTINGS_FLOAT_FORMAT, data->g) << "\n";
            out << entry.key << "B=" << std::format(SETTINGS_FLOAT_FORMAT, data->b) << "\n";
            break;
        }
        case TYPE_VEC4:
        {
            vec4* data = (vec4*)(selfPointer + entry.offset);
            out << entry.key << "R=" << std::format(SETTINGS_FLOAT_FORMAT, data->r) << "\n";
            out << entry.key << "G=" << std::format(SETTINGS_FLOAT_FORMAT, data->g) << "\n";
            out << entry.key << "B=" << std::format(SETTINGS_FLOAT_FORMAT, data->b) << "\n";
            out << entry.key << "A=" << std::format(SETTINGS_FLOAT_FORMAT, data->a) << "\n";
            break;
        }
        default:
            break;
    }
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